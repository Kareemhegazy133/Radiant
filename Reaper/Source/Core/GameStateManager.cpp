#include "GameStateManager.h"

GameStateManager::GameStateManager()
{
	GAME_TRACE("GameStateManager Constructor");

	RADIANT_ASSERT(!s_Instance, "GameStateManager already exists!");
	s_Instance = this;

	PushState<MainMenuState>();
}

GameStateManager::~GameStateManager()
{
	GAME_TRACE("GameStateManager Destructor");
	
	// Clear all states
	while (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnExit();
		m_StateStack[--m_StateStackSize].Reset();
	}
	
	auto r = m_StateStack[m_StateStackSize];
}

void GameStateManager::PopState()
{
	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnExit();
		m_StateStack[--m_StateStackSize].Reset();
	}
}
