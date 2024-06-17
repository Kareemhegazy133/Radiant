#pragma once

#include <Engine.h>

#include "Entities/Player.h"

using namespace Engine;

class Level : public World
{

public:
    Level();
    ~Level();

    void OnUpdate(Timestep ts) override;
    void OnRender();

    void OnEvent(Event& e);

private:
    Player m_Player;
};

