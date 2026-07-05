#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

#include "Core/GameState.h"

/**
 * Pause-menu state pushed on top of GameplayState (Escape). Owns no assets or
 * layers — the UILayer swaps to the pause menu based on the top state type.
 * Note: entering it does not halt the simulation; the GameLayer underneath
 * keeps updating (pause gating is future work).
 */
class GamePausedState : public GameState
{
public:
	void OnEnter() override;
	void OnExit() override;

	GameStateType GetStateType() const override { return GameStateType::GamePaused; }
};