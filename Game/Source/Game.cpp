#include <Engine.h>
#include <Core/EntryPoint.h>

#include "Layers/GameLayer.h"

// TODO: Abstract sf::X from Game

class TheReaper : public Engine::GameApplication
{

public:
	// TODO Support Multiple Resolutions
	TheReaper(GameApplicationCommandLineArgs args)
		: GameApplication("The Reaper", 1280, 720, args)
	{
		PushLayer(new GameLayer());
	}

};

Engine::GameApplication* Engine::CreateGameApplication(GameApplicationCommandLineArgs args)
{
	return new TheReaper(args);
}
