#pragma once

#include <Engine.h>

#include "Levels/GameLevel.h"
#include "Levels/MainMenuLevel.h"

using namespace Engine;

class GameLayer : public Layer
{
public:
	GameLayer();
	virtual ~GameLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	void OnEvent(Event& e) override;

	bool OnKeyPressed(KeyPressedEvent& e);

private:
	Ref<GameLevel> m_GameLevel;
	Ref<MainMenuLevel> m_MainMenuLevel;

	enum class GameState
	{
		MainMenu = 0,
		Loading,
		Playing,
		Paused,
		GameOver
	};

	GameState m_CurrentState = GameState::Playing;
};

