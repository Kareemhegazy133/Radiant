#include "Enginepch.h"

#include "Character.h"

namespace Engine {

    Character::Character(const std::string& entityName, const std::string& textureIdentifier, const sf::Vector2f& position)
        : Entity(World::GetWorld().CreateGameObject(entityName), &World::GetWorld()), sprite(AddComponent<SpriteComponent>(textureIdentifier))
    {
        transform.SetPosition(position);
    }

    Character::~Character()
    {

    }

}