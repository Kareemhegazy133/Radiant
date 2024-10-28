#include <Radiant.h>
#include <Core/EntryPoint.h>

#include "Layers/GameLayer.h"

class TheReaper : public Radiant::GameApplication
{
public:
	// TODO Support Multiple Resolutions
	TheReaper(const GameApplicationSpecification& specification)
		: GameApplication(specification)
	{
		PushLayer(gameLayer);
	}

	~TheReaper()
	{
		PopLayer(gameLayer);
	}

private:
	Layer* gameLayer = new GameLayer();
};

Radiant::GameApplication* Radiant::CreateGameApplication()
{
	GameApplicationSpecification specification;
	specification.Name = "TheReaper";
	specification.WindowWidth = 1280;
	specification.WindowHeight = 720;
	specification.VSync = true;
	return new TheReaper(specification);
}
