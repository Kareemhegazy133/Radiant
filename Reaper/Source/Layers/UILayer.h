#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

/** Shared UI metrics; button sizes scale with the current window size. */
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

/**
 * Persistent ImGui overlay that draws whichever screen matches the current
 * game state (main menu / gameplay HUD / pause menu). Pushed once by the
 * Reaper application and lives for its whole run — state changes swap what it
 * renders, not the layer itself. Also handles Escape-to-resume while paused.
 */
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