#include "GameLayer.h"

#include "GameTheme.h"

// TODO: Serialize and Deserialize the game layer

using namespace Engine;

GameLayer::GameLayer() : Layer("GameLayer")
{
    GAME_TRACE("GameLayer Init");

    ResourceManager::Init();
    LoadAssets();
}

GameLayer::~GameLayer()
{
    ResourceManager::Shutdown();
    GAME_TRACE("GameLayer Shutdown");
}

void GameLayer::OnAttach()
{
    // TODO: Look to implement on demand creation/destruction
    m_MainMenuState = CreateRef<MainMenuState>(this);
    m_GamePausedState = CreateRef<GamePausedState>(this);
    m_GameplayState = CreateRef<GameplayState>(this);

    PushState(m_MainMenuState);
}

void GameLayer::OnDetach()
{
    m_MainMenuState.reset();
    m_GamePausedState.reset();
    m_GameplayState.reset();

    // TODO: UnloadAssets();
}

void GameLayer::PushState(const Ref<GameState>& newState)
{
    if (newState)
    {
        newState->OnEnter();
        m_StateStack.emplace_back(newState);
    }
}

void GameLayer::PopState()
{
    if (!m_StateStack.empty())
    {
        m_StateStack.back()->OnExit();
        m_StateStack.pop_back();
    }
}

void GameLayer::ChangeState(const Ref<GameState>& newState)
{
    m_NextState = newState;
    while (!m_StateStack.empty())
    {
        PopState();
    }
    PushState(newState);
}

void GameLayer::OnUpdate(Timestep ts)
{
    if (!m_StateStack.empty())
    {
        m_StateStack.back()->OnUpdate(ts);
    }
}


void GameLayer::OnEvent(Event& e)
{
    if (!m_StateStack.empty())
    {
        m_StateStack.back()->OnEvent(e);
    }
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
