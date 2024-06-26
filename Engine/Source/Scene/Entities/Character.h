#pragma once

#include "Entity.h"

namespace Engine {

    class Character : public Entity
    {
    public:
        Character(const std::string& entityName, const std::string& textureIdentifier, const sf::Vector2f& position);

        virtual ~Character();

        virtual void OnUpdate(Timestep ts) = 0;

        virtual void OnCollisionBegin(Entity& other) = 0;
        virtual void OnCollisionEnd(Entity& other) = 0;

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