#include "MainMenuState.h"

#include "Core/GameLayer.h"

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

	float windowWidth = ImGui::GetWindowWidth();
	float windowHeight = ImGui::GetWindowHeight();
	ImVec2 buttonSize(windowWidth * 0.2f, windowHeight * 0.1f);

	float horizontalOffset = windowWidth * 0.075f;
	float verticalOffset = windowHeight * 0.4f;

	float vSpace = 20.0f;
	ImGui::SetCursorPos(ImVec2(horizontalOffset, verticalOffset));
	if (ImGui::Button("Play", buttonSize))
	{
		GameLayer::Get()->ChangeState(GameLayer::GetGameplayState());
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

void MainMenuState::OnEvent(Event& e)
{

}
