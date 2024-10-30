#include <Radiant.h>
#include <Core/EntryPoint.h>

#include "ReaperContext.h"
#include "GameLayer.h"

class TheReaper : public Radiant::GameApplication
{
public:
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
	ReaperContext gameContext;
	Layer* gameLayer = new GameLayer();
};

Radiant::GameApplication* Radiant::CreateGameApplication()
{
	GameApplicationSpecification specification;
	specification.Name = "TheReaper";
	specification.WindowWidth = 1280;
	specification.WindowHeight = 720;
	specification.VSync = true;
	specification.IconPath = "Assets/Textures/ReaperIcon.jpg";
	specification.FontPath = "Assets/Fonts/Euljiro.ttf";
	specification.FontSize = 20.0f;
	return new TheReaper(specification);
}
