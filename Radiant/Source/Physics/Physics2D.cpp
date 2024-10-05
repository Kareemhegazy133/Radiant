#include "rdpch.h"
#include "Physics2D.h"

#include <box2d/b2_world.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>

#include "Renderer/Renderer2D.h"

namespace Radiant {

	Scope<Physics2D::Physics2DData> Physics2D::s_Physics2DData = CreateScope<Physics2D::Physics2DData>();

	static b2BodyType RigidBody2DTypeToBox2DBody(RigidBody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
			case RigidBody2DComponent::BodyType::Static:    return b2_staticBody;
			case RigidBody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
			case RigidBody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
		}

		RADIANT_ASSERT(false, "Unknown body type");
		return b2_staticBody;
	}

	void Physics2D::Init(Level* level)
	{
		RADIANT_TRACE("Physics System Initializer");

		s_Physics2DData->LevelPtr = level;
		s_Physics2DData->B2DWorld = new b2World({ 0.0f, -9.8f });
		s_Physics2DData->CollisionListener = new CollisionListener2D(level);

		s_Physics2DData->B2DWorld->SetContactListener(s_Physics2DData->CollisionListener);
		s_Physics2DData->B2DWorld->SetContactFilter(s_Physics2DData->CollisionListener);

		RADIANT_TRACE("Physics System Initialized");
	}

	void Physics2D::Shutdown()
	{
		RADIANT_TRACE("Physics System (Shutdown)\"Destructor\"");

		delete s_Physics2DData->B2DWorld;
		s_Physics2DData->B2DWorld = nullptr;

		delete s_Physics2DData->CollisionListener;
		s_Physics2DData->CollisionListener = nullptr;

		RADIANT_TRACE("Physics System Shutdown");
	}

	void Physics2D::CreatePhysicsBody(Entity& entity, RigidBody2DComponent& component)
	{
		auto& transform = entity.GetComponent<TransformComponent>();

		b2BodyDef bodyDef;
		bodyDef.type = RigidBody2DTypeToBox2DBody(component.Type);
		bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
		bodyDef.angle = transform.Rotation.z;

		bodyDef.userData.pointer = static_cast<uintptr_t>(entity.GetUUID());

		b2Body* body = s_Physics2DData->B2DWorld->CreateBody(&bodyDef);
		body->SetFixedRotation(component.FixedRotation);
		component.RuntimeBody = body;
	}

	void Physics2D::DestroyPhysicsBody(Entity& entity, RigidBody2DComponent& component)
	{
		b2Body* body = static_cast<b2Body*>(component.RuntimeBody);
		s_Physics2DData->B2DWorld->DestroyBody(body);
		component.RuntimeBody = nullptr;
	}

	void Physics2D::CreateBoxColliderFixture(Entity& entity, BoxCollider2DComponent& component)
	{
		auto& transform = entity.GetComponent<TransformComponent>();
		auto& sprite = entity.GetComponent<SpriteComponent>();

		b2PolygonShape boxShape;
		boxShape.SetAsBox(
			component.Size.x * transform.Scale.x,
			component.Size.y * transform.Scale.y,
			b2Vec2(component.Offset.x, component.Offset.y),
			transform.Translation.z);

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &boxShape;
		fixtureDef.density = component.Density;
		fixtureDef.friction = component.Friction;
		fixtureDef.restitution = component.Restitution;
		fixtureDef.restitutionThreshold = component.RestitutionThreshold;

		if (auto* rb2d = entity.TryGetComponent<RigidBody2DComponent>())
		{
			static_cast<b2Body*>(rb2d->RuntimeBody)->CreateFixture(&fixtureDef);
		}
		else
		{
			RADIANT_ASSERT("Entity must have a RigidBody2DComponent to create a box collider fixture");
		}
	}

	void Physics2D::OnUpdate(Timestep ts)
	{
		RADIANT_PROFILE_FUNCTION();

		const int32_t velocityIterations = 6;
		const int32_t positionIterations = 2;
		s_Physics2DData->B2DWorld->Step(ts, velocityIterations, positionIterations);
	}

	void Physics2D::SubmitEntitiesTransforms(Entity& entity)
	{
		RADIANT_PROFILE_FUNCTION();

		auto& transform = entity.GetComponent<TransformComponent>();
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();

		RADIANT_ASSERT(rb2d.RuntimeBody, "Entity must have a RigidBody RuntimeBody to Submit Transform for physics.");

		b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);
		body->SetTransform(b2Vec2(transform.Translation.x, transform.Translation.y), transform.Rotation.z);

		if (auto* bc2d = entity.TryGetComponent<BoxCollider2DComponent>())
		{
			b2PolygonShape boxShape;
			boxShape.SetAsBox(
				bc2d->Size.x * transform.Scale.x,
				bc2d->Size.y * transform.Scale.y,
				b2Vec2(bc2d->Offset.x, bc2d->Offset.y),
				transform.Rotation.z
			);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &boxShape;
			fixtureDef.density = bc2d->Density;
			fixtureDef.friction = bc2d->Friction;
			fixtureDef.restitution = bc2d->Restitution;
			fixtureDef.restitutionThreshold = bc2d->RestitutionThreshold;

			body->DestroyFixture(body->GetFixtureList());
			body->CreateFixture(&fixtureDef);
		}
	}

	void Physics2D::UpdateEntitiesTransforms(Entity& entity)
	{
		RADIANT_PROFILE_FUNCTION();

		auto& transform = entity.GetComponent<TransformComponent>();
		auto* rb2d = entity.TryGetComponent<RigidBody2DComponent>();
		if (!rb2d)
		{
			RADIANT_WARN("Cannot Update Entity: {0}'s Transform does not have a RigidBody", entity.GetComponent<MetadataComponent>().Tag);
			return;
		}

		b2Body* body = static_cast<b2Body*>(rb2d->RuntimeBody);

		const auto& position = body->GetPosition();
		transform.Translation.x = position.x;
		transform.Translation.y = position.y;
		transform.Rotation.z = body->GetAngle();
	}

	void Physics2D::DebugDraw(TransformComponent& tc, BoxCollider2DComponent& bc2d)
	{
		// Box Colliders
		{
			glm::vec3 translation = tc.Translation + glm::vec3(bc2d.Offset, 0.001f);
			glm::vec3 scale = tc.Scale * glm::vec3(bc2d.Size * 2.0f, 1.0f);

			glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
				* glm::rotate(glm::mat4(1.0f), tc.Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f))
				* glm::scale(glm::mat4(1.0f), scale);

			Renderer2D::DrawRect(transform, glm::vec4(1.0f, 0.5f, 0.7f, 1.0f));
		}
	}

}