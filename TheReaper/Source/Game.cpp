#include <Radiant.h>
#include <Core/EntryPoint.h>

#include "Layers/GameLayer.h"

class TheReaper : public Radiant::GameApplication
{
public:
	// TODO Support Multiple Resolutions
	TheReaper(GameApplicationCommandLineArgs args)
		: GameApplication("TheReaper", 1280, 720, args)
	{
		PushLayer(gameLayer);
	}

	~TheReaper()
	{
		PopLayer(gameLayer);
	}

	Layer* gameLayer = new GameLayer();
};

Radiant::GameApplication* Radiant::CreateGameApplication(GameApplicationCommandLineArgs args)
{
	return new TheReaper(args);
}
