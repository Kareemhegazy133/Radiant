#pragma once

#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"

#include "World/Components.h"

namespace Engine {

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

	void CreatePhysicsBody(b2World* physicsWorld, Entity entity, Rigidbody2DComponent& component)
	{
		auto& transform = entity.GetComponent<TransformComponent>();

		b2BodyDef bodyDef;
		bodyDef.type = Rigidbody2DTypeToBox2DBody(component.Type);
		bodyDef.position.Set(transform.getPosition().x, transform.getPosition().y);
		bodyDef.angle = DEG_TO_RAD(transform.getRotation());

		b2Body* body = physicsWorld->CreateBody(&bodyDef);
		body->SetFixedRotation(component.FixedRotation);
		component.RuntimeBody = body;
	}

    void CreateBoxColliderFixture(Entity entity, BoxCollider2DComponent& component)
	{
		auto& transform = entity.GetComponent<TransformComponent>();
		auto& sprite = entity.GetComponent<SpriteComponent>();

		b2PolygonShape boxShape;
		// Offsetting the Center.y because SFML to Box2d Coordinate System's Y Axis is Mirrored
		boxShape.SetAsBox(
			sprite.getTextureSize().x * transform.getScale().x * 0.5f,
			sprite.getTextureSize().y * transform.getScale().y * 0.5f,
			b2Vec2(0.f, sprite.getTextureSize().y * 0.5f), DEG_TO_RAD(transform.getRotation()));

		/*ENGINE_INFO("Sprite x: {0}, y: {1}, transform x: {2}, y: {3}, Rect x: {4}, y: {5}",
			sprite.getTextureSize().x,
			sprite.getTextureSize().y,
			transform.getScale().x,
			transform.getScale().y,
			sprite.getTextureSize().x * transform.getScale().x * 0.5f,
			sprite.getTextureSize().y * transform.getScale().y * 0.5f);*/

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

	void UpdatePhysicsBody(Rigidbody2DComponent& rb2d, TransformComponent& transform)
	{
		b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);
		body->SetTransform(b2Vec2(transform.getPosition().x, transform.getPosition().y), DEG_TO_RAD(transform.getRotation()));
		/*ENGINE_INFO("Transform x: {0}, y: {1}, Body x: {0}, y: {1}",
			transform.getPosition().x,
			transform.getPosition().y,
			body->GetPosition().x,
			body->GetPosition().y
			);*/
	}

	void UpdateBoxColliderFixture(BoxCollider2DComponent& bc2d, TransformComponent& transform, SpriteComponent& sprite)
	{
		b2Fixture* fixture = static_cast<b2Fixture*>(bc2d.RuntimeFixture);

		// Create a new shape
		b2PolygonShape boxShape;
		// Offsetting the Center.y because SFML to Box2d Coordinate System's Y Axis is Mirrored
		boxShape.SetAsBox(
			sprite.getTextureSize().x * transform.getScale().x * 0.5f,
			sprite.getTextureSize().y * transform.getScale().y * 0.5f,
			b2Vec2(0.f, sprite.getTextureSize().y * 0.5f), DEG_TO_RAD(transform.getRotation()));

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