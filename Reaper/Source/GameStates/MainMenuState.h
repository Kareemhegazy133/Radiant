#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

#include "Core/GameState.h"

/**
 * Front-end state: loads the main-menu asset registry on enter and clears
 * assets on exit (never saving — running the game must not write assets,
 * RAD-17). The UILayer draws the actual menu; this state only scopes assets.
 */
class MainMenuState : public GameState
{
public:
	void OnEnter() override;
	void OnExit() override;

	GameStateType GetStateType() const override { return  GameStateType::MainMenu; }
};