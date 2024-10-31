#pragma once

#include <Radiant.h>

#include "Core/GameState.h"

class MainMenuState : public GameState
{
public:
	void OnEnter() override;
	void OnExit() override;

	GameStateType GetStateType() const override { return  GameStateType::MainMenu; }
};