#pragma once

#include "Entity.h"

namespace Engine {

	class Ability : public Entity
	{
	public:
		Ability(const std::string& entityName);
		virtual ~Ability();

		virtual void Activate(Entity& caster) = 0;
		virtual void Deactivate() = 0;

		virtual void CreatePhysicsBoxCollider();
		virtual void DestroyPhysicsBoxCollider();

		bool OnCooldown() const;

		virtual void OnUpdate(Timestep ts) = 0;

		virtual void OnCollisionBegin(Entity& other) override;
		virtual void OnCollisionEnd(Entity& other) override;

	protected:
		MetadataComponent& metadata = GetComponent<MetadataComponent>();
		TransformComponent& transform = GetComponent<TransformComponent>();
		AbilityComponent& ability = AddComponent<AbilityComponent>();
		SpriteComponent& sprite = AddComponent<SpriteComponent>();
		AnimationComponent& animation = AddComponent<AnimationComponent>();
	};
}
