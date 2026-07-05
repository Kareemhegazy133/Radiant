#include "Radiant/rdpch.h"
#include "CollisionListener2D.h"

#include <box2d/b2_body.h>
#include <box2d/b2_contact.h>

#include "Radiant/ECS/Level.h"
#include "Radiant/ECS/Entity.h"
//#include "Radiant/GAS/Ability.h"

namespace Radiant {

	CollisionListener2D::CollisionListener2D(Level* level)
		: m_Level(level)
	{
		RADIANT_TRACE("CollisionListener2D Constructed");
	}

	CollisionListener2D::~CollisionListener2D()
	{
		RADIANT_TRACE("CollisionListener2D Destructed");
	}

	void CollisionListener2D::BeginContact(b2Contact* contact)
	{
		b2Body* bodyA = contact->GetFixtureA()->GetBody();
		b2Body* bodyB = contact->GetFixtureB()->GetBody();

		Entity entityA = m_Level->GetEntityByUUID(static_cast<UUID>(bodyA->GetUserData().pointer));
		Entity entityB = m_Level->GetEntityByUUID(static_cast<UUID>(bodyB->GetUserData().pointer));

		// No validity checks yet: GetEntityByUUID returns an invalid entity for a
		// mid-destruction body, and GetComponent on it asserts (RAD-29)
		auto& entityARB2D = entityA.GetComponent<RigidBody2DComponent>();

		//RADIANT_TRACE("Entity: {} collided with Entity {}.", entityA.GetComponent<MetadataComponent>().Tag, entityB.GetComponent<MetadataComponent>().Tag);

		if (entityARB2D.OnCollisionBegin)
		{
			entityARB2D.OnCollisionBegin(entityB);
		}

		auto& entityBRB2D = entityB.GetComponent<RigidBody2DComponent>();
		if (entityBRB2D.OnCollisionBegin)
		{
			entityBRB2D.OnCollisionBegin(entityA);
		}
	}

	void CollisionListener2D::EndContact(b2Contact* contact)
	{
		b2Body* bodyA = contact->GetFixtureA()->GetBody();
		b2Body* bodyB = contact->GetFixtureB()->GetBody();

		Entity entityA = m_Level->GetEntityByUUID(static_cast<UUID>(bodyA->GetUserData().pointer));
		Entity entityB = m_Level->GetEntityByUUID(static_cast<UUID>(bodyB->GetUserData().pointer));

		auto& entityARB2D = entityA.GetComponent<RigidBody2DComponent>();
		if (entityARB2D.OnCollisionEnd)
		{
			entityARB2D.OnCollisionEnd(entityB);
		}

		auto& entityBRB2D = entityB.GetComponent<RigidBody2DComponent>();
		if (entityBRB2D.OnCollisionEnd)
		{
			entityBRB2D.OnCollisionEnd(entityA);
		}
	}

	bool CollisionListener2D::ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB)
	{
		// No collision filtering yet — filtering arrives with the Phase 2 collision
		// event rework (RAD-29)
		return true;
	}
}