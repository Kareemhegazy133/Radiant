#include "UILayer.h"

#include "Core/ReaperContext.h"
#include "Core/GameStateManager.h"

UILayer::UILayer()
	: Layer("UILayer")
{
	GAME_TRACE("UILayer Constructor");

}

UILayer::~UILayer()
{
	GAME_TRACE("UILayer Destructor");
}

void UILayer::OnAttach()
{
#ifndef RD_DIST
	// RAD-25 debug scaffolding: fires every 2 SIMULATION seconds, so the sim/real
	// gap in the log makes dilation visible — 4 real seconds apart at scale 0.5,
	// silent while paused. Captures nothing; cleared in OnDetach regardless
	// (the TimerManager lifetime contract: owners clear their handles at teardown).
	m_DemoTimer = GameApplication::GetTimerManager().SetTimer(2.0f, []()
	{
		GAME_INFO("Demo timer: sim {:.2f}s / real {:.2f}s", Time::GetSimulationTime(), Time::GetRealTime());
	}, /*looping*/ true);
#endif
}

void UILayer::OnDetach()
{
#ifndef RD_DIST
	GameApplication::GetTimerManager().ClearTimer(m_DemoTimer);
#endif
}

void UILayer::OnImGuiRender()
{
	switch (GameStateManager::GetCurrentStateType())
	{
	case GameStateType::MainMenu:
		RenderMainMenu();
		break;
	case GameStateType::Gameplay:
		RenderGameplayHUD();
		break;
	case GameStateType::GamePaused:
		RenderPauseMenu();
		break;
	}
}

void UILayer::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<KeyPressedEvent>(RADIANT_BIND_EVENT_FN(UILayer::OnKeyPressed));
}

void UILayer::RenderMainMenu()
{
	BeginFullScreenWindow("MainMenu");

	Ref<Texture2D> mainMenuBG = AssetManager::GetAssetFromFilePath<Texture2D>("Assets/Textures/UI/MainMenuBackground.png");
	uint64_t textureID = mainMenuBG->GetRendererID();
	ImGui::Image(textureID, ImGui::GetIO().DisplaySize, ImVec2(0, 1), ImVec2(1, 0));

	// Define initial positions and offsets
	ImVec2 buttonSize = UISettings.LargeButtonSize();
	float horizontalOffset = (ImGui::GetWindowWidth() - buttonSize.x) * 0.075f;
	float verticalOffset = ImGui::GetWindowHeight() * 0.4f;
	ImVec2 buttonPosition(horizontalOffset, verticalOffset);
	float verticalSpacing = UISettings.LargeVSpace;

	if (Button("Play", buttonPosition, buttonSize))
	{
		GameStateManager::Get()->ChangeState<GameplayState>();
	}

	// Adjust position for the next button
	buttonPosition.y += buttonSize.y + verticalSpacing;

	if (Button("Options", buttonPosition, buttonSize))
	{
		
	}

	// Adjust position for the next button
	buttonPosition.y += buttonSize.y + verticalSpacing;

	if (Button("Quit", buttonPosition, buttonSize))
	{
		GameApplication::Get().Close();
	}

	EndFullScreenWindow();
}

void UILayer::RenderGameplayHUD()
{
	
}

void UILayer::RenderPauseMenu()
{
	BeginFullScreenWindow("PauseMenu");

	ImVec2 buttonSize = UISettings.LargeButtonSize();
	float horizontalOffset = (ImGui::GetWindowWidth() - buttonSize.x) * 0.5f;
	float verticalOffset = ImGui::GetWindowHeight() * 0.3f;
	ImVec2 buttonPosition(horizontalOffset, verticalOffset);
	float verticalSpacing = UISettings.MediumVSpace;

	if (Button("Resume", buttonPosition, buttonSize))
	{
		GameStateManager::Get()->PopState();	// Top-most state should be the GamePausedState
	}

	// Adjust position for the next button
	buttonPosition.y += buttonSize.y + verticalSpacing;

	if (Button("Options", buttonPosition, buttonSize))
	{

	}

	// Adjust position for the next button
	buttonPosition.y += buttonSize.y + verticalSpacing;

	if (Button("Back to Main Menu", buttonPosition, buttonSize))
	{
		GameStateManager::Get()->ChangeState<MainMenuState>();
	}

	// Adjust position for the next button
	buttonPosition.y += buttonSize.y + verticalSpacing;

	if (Button("Exit to Desktop", buttonPosition, buttonSize))
	{
		GameApplication::Get().Close();
	}

	EndFullScreenWindow();
}

bool UILayer::OnKeyPressed(KeyPressedEvent& e)
{
	if (e.GetKeyCode() == Key::Escape && GameStateManager::GetCurrentStateType() == GameStateType::GamePaused)
	{
		GameStateManager::Get()->PopState();	// Top-most state should be the GamePausedState
		return true;
	}

#ifndef RD_DIST
	// RAD-25 debug scaffolding: time-scale keys (initial press only — the OS
	// key-repeat stream would spam SetTimeScale and the log). F3 doubles as the
	// proof that UI stays fully interactive while the simulation is frozen —
	// ImGui runs at render rate, untouched by the time scale.
	if (!e.IsRepeat())
	{
		switch (e.GetKeyCode())
		{
			case Key::F1: Time::SetTimeScale(1.0f); GAME_INFO("Debug: time scale -> 1.0");          return true;
			case Key::F2: Time::SetTimeScale(0.5f); GAME_INFO("Debug: time scale -> 0.5");          return true;
			case Key::F3: Time::SetTimeScale(0.0f); GAME_INFO("Debug: time scale -> 0.0 (paused)"); return true;
		}
	}
#endif

	return false;
}

void UILayer::BeginFullScreenWindow(const char* windowName)
{
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	ImGui::Begin(
		windowName,
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse
	);
}

void UILayer::EndFullScreenWindow()
{
	ImGui::End();
	ImGui::PopStyleVar();
}

bool UILayer::Button(const char* label, ImVec2 position, ImVec2 size)
{
	ImGui::SetCursorPos(position);
	return ImGui::Button(label, size);
}
