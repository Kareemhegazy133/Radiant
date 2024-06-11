#pragma once

#include <Engine.h>

#include "Player.h"

using namespace Engine;

class Level : public World
{

public:
    Level(sf::RenderWindow* renderWindow);
    ~Level();

    void OnUpdate(Timestep ts) override;
    void OnRender();

    void OnEvent(Event& e);

private:
    Player m_Player;
};

