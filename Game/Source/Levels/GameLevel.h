#pragma once

#include <Engine.h>

#include "Menus/PauseMenu.h"

#include "Gameplay/Entities/Player.h"

using namespace Engine;

class GameLevel : public Level
{

public:

    GameLevel();
    ~GameLevel();

    void OnUpdate(Timestep ts) override;
    void OnRender() override;

    inline void UpdatePauseMenu(Timestep ts) { m_PauseMenu.OnUpdate(ts); }

    // TODO: Maybe add this function's implementation to Level.h with templates
    void OnEvent(Event& e) override;

private:
    // TODO: Maybe add this to Level.h
    bool OnKeyPressed(KeyPressedEvent& e);

private:
    Physics2D m_Physics;

private:
    Player m_Player;

    PauseMenu m_PauseMenu;
};

