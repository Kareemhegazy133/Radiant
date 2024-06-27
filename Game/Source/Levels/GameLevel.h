#pragma once

#include <Engine.h>

#include "Gameplay/Entities/Player.h"

using namespace Engine;

class GameLevel : public Level
{

public:

    GameLevel();
    ~GameLevel();

    void OnUpdate(Timestep ts) override;
    void OnRender() override;

    void OnEvent(Event& e) override;

private:
    Physics2D m_Physics;

private:
    Player m_Player;

};

