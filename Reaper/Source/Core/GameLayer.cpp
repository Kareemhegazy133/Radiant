#include "GameLayer.h"

GameLayer::GameLayer()
	: Layer("GameLayer")
{
	GAME_TRACE("GameLayer Constructor");

	RADIANT_ASSERT(!s_Instance, "GameLayer already exists!");
	s_Instance = this;

}

GameLayer::~GameLayer()
{
	GAME_TRACE("GameLayer Destructor");

	delete s_Instance;
	s_Instance = nullptr;
}

void GameLayer::OnAttach()
{
	PushState(m_MainMenuState);
}

void GameLayer::OnDetach()
{
	m_MainMenuState.Reset();
	m_GamePausedState.Reset();
	m_GameplayState.Reset();

	if (m_NextState)
	{
		m_NextState.Reset();
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
	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnRender();
	}
}

void GameLayer::OnEvent(Event& e)
{
	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnEvent(e);
	}
}

void GameLayer::PopState()
{
	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnExit();
		m_StateStack[--m_StateStackSize].Reset();
	}
}