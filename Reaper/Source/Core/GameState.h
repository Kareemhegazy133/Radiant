#pragma once

#include <Radiant/Radiant.h>

enum class GameStateType
{
	None,
	MainMenu,
	Gameplay,
	GamePaused
};

class GameState : public RefCounted
{
public:
	virtual ~GameState() = default;

	// Called when the state is entered
	virtual void OnEnter() = 0;

	// Called when the state is exited
	virtual void OnExit() = 0;

	virtual GameStateType GetStateType() const = 0;

	virtual bool operator==(const GameState& other) const
	{
		return GetStateType() == other.GetStateType();
	}

	virtual bool operator!=(const GameState& other) const
	{
		return !(*this == other);
	}

};
