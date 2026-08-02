#include "Radiant/rdpch.h"
#include "PhysicsBody.h"

#include "Level.h"

#include "Radiant/Core/Time.h"
#include "Radiant/Physics/PhysicsWorld2D.h"

namespace Radiant {

	bool PhysicsBody::IsValid() const
	{
		// Order matters: IsValid() is what proves m_Level is non-null, so the
		// world check cannot come first
		return m_Entity.IsValid()
			&& m_Entity.m_Level->m_PhysicsWorld != nullptr
			&& m_Entity.HasComponent<RigidBody2DComponent>();
	}

	PhysicsWorld2D* PhysicsBody::Resolve(const char* verb) const
	{
		// A default-constructed PhysicsBody has no Level to route through, and
		// so no Level that could warn about it — this is the one case that
		// cannot be delegated downward
		if (!m_Entity.m_Level)
		{
			RADIANT_WARN("PhysicsBody: {0} called on a handle with no level", verb);
			return nullptr;
		}

		return m_Entity.m_Level->ResolvePhysics(m_Entity, verb);
	}

	// Every verb below is the same two lines, and that is the point: the
	// entity guard lives once in Level::ResolvePhysics and the id resolution
	// lives once in PhysicsWorld2D's ResolveBody, so adding a verb costs one
	// line here and three there — never a copied guard block (playbook §4).
	//
	// The verb name travels as a string because it is what the WARN reports:
	// "which call did this" is the whole value of the log line, and a generic
	// message would regress the audit trail RAD-28 established.

	void PhysicsBody::ApplyForce(const glm::vec2& force)
	{
		if (PhysicsWorld2D* physics = Resolve("ApplyForce"))
			physics->ApplyForce(m_Entity, force);
	}

	void PhysicsBody::ApplyForceAtPoint(const glm::vec2& force, const glm::vec2& worldPoint)
	{
		if (PhysicsWorld2D* physics = Resolve("ApplyForceAtPoint"))
			physics->ApplyForceAtPoint(m_Entity, force, worldPoint);
	}

	void PhysicsBody::ApplyTorque(float torque)
	{
		if (PhysicsWorld2D* physics = Resolve("ApplyTorque"))
			physics->ApplyTorque(m_Entity, torque);
	}

	void PhysicsBody::ApplyLinearImpulse(const glm::vec2& impulse)
	{
		if (PhysicsWorld2D* physics = Resolve("ApplyLinearImpulse"))
			physics->ApplyLinearImpulse(m_Entity, impulse);
	}

	void PhysicsBody::ApplyLinearImpulseAtPoint(const glm::vec2& impulse, const glm::vec2& worldPoint)
	{
		if (PhysicsWorld2D* physics = Resolve("ApplyLinearImpulseAtPoint"))
			physics->ApplyLinearImpulseAtPoint(m_Entity, impulse, worldPoint);
	}

	void PhysicsBody::ApplyAngularImpulse(float impulse)
	{
		if (PhysicsWorld2D* physics = Resolve("ApplyAngularImpulse"))
			physics->ApplyAngularImpulse(m_Entity, impulse);
	}

	void PhysicsBody::SetLinearVelocity(const glm::vec2& velocity)
	{
		if (PhysicsWorld2D* physics = Resolve("SetLinearVelocity"))
			physics->SetLinearVelocity(m_Entity, velocity);
	}

	void PhysicsBody::SetAngularVelocity(float angularVelocity)
	{
		if (PhysicsWorld2D* physics = Resolve("SetAngularVelocity"))
			physics->SetAngularVelocity(m_Entity, angularVelocity);
	}

	// The getters answer zero for "no body", which is the same answer a body at
	// rest gives. That collision is deliberate: gameplay asking how fast
	// something is moving wants a number it can use, and operator bool is
	// there for callers that need to tell the two apart.

	glm::vec2 PhysicsBody::GetLinearVelocity()
	{
		if (PhysicsWorld2D* physics = Resolve("GetLinearVelocity"))
			return physics->GetLinearVelocity(m_Entity);

		return { 0.0f, 0.0f };
	}

	float PhysicsBody::GetAngularVelocity()
	{
		if (PhysicsWorld2D* physics = Resolve("GetAngularVelocity"))
			return physics->GetAngularVelocity(m_Entity);

		return 0.0f;
	}

	void PhysicsBody::MoveKinematic(const glm::vec2& position, float rotation)
	{
		// The fixed delta is read HERE rather than inside PhysicsWorld2D, which
		// keeps the physics module a pure function of its inputs — no clock, so
		// it stays constructible in a test with no GameApplication. This layer
		// already lives inside the application, so asking it the time costs
		// nothing architecturally.
		//
		// Time, not a cached "last step's delta": scripts run BEFORE the first
		// Step, so a cached member would be 0 on the first fixed update and
		// b2Body_SetTargetTransform would silently decline it (timeStep <= 0).
		// One dropped step out of sixty is invisible, which makes it a worse
		// bug than a loud one.
		if (PhysicsWorld2D* physics = Resolve("MoveKinematic"))
			physics->MoveKinematic(m_Entity, position, rotation, static_cast<float>(Time::GetFixedDeltaTime()));
	}

}