#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

enum class GameStateType
{
	None,
	MainMenu,
	Gameplay,
	GamePaused
};

/**
 * One game state (screen/mode) in Reaper's stack FSM (see GameStateManager).
 *
 * Contract: OnEnter runs when the state is pushed; OnExit runs when it is
 * popped — including during ChangeState's stack unwind and manager shutdown.
 * States, not layers, own asset scope: a state loads its asset registry in
 * OnEnter and clears assets in OnExit, because layer OnDetach runs deferred at
 * end of frame and would wipe the NEXT state's freshly loaded assets.
 * Ref-counted; owned by the GameStateManager's stack.
 */
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
