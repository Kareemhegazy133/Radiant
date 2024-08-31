#pragma once

#include "Core/Timestep.h"

#include "CollisionListener.h"
#include "DebugDraw.h"

#include "ECS/Components.h"

class b2World;

namespace Engine {

	class Entity;

	class Physics2D
	{
	public:
		
		Physics2D(Level* level);
		~Physics2D();

		void OnUpdate(Timestep ts);

		void CreatePhysicsBody(Entity entity, Rigidbody2DComponent& component);

		void CreateBoxColliderFixture(Entity entity, BoxCollider2DComponent& component);
		void UpdateBoxColliderFixture(BoxCollider2DComponent& bc2d, TransformComponent& transform, SpriteComponent& sprite);
		void DestroyBoxColliderFixture(Entity entity, BoxCollider2DComponent& component);

	private:
		Level* m_Level = nullptr;
		b2World* m_PhysicsWorld = nullptr;

		CollisionListener m_CollisionListener;

		// For debugging purposes
		DebugDraw m_DebugDraw;
	};
}