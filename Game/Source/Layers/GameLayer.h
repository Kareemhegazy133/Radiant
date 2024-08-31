#pragma once

#include <Engine.h>

#include "GameStates/MainMenuState.h"
#include "GameStates/GamePausedState.h"
#include "GameStates/GameplayState.h"

using namespace Engine;

class GameLayer : public Layer
{
public:

	GameLayer();
	~GameLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void PushState(const Ref<GameState>& newState);
	void PopState();
	void ChangeState(const Ref<GameState>& newState);

	void OnUpdate(Timestep ts) override;
	void OnEvent(Event& e) override;

	const Ref<GameState>& GetNextState() { return m_NextState; }
	const Ref<GameState>& GetMainMenuState() { return m_MainMenuState; }
	const Ref<GameState>& GetGamePausedState() { return m_GamePausedState; }
	const Ref<GameState>& GetGameplayState() { return m_GameplayState; }

private:
	void LoadAssets();

private:

	Ref<GameState> m_NextState;
	Ref<MainMenuState> m_MainMenuState;
	Ref<GamePausedState> m_GamePausedState;
	Ref<GameplayState> m_GameplayState;

	std::vector<Ref<GameState>> m_StateStack;
};

