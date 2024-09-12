#pragma once

#include <Engine.h>

using namespace Engine;

enum class AbilityState
{
    Active = 0
};

class Fireball : public Ability
{

public:
    Fireball(ScriptableEntity* caster)
    {
        Caster = caster;
    }

    void OnCreate() override;
    void OnUpdate(Timestep ts) override;
    void OnDestroy() override;

    void Activate() override;
    void Deactivate() override;

    void OnCollisionBegin(Entity& other) override;
    void OnCollisionEnd(Entity& other) override;

private:

    sf::Vector2f m_SocketOffset = { 75.f, 50.f };
    float m_ActiveDuration = 0.0f;
    sf::Vector2f m_Direction;
};