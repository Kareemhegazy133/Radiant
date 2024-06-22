#pragma once

#include "Core/Timestep.h"

#include "CollisionListener.h"

#include "World/Components.h"

class b2World;

namespace Engine {

	class Entity;

	class Physics2D
	{
	public:
		
		Physics2D();
		~Physics2D();

		void OnUpdate(Timestep ts);

		void CreatePhysicsBody(Entity entity, Rigidbody2DComponent& component);
		void CreateBoxColliderFixture(Entity entity, BoxCollider2DComponent& component);
		void UpdateBoxColliderFixture(BoxCollider2DComponent& bc2d, TransformComponent& transform, SpriteComponent& sprite);

		inline static Physics2D& Get() { return *s_Instance; }

	private:
		static Physics2D* s_Instance;
		b2World* m_PhysicsWorld;

		CollisionListener m_CollisionListener;
	};
}