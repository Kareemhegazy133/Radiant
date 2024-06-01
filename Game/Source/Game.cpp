#include <Engine.h>
#include <Core/EntryPoint.h>

#include "GameLayer.h"

class TheReaper : public Engine::GameApplication
{

public:
	TheReaper(GameApplicationCommandLineArgs args) : GameApplication("The Reaper", args)
	{
		this->GetTextureManager().loadTexture("Player", "Assets/Player 2.png");
		this->GetTextureManager().loadTexture("Platform", "Assets/Platform.png");
		PushLayer(new GameLayer());
	}

};

Engine::GameApplication* Engine::CreateGameApplication(GameApplicationCommandLineArgs args)
{
	return new TheReaper(args);
}
