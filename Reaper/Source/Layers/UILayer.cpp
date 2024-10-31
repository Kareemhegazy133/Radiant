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
	// Set ImGui window position and size
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	ImGui::Begin(
		"MainMenu",
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse
	);

	Ref<Texture2D> mainMenuBG = AssetManager::GetAssetFromFilePath<Texture2D>("Assets/Textures/UI/MainMenuBackground.png");
	uint64_t textureID = mainMenuBG->GetRendererID();
	ImGui::Image(textureID, ImGui::GetIO().DisplaySize, ImVec2(0, 1), ImVec2(1, 0));

	float windowWidth = ImGui::GetWindowWidth();
	float windowHeight = ImGui::GetWindowHeight();
	ImVec2 buttonSize(windowWidth * 0.2f, windowHeight * 0.1f);

	float horizontalOffset = windowWidth * 0.075f;
	float verticalOffset = windowHeight * 0.4f;

	float vSpace = 20.0f;
	ImGui::SetCursorPos(ImVec2(horizontalOffset, verticalOffset));
	if (ImGui::Button("Play", buttonSize))
	{
		GameStateManager::Get()->ChangeState<GameplayState>();
	}

	ImGui::SetCursorPos(ImVec2(horizontalOffset, verticalOffset + buttonSize.y + vSpace));
	if (ImGui::Button("Options", buttonSize))
	{
		// Add options logic here
	}

	ImGui::SetCursorPos(ImVec2(horizontalOffset, verticalOffset + 2 * (buttonSize.y + vSpace)));
	if (ImGui::Button("Quit", buttonSize))
	{
		GameApplication::Get().Close();
	}

	ImGui::End();

	ImGui::PopStyleVar();
}

void UILayer::RenderGameplayHUD()
{
	
}

void UILayer::RenderPauseMenu()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	ImGui::Begin(
		"PauseMenu",
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse
	);

	float windowWidth = ImGui::GetWindowWidth();
	float windowHeight = ImGui::GetWindowHeight();
	ImVec2 buttonSize(windowWidth * 0.2f, windowHeight * 0.1f);

	float horizontalOffset = (windowWidth - buttonSize.x) * 0.5f;
	float verticalOffset = windowHeight * 0.3f;

	float vSpace = 15.0f;
	ImGui::SetCursorPos(ImVec2(horizontalOffset, verticalOffset));
	if (ImGui::Button("Resume", buttonSize))
	{
		GameStateManager::Get()->PopState();	// Top-most state should be the GamePausedState
	}

	ImGui::SetCursorPos(ImVec2(horizontalOffset, verticalOffset + buttonSize.y + vSpace));
	if (ImGui::Button("Options", buttonSize))
	{

	}

	ImGui::SetCursorPos(ImVec2(horizontalOffset, verticalOffset + 2 * (buttonSize.y + vSpace)));
	if (ImGui::Button("Back to Main Menu", buttonSize))
	{
		GameStateManager::Get()->ChangeState<MainMenuState>();
	}

	ImGui::SetCursorPos(ImVec2(horizontalOffset, verticalOffset + 3 * (buttonSize.y + vSpace)));
	if (ImGui::Button("Exit to Desktop", buttonSize))
	{
		GameApplication::Get().Close();
	}

	ImGui::End();
	ImGui::PopStyleVar();
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
