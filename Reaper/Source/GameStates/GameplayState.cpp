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

	// Gameplay assets are scoped to this state. Clearing must happen here — before
	// the next state's OnEnter loads its registry — not in GameLayer::OnDetach,
	// which runs deferred at end of frame (it would wipe the next state's assets).
	AssetManager::ClearAssets();

	GAME_TRACE("GameplayState Exited");
}
