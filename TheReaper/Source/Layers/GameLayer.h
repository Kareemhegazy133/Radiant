#pragma once

#include <Radiant.h>

#include "GameStates/MainMenuState.h"
#include "GameStates/GamePausedState.h"
#include "GameStates/GameplayState.h"

class GameLayer : public Layer
{
public:

	GameLayer();
	~GameLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	virtual void OnImGuiRender() override;

	void OnEvent(Event& e) override;

	template<typename T>
	void PushState(Ref<T> newState)
	{
		RADIANT_PROFILE_FUNCTION();

		static_assert(std::is_base_of<GameState, T>::value, "PushState<T> can only be used with types derived from GameState");
		if (!newState)
		{
			newState = CreateRef<T>();
		}

		newState->OnEnter();
		m_StateStack.emplace_back(newState);

	}

	void PopState();

	template<typename T>
	void ChangeState(Ref<T> newState)
	{
		RADIANT_PROFILE_FUNCTION();

		static_assert(std::is_base_of<GameState, T>::value, "ChangeState<T> can only be used with types derived from GameState");

		m_NextState = newState;
		while (!m_StateStack.empty())
		{
			PopState();
		}
		PushState(newState);
	}

	Ref<GameState> GetNextState() { return m_NextState; }
	Ref<MainMenuState> GetMainMenuState() { return m_MainMenuState; }
	Ref<GamePausedState> GetGamePausedState() { return m_GamePausedState; }
	Ref<GameplayState> GetGameplayState() { return m_GameplayState; }

	static GameLayer* Get() { return s_Instance; }

private:
	inline static GameLayer* s_Instance = nullptr;

	Ref<GameState> m_NextState;
	Ref<MainMenuState> m_MainMenuState;
	Ref<GamePausedState> m_GamePausedState;
	Ref<GameplayState> m_GameplayState;

	// TODO: make this an std::array
	std::vector<Ref<GameState>> m_StateStack;
};
