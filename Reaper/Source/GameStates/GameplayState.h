#pragma once

#include <Radiant/Radiant.h>

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