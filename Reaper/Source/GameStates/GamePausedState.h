#pragma once

#include <Radiant/Radiant.h>

#include "Core/GameState.h"

class GamePausedState : public GameState
{
public:
	void OnEnter() override;
	void OnExit() override;

	GameStateType GetStateType() const override { return GameStateType::GamePaused; }
};