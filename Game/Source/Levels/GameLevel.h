#pragma once

#include <Engine.h>

#include "Menus/CharacterInfoMenu.h"
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

    inline void RenderPauseMenu() { m_PauseMenu.OnRender(); }

    // TODO: Maybe add this function's implementation to Level.h with templates
    void OnEvent(Event& e) override;

private:
    // TODO: Maybe add this to Level.h
    bool OnKeyPressed(KeyPressedEvent& e);

private:
    Physics2D m_Physics;

private:
    Player m_Player;

    CharacterInfoMenu m_CharacterInfoMenu;
    PauseMenu m_PauseMenu;
};

