#pragma once

#include <Engine.h>

#include "Menus/MainMenu.h"

using namespace Engine;

class MainMenuLevel : public Level
{

public:

    MainMenuLevel();
    ~MainMenuLevel();

    void OnUpdate(Timestep ts) override;
    void OnRender() override;

    void OnEvent(Event& e) override;

private:
    bool OnKeyPressed(KeyPressedEvent& e);

private:
    MainMenu m_MainMenu;
};

