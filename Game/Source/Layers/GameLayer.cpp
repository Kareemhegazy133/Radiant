#include "GameLayer.h"
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
	m_GameLevel = CreateRef<GameLevel>();
    m_MainMenuLevel = CreateRef<MainMenuLevel>();
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(Timestep ts)
{
    switch (m_CurrentState)
    {
    case GameState::MainMenu:

        break;
    case GameState::Loading:
        
        break;
    case GameState::Playing:
        m_GameLevel->OnUpdate(ts);
        m_GameLevel->OnRender();
        break;
    case GameState::Paused:

        break;

    }
}


void GameLayer::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(ENGINE_BIND_EVENT_FN(GameLayer::OnKeyPressed));
    if (e.Handled) return;

    m_GameLevel->OnEvent(e);
}

bool GameLayer::OnKeyPressed(KeyPressedEvent& e)
{
    if (e.GetKeyCode() == Key::Escape)
    {
        if (m_CurrentState == GameState::Playing)
            m_CurrentState = GameState::Paused;
        else if (m_CurrentState == GameState::Paused)
            m_CurrentState = GameState::Playing;

        return true;
    }

    return false;
}
