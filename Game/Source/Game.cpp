#include <Engine.h>
#include <Core/EntryPoint.h>

#include "Layers/GameLayer.h"

class TheReaper : public Engine::GameApplication
{

public:
	TheReaper(GameApplicationCommandLineArgs args)
		: GameApplication("The Reaper", 1280, 720, args)
	{
		InitializeSystems();
		PushLayer(new GameLayer());
	}

	void InitializeSystems()
	{
		FontManager::LoadFont("Dimbo_Regular", "Assets/Font/Dimbo_Regular.ttf");

		TextureManager::LoadTexture("Player", "Assets/Player/Reaper/Reaper_Idle.png");
		TextureManager::LoadTexture("Fireball", "Assets/Abilities/Fireball.png");
		TextureManager::LoadTexture("Platform", "Assets/Platform.png");
		TextureManager::LoadTexture("PauseMenu", "Assets/UI/Menus/PauseMenu.png");
		TextureManager::LoadTexture("ResumeButton", "Assets/UI/Buttons/ResumeButton.png");
		TextureManager::LoadTexture("CloseButton", "Assets/UI/Buttons/CloseButton.png");
		TextureManager::LoadTexture("SettingsButton", "Assets/UI/Buttons/SettingsButton.png");
		TextureManager::LoadTexture("MainMenuButton", "Assets/UI/Buttons/MainMenuButton.png");
	}
};

Engine::GameApplication* Engine::CreateGameApplication(GameApplicationCommandLineArgs args)
{
	return new TheReaper(args);
}
