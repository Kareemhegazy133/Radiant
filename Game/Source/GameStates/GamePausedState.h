#pragma once

#include <Engine.h>

#include "Menus/PauseMenu.h"

using namespace Engine;

class GameLayer;

class GamePausedState : public GameState
{
public:
    GamePausedState(GameLayer* gameLayer)
        : m_PauseMenuScreen(gameLayer)
    {
    }

    void OnEnter() override
    {
        m_PauseMenuScreen.SetVisibility(true);
    }

    void OnExit() override
    {
        m_PauseMenuScreen.SetVisibility(false);
    }

    void OnUpdate(Timestep ts) override
    {
        m_PauseMenuScreen.OnRender();
    }

    void OnEvent(Event& e) override
    {
        m_PauseMenuScreen.OnEvent(e);
    }

private:
    PauseMenu m_PauseMenuScreen;
};
