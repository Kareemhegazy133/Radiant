#include "Enginepch.h"

#include <box2d/b2_world.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>

#include "ECS/Entity.h"

#include "Physics2D.h"

namespace Engine {

	Physics2D::Physics2D(Level* level)
		: m_Level(level), m_PhysicsWorld(new b2World({ 0.0f, 9.8f })), m_CollisionListener(level)
	{
		m_PhysicsWorld->SetContactListener(&m_CollisionListener);
		m_PhysicsWorld->SetContactFilter(&m_CollisionListener);

		// For debugging purposes
		m_PhysicsWorld->SetDebugDraw(&m_DebugDraw);
		m_DebugDraw.SetFlags(b2Draw::e_aabbBit);
		std::cout << "Physics System Constructed: " << this << std::endl;
	}

	Physics2D::~Physics2D()
	{
		delete m_PhysicsWorld;
		m_PhysicsWorld = nullptr;
		std::cout << "Physics System Destroyed: " << this << std::endl;
	}

	void Physics2D::OnUpdate(Timestep ts)
	{
		const int32_t velocityIterations = 6;
		const int32_t positionIterations = 3;
		m_PhysicsWorld->Step(ts, velocityIterations, positionIterations);
		// For debugging purposes
		m_PhysicsWorld->DebugDraw();
	}

	static b2BodyType Rigidbody2DTypeToBox2DBody(Rigidbody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Rigidbody2DComponent::BodyType::Static:    return b2_staticBody;
		case Rigidbody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
		case Rigidbody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
		}

		ENGINE_ASSERT(false, "Unknown body type");
		return b2_staticBody;
	}

	void Physics2D::CreatePhysicsBody(Entity entity, Rigidbody2DComponent& component)
	{
		auto& transform = entity.GetComponent<TransformComponent>();

		b2BodyDef bodyDef;
		bodyDef.type = Rigidbody2DTypeToBox2DBody(component.Type);
		bodyDef.position.Set(transform.GetPosition().x, transform.GetPosition().y);
		bodyDef.angle = DEG_TO_RAD(transform.GetRotation());

		bodyDef.userData.pointer = static_cast<uintptr_t>(uint32_t(entity));

		b2Body* body = m_PhysicsWorld->CreateBody(&bodyDef);
		body->SetFixedRotation(component.FixedRotation);
		component.RuntimeBody = body;
		transform.RuntimeBody = body;
	}

	void Physics2D::CreateBoxColliderFixture(Entity entity, BoxCollider2DComponent& component)
	{
		auto& transform = entity.GetComponent<TransformComponent>();
		auto& sprite = entity.GetComponent<SpriteComponent>();

		b2PolygonShape boxShape;
		boxShape.SetAsBox(
			sprite.GetTextureSize().x * 0.5f,
			sprite.GetTextureSize().y * 0.5f,
			b2Vec2(component.Offset.x, component.Offset.y),
			DEG_TO_RAD(transform.GetRotation())
		);

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &boxShape;
		fixtureDef.density = component.Density;
		fixtureDef.friction = component.Friction;
		fixtureDef.restitution = component.Restitution;
		fixtureDef.restitutionThreshold = component.RestitutionThreshold;

		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		b2Fixture* fixture = static_cast<b2Body*>(rb2d.RuntimeBody)->CreateFixture(&fixtureDef);
		component.RuntimeFixture = fixture;
	}

	void Physics2D::UpdateBoxColliderFixture(BoxCollider2DComponent& bc2d, TransformComponent& transform, SpriteComponent& sprite)
	{
		if (!bc2d.RuntimeFixture) return;

		b2Fixture* fixture = static_cast<b2Fixture*>(bc2d.RuntimeFixture);

		// Create a new shape
		b2PolygonShape boxShape;
		boxShape.SetAsBox(
			sprite.GetTextureSize().x * 0.5f,
			sprite.GetTextureSize().y * 0.5f,
			b2Vec2(bc2d.Offset.x, bc2d.Offset.y),
			DEG_TO_RAD(transform.GetRotation())
		);

		// Create a new fixture definition
		b2FixtureDef fixtureDef;
		fixtureDef.shape = &boxShape;
		fixtureDef.density = fixture->GetDensity();
		fixtureDef.friction = fixture->GetFriction();
		fixtureDef.restitution = fixture->GetRestitution();
		fixtureDef.filter = fixture->GetFilterData();

		// Destroy the old fixture and create a new one with the modified shape
		b2Body* rb2d = fixture->GetBody();
		rb2d->DestroyFixture(fixture);
		bc2d.RuntimeFixture = rb2d->CreateFixture(&fixtureDef);
	}

	void Physics2D::DestroyBoxColliderFixture(Entity entity, BoxCollider2DComponent& component)
	{
		b2Fixture* fixture = static_cast<b2Fixture*>(component.RuntimeFixture);
		b2Body* rb2d = fixture->GetBody();
		rb2d->DestroyFixture(fixture);
		component.RuntimeFixture = nullptr;
	}

}