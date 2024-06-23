#pragma once

#include <Engine.h>

using namespace Engine;

class Player : public Character
{

public:

    Player();
    ~Player();

    void OnUpdate(Timestep ts) override;

    void OnCollisionBegin(GameplayEntity& other) override;
    void OnCollisionEnd(GameplayEntity& other) override;

private:
    Rigidbody2DComponent& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);

};