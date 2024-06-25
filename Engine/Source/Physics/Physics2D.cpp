#include "Enginepch.h"

#include "box2d/b2_world.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"

#include "Scene/GameObject.h"

#include "Physics2D.h"

namespace Engine {

	Physics2D* Physics2D::s_Instance = nullptr;

	Physics2D::Physics2D()
		: m_PhysicsWorld(new b2World({ 0.0f, 9.8f }))
	{
		ENGINE_ASSERT(!s_Instance, "Physics System already exists!");
		s_Instance = this;

		m_PhysicsWorld->SetContactListener(&m_CollisionListener);
		m_PhysicsWorld->SetContactFilter(&m_CollisionListener);
	}

	Physics2D::~Physics2D()
	{
		delete m_PhysicsWorld;
	}

	void Physics2D::OnUpdate(Timestep ts)
	{
		const int32_t velocityIterations = 6;
		const int32_t positionIterations = 3;
		m_PhysicsWorld->Step(ts, velocityIterations, positionIterations);
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

	void Physics2D::CreatePhysicsBody(GameObject gameObject, Rigidbody2DComponent& component)
	{
		auto& transform = gameObject.GetComponent<TransformComponent>();

		b2BodyDef bodyDef;
		bodyDef.type = Rigidbody2DTypeToBox2DBody(component.Type);
		bodyDef.position.Set(transform.GetPosition().x, transform.GetPosition().y);
		bodyDef.angle = DEG_TO_RAD(transform.GetRotation());

		bodyDef.userData.pointer = static_cast<uintptr_t>(uint32_t(gameObject));

		b2Body* body = m_PhysicsWorld->CreateBody(&bodyDef);
		body->SetFixedRotation(component.FixedRotation);
		component.RuntimeBody = body;
		transform.RuntimeBody = body;
	}

	void Physics2D::CreateBoxColliderFixture(GameObject gameObject, BoxCollider2DComponent& component)
	{
		auto& transform = gameObject.GetComponent<TransformComponent>();
		auto& sprite = gameObject.GetComponent<SpriteComponent>();

		b2PolygonShape boxShape;
		// Offsetting the Center because SFML to Box2d Coordinate System's Y Axis is Mirrored
		boxShape.SetAsBox(
			sprite.GetTextureSize().x * transform.GetScale().x * 0.5f,
			sprite.GetTextureSize().y * transform.GetScale().y * 0.5f,
			b2Vec2(sprite.GetTextureSize().x * 0.5f, sprite.GetTextureSize().y * 0.5f), DEG_TO_RAD(transform.GetRotation()));

		/*ENGINE_INFO("Sprite x: {0}, y: {1}, transform x: {2}, y: {3}, Rect x: {4}, y: {5}",
			sprite.GetTextureSize().x,
			sprite.GetTextureSize().y,
			transform.GetScale().x,
			transform.GetScale().y,
			sprite.GetTextureSize().x * transform.GetScale().x * 0.5f,
			sprite.GetTextureSize().y * transform.GetScale().y * 0.5f);*/

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &boxShape;
		fixtureDef.density = component.Density;
		fixtureDef.friction = component.Friction;
		fixtureDef.restitution = component.Restitution;
		fixtureDef.restitutionThreshold = component.RestitutionThreshold;

		auto& rb2d = gameObject.GetComponent<Rigidbody2DComponent>();
		b2Fixture* fixture = static_cast<b2Body*>(rb2d.RuntimeBody)->CreateFixture(&fixtureDef);
		component.RuntimeFixture = fixture;
	}

	void Physics2D::UpdateBoxColliderFixture(BoxCollider2DComponent& bc2d, TransformComponent& transform, SpriteComponent& sprite)
	{
		b2Fixture* fixture = static_cast<b2Fixture*>(bc2d.RuntimeFixture);

		// Create a new shape
		b2PolygonShape boxShape;
		// Offsetting the Center because SFML to Box2d Coordinate System's Y Axis is Mirrored
		boxShape.SetAsBox(
			sprite.GetTextureSize().x * transform.GetScale().x * 0.5f,
			sprite.GetTextureSize().y * transform.GetScale().y * 0.5f,
			b2Vec2(sprite.GetTextureSize().x * 0.5f, sprite.GetTextureSize().y * 0.5f), DEG_TO_RAD(transform.GetRotation()));

		//ENGINE_INFO("Centroid x: {0}, y: {1}", boxShape.m_centroid.x, boxShape.m_centroid.y);

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
}