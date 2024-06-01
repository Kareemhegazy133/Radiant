#pragma once

#include <Engine.h>

using namespace Engine;

class Player : public Character
{

public:

    Player();
    ~Player();

    void OnUpdate(Timestep ts) override;

private:
    sf::Vector2f m_Position = { 300.f, 0.f };
};