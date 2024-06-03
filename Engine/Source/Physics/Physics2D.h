#pragma once

#include "box2d/b2_world_callbacks.h"

#include "Core/Timestep.h"

#include "World/Components.h"

class b2World;

namespace Engine {

	class Entity;

	class Physics2D : public b2ContactListener
	{
	public:
		
		Physics2D();
		~Physics2D();

		void OnUpdate(Timestep ts);

		void CreatePhysicsBody(Entity entity, Rigidbody2DComponent& component);
		void CreateBoxColliderFixture(Entity entity, BoxCollider2DComponent& component);
		void UpdatePhysicsBody(Rigidbody2DComponent& rb2d, TransformComponent& transform);
		void UpdateBoxColliderFixture(BoxCollider2DComponent& bc2d, TransformComponent& transform, SpriteComponent& sprite);

		void UpdateEntityTransform(Rigidbody2DComponent& rb2d, TransformComponent& transform);

		void OnCollisionBegin(Entity entityA, Entity entityB);
		void OnCollisionEnd(Entity entityA, Entity entityB);

		inline static Physics2D& Get() { return *s_Instance; }

	private:
		void BeginContact(b2Contact* contact) override;
		void EndContact(b2Contact* contact) override;

	private:
		static Physics2D* s_Instance;
		b2World* m_PhysicsWorld;
	};
}