#pragma once

#include <Engine.h>

#include "Menus/MainMenu.h"

using namespace Engine;

class GameLayer;

class MainMenuState : public GameState
{
public:
    MainMenuState(GameLayer* gameLayer)
        : m_MainMenuScreen(gameLayer)
    {
    }

    void OnEnter() override
    {
        m_MainMenuScreen.SetVisibility(true);
    }

    void OnExit() override
    {
        m_MainMenuScreen.SetVisibility(false);
        
    }

    void OnUpdate(Timestep ts) override
    {
        m_MainMenuScreen.OnRender();
    }

    void OnEvent(Event& e) override
    {
        m_MainMenuScreen.OnEvent(e);
    }

private:
    MainMenu m_MainMenuScreen;
};
