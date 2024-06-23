#include "Enginepch.h"

#include "Core/GameApplication.h"

#include "Ability.h"

namespace Engine {

    Ability::Ability(const std::string& entityName, const std::string& textureIdentifier)
        : GameplayEntity(World::GetWorld().CreateEntity(entityName), &World::GetWorld()), sprite(AddComponent<SpriteComponent>(textureIdentifier))
    {

    }

    Ability::~Ability()
    {

    }

    bool Ability::OnCooldown() const
    {
        float elapsedTime = GameApplication::Get().SFMLGetTime() - ability.LastActivatedTime;
        return elapsedTime < ability.Cooldown;
    }

    void Ability::OnCollisionBegin(GameplayEntity& other)
    {

    }

    void Ability::OnCollisionEnd(GameplayEntity& other)
    {

    }

}


