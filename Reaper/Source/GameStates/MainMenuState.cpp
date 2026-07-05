#include "MainMenuState.h"

#include "Core/ReaperContext.h"

void MainMenuState::OnEnter()
{
	GAME_TRACE("MainMenuState Entered");

	if (!AssetManager::LoadAssetRegistry(ReaperContext::GetMainMenuAssetRegistryPath()))
	{
		AssetManager::LoadAsset<Texture2D>("Assets/Textures/UI/MainMenuBackground.png");
	}
}

void MainMenuState::OnExit()
{
	// Deliberately no SaveAssetRegistry: running the game never writes assets (RAD-17)
	AssetManager::ClearAssets();

	GAME_TRACE("MainMenuState Exited");
}
