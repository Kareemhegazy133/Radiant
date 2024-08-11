#pragma once

#include "Core/Timestep.h"

#include "CollisionListener.h"
#include "DebugDraw.h"

#include "Scene/Components.h"

class b2World;

namespace Engine {

	class GameObject;

	class Physics2D
	{
	public:
		
		Physics2D();
		~Physics2D();

		void OnUpdate(Timestep ts);

		void CreatePhysicsBody(GameObject gameObject, Rigidbody2DComponent& component);

		void CreateBoxColliderFixture(GameObject gameObject, BoxCollider2DComponent& component);
		void UpdateBoxColliderFixture(BoxCollider2DComponent& bc2d, TransformComponent& transform, SpriteComponent& sprite);
		void DestroyBoxColliderFixture(GameObject gameObject, BoxCollider2DComponent& component);

		inline static Physics2D& Get() { return *s_Instance; }

	private:
		static Physics2D* s_Instance;
		b2World* m_PhysicsWorld;

		CollisionListener m_CollisionListener;

		// For debugging purposes
		DebugDraw m_DebugDraw;
	};
}