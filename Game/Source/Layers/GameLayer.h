#pragma once

#include <Engine.h>

using namespace Engine;

class GameLayer : public Layer
{
public:

    enum class GameState
    {
        MainMenu = 0,
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
	inline GameState& GetGameState() { return m_CurrentState; }

private:
	bool OnKeyPressed(KeyPressedEvent& e);

private:
	Scope<Level> m_CurrentLevel;
	GameState m_CurrentState;
};

