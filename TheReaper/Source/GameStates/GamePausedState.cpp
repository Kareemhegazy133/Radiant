#include "GamePausedState.h"

#include "Core/GameLayer.h"

void GamePausedState::OnEnter()
{
	GAME_INFO("PauseMenu OnEnter");

}

void GamePausedState::OnExit()
{
	GAME_INFO("PauseMenu OnExit");
}

void GamePausedState::OnUpdate(Timestep ts)
{

}

void GamePausedState::OnRender()
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
		GameLayer::Get()->ChangeState(GameLayer::GetGameplayState());
	}

	ImGui::SetCursorPos(ImVec2(horizontalOffset, verticalOffset + buttonSize.y + vSpace));
	if (ImGui::Button("Options", buttonSize))
	{

	}

	ImGui::SetCursorPos(ImVec2(horizontalOffset, verticalOffset + 2 * (buttonSize.y + vSpace)));
	if (ImGui::Button("Back to Main Menu", buttonSize))
	{
		GameLayer::Get()->ChangeState(GameLayer::GetMainMenuState());
	}

	ImGui::SetCursorPos(ImVec2(horizontalOffset, verticalOffset + 3 * (buttonSize.y + vSpace)));
	if (ImGui::Button("Exit to Desktop", buttonSize))
	{
		GameApplication::Get().Close();
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void GamePausedState::OnEvent(Event& e)
{

}
