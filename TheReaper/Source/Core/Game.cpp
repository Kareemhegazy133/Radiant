#include <Radiant.h>
#include <Core/EntryPoint.h>

#include "GameContext.h"
#include "Core/GameLayer.h"

class TheReaper : public Radiant::GameApplication
{
public:
	TheReaper(const GameApplicationSpecification& specification)
		: GameApplication(specification)
	{
		GameContext::Init();
		PushLayer(gameLayer);
	}

	~TheReaper()
	{
		PopLayer(gameLayer);
		GameContext::Shutdown();
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
	specification.IconPath = "Assets/Textures/TheReaperIcon.jpg";
	specification.FontPath = "Assets/Fonts/Euljiro.ttf";
	specification.FontSize = 20.0f;
	return new TheReaper(specification);
}
