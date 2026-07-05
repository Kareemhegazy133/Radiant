#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

#include "GameStates/MainMenuState.h"
#include "GameStates/GamePausedState.h"
#include "GameStates/GameplayState.h"

/**
 * Stack-based finite state machine driving Reaper's flow. Push overlays a state
 * on the current one (e.g. GamePaused on top of Gameplay); ChangeState unwinds
 * the whole stack and starts fresh. UILayer renders whichever state type is on
 * top. Fixed-capacity stack (3 states — asserts on overflow). Process singleton
 * created (and owned) by ReaperContext; starts in MainMenuState.
 */
class GameStateManager
{
public:
	GameStateManager();
	~GameStateManager();

	/** Constructs T, pushes it, and fires its OnEnter. Asserts when the stack is full. */
	template<typename T>
	void PushState()
	{
		static_assert(std::is_base_of<GameState, T>::value, "GameStateManager: PushState<T> can only be used with types derived from GameState");

		Ref<GameState> newState = Ref<T>::Create();

		RADIANT_ASSERT(m_StateStackSize < m_StateStack.size(), "GameStateManager: m_StateStackSize has exceeded the array size");
		m_StateStack[m_StateStackSize++] = newState;

		newState->OnEnter();
	}

	/** Pops every live state (each gets OnExit, top-down), then pushes T. */
	template<typename T>
	void ChangeState()
	{
		static_assert(std::is_base_of<GameState, T>::value, "GameStateManager: ChangeState<T> can only be used with types derived from GameState");

		while (m_StateStackSize > 0)
		{
			PopState();
		}
		PushState<T>();
	}

	/** Fires OnExit on the top state and releases it; no-op on an empty stack. */
	void PopState();

	// Precondition: at least one state on the stack — an empty stack underflows
	// the index (size_t wraps) with no assert guarding it
	static GameStateType GetCurrentStateType() { return Get()->m_StateStack[Get()->m_StateStackSize - 1]->GetStateType(); }

	static GameStateManager* Get() { return s_Instance; }

private:
	inline static GameStateManager* s_Instance = nullptr;

	std::array<Ref<GameState>, 3> m_StateStack;
	size_t m_StateStackSize = 0;
};
