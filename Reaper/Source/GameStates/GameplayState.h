#pragma once

#include <Radiant.h>

#include "Core/GameState.h"

class GameplayState : public GameState
{
public:
	virtual ~GameplayState() = default;

	void OnEnter() override;
	void OnExit() override;

	void OnUpdate(Timestep ts) override;
	void OnRender() override;

	void OnEvent(Event& e) override;

private:
	bool OnWindowResized(WindowResizeEvent& e);
	bool OnKeyPressed(KeyPressedEvent& e);

// TEMP
private:
	void CreateDEBUG();
	void LoadDEBUG();

private:
	Ref<Framebuffer> m_Framebuffer;
	Ref<Level> m_Level;

	// TEMP
	Entity m_Camera;

};