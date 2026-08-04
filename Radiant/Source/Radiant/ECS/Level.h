#pragma once

#include <entt.hpp>

#include "Radiant/Core/GameApplication.h"
#include "Radiant/Core/UUID.h"
#include "Radiant/Physics/ContactEvent.h"
#include "Radiant/Physics/TeleportType.h"

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
		 * Opaque, copyable identity for a registered collision callback. Index
		 * says which slot, Generation says which lifetime of that slot — so a
		 * handle to a removed callback is always safe, even after its slot has
		 * been reused: RemoveCollisionCallback on it is a no-op instead of
		 * unregistering whoever inherited the slot. Same shape and reasoning as
		 * TimerHandle (playbook §2).
		 */
		struct CollisionCallbackHandle
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
		 * body, pushes the pose into Box2D (body woken).
		 * rotationZ is radians; the overload without it keeps the current
		 * rotation. teleportType decides what the body's velocity does on
		 * arrival and defaults to KeepVelocity — portal semantics, and what
		 * this verb has always done; pass ResetVelocity for respawn semantics
		 * (see TeleportType). Body-less entities ignore it: there is no
		 * velocity to keep or reset. Invalid handles warn and recover.
		 */
		void Teleport(Entity entity, const glm::vec3& translation, float rotationZ, TeleportType teleportType = TeleportType::KeepVelocity);
		void Teleport(Entity entity, const glm::vec3& translation, TeleportType teleportType = TeleportType::KeepVelocity);

		/**
		 * Moves an entity CONTINUOUSLY — the everyday "it moved" verb, where
		 * Teleport above is "it jumped". Same two halves as Teleport (ECS
		 * transform write, plus a push into Box2D for body-backed entities, so
		 * this is never the silent no-op a raw TransformComponent write is on a
		 * body — RAD-28), and two deliberate differences:
		 *
		 *   - The render snapshot is PRESERVED, so motion stays interpolated
		 *     between simulation steps. Teleport stamps it instead, which is
		 *     what stops a jump smearing across a frame but would pin
		 *     continuous motion to fixed-step granularity.
		 *   - Nothing is logged. Teleport TRACEs because it is a rare,
		 *     deliberate act; a follow-camera calling this every step would
		 *     emit 60 lines a second (playbook §4).
		 *
		 * translation is world units; rotationZ is RADIANS about Z, and the
		 * transform's X/Y Euler terms are left untouched — the same choice
		 * Teleport makes. Velocity is always kept: a continuous move has no
		 * reason to discard it, and a caller who wants it cleared wants
		 * Teleport(ResetVelocity). Never a swept move — no collisions occur
		 * along the way (Radiant has no query API until RAD-76, where UE's
		 * SetActorLocation offers bSweep). Moving a DYNAMIC body this way every
		 * step fights the solver, which will correct it: continuous movement of
		 * a dynamic body belongs to PhysicsBody's force and velocity verbs, and
		 * of a kinematic body to MoveKinematic. Invalid handles warn and
		 * recover.
		 *
		 * The overloads keep the half you do not name: SetLocation keeps the
		 * current rotation, SetRotation keeps the current translation.
		 */
		void SetTransform(Entity entity, const glm::vec3& translation, float rotationZ);
		void SetLocation(Entity entity, const glm::vec3& translation);
		void SetRotation(Entity entity, float rotationZ);

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
		 * VFX, abilities) that belong to no single entity's script. These run
		 * BEFORE per-entity script hooks, so they see the collision before
		 * gameplay starts reacting to it. An empty callable is a programmer
		 * error (asserted; returns an invalid handle).
		 *
		 * CALLBACK LIFETIME CONTRACT: the Level owns the callback by value, and
		 * therefore owns whatever it captured. A callback capturing an object
		 * (an Entity, a system pointer, `this`) outlives its target unless the
		 * owner removes the handle in its teardown path — RemoveCollisionCallback
		 * releases the callback and its captures immediately. The Level cannot
		 * detect a subscriber that died without unregistering.
		 *
		 * Registering from inside a collision callback is legal: the new
		 * callback starts receiving events with the NEXT batch, never the one
		 * being dispatched.
		 */
		CollisionCallbackHandle AddCollisionCallback(std::function<void(const CollisionEvent&)> callback);

		/**
		 * Unregisters the callback, releases it (and its captures), and
		 * resets the handle. Stale, invalid, and already-removed handles are
		 * benign no-ops by design — this is the correct idiom for "remove if
		 * still registered". Safe to call from inside a collision callback,
		 * including on itself: the release is deferred to the end of the batch,
		 * since freeing a std::function that is currently executing would
		 * destroy the running lambda's own captures.
		 */
		void RemoveCollisionCallback(CollisionCallbackHandle& handle);

		/**
		 * Advances the simulation one FIXED step: runs native scripts (lazily
		 * instantiating unconstructed instances — see EntityBehaviour), steps
		 * the physics world, then drains the world's move events — only the
		 * bodies that actually moved — back into ECS transforms as a dedicated
		 * post-step pass. Physics owns the transform of
		 * dynamic bodies — nothing here pushes ECS transforms into Box2D
		 * (RAD-28); pushes happen only at spawn and through the explicit
		 * verbs. Call with the engine's fixed delta (from Layer::OnFixedUpdate)
		 * — never a variable frame delta.
		 *
		 * SCRIPT-PASS CONTRACT (RAD-95). Scripts may create and destroy
		 * entities, and those changes take effect IMMEDIATELY — but the set of
		 * scripts to run is snapshotted before the first one executes, so:
		 * an entity spawned during a step first receives its own OnCreate and
		 * OnUpdate on the NEXT step, and an entity destroyed during a step is
		 * skipped for the remainder of this one. Both are guarantees gameplay
		 * may rely on, not implementation details. A spawned entity also has
		 * no render snapshot for the frame it was born in (the snapshot pass
		 * runs before scripts), so it draws un-interpolated once — correct,
		 * since there is no previous pose to blend from.
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
		 * What a move does to the render snapshot — an enum rather than a bool
		 * because `WriteTransform(e, p, r, true, …)` says nothing at the call
		 * site (playbook §4: semantic flags are enums). It is also the ONLY
		 * axis, deliberately: "stamp the snapshot" and "push through the
		 * logging, velocity-carrying physics Teleport" both mean *this move was
		 * discontinuous*, so a second parameter for the physics half would let
		 * a caller express a state that must never exist.
		 */
		enum class SnapshotPolicy
		{
			Preserve,             // continuous — rendering interpolates toward the new pose
			StampToDestination    // discontinuous — no interpolation, so a jump does not smear
		};

		/**
		 * The ECS half every move shares: validity guard, transform write, and
		 * the snapshot policy. Returns false when the handle was invalid (and
		 * warned), so the caller skips its physics push.
		 *
		 * The physics half deliberately stays with each public verb, because
		 * that is the part that genuinely differs — Teleport needs the logging,
		 * velocity-policy push and SetTransform needs the quiet one. This
		 * helper owns what is actually common, which is also the only part that
		 * MUST live on Level: writing TransformComponent and the snapshot is
		 * registry-owner work (playbook §4, the RAD-90 split).
		 *
		 * Pass __func__ for verb, never a literal — see the physics resolvers
		 * for the full reasoning; the WARN has to name the verb the caller
		 * actually invoked, not this helper.
		 */
		bool WriteTransform(Entity entity, const glm::vec3& translation, float rotationZ, SnapshotPolicy snapshot, const char* verb);

		/**
		 * The one place a physics-only verb answers "may I touch this entity's
		 * physics?" — PhysicsBody's whole guard, so no verb carries its own
		 * (playbook §4, one resolver per layer). Returns null on an invalid
		 * handle, after a WARN naming the verb, matching DestroyEntity and
		 * Teleport; and null WITHOUT a warning on a scratch level, because
		 * having no world is a fact about the level rather than a mistake by
		 * the caller (the same contract OnFixedUpdate's null check uses).
		 *
		 * Stays private deliberately: it hands back the physics world, and the
		 * world is Level-owned state, not public API. PhysicsBody is a friend
		 * precisely so this can remain so.
		 */
		PhysicsWorld2D* ResolvePhysics(Entity entity, const char* verb);

		/**
		 * Hands the physics world's contact batch to gameplay: level-wide
		 * callbacks first (whole event), then each live side's script hook.
		 * Both participants
		 * are re-resolved from their UUIDs immediately before every call, because
		 * an earlier callback in the same batch may have destroyed them.
		 * Called from OnFixedUpdate after the move drain, so handlers read this
		 * step's transforms.
		 */
		void DispatchContactEvents();

		/** Delivers one side's collision hook, if it has a live, active script. */
		void NotifyScript(Entity entity, Entity other, ContactPhase phase);

		/** Frees a callback slot's function and retires every handle to it. */
		void ReleaseCallbackSlot(uint32_t index);

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

		// The script set for the current fixed step, rebuilt each step before
		// any script runs (see OnFixedUpdate's script-pass contract). A member
		// rather than a local so the pass never allocates once capacity has
		// settled — clear() keeps the buffer, a local vector would hit the heap
		// 60 times a second.
		//
		// Being a member is what makes the pass NON-REENTRANT: re-entering
		// OnFixedUpdate would clear() the buffer the outer walk is iterating.
		// m_RunningScripts below is the tripwire, so that contract is enforced
		// rather than merely written here.
		std::vector<entt::entity> m_ScriptUpdateList;

		// Level-wide collision callbacks. Slot storage plus a free list, the
		// same pool shape TimerManager uses: slots never shrink, so an index
		// stays meaningful forever and the generation counter retires handles
		// to a recycled slot.
		struct CollisionCallback
		{
			// Named Function, not Callback: this struct IS the callback, and a
			// CollisionCallback::Callback reads as though it held another one
			std::function<void(const CollisionEvent&)> Function;
			uint32_t Generation = 0;
			bool Active = false;
		};

		// deque, NOT vector, and this is load-bearing: registering a callback
		// from inside a collision callback is documented as legal, and a vector
		// growing past capacity would free the buffer holding the std::function
		// that is mid-call — a use-after-free on return. Deque insertion
		// invalidates iterators but never references to existing elements, and
		// indexing stays O(1).
		std::deque<CollisionCallback> m_CollisionCallbacks;
		std::vector<uint32_t> m_FreeCallbackSlots;       // released slots awaiting reuse
		std::vector<uint32_t> m_PendingCallbackReleases; // removals deferred until dispatch ends

		// True only while DispatchContactEvents is walking a batch. Two jobs:
		// it defers callback releases (a callback may be removing itself), and
		// it is the tripwire for a handler re-entering the fixed update — which
		// would Step the world again and clear the buffer being walked.
		bool m_DispatchingContacts = false;

		// True only while the script pass is walking m_ScriptUpdateList. The
		// tripwire for re-entering OnFixedUpdate from gameplay: re-entry would
		// clear() and refill the buffer the outer walk is iterating, which is
		// iterator invalidation, not merely surprising ordering. A script
		// cannot reach OnFixedUpdate (that is what GameplayLevel is for), so
		// this guards the remaining path — the owner calling it re-entrantly.
		bool m_RunningScripts = false;

		// For Debugging Purposes
		bool m_ShowPhysicsColliders = true;

		friend class Entity;
		// Reaches ResolvePhysics (above) and nothing else. The alternative was
		// a public physics-world accessor, which would expose Level-owned
		// state to every consumer to serve one type in this same module.
		friend class PhysicsBody;
		friend class LevelSerializer;
	};

}

#include "EntityTemplates.h"