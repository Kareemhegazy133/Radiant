#include "Enginepch.h"

#include "box2d/b2_world.h"
#include "box2d/b2_contact.h"

#include "CollisionListener.h"

#include "World/Entity.h"

namespace Engine {

    CollisionListener::CollisionListener()
    {
        b2World physicsWorld = Physics2D::Get().GetPhysicsWorld();
        physicsWorld.SetContactListener(this);
    }

    CollisionListener::~CollisionListener()
    {
    }

    void CollisionListener::BeginContact(b2Contact* contact)
    {
        b2Body* bodyA = contact->GetFixtureA()->GetBody();
        b2Body* bodyB = contact->GetFixtureB()->GetBody();

        // Access the entity data
        Entity* entityDataA = reinterpret_cast<Entity*>(bodyA->GetUserData().pointer);
        Entity* entityDataB = reinterpret_cast<Entity*>(bodyB->GetUserData().pointer);

        Physics2D::Get().OnCollisionBegin(entityDataA, entityDataB);
    }

    void CollisionListener::EndContact(b2Contact* contact)
    {
        b2Body* bodyA = contact->GetFixtureA()->GetBody();
        b2Body* bodyB = contact->GetFixtureB()->GetBody();

        // Access the entity data
        Entity* entityDataA = reinterpret_cast<Entity*>(bodyA->GetUserData().pointer);
        Entity* entityDataB = reinterpret_cast<Entity*>(bodyB->GetUserData().pointer);

        Physics2D::Get().OnCollisionEnd(entityDataA, entityDataB);
    }
}