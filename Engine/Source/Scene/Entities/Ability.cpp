#include "Enginepch.h"

#include "Core/GameApplication.h"

#include "Ability.h"

namespace Engine {

    Ability::Ability(const std::string& entityName)
        : Entity(Scene::GetScene().CreateGameObject(entityName), &Scene::GetScene())
    {

    }

    Ability::~Ability()
    {

    }

    void Ability::CreatePhysicsBoxCollider()
    {
        AddComponent<BoxCollider2DComponent>();
    }

    void Ability::DestroyPhysicsBoxCollider()
    {
        RemoveComponent<BoxCollider2DComponent>();
    }

    bool Ability::OnCooldown() const
    {
        float elapsedTime = GameApplication::Get().SFMLGetTime() - ability.LastActivatedTime;
        return elapsedTime < ability.Cooldown;
    }

    void Ability::OnCollisionBegin(Entity& other)
    {

    }

    void Ability::OnCollisionEnd(Entity& other)
    {

    }

}


