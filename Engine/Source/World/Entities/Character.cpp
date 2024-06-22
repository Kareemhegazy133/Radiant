#include "Enginepch.h"

#include "Character.h"

namespace Engine {

    Character::Character(const std::string& entityName, const std::string& textureIdentifier, const sf::Vector2f& position)
        : Entity(World::GetWorld().CreateEntity(entityName), &World::GetWorld()), sprite(AddComponent<SpriteComponent>(textureIdentifier))
    {
        transform.setPosition(position);
    }

    Character::~Character()
    {

    }

    void Character::OnCollisionBegin(Entity& other)
    {

    }

    void Character::OnCollisionEnd(Entity& other)
    {

    }

}