#include "Enginepch.h"

#include "Character.h"

namespace Engine {

    Character::Character(const std::string& entityName)
        : Entity(World::GetWorld().CreateEntity(entityName), &World::GetWorld())
    {

    }

    Character::~Character()
    {

    }

	/*Character::Character(const std::string& entityName, const std::string& textureIdentifier, const sf::Vector2f& position)
        : Entity(World::GetWorld().CreateEntity(entityName), &World::GetWorld())
	{
        auto& transform = AddComponent<TransformComponent>();
        transform.setPosition(position.x, position.y);
        AddComponent<SpriteComponent>(textureIdentifier);
        AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
        AddComponent<BoxCollider2DComponent>();
	}*/
}