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
    SetGameState(GameState::MainMenu);
}

void GameLayer::OnDetach()
{
    m_CurrentLevel.reset();
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
    m_CurrentState = newState;
    m_CurrentLevel.reset();

    switch (m_CurrentState)
    {
    case GameState::MainMenu:
        m_CurrentLevel = CreateScope<MainMenuLevel>();
        break;
    case GameState::Loading:

        break;
    case GameState::Playing:
        m_CurrentLevel = CreateScope<GameLevel>();
        break;
    case GameState::Paused:

        break;

    }
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
    else if (e.GetKeyCode() == Key::X)
    {
        if (m_CurrentState == GameState::MainMenu)
            SetGameState(GameState::Playing);
        else if (m_CurrentState == GameState::Playing)
            SetGameState(GameState::MainMenu);

        return true;
    }

    return false;
}
