#include <Engine.h>
#include <Core/EntryPoint.h>

#include "GameTheme.h"
#include "Layers/GameLayer.h"

// TODO: Abstract sf::X from Game

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
		FontManager::LoadFont("Euljiro", "Assets/Font/Euljiro.ttf");

		TextureManager::LoadTexture("Player", "Assets/Player/Reaper/Reaper_Idle.png");
		TextureManager::LoadTexture("Fireball", "Assets/Abilities/Fireball.png");
		TextureManager::LoadTexture("Platform", "Assets/Platform.png");

		TextureManager::LoadTexture("LargeButton", "Assets/UI/Buttons/LargeButton.png");
		TextureManager::LoadTexture("LargeButtonHovered", "Assets/UI/Buttons/LargeButtonHovered.png");
		TextureManager::LoadTexture("MediumButton", "Assets/UI/Buttons/MediumButton.png");
		TextureManager::LoadTexture("MediumButtonHovered", "Assets/UI/Buttons/MediumButtonHovered.png");

		TextureManager::LoadTexture("MainMenu", "Assets/UI/Menus/MainMenu.png");
		TextureManager::LoadTexture("PauseMenu", "Assets/UI/Menus/PauseMenu.png");

		TextureManager::LoadTexture("CharacterInfoMenu", "Assets/UI/Menus/CharacterInfoMenu.png");
		TextureManager::LoadTexture("CloseButton", "Assets/UI/Buttons/CloseButton.png");
		TextureManager::LoadTexture("CloseButtonHovered", "Assets/UI/Buttons/CloseButtonHovered.png");

		GameTheme::Initialize();
	}

};

Engine::GameApplication* Engine::CreateGameApplication(GameApplicationCommandLineArgs args)
{
	return new TheReaper(args);
}
