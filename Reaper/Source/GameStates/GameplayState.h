#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

#include "Core/GameState.h"

/**
 * Playing state: creates and pushes the GameLayer on enter — ownership of the
 * layer transfers to the engine LayerStack, which deletes it after the deferred
 * pop. Gameplay assets are scoped to this state: loaded in GameLayer::OnAttach,
 * cleared here in OnExit (see the OnExit comment for why not in OnDetach).
 */
class GameplayState : public GameState
{
public:
	void OnEnter() override;
	void OnExit() override;

	GameStateType GetStateType() const override { return GameStateType::Gameplay; }

private:
	Layer* m_GameLayer;
};