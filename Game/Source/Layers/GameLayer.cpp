#include "GameLayer.h"

#include "GameTheme.h"

#include "Levels/MainMenuLevel.h"
#include "Levels/GameLevel.h"

// TODO: Serialize and Deserialize the game layer

using namespace Engine;

GameLayer* GameLayer::s_Instance = nullptr;

GameLayer::GameLayer() : Layer("GameLayer")
{
    ENGINE_ASSERT(!s_Instance, "GameLayer already exists!");
    s_Instance = this;
    ENGINE_INFO("GameLayer Init");

    ResourceManager::Init();
}

GameLayer::~GameLayer()
{
    ResourceManager::Shutdown();
    ENGINE_INFO("GameLayer Shutdown");
}

void GameLayer::OnAttach()
{
    LoadAssets();
    SetGameState(GameState::MainMenu);
}

void GameLayer::OnDetach()
{
    // TODO: UnloadAssets();
}

void GameLayer::OnUpdate(Timestep ts)
{
    if (m_CurrentLevel)
    {
        if (m_CurrentState == GameState::Paused)
        {
            GameLevel* gameLevel = static_cast<GameLevel*>(m_CurrentLevel.get());
            gameLevel->RenderPauseMenu();
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

void GameLayer::LoadAssets()
{
    // TODO: Load and Unload resources at proper times
    ResourceManager::LoadFont("Euljiro", "Assets/Font/Euljiro.ttf");

    ResourceManager::LoadTexture("PlayerIdle", "Assets/Player/Reaper/ReaperIdle.png");
    ResourceManager::LoadTexture("PlayerWalk", "Assets/Player/Reaper/ReaperWalk.png");
    ResourceManager::LoadTexture("PlayerThrow", "Assets/Player/Reaper/ReaperThrow.png");
    ResourceManager::LoadTexture("PlayerIcon", "Assets/Player/Reaper/ReaperIcon.png");

    ResourceManager::LoadTexture("ZombieIdle", "Assets/Enemies/Zombie/ZombieIdle.png");
    ResourceManager::LoadTexture("FireballActive", "Assets/Abilities/Fireball.png");
    ResourceManager::LoadTexture("Platform", "Assets/Platform.png");

    ResourceManager::LoadTexture("LargeButton", "Assets/UI/Buttons/LargeButton.png");
    ResourceManager::LoadTexture("LargeButtonHovered", "Assets/UI/Buttons/LargeButtonHovered.png");
    ResourceManager::LoadTexture("MediumButton", "Assets/UI/Buttons/MediumButton.png");
    ResourceManager::LoadTexture("MediumButtonHovered", "Assets/UI/Buttons/MediumButtonHovered.png");

    ResourceManager::LoadTexture("MainMenu", "Assets/UI/Menus/MainMenu.png");
    ResourceManager::LoadTexture("PauseMenu", "Assets/UI/Menus/PauseMenu.png");

    ResourceManager::LoadTexture("CharacterInfoMenu", "Assets/UI/Menus/CharacterInfoMenu.png");
    ResourceManager::LoadTexture("CloseButton", "Assets/UI/Buttons/CloseButton.png");
    ResourceManager::LoadTexture("CloseButtonHovered", "Assets/UI/Buttons/CloseButtonHovered.png");
    ResourceManager::LoadTexture("CommonButton", "Assets/UI/Buttons/CommonButton.png");
    ResourceManager::LoadTexture("CommonButtonHovered", "Assets/UI/Buttons/CommonButtonHovered.png");
    ResourceManager::LoadTexture("CommonButtonDisabled", "Assets/UI/Buttons/CommonButtonDisabled.png");
    ResourceManager::LoadTexture("RightArrowButton", "Assets/UI/Buttons/RightArrowButton.png");
    ResourceManager::LoadTexture("RightArrowButtonHovered", "Assets/UI/Buttons/RightArrowButtonHovered.png");
    ResourceManager::LoadTexture("RightArrowButtonDisabled", "Assets/UI/Buttons/RightArrowButtonDisabled.png");
    ResourceManager::LoadTexture("LeftArrowButton", "Assets/UI/Buttons/LeftArrowButton.png");
    ResourceManager::LoadTexture("LeftArrowButtonHovered", "Assets/UI/Buttons/LeftArrowButtonHovered.png");
    ResourceManager::LoadTexture("LeftArrowButtonDisabled", "Assets/UI/Buttons/LeftArrowButtonDisabled.png");
    ResourceManager::LoadTexture("CoinIcon", "Assets/UI/Icons/CoinIcon.png");
    ResourceManager::LoadTexture("DiamondIcon", "Assets/UI/Icons/DiamondIcon.png");

    GameTheme::Initialize();
}

void GameLayer::SetGameStateInternal(GameState newState)
{
    switch (newState)
    {
    case GameState::MainMenu:
        if (m_CurrentState == GameState::Paused)
        {
            m_CurrentLevel.reset();
        }

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
