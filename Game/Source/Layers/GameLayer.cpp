#include "GameLayer.h"

#include "Levels/MainMenuLevel.h"
#include "Levels/GameLevel.h"

// TODO: Serialize and Deserialize the game layer

using namespace Engine;

GameLayer::GameLayer() : Layer("GameLayer")
{
	GameApplication::Get().GetTextureManager().loadTexture("Player", "Assets/Player/Reaper/Reaper_Idle.png");
	GameApplication::Get().GetTextureManager().loadTexture("Fireball", "Assets/Abilities/Fireball.png");
	GameApplication::Get().GetTextureManager().loadTexture("Platform", "Assets/Platform.png");

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
        m_CurrentLevel->OnUpdate(ts);
        m_CurrentLevel->OnRender();
    }
}


void GameLayer::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(ENGINE_BIND_EVENT_FN(GameLayer::OnKeyPressed));
    if (e.Handled) return;

    m_CurrentLevel->OnEvent(e);
}

void GameLayer::SetGameState(GameState newState)
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
        break;
    }
}

bool GameLayer::OnKeyPressed(KeyPressedEvent& e)
{
    if (e.GetKeyCode() == Key::Escape)
    {
        SetGameState(GameState::Paused);
        return true;
    }
    else if (e.GetKeyCode() == Key::X)
    {
        SetGameState(GameState::Playing);
        return true;
    }

    return false;
}
