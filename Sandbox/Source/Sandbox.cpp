#include <Radiant.h>
#include <Core/EntryPoint.h>

#include "Layers/SandboxLayer.h"

class Sandbox : public Radiant::GameApplication
{

public:
	// TODO Support Multiple Resolutions
	Sandbox(GameApplicationCommandLineArgs args)
		: GameApplication("Sandbox", 1280, 720, args)
	{
		PushLayer(new SandboxLayer());
	}

};

Radiant::GameApplication* Radiant::CreateGameApplication(GameApplicationCommandLineArgs args)
{
	return new Sandbox(args);
}
