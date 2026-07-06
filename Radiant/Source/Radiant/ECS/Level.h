#pragma once

#include <entt.hpp>

#include "Radiant/Core/GameApplication.h"
#include "Radiant/Core/UUID.h"

#include "Entity.h"

namespace Radiant {

	/**
	 * The world: one set of entities plus the loops that update and render them.
	 *
	 * Owns the entt registry privately — all access goes through Level/Entity APIs;
	 * the registry is never exposed. A Level is a ref-counted Asset (held via
	 * Ref<Level>, loadable through the AssetManager). Main-thread only.
	 *
	 * Lifecycle contract: entity creation and destruction are IMMEDIATE (no command
	 * buffer) — never call DestroyEntity while iterating a view containing that
	 * entity. Physics body lifecycle is driven by the entt signals registered in the
	 * constructor: adding a RigidBody2DComponent creates the Box2D body, removing it
	 * destroys the body — component presence IS the physics binding.
	 *
	 * The owner (Reaper's GameLayer) drives OnFixedUpdate at the engine's fixed
	 * simulation rate and OnRender once per rendered frame.
	 * Physics currently binds to the process-wide Physics2D singleton, so only one
	 * fully-initialized Level may exist at a time (RAD-27).
	 */
	class Level : public Asset
	{
	public:
		/**
		 * Constructs an empty level. Constructing with initialize == true rebinds the
		 * process-wide Physics2D singleton to this level (RAD-27) and connects the
		 * physics component signals. Pass initialize == false only for scratch levels
		 * that merely stage entities for (de)serialization — they get no physics and
		 * no signals.
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
		 * Advances the simulation one FIXED step: runs native scripts (lazily
		 * instantiating unconstructed instances — see ScriptableEntity), pushes
		 * active rigidbody entities' ECS transforms into Box2D, steps the physics
		 * world, then reads stepped body transforms back into the ECS as a
		 * dedicated post-step pass. Call with the engine's fixed delta (from
		 * Layer::OnFixedUpdate) — never a variable frame delta.
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
		void OnRigidBody2DComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnRigidBody2DComponentDestroy(entt::registry& registry, entt::entity entity);
		void OnBoxCollider2DComponentConstruct(entt::registry& registry, entt::entity entity);

		// Keeps MetadataComponent pool order deterministic (creation order) after
		// destruction/deserialization, so iteration order is stable across runs
		void SortEntities();

	private:
		entt::registry m_Registry;

		std::string m_Name;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		std::unordered_map<UUID, Entity> m_EntityMap;

		// For Debugging Purposes
		bool m_ShowPhysicsColliders = true;

		friend class Entity;
		friend class Physics2D;
		friend class LevelSerializer;
	};

}

#include "EntityTemplates.h"