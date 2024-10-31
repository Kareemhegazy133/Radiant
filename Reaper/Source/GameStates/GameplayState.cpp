#include "GameplayState.h"

#include "Core/GameStateManager.h"
#include "Layers/GameLayer.h"

void GameplayState::OnEnter()
{
	GAME_TRACE("GameplayState Entered");

	m_GameLayer = new GameLayer();
	GameApplication::Get().PushLayer(m_GameLayer);
}

void GameplayState::OnExit()
{
	GameApplication::Get().PopLayer(m_GameLayer);
	m_GameLayer = nullptr;

	GAME_TRACE("GameplayState Exited");
}
