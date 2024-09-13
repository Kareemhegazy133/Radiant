#include "Enginepch.h"

#include "box2d/b2_body.h"
#include "box2d/b2_contact.h"

#include "ECS/Level.h"
#include "ECS/Entity.h"
#include "GAS/Ability.h"

#include "CollisionListener.h"

namespace Engine {
	
	CollisionListener::CollisionListener(Level* level)
		: m_Level(level)
	{
	}

	void CollisionListener::BeginContact(b2Contact* contact)
	{
		b2Body* bodyA = contact->GetFixtureA()->GetBody();
		b2Body* bodyB = contact->GetFixtureB()->GetBody();

		// Get the entity handle
		entt::entity entityAHandle = static_cast<entt::entity>(bodyA->GetUserData().pointer);
		entt::entity entityBHandle = static_cast<entt::entity>(bodyB->GetUserData().pointer);

		Entity entityA(entityAHandle, m_Level);
		Entity entityB(entityBHandle, m_Level);

		auto& entityARB2D = entityA.GetComponent<Rigidbody2DComponent>();
		if (entityARB2D.OnCollisionBegin)
		{
			entityARB2D.OnCollisionBegin(entityB);
		}

		auto& entityBRB2D = entityB.GetComponent<Rigidbody2DComponent>();
		if (entityBRB2D.OnCollisionBegin)
		{
			entityBRB2D.OnCollisionBegin(entityA);
		}
	}

	void CollisionListener::EndContact(b2Contact* contact)
	{
		b2Body* bodyA = contact->GetFixtureA()->GetBody();
		b2Body* bodyB = contact->GetFixtureB()->GetBody();

		// Get the entity handle
		entt::entity entityAHandle = static_cast<entt::entity>(bodyA->GetUserData().pointer);
		entt::entity entityBHandle = static_cast<entt::entity>(bodyB->GetUserData().pointer);

		Entity entityA(entityAHandle, m_Level);
		Entity entityB(entityBHandle, m_Level);

		auto& entityARB2D = entityA.GetComponent<Rigidbody2DComponent>();
		if (entityARB2D.OnCollisionEnd)
		{
			entityARB2D.OnCollisionEnd(entityB);
		}

		auto& entityBRB2D = entityB.GetComponent<Rigidbody2DComponent>();
		if (entityBRB2D.OnCollisionEnd)
		{
			entityBRB2D.OnCollisionEnd(entityA);
		}
	}

	bool CollisionListener::ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB)
	{
		b2Body* bodyA = fixtureA->GetBody();
		b2Body* bodyB = fixtureB->GetBody();

		// Get the entity handle
		entt::entity entityAHandle = static_cast<entt::entity>(bodyA->GetUserData().pointer);
		entt::entity entityBHandle = static_cast<entt::entity>(bodyB->GetUserData().pointer);

		Entity entityA(entityAHandle, m_Level);
		Entity entityB(entityBHandle, m_Level);

		//ENGINE_INFO("Entity A: {0}, Entity B: {1}", entityA.GetComponent<MetadataComponent>().Tag, entityB.GetComponent<MetadataComponent>().Tag);

		if (entityA.GetComponent<MetadataComponent>().Type == typeid(Ability))
		{
			auto& entityANSC = entityA.GetComponent<NativeScriptComponent>();
			if (entityANSC.Instance)
			{
				Ability* ability = static_cast<Ability*>(entityANSC.Instance);

				if (ability && ability->Caster && *(ability->Caster) == entityB)
				{
					return false;
				}
			}
		}

		if (entityB.GetComponent<MetadataComponent>().Type == typeid(Ability))
		{
			auto& entityBNSC = entityB.GetComponent<NativeScriptComponent>();
			if (entityBNSC.Instance)
			{
				Ability* ability = static_cast<Ability*>(entityBNSC.Instance);

				if (ability && ability->Caster && *(ability->Caster) == entityA)
				{
					return false;
				}
			}
		}

		return true;
	}
}