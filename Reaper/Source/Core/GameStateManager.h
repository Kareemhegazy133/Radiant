#pragma once

#include <Radiant.h>

#include "GameStates/MainMenuState.h"
#include "GameStates/GamePausedState.h"
#include "GameStates/GameplayState.h"

class GameStateManager
{
public:
	GameStateManager();
	~GameStateManager();

	template<typename T>
	void PushState()
	{
		static_assert(std::is_base_of<GameState, T>::value, "GameStateManager: PushState<T> can only be used with types derived from GameState");

		Ref<GameState> newState = Ref<T>::Create();

		RADIANT_ASSERT(m_StateStackSize < m_StateStack.size(), "GameStateManager: m_StateStackSize has exceeded the array size");
		m_StateStack[m_StateStackSize++] = newState;

		newState->OnEnter();
	}

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

	void PopState();

	static GameStateType GetCurrentStateType() { return Get()->m_StateStack[Get()->m_StateStackSize - 1]->GetStateType(); }

	static GameStateManager* Get() { return s_Instance; }

private:
	inline static GameStateManager* s_Instance = nullptr;

	std::array<Ref<GameState>, 3> m_StateStack;
	size_t m_StateStackSize = 0;
};
