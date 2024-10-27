#include "GameLayer.h"

using namespace Radiant;

GameLayer::GameLayer()
	: Layer("GameLayer")
{
	GAME_TRACE("GameLayer Constructor");

	RADIANT_ASSERT(!s_Instance, "GameLayer already exists!");
	s_Instance = this;

	AssetManager::Init();
}

GameLayer::~GameLayer()
{
	GAME_TRACE("GameLayer Destructor");

	delete s_Instance;
	s_Instance = nullptr;
}

void GameLayer::OnAttach()
{
	RADIANT_PROFILE_FUNCTION();

	PushState(m_MainMenuState);
}

void GameLayer::OnDetach()
{
	RADIANT_PROFILE_FUNCTION();

	m_MainMenuState.reset();
	m_GamePausedState.reset();
	m_GameplayState.reset();

	if (m_NextState)
	{
		m_NextState.reset();
	}
}

void GameLayer::OnUpdate(Timestep ts)
{
	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnUpdate(ts);
	}
}

void GameLayer::OnImGuiRender()
{
	RADIANT_PROFILE_FUNCTION();

	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnRender();
	}
}

void GameLayer::OnEvent(Event& e)
{
	RADIANT_PROFILE_FUNCTION();

	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnEvent(e);
	}
}

void GameLayer::PopState()
{
	RADIANT_PROFILE_FUNCTION();

	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnExit();
		m_StateStack[--m_StateStackSize].reset();
	}
}
