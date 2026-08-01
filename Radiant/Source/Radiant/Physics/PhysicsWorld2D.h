#pragma once

#include "Radiant/Core/Timestep.h"
#include "Radiant/Core/UUID.h"
#include "Radiant/Physics/ContactEvent.h"

#include <glm/glm.hpp>

#include <box2d/id.h>

#include <string>
#include <vector>

namespace Radiant {

	class Entity;
	struct RigidBody2DComponent;
	struct BoxCollider2DComponent;

	/**
	 * One Level's Box2D v3 world. Each instance is an independent simulation —
	 * bodies in one world never interact with bodies in another — which is what
	 * lets multiple Levels coexist (RAD-27). The full surface (RAD-28): entt
	 * signals drive body/shape lifecycle, the explicit verbs (Teleport,
	 * UpdateBoxShape) are the only ECS→Box2D writes, and Step reports results
	 * through the move-event buffer.
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
		 * One "this body moved during the last Step" record, translated to
		 * engine types at the module boundary — Box2D types never leave
		 * Physics/. Position is world units; Rotation is radians.
		 */
		struct BodyMoveEvent
		{
			UUID EntityId;
			glm::vec2 Position;
			float Rotation;
		};

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
		 * Explicitly moves the entity's body to a world pose (position in
		 * world units, rotation in radians) — the ONLY ECS→Box2D transform
		 * push (RAD-28). A teleport, not a swept move: velocity is kept, no
		 * collisions occur along the way, and the body is woken — SetTransform
		 * alone would leave a sleeping body teleported into mid-air hanging
		 * there until touched. Call through Level::Teleport, which also writes
		 * the ECS transform and render snapshot. Zero/stale body ids are
		 * survivable skips.
		 */
		void Teleport(Entity& entity, const glm::vec2& position, float rotation);

		/**
		 * Creates the box shape on the entity's EXISTING body and stores the
		 * packed id in component.RuntimeShapeId: half-extents = Size * transform
		 * scale, offset relative to the body, no local rotation — the shape is
		 * body-local and the body already carries the world rotation. Asserts if
		 * the entity has no RigidBody2DComponent — the rigidbody must be added
		 * first. Invoked via the on_construct entt signal. Must not run during
		 * a world step.
		 */
		void CreateBoxShape(Entity& entity, BoxCollider2DComponent& component);

		/**
		 * Destroys the entity's box shape and zeroes RuntimeShapeId. A zero id
		 * is a silent no-op (shape never created, or already gone with its
		 * body); a stale id warns and recovers — it means a lifecycle path
		 * outside the entt signals touched the shape. Invoked via the
		 * on_destroy entt signal. Must not run during a world step.
		 */
		void DestroyBoxShape(Entity& entity, BoxCollider2DComponent& component);

		/**
		 * Re-applies the collider component's properties to the entity's
		 * EXISTING shape, in place: geometry from Size * transform scale and
		 * Offset (body-local, no rotation), then density/friction/restitution,
		 * then one body-mass refresh — v3's shape setters deliberately leave
		 * mass untouched. Never destroys the shape, so its contacts survive
		 * the refresh. Call through Level::RefreshCollider after mutating
		 * collider fields or the transform's Scale. A collider without a live
		 * shape warns and recovers.
		 */
		void UpdateBoxShape(Entity& entity, BoxCollider2DComponent& component);

		/**
		 * Advances the simulation one FIXED step with 4 sub-steps (v3's solver
		 * unit, replacing v2's velocity/position iteration pair), then refills
		 * the move-event buffer (GetMoveEvents) with the bodies that moved and
		 * the contact-event buffer (GetContactEvents) with the touches that
		 * began or ended. Both drains copy Box2D's transient arrays into engine
		 * types immediately, which is what lets consumers mutate the world
		 * freely afterwards. Call with the engine's fixed delta from the
		 * accumulator loop — never a variable frame delta (playbook §1).
		 */
		void Step(Timestep ts);

		/**
		 * The bodies that moved during the last Step, translated to engine
		 * types. Valid until the next Step (the buffer is refilled in place) —
		 * consume within the same fixed update, never cache. Sleeping and
		 * static bodies produce no entries: readback cost scales with
		 * activity, not population.
		 */
		const std::vector<BodyMoveEvent>& GetMoveEvents() const { return m_MoveEvents; }

		/**
		 * The touches that began or ended during the last Step, translated to
		 * engine types. Valid until the next Step (the buffer is refilled in
		 * place) — consume within the same fixed update, never cache. Begins
		 * come before ends: a batch may hold both an End for a contact that
		 * died and a Begin for one that replaced it, and dispatching begins
		 * first keeps an overlap counter from transiently crossing zero.
		 *
		 * Only TRANSITIONS appear here — a settled stack of resting bodies
		 * produces nothing, step after step. Shapes report only if they (or
		 * their partner) opted in via BoxCollider2DComponent::EnableContactEvents.
		 */
		const std::vector<ContactEvent>& GetContactEvents() const { return m_ContactEvents; }

	private:
		// Generation handle to the Box2D world — zero-initialized is the null id
		// (box2d/id.h). The only Box2D state this class holds.
		b2WorldId m_WorldId = {};

		// Identity for the lifecycle logs only; not used by simulation
		std::string m_DebugName;

		// Reusable move-event buffer refilled by Step — clear() keeps
		// capacity, so steady-state refills allocate nothing
		std::vector<BodyMoveEvent> m_MoveEvents;

		// Reusable contact-event buffer, same refill contract as m_MoveEvents
		std::vector<ContactEvent> m_ContactEvents;
	};

}