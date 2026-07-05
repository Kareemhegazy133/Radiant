#pragma once

#include <Radiant/Radiant.h>

struct UIConfig
{
	float SmallVSpace{ 10.0f };
	float MediumVSpace{ 15.0f };
	float LargeVSpace{ 20.0f };

	ImVec2 SmallButtonSize()
	{
		return ImVec2(ImGui::GetWindowWidth() * 0.1f, ImGui::GetWindowHeight() * 0.1f);
	}

	ImVec2 MediumButtonSize()
	{
		return ImVec2(ImGui::GetWindowWidth() * 0.15f, ImGui::GetWindowHeight() * 0.1f);
	}

	ImVec2 LargeButtonSize()
	{
		return ImVec2(ImGui::GetWindowWidth() * 0.2f, ImGui::GetWindowHeight() * 0.1f);
	}
};

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

private:
	void BeginFullScreenWindow(const char* windowName);
	void EndFullScreenWindow();

	bool Button(const char* label, ImVec2 position, ImVec2 size);

private:
	static inline UIConfig UISettings;
};