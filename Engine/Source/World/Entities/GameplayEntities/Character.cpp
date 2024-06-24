#include "Enginepch.h"

#include "Character.h"

namespace Engine {

    Character::Character(const std::string& entityName, const std::string& textureIdentifier, const sf::Vector2f& position)
        : GameplayEntity(World::GetWorld().CreateEntity(entityName), &World::GetWorld()), sprite(AddComponent<SpriteComponent>(textureIdentifier))
    {
        transform.SetPosition(position);
    }

    Character::~Character()
    {

    }

}