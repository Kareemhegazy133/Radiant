#include <Engine.h>
#include <Core/EntryPoint.h>

#include "Layers/GameLayer.h"

class TheReaper : public Engine::GameApplication
{

public:
	TheReaper(GameApplicationCommandLineArgs args)
		: GameApplication("The Reaper", args)
	{
		PushLayer(new GameLayer());
	}

};

Engine::GameApplication* Engine::CreateGameApplication(GameApplicationCommandLineArgs args)
{
	return new TheReaper(args);
}
