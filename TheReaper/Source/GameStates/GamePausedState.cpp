#include "GamePausedState.h"

#include "Layers/GameLayer.h"

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

	ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.4f);
	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 200) * 0.5f);

	if (ImGui::Button("Resume", ImVec2(200, 50)))
	{
		GameLayer::Get()->ChangeState(GameLayer::GetGameplayState());
	}

	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 200) * 0.5f);
	if (ImGui::Button("Options", ImVec2(200, 50)))
	{

	}

	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 200) * 0.5f);
	if (ImGui::Button("Back to Main Menu", ImVec2(200, 50)))
	{
		GameLayer::Get()->ChangeState(GameLayer::GetMainMenuState());
	}

	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 200) * 0.5f);
	if (ImGui::Button("Exit to Desktop", ImVec2(200, 50)))
	{
		GameApplication::Get().Close();
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void GamePausedState::OnEvent(Event& e)
{

}
