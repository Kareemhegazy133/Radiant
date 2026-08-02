#pragma once

#include "Entity.h"

#include <glm/glm.hpp>

namespace Radiant {

	class PhysicsWorld2D;

	/**
	 * How gameplay pushes an entity's physics body: forces, impulses,
	 * velocities and kinematic targets. Obtained from
	 * Entity::GetPhysicsBody(), never constructed directly by gameplay.
	 *
	 * The split it completes (RAD-90): Level owns the verbs that touch ECS
	 * state — Teleport and RefreshCollider write TransformComponent and the
	 * render snapshot, so only the type owning the registry can perform them.
	 * Everything here touches physics state and nothing else, so it lives on
	 * the entity's physics handle instead of swelling Level and Entity with a
	 * verb and a forwarder apiece. Unreal draws the same line: the verbs hang
	 * off a per-body handle (FBodyInstance), not off the world.
	 *
	 * Ownership: owns nothing. It stores one Entity — a 16-byte value handle —
	 * and reaches the body through the Level on every call. The Level owns the
	 * physics world, the world owns the body, and the entity's
	 * RigidBody2DComponent holds the claim ticket; this type holds none of
	 * them. Copy it freely.
	 *
	 * Lifetime & threading: TRANSIENT, exactly like the Entity inside it.
	 * Obtain it, use it, drop it — never store one across a fixed update, or
	 * across anything that may destroy the entity. A stale handle is a warned
	 * no-op, never a crash. Main-thread only.
	 *
	 * Nothing is cached, deliberately: the body id is re-resolved per call. A
	 * cached id would save one array read and would forfeit the
	 * warn-clear-and-recover policy the moment something destroyed the body
	 * underneath it (playbook §4).
	 *
	 * WHEN THE EFFECT IS OBSERVED depends on where you call from. Scripts run
	 * BEFORE the physics step, so a verb called from OnUpdate lands in the step
	 * about to run. Collision handlers run AFTER it, so the same call from
	 * OnCollisionBegin lands in the NEXT step. Both are correct; the asymmetry
	 * falls out of the fixed-update order (see Level::OnFixedUpdate) and is not
	 * something this type can or should hide.
	 */
	class PhysicsBody
	{
	public:
		/** An invalid handle: every verb on it is a warned no-op. */
		PhysicsBody() = default;

		/** Use Entity::GetPhysicsBody() — explicit so an Entity never converts silently. */
		explicit PhysicsBody(Entity entity)
			: m_Entity(entity) {}

		/**
		 * True when this entity can take physics verbs RIGHT NOW: live handle,
		 * live Level with a physics world, and a RigidBody2DComponent present.
		 *
		 * It deliberately does NOT ask Box2D whether the body id is still
		 * live. That question belongs to the verb, because the verb is what can
		 * warn, clear the dead id and recover from it — answering it here would
		 * either duplicate that recovery or, worse, report "fine" and leave the
		 * dead id in place. So this is a cheap, vendor-free question about the
		 * ENTITY, and the verb still guards itself.
		 */
		bool IsValid() const;
		operator bool() const { return IsValid(); }

		// --- Sustained pushes: re-apply EVERY fixed step while they last ---
		// Force accumulates until the next step consumes it, so one call buys
		// one step's worth of push. Acceleration is force / mass.

		/** Pushes at the centre of mass, in newtons (N). Never induces spin. */
		void ApplyForce(const glm::vec2& force);

		/**
		 * Pushes at a point in WORLD coordinates, in newtons (N). Off the
		 * centre of mass this also turns the body — the reason to pick this
		 * over ApplyForce.
		 */
		void ApplyForceAtPoint(const glm::vec2& force, const glm::vec2& worldPoint);

		/** Twists about z without pushing, in newton-metres (N·m). Positive is counter-clockwise. */
		void ApplyTorque(float torque);

		// --- Instantaneous hits: applied ONCE, not per step ----------------

		/**
		 * Adds an instantaneous change in momentum at the centre of mass, in
		 * newton-seconds (N·s): a jump, an explosion, a bullet hit. Velocity
		 * changes immediately by impulse / mass, so a heavier body moves less
		 * for the same impulse; applied at the centre, it never induces spin.
		 * Prefer a force for anything sustained.
		 */
		void ApplyLinearImpulse(const glm::vec2& impulse);

		/**
		 * As ApplyLinearImpulse, at a point in WORLD coordinates — off the
		 * centre of mass it spins the body too. Impulse in newton-seconds.
		 */
		void ApplyLinearImpulseAtPoint(const glm::vec2& impulse, const glm::vec2& worldPoint);

		/** "Start spinning now": instantaneous angular momentum, in kg·m²/s. */
		void ApplyAngularImpulse(float impulse);

		// --- Direct velocity: you are the authority --------------------------

		/**
		 * Overwrites linear velocity, in metres per second. Ignores mass and
		 * discards what the solver computed, so a body set every step is no
		 * longer really being simulated in that axis — right for a character
		 * controller or a conveyor, wrong for almost everything else. Setting
		 * {0,0} does not wake a sleeping body (it is already stopped).
		 */
		void SetLinearVelocity(const glm::vec2& velocity);

		/** Overwrites angular velocity, in radians per second. */
		void SetAngularVelocity(float angularVelocity);

		/**
		 * Linear velocity in metres per second, or {0,0} with no live body.
		 * Non-const deliberately: resolving can CLEAR a stale body id as part
		 * of recovering from it, and `const` would be claiming this call only
		 * observes (see PhysicsWorld2D::GetLinearVelocity).
		 */
		glm::vec2 GetLinearVelocity();

		/** Angular velocity in radians per second, or 0. Non-const for the same reason. */
		float GetAngularVelocity();

		// --- Kinematic movement ---------------------------------------------

		/**
		 * Drives a KINEMATIC body toward a world pose by next step, letting
		 * the solver compute the velocity that gets it there — so the body
		 * genuinely moves, friction carries its riders, and contacts happen
		 * along the way. This is how a moving platform is moved; setting its
		 * transform per step instead gives a platform that slides out from
		 * under everything standing on it.
		 *
		 * CALL EVERY FIXED STEP. A body that stops receiving targets keeps
		 * moving at its last computed velocity — Box2D has no auto-stop.
		 *
		 * Takes the fixed delta from Time internally, so gameplay never
		 * handles a timestep here; position is world units, rotation radians.
		 */
		void MoveKinematic(const glm::vec2& position, float rotation);

	private:
		/**
		 * This layer's single guard (playbook §4, one resolver per layer):
		 * catches the level-less handle — a default-constructed PhysicsBody,
		 * which cannot reach a Level to be warned by one — and defers
		 * everything else to Level::ResolvePhysics. Null means "do nothing";
		 * whichever layer declined has already said so if it was worth saying.
		 */
		PhysicsWorld2D* Resolve(const char* verb) const;

		Entity m_Entity;
	};

}