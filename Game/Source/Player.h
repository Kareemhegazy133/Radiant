#pragma once

#include <Engine.h>

using namespace Engine;

class Player : public Character
{

public:

    Player();
    ~Player();

    void OnUpdate(Timestep ts) override;

    void OnCollisionBegin(Entity& other) override;
    void OnCollisionEnd(Entity& other) override;
};