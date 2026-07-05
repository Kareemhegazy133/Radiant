#include <Radiant/Radiant.h>
#include <Radiant/Core/EntryPoint.h>

#include "Layers/SandboxLayer.h"

class Sandbox : public Radiant::GameApplication
{

public:
	// TODO Support Multiple Resolutions
	Sandbox(const GameApplicationSpecification& specification)
		: GameApplication(specification)
	{
		PushLayer(sandboxLayer);
	}

	~Sandbox()
	{
		PopLayer(sandboxLayer);
	}

	Layer* sandboxLayer = new SandboxLayer();
};

Radiant::GameApplication* Radiant::CreateGameApplication()
{
	GameApplicationSpecification specification;
	specification.Name = "Sandbox";
	specification.WindowWidth = 1280;
	specification.WindowHeight = 720;
	specification.VSync = true;

	return new Sandbox(specification);
}
