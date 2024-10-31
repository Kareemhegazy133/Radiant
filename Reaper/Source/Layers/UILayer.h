#pragma once

#include <Radiant.h>

class UILayer : public Layer
{
public:
	UILayer();
	~UILayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	virtual void OnImGuiRender() override;

	void OnEvent(Event& e) override;

private:
	void RenderMainMenu();
	void RenderGameplayHUD();
	void RenderPauseMenu();

	bool OnKeyPressed(KeyPressedEvent& e);

};