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
}

void UILayer::OnDetach()
{
}

void UILayer::OnUpdate(Timestep ts)
{
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
