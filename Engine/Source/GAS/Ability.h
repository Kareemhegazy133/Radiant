#pragma once

#include "Enginepch.h"

#include "Core/GameApplication.h"

#include "ECS/ScriptableEntity.h"
#include "ECS/Components.h"

namespace Engine {

	class Ability : public ScriptableEntity
	{
	public:

		virtual void OnCreate() = 0;
		virtual void OnUpdate(Timestep ts) = 0;
		virtual void OnDestroy() = 0;

		virtual void OnCollisionBegin(Entity& other) = 0;
		virtual void OnCollisionEnd(Entity& other) = 0;

		virtual void Activate() = 0;
		virtual void Deactivate() = 0;

		void CreatePhysicsBoxCollider()
		{
			AddComponent<BoxCollider2DComponent>();
		}

		void DestroyPhysicsBoxCollider()
		{
			RemoveComponent<BoxCollider2DComponent>();
		}

		bool OnCooldown() const
		{
			float elapsedTime = GameApplication::Get().SFMLGetTime() - LastActivatedTime;
			return elapsedTime < Cooldown;
		}

	public:
		ScriptableEntity* Caster = nullptr;
		float Speed;

		float MaxDuration;
		float Cooldown;
		float LastActivatedTime;

	protected:
		MetadataComponent& metadata = GetComponent<MetadataComponent>();
		TransformComponent& transform = GetComponent<TransformComponent>();
		SpriteComponent& sprite = AddComponent<SpriteComponent>();
		AnimationComponent& animation = AddComponent<AnimationComponent>();
	};
}
