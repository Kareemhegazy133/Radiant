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
		FontManager::LoadFont("Mantinia_Regular", "Assets/Font/Mantinia_Regular.otf");
	}
};

Engine::GameApplication* Engine::CreateGameApplication(GameApplicationCommandLineArgs args)
{
	return new TheReaper(args);
}
