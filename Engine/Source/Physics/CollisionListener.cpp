#include "Enginepch.h"

#include "box2d/b2_body.h"
#include "box2d/b2_contact.h"

#include "Scene/Entities/Entity.h"
#include "Scene/Entities/Ability.h"

#include "CollisionListener.h"

namespace Engine {

	void CollisionListener::BeginContact(b2Contact* contact)
	{
		b2Body* bodyA = contact->GetFixtureA()->GetBody();
		b2Body* bodyB = contact->GetFixtureB()->GetBody();

		// Get the entity handle
		entt::entity entityAHandle = static_cast<entt::entity>(bodyA->GetUserData().pointer);
		entt::entity entityBHandle = static_cast<entt::entity>(bodyB->GetUserData().pointer);

		Entity entityA(entityAHandle, &Scene::GetScene());
		Entity entityB(entityBHandle, &Scene::GetScene());

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

		Entity entityA(entityAHandle, &Scene::GetScene());
		Entity entityB(entityBHandle, &Scene::GetScene());

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

		Entity entityA(entityAHandle, &Scene::GetScene());
		Entity entityB(entityBHandle, &Scene::GetScene());

		//ENGINE_INFO("Entity A: {0}, Entity B: {1}", entityA.GetComponent<MetadataComponent>().Tag, entityB.GetComponent<MetadataComponent>().Tag);

		auto* entityAAbilityComponent = entityA.TryGetComponent<AbilityComponent>();
		if (entityAAbilityComponent)
		{
			if (*(entityAAbilityComponent->Caster) == entityB) {
				return false;
			}
		}

		auto* entityBAbilityComponent = entityB.TryGetComponent<AbilityComponent>();
		if (entityBAbilityComponent)
		{
			if (*(entityBAbilityComponent->Caster) == entityA) {
				return false;
			}
		}

		return true;
	}
}