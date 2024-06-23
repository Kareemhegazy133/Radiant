#pragma once

#include "GameplayEntity.h"

namespace Engine {

    class Character : public GameplayEntity
    {
    public:
        Character(const std::string& entityName, const std::string& textureIdentifier, const sf::Vector2f& position);

        virtual ~Character();

        virtual void OnUpdate(Timestep ts) = 0;

        virtual void OnCollisionBegin(GameplayEntity& other) = 0;
        virtual void OnCollisionEnd(GameplayEntity& other) = 0;

    protected:
        MetadataComponent& metadata = GetComponent<MetadataComponent>();
        TransformComponent& transform = GetComponent<TransformComponent>();
        CharacterComponent& character = AddComponent<CharacterComponent>();
        SpriteComponent& sprite;
        AnimationComponent& animation = AddComponent<AnimationComponent>();

        AttributesComponent& attributes = AddComponent<AttributesComponent>();
        AbilitiesComponent& abilities = AddComponent<AbilitiesComponent>();
    };

}
