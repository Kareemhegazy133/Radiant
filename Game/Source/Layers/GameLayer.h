#pragma once

#include <Engine.h>

#include "Levels/GameLevel.h"
#include "Levels/MainMenuLevel.h"

using namespace Engine;

class GameLayer : public Layer
{
public:

	enum class GameState
	{
		MainMenu = 0,
		Loading,
		Playing,
		Paused,
		GameOver
	};

	GameLayer();
	virtual ~GameLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	void OnEvent(Event& e) override;

	void SetGameState(GameState newState);

	bool OnKeyPressed(KeyPressedEvent& e);

private:
	Scope<Level> m_CurrentLevel = nullptr;

	GameState m_CurrentState;
};

