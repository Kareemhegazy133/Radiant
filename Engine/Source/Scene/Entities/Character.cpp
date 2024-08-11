#include "Enginepch.h"

#include "Character.h"

namespace Engine {

    Character::Character(const std::string& entityName, const sf::Vector2f& position)
        : Entity(Scene::GetScene().CreateGameObject(entityName), &Scene::GetScene())
    {
        transform.SetPosition(position);
    }

    Character::~Character()
    {

    }

}