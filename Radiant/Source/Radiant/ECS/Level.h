#pragma once

#include <entt.hpp>

#include "Radiant/Core/GameApplication.h"
#include "Radiant/Core/UUID.h"
#include "Radiant/Physics/ContactEvent.h"

#include "Entity.h"

#include <deque>
#include <functional>

namespace Radiant {

	class PhysicsWorld2D;

	/**
	 * The world: one set of entities plus the loops that update and render them.
	 *
	 * Owns the entt registry privately — all access goes through Level/Entity APIs;
	 * the registry is never exposed. A Level is a ref-counted Asset (held via
	 * Ref<Level>, loadable through the AssetManager). Main-thread only.
	 *
	 * Lifecycle contract: entity creation and destruction are IMMEDIATE (no command
	 * buffer) — never call DestroyEntity while iterating a view containing that
	 * entity. Physics lifecycle is driven by the entt signals registered in the
	 * constructor: adding a RigidBody2DComponent creates the Box2D body and
	 * removing it destroys the body; the same holds for BoxCollider2DComponent
	 * and its shape — component presence IS the physics binding.
	 *
	 * The owner (Reaper's GameLayer) drives OnFixedUpdate at the engine's fixed
	 * simulation rate and OnRender once per rendered frame.
	 * Each Level owns its physics world (Scope<PhysicsWorld2D>) — worlds are
	 * independent simulations, so any number of Levels may coexist (RAD-27).
	 */
	class Level : public Asset
	{
	public:
		/**
		 * A contact resolved to entities — the gameplay-facing twin of the
		 * physics module's ContactEvent. EITHER handle may be invalid, meaning
		 * that side was destroyed before the notification was delivered; never
		 * both, since an event with nobody left to tell is skipped. The handles
		 * are valid for the duration of the call only — resolve by UUID if you
		 * need to remember a participant.
		 */
		struct CollisionEvent
		{
			Entity A;
			Entity B;
			ContactPhase Phase;
		};

		/**
		 * Opaque, copyable identity for a registered collision observer. Index
		 * says which slot, Generation says which lifetime of that slot — so a
		 * handle to a removed observer is always safe, even after its slot has
		 * been reused: RemoveCollisionObserver on it is a no-op instead of
		 * unregistering whoever inherited the slot. Same shape and reasoning as
		 * TimerHandle (playbook §2).
		 */
		struct CollisionObserverHandle
		{
			static constexpr uint32_t InvalidIndex = 0xFFFFFFFF;

			uint32_t Index = InvalidIndex;
			uint32_t Generation = 0;
		};

		/**
		 * Constructs an empty level. Constructing with initialize == true creates
		 * this level's own physics world and connects the physics component
		 * signals. Pass initialize == false only for scratch levels that merely
		 * stage entities for (de)serialization — they get no world and no
		 * signals, and their destruction touches no other Level's physics.
		 */
		Level(const std::string& name = "UntitledLevel", bool initialize = true);
		~Level();

		/**
		 * Creates a live entity immediately, with a fresh UUID, a TransformComponent,
		 * and a MetadataComponent tagged with name ("Entity" if empty).
		 */
		Entity CreateEntity(const std::string& name = std::string());

		/**
		 * As CreateEntity, but with a caller-supplied UUID (deserialization path).
		 * The UUID must be unique within this level — a duplicate silently replaces
		 * the previous entity's entry in the UUID lookup map.
		 */
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());

		/**
		 * Destroys the entity IMMEDIATELY: runs the native script's OnDestroy (if an
		 * instance exists), removes signal-bound components first so their on_destroy
		 * handlers can still read Metadata/Transform, then frees the entt handle.
		 * Never call while iterating a registry view. All Entity handles to the
		 * destroyed entity are dangling afterwards.
		 */
		void DestroyEntity(Entity entity);

		/** UUID overload of DestroyEntity; a no-op for unknown UUIDs. */
		void DestroyEntity(UUID entityID);

		/**
		 * Moves an entity discontinuously — the ONLY way to reposition an
		 * entity with a body (physics owns dynamic transforms; direct writes
		 * to TransformComponent have no physical effect — RAD-28). Works on
		 * any entity: writes the ECS transform, resets the render snapshot so
		 * the jump doesn't smear across a frame, and, if the entity has a
		 * body, pushes the pose into Box2D (velocity kept, body woken).
		 * rotationZ is radians; the overload without it keeps the current
		 * rotation. Invalid handles warn and recover.
		 */
		void Teleport(Entity entity, const glm::vec3& translation, float rotationZ);
		void Teleport(Entity entity, const glm::vec3& translation);

		/**
		 * Re-applies an entity's BoxCollider2DComponent fields and transform
		 * Scale to its live Box2D shape, in place — call after mutating
		 * collider properties or Scale; nothing detects those writes
		 * implicitly (RAD-28). Asserts if the entity has no collider
		 * component; a collider without a live shape warns and recovers.
		 */
		void RefreshCollider(Entity entity);

		/**
		 * Registers a callback invoked once per collision with BOTH
		 * participants — the channel for level-wide systems (damage, audio,
		 * VFX, abilities) that belong to no single entity's script. Observers
		 * run BEFORE per-entity script hooks, so they see the collision before
		 * gameplay starts reacting to it. An empty callable is a programmer
		 * error (asserted; returns an invalid handle).
		 *
		 * CALLBACK LIFETIME CONTRACT: the Level owns the callback by value, and
		 * therefore owns whatever it captured. A callback capturing an object
		 * (an Entity, a system pointer, `this`) outlives its target unless the
		 * owner removes the handle in its teardown path — RemoveCollisionObserver
		 * releases the callback and its captures immediately. The Level cannot
		 * detect a subscriber that died without unregistering.
		 *
		 * Registering from inside a collision callback is legal: the new
		 * observer starts receiving events with the NEXT batch, never the one
		 * being dispatched.
		 */
		CollisionObserverHandle AddCollisionObserver(std::function<void(const CollisionEvent&)> observer);

		/**
		 * Unregisters the observer, releases its callback (and captures), and
		 * resets the handle. Stale, invalid, and already-removed handles are
		 * benign no-ops by design — this is the correct idiom for "remove if
		 * still registered". Safe to call from inside a collision callback,
		 * including on itself: the release is deferred to the end of the batch,
		 * since freeing a std::function that is currently executing would
		 * destroy the running lambda's own captures.
		 */
		void RemoveCollisionObserver(CollisionObserverHandle& handle);

		/**
		 * Advances the simulation one FIXED step: runs native scripts (lazily
		 * instantiating unconstructed instances — see ScriptableEntity), steps
		 * the physics world, then drains the world's move events — only the
		 * bodies that actually moved — back into ECS transforms as a dedicated
		 * post-step pass. Physics owns the transform of
		 * dynamic bodies — nothing here pushes ECS transforms into Box2D
		 * (RAD-28); pushes happen only at spawn and through the explicit
		 * verbs. Call with the engine's fixed delta (from Layer::OnFixedUpdate)
		 * — never a variable frame delta.
		 */
		void OnFixedUpdate(Timestep ts);

		/**
		 * Renders sprite entities through Renderer2D using the first CameraComponent
		 * found with Primary set (iteration order — with several Primary cameras the
		 * pick is effectively arbitrary; with none, nothing renders). alpha in [0,1]
		 * (Time::GetAlpha()) blends movable entities between the last two simulation
		 * states: entities with a TransformSnapshotComponent draw
		 * lerp(snapshot, current, alpha); everything else draws current. Read-only
		 * with respect to simulation state (playbook §1): physics readback happens
		 * in OnFixedUpdate's post-step pass, not here.
		 */
		void OnRender(float alpha);

		/**
		 * Propagates the new viewport size to every camera that is not
		 * FixedAspectRatio. No-op when the size is unchanged.
		 */
		void OnViewportResize(uint32_t width, uint32_t height);

		/**
		 * Linear scan over all entities; returns the first whose tag matches, or an
		 * invalid Entity when none does. O(n) — do not call per frame in hot paths.
		 */
		Entity FindEntityByName(std::string_view name);

		/**
		 * Resolves a persistent UUID to a live entity; returns an invalid Entity for
		 * unknown UUIDs. UUIDs are the durable identity — entt handles are transient
		 * and never serialized.
		 */
		Entity GetEntityByUUID(UUID uuid);

		/** The level's asset handle doubles as its persistent identity. */
		UUID GetUUID() const { return Handle; }

		/**
		 * Collects every asset handle referenced by this level's components (sprite
		 * textures, fonts). Handles that fail AssetManager validation are excluded.
		 */
		std::unordered_set<AssetHandle> GetAssetList();

		void SetName(const std::string& name) { m_Name = name; }
		const std::string& GetName() const { return m_Name; }

	protected:
		// Live registry view — never destroy entities or add/remove the iterated
		// component types while iterating it (creation/destruction is immediate)
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

	private:
		/**
		 * Hands the physics world's contact batch to gameplay: observers first
		 * (whole event), then each live side's script hook. Both participants
		 * are re-resolved from their UUIDs immediately before every call, because
		 * an earlier callback in the same batch may have destroyed them.
		 * Called from OnFixedUpdate after the move drain, so handlers read this
		 * step's transforms.
		 */
		void DispatchContactEvents();

		/** Delivers one side's collision hook, if it has a live, active script. */
		void NotifyScript(Entity entity, Entity other, ContactPhase phase);

		/** Frees an observer slot's callback and retires every handle to it. */
		void ReleaseObserverSlot(uint32_t index);

		void OnRigidBody2DComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnRigidBody2DComponentDestroy(entt::registry& registry, entt::entity entity);
		void OnBoxCollider2DComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnBoxCollider2DComponentDestroy(entt::registry& registry, entt::entity entity);

		// Keeps MetadataComponent pool order deterministic (creation order) after
		// destruction/deserialization, so iteration order is stable across runs
		void SortEntities();

	private:
		// This level's own physics world — created in the constructor for live
		// levels, null for scratch levels (initialize == false), destroyed with
		// the level AFTER the destructor body's entity loop (bodies die while
		// the world is still alive). Declared BEFORE m_Registry so the world
		// also outlives the registry during member teardown — no signal can
		// ever fire into a dead world regardless of entt's destruction
		// behavior. PhysicsWorld2D is forward-declared, so the out-of-line
		// ~Level() is required for the unique_ptr to destroy it.
		Scope<PhysicsWorld2D> m_PhysicsWorld;

		entt::registry m_Registry;

		std::string m_Name;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		std::unordered_map<UUID, Entity> m_EntityMap;

		// Level-wide collision observers. Slot storage plus a free list, the
		// same pool shape TimerManager uses: slots never shrink, so an index
		// stays meaningful forever and the generation counter retires handles
		// to a recycled slot.
		struct CollisionObserver
		{
			std::function<void(const CollisionEvent&)> Callback;
			uint32_t Generation = 0;
			bool Active = false;
		};

		// deque, NOT vector, and this is load-bearing: registering an observer
		// from inside a collision callback is documented as legal, and a vector
		// growing past capacity would free the buffer holding the std::function
		// that is mid-call — a use-after-free on return. Deque insertion
		// invalidates iterators but never references to existing elements, and
		// indexing stays O(1).
		std::deque<CollisionObserver> m_CollisionObservers;
		std::vector<uint32_t> m_FreeObserverSlots;       // released slots awaiting reuse
		std::vector<uint32_t> m_PendingObserverReleases; // removals deferred until dispatch ends

		// True only while DispatchContactEvents is walking a batch. Two jobs:
		// it defers observer releases (a callback may be removing itself), and
		// it is the tripwire for a handler re-entering the fixed update — which
		// would Step the world again and clear the buffer being walked.
		bool m_DispatchingContacts = false;

		// For Debugging Purposes
		bool m_ShowPhysicsColliders = true;

		friend class Entity;
		friend class LevelSerializer;
	};

}

#include "EntityTemplates.h"