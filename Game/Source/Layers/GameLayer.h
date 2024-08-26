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
	~GameLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	void OnEvent(Event& e) override;

	inline static void SetGameState(GameState newState) { s_Instance->SetGameStateInternal(newState); }
	inline static GameState& GetGameState() { return s_Instance->m_CurrentState; }

private:
	void LoadAssets();
	void SetGameStateInternal(GameState newState);

private:
	static GameLayer* s_Instance;

	Scope<Level> m_CurrentLevel;
	GameState m_CurrentState;

};

