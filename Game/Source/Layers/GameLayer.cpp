#include "GameLayer.h"

#include "Levels/MainMenuLevel.h"
#include "Levels/GameLevel.h"

// TODO: Serialize and Deserialize the game layer

using namespace Engine;

GameLayer* GameLayer::s_Instance = nullptr;

GameLayer::GameLayer() : Layer("GameLayer")
{
    ENGINE_ASSERT(!s_Instance, "GameLayer already exists!");
    s_Instance = this;
}

void GameLayer::OnAttach()
{
    SetGameState(GameState::MainMenu);
}

void GameLayer::OnDetach()
{

}

void GameLayer::OnUpdate(Timestep ts)
{
    if (m_CurrentLevel)
    {
        if (m_CurrentState == GameState::Paused)
        {
            GameLevel* gameLevel = static_cast<GameLevel*>(m_CurrentLevel.get());
            gameLevel->UpdatePauseMenu(ts);
            return;
        }

        m_CurrentLevel->OnUpdate(ts);
        m_CurrentLevel->OnRender();
    }
}


void GameLayer::OnEvent(Event& e)
{
    m_CurrentLevel->OnEvent(e);
}

void GameLayer::SetGameStateInternal(GameState newState)
{
    switch (newState)
    {
    case GameState::MainMenu:
        m_CurrentLevel = CreateScope<MainMenuLevel>();
        m_CurrentState = GameState::MainMenu;
        break;
    case GameState::Playing:
        if (m_CurrentState == GameState::MainMenu)
        {
            m_CurrentLevel.reset();
            m_CurrentLevel = CreateScope<GameLevel>();
            m_CurrentState = GameState::Playing;
        }
        else if (m_CurrentState == GameState::Paused)
        {
            m_CurrentState = GameState::Playing;
        }
        break;
    case GameState::Paused:
        m_CurrentState = GameState::Paused;
        break;
    }
}
