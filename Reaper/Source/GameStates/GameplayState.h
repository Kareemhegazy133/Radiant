#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

#include "Core/GameState.h"

class GameplayState : public GameState
{
public:
	void OnEnter() override;
	void OnExit() override;

	GameStateType GetStateType() const override { return GameStateType::Gameplay; }

private:
	Layer* m_GameLayer;
};