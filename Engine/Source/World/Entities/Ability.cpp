#include "Enginepch.h"

#include "Ability.h"

namespace Engine {

    Ability::Ability(const std::string& entityName, const std::string& textureIdentifier)
        : Entity(World::GetWorld().CreateEntity(entityName), &World::GetWorld()), m_textureIdentifier(textureIdentifier)
    {
        AddComponent<SpriteComponent>(m_textureIdentifier);
    }

    Ability::~Ability()
    {

    }

    void Ability::OnCollisionBegin(Entity& other)
    {

    }

    void Ability::OnCollisionEnd(Entity& other)
    {

    }

}


