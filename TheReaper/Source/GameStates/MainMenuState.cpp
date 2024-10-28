#include "MainMenuState.h"

#include <imgui/imgui.h>

#include "Layers/GameLayer.h"

void MainMenuState::OnEnter()
{
	GAME_INFO("MainMenu OnEnter");

	if (!AssetManager::LoadAssetRegistry(m_AssetRegistryPath))
	{
		m_BgTexture = AssetManager::LoadAsset<Texture2D>("Assets/Textures/UI/MainMenuBackground.png");
	}
	else
	{
		m_BgTexture = AssetManager::GetAssetFromFilePath<Texture2D>("Assets/Textures/UI/MainMenuBackground.png");
	}
	
}

void MainMenuState::OnExit()
{
	GAME_INFO("MainMenu OnExit");

	AssetManager::SaveAssetRegistry(m_AssetRegistryPath);
	AssetManager::ClearAssets();
}

void MainMenuState::OnUpdate(Timestep ts)
{

}

void MainMenuState::OnRender()
{
	// Set ImGui window position and size
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

	// Center-align window with no padding
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

	uint64_t textureID = m_BgTexture->GetRendererID();
	ImGui::Image(textureID, ImGui::GetIO().DisplaySize, ImVec2(0, 1), ImVec2(1, 0));

	// Add spacing to center buttons vertically
	ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.4f);

	// Center buttons horizontally
	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 200) * 0.5f); // Adjust 200 to be the width of your buttons

	if (ImGui::Button("Play", ImVec2(200, 50)))
	{
		GameLayer::Get()->ChangeState(GameLayer::GetGameplayState());
	}

	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 200) * 0.5f);
	if (ImGui::Button("Options", ImVec2(200, 50)))
	{
		// Open options menu or change to options state
	}

	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 200) * 0.5f);
	if (ImGui::Button("Quit", ImVec2(200, 50)))
	{
		GameApplication::Get().Close();
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void MainMenuState::OnEvent(Event& e)
{

}
