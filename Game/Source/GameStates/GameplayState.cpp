#include "GameplayState.h"

#include "Layers/GameLayer.h"

void GameplayState::OnExit()
{
    // Destroy the level if transitioning to MainMenuState
    if (m_GameLayer->GetNextState() == m_GameLayer->GetMainMenuState())
    {
        m_Level.reset();
    }
}

bool GameplayState::OnKeyPressed(KeyPressedEvent& e)
{
    if (e.GetKeyCode() == Key::Escape)
    {
        m_GameLayer->PushState(m_GameLayer->GetGamePausedState());
    }
    return true;
}