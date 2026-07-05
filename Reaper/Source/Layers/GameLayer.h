#pragma once

#include <Radiant/Radiant.h>

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