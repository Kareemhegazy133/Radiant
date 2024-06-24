#pragma once

#include <Engine.h>

using namespace Engine;

class Fireball : public Ability
{

public:

    Fireball();
    ~Fireball();

    void Activate(Entity& caster) override;
    void Deactivate() override;

    void OnUpdate(Timestep ts) override;

    void OnCollisionBegin(Entity& other) override;
    void OnCollisionEnd(Entity& other) override;

private:
    Rigidbody2DComponent& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Kinematic);

    sf::Vector2f m_SocketOffset = { 75.f, 50.f };
    float m_ActiveDuration = 0.0f;
};