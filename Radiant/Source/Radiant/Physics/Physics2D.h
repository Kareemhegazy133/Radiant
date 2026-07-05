#pragma once

#include "Radiant/Core/Timestep.h"

#include "CollisionListener2D.h"

#include "Radiant/ECS/Level.h"
#include "Radiant/ECS/Entity.h"
#include "Radiant/ECS/Components.h"

class b2World;

namespace Radiant {

	class Physics2D
	{
	public:
		static void Init(Level* level);
		static void Shutdown();

		static void CreatePhysicsBody(Entity& entity, RigidBody2DComponent& component);
		static void DestroyPhysicsBody(Entity& entity, RigidBody2DComponent& component);
		static void CreateBoxColliderFixture(Entity& entity, BoxCollider2DComponent& component);

		static void OnUpdate(Timestep ts);

		static void SubmitEntitiesTransforms(Entity& entity);
		static void UpdateEntitiesTransforms(Entity& entity);

		// For Debugging Purposes
		// This function must be called after Renderer2D::BeginScene()
		static void DebugDraw(TransformComponent& transform, BoxCollider2DComponent& bc2d);
	
	private:
		struct Physics2DData
		{
			Level* LevelPtr = nullptr;
			b2World* B2DWorld = nullptr;

			CollisionListener2D* CollisionListener = nullptr;
		};
		
		static Scope<Physics2DData> s_Physics2DData;
	};
}