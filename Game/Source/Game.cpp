#include <Engine.h>
#include <Core/EntryPoint.h>

#include "GameLayer.h"

namespace Engine {

	class TheReaper : public GameApplication
	{

	public:
		TheReaper(GameApplicationCommandLineArgs args) : GameApplication("The Reaper", args)
		{
			this->GetTextureManager().loadTexture("Player", "Assets/Player.png");
			PushLayer(new GameLayer());
		}

	};

	GameApplication* CreateGameApplication(GameApplicationCommandLineArgs args)
	{
		return new TheReaper(args);
	}

}

