#pragma once

#include "Radiant/Core/Timestep.h"

#include <glm/glm.hpp>

#include <box2d/id.h>

#include <string>

namespace Radiant {

	class Entity;
	struct RigidBody2DComponent;
	struct BoxCollider2DComponent;

	/**
	 * One Level's Box2D v3 world. Each instance is an independent simulation —
	 * bodies in one world never interact with bodies in another — which is what
	 * lets multiple Levels coexist (RAD-27). Grows the body/shape lifecycle and
	 * the step/sync passes as the RAD-27 port progresses.
	 *
	 * Ownership: owned by its Level via Scope<PhysicsWorld2D>, created in the
	 * Level constructor and destroyed with the Level. Owns the underlying Box2D
	 * world through m_WorldId; copy and move are deleted — a copied handle means
	 * two owners and a double destroy, and nothing ever needs to move it (the
	 * owning Scope does).
	 *
	 * Lifetime & threading: exists exactly as long as its Level's live phase.
	 * Main-thread only. Never create or destroy bodies during a world step —
	 * Box2D forbids world mutation while stepping.
	 *
	 * Failure semantics: if world creation fails (constructor logs ERROR and
	 * asserts), the instance degrades to a safe no-op — CreateBody and Step
	 * guard the null world; every other method exits through body-id validity
	 * checks, since no body can exist without a world.
	 *
	 * Units: positions are world units (meters-ish, mapping to Transform
	 * Translation.xy); angles are radians (Transform Rotation.z).
	 */
	class PhysicsWorld2D
	{
	public:
		/**
		 * Creates the Box2D world. debugName appears in the create/destroy logs
		 * (the paired-lifetime evidence for RAD-27's AC) — pass the owning
		 * Level's name. Gravity defaults to the engine's historical {0, -9.8}.
		 */
		explicit PhysicsWorld2D(const std::string& debugName, const glm::vec2& gravity = { 0.0f, -9.8f });

		/** Destroys the world and every body/shape still in it. */
		~PhysicsWorld2D();

		PhysicsWorld2D(const PhysicsWorld2D&) = delete;
		PhysicsWorld2D& operator=(const PhysicsWorld2D&) = delete;
		PhysicsWorld2D(PhysicsWorld2D&&) = delete;
		PhysicsWorld2D& operator=(PhysicsWorld2D&&) = delete;

		/**
		 * Creates the entity's Box2D body from its transform (position =
		 * Translation.xy, rotation = Rotation.z radians), stamps the entity's
		 * UUID into the body's user data (contact resolution, RAD-29), and
		 * stores the packed id in component.RuntimeBodyId. Invoked via the
		 * on_construct entt signal. Must not run during a world step.
		 */
		void CreateBody(Entity& entity, RigidBody2DComponent& component);

		/**
		 * Destroys the entity's body and zeroes RuntimeBodyId. A zero or stale
		 * id is a no-op, never a crash. Invoked via the on_destroy entt signal.
		 * Must not run during a world step.
		 */
		void DestroyBody(Entity& entity, RigidBody2DComponent& component);

		/**
		 * Creates the box shape on the entity's EXISTING body: half-extents =
		 * Size * transform scale, offset relative to the body. Asserts if the
		 * entity has no RigidBody2DComponent — the rigidbody must be added
		 * first.
		 */
		void CreateBoxShape(Entity& entity, BoxCollider2DComponent& component);

		/**
		 * Advances the simulation one FIXED step with 4 sub-steps (v3's solver
		 * unit, replacing v2's velocity/position iteration pair). Call with the
		 * engine's fixed delta from the accumulator loop — never a variable
		 * frame delta (playbook §1).
		 */
		void Step(Timestep ts);

		/**
		 * Pushes the entity's ECS transform into its body, then destroys and
		 * recreates its box shape from the current collider properties. The
		 * per-step rebuild destroys contact persistence, sleeping, and
		 * warm-starting, and allocates every step — known defect ported 1:1
		 * (RAD-28); pushes become explicit-teleport-only and rebuilds become
		 * property-change-only there. Asserts if the entity has no body id.
		 */
		void SubmitTransform(Entity& entity);

		/**
		 * Reads the body's position/rotation back into the entity's transform
		 * (Translation.xy, Rotation.z radians). Warns and returns for entities
		 * without a rigidbody. Runs as the dedicated post-step sync pass
		 * (playbook §4) — never from a render path.
		 */
		void RetrieveTransform(Entity& entity);

	private:
		// Generation handle to the Box2D world — zero-initialized is the null id
		// (box2d/id.h). The only Box2D state this class holds.
		b2WorldId m_WorldId = {};

		// Identity for the lifecycle logs only; not used by simulation
		std::string m_DebugName;
	};

}