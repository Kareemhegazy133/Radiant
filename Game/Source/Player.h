#pragma once

#include <Engine.h>

using namespace Engine;

class Player : public Character
{

public:

    Player();
    ~Player();

    void OnUpdate(Timestep ts) override;

    void OnCollisionBegin(const Entity& other) override;
    void OnCollisionEnd(const Entity& other) override;
};