#pragma once

#include <Radiant.h>

using namespace Radiant;

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
	void SaveDEBUG();

private:
	Ref<Level> m_Level;
	Ref<Framebuffer> m_Framebuffer;

	// TEMP
	Entity m_Camera;

};