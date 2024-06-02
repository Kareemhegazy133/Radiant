#include "Enginepch.h"

#include "Character.h"

namespace Engine {

    Character::Character(const std::string& entityName, const std::string& textureIdentifier, const sf::Vector2f& position)
        : Entity(World::GetWorld().CreateEntity(entityName), &World::GetWorld()), m_textureIdentifier(textureIdentifier), m_Position(position)
    {
        auto& transform = GetComponent<TransformComponent>();
        transform.setPosition(m_Position);

        AddComponent<SpriteComponent>(m_textureIdentifier);
    }

    Character::~Character()
    {

    }

}