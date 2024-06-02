#include <Engine.h>
#include <Core/EntryPoint.h>

#include "GameLayer.h"

class TheReaper : public Engine::GameApplication
{

public:
	TheReaper(GameApplicationCommandLineArgs args) : GameApplication("The Reaper", args)
	{
		m_TextureManager->loadTexture("Player", "Assets/Player/Reaper/Reaper_Idle.png");
		m_TextureManager->loadTexture("Platform", "Assets/Platform.png");
		PushLayer(new GameLayer());
	}

};

Engine::GameApplication* Engine::CreateGameApplication(GameApplicationCommandLineArgs args)
{
	return new TheReaper(args);
}
