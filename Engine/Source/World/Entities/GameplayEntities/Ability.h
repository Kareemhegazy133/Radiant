#pragma once

#include "GameplayEntity.h"

namespace Engine {

	class Ability : public GameplayEntity
	{
	public:
		Ability(const std::string& entityName, const std::string& textureIdentifier);

		virtual ~Ability();

		virtual void Activate(GameplayEntity& caster) = 0;
		virtual void Deactivate() = 0;

		bool OnCooldown() const;

		virtual void OnUpdate(Timestep ts) = 0;

		virtual void OnCollisionBegin(GameplayEntity& other) override;
		virtual void OnCollisionEnd(GameplayEntity& other) override;

	protected:
		MetadataComponent& metadata = GetComponent<MetadataComponent>();
		TransformComponent& transform = GetComponent<TransformComponent>();
		AbilityComponent& ability = AddComponent<AbilityComponent>();
		SpriteComponent& sprite;
		AnimationComponent& animation = AddComponent<AnimationComponent>();
	};
}
