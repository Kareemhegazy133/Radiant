#pragma once

#include "Radiant/Core/Timestep.h"

#include "CollisionListener2D.h"

#include "Radiant/ECS/Level.h"
#include "Radiant/ECS/Entity.h"
#include "Radiant/ECS/Components.h"

class b2World;

namespace Radiant {

	/**
	 * Box2D 2.4 integration. Fully static: one PROCESS-WIDE data block holding the
	 * b2World (gravity hardcoded to {0, -9.8}), a non-owning pointer to the bound
	 * Level, and the collision listener. At most one Level's physics can therefore
	 * exist at a time, and re-initializing overwrites the previous world without
	 * freeing it — known limitation; the Phase 2 rework moves the world into the
	 * Level (RAD-27).
	 *
	 * Units: positions are world units (meters-ish, mapping to Transform
	 * Translation.xy); angles are radians (Transform Rotation.z). Main-thread
	 * only. Never create or destroy bodies during a world step — Box2D forbids
	 * world mutation from inside Step (including contact callbacks).
	 */
	class Physics2D
	{
	public:
		/**
		 * Binds the singleton to level and creates a fresh b2World plus collision
		 * listener. Called by the Level constructor. Does NOT free a previously
		 * bound world — rebinding leaks it (RAD-27).
		 */
		static void Init(Level* level);

		/**
		 * Destroys the current world and collision listener. Called from the Level
		 * destructor. Every RigidBody2DComponent::RuntimeBody dangles afterwards.
		 */
		static void Shutdown();

		/**
		 * Creates the b2Body from the entity's transform (position = Translation.xy,
		 * angle = Rotation.z radians), stamps the entity's UUID into the body's user
		 * data for contact resolution, and stores the b2Body* in
		 * component.RuntimeBody. Invoked via the on_construct entt signal.
		 */
		static void CreatePhysicsBody(Entity& entity, RigidBody2DComponent& component);

		/** Destroys the body and nulls RuntimeBody. Invoked via the on_destroy entt signal. Must not run during a world step. */
		static void DestroyPhysicsBody(Entity& entity, RigidBody2DComponent& component);

		/**
		 * Creates the box fixture on the entity's EXISTING body: half-extents =
		 * Size * transform scale, offset/angle relative to the body. Asserts if the
		 * entity has no RigidBody2DComponent — the rigidbody must be added first.
		 */
		static void CreateBoxColliderFixture(Entity& entity, BoxCollider2DComponent& component);

		/**
		 * Steps the world once with the variable frame timestep (6 velocity / 2
		 * position iterations). Variable-step simulation is framerate-dependent —
		 * a known defect; fixed timestep arrives with RAD-25.
		 */
		static void OnUpdate(Timestep ts);

		/**
		 * Pushes the entity's ECS transform into its body, then destroys and
		 * recreates its fixture from the current collider properties. The per-frame
		 * fixture rebuild destroys contact persistence, sleeping, and warm-starting,
		 * and allocates every frame — known defect (RAD-28); rebuilds will become
		 * property-change-only. Asserts if RuntimeBody is null.
		 */
		static void SubmitEntitiesTransforms(Entity& entity);

		/**
		 * Reads the body's position/angle back into the entity's transform
		 * (Translation.xy, Rotation.z). Warns and returns for entities without a
		 * rigidbody. Currently invoked only from the sprite render loop, so bodies
		 * without sprites never sync back — known defect (RAD-28).
		 */
		static void UpdateEntitiesTransforms(Entity& entity);

		// For Debugging Purposes
		// This function must be called after Renderer2D::BeginScene()
		static void DebugDraw(TransformComponent& transform, BoxCollider2DComponent& bc2d);
	
	private:
		struct Physics2DData
		{
			Level* LevelPtr = nullptr;
			b2World* B2DWorld = nullptr;

			CollisionListener2D* CollisionListener = nullptr;
		};
		
		static Scope<Physics2DData> s_Physics2DData;
	};
}