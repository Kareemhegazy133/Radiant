#include "MainMenuLevel.h"

#include "Layers/GameLayer.h"

MainMenuLevel::MainMenuLevel()
{
	std::cout << "MainMenuLevel Created!" << std::endl;
}

MainMenuLevel::~MainMenuLevel()
{
	std::cout << "MainMenuLevel Destroyed!" << std::endl;
}

void MainMenuLevel::OnUpdate(Timestep ts)
{
	ENGINE_INFO("MainMenuLevel Updating");
}

void MainMenuLevel::OnRender()
{

}

void MainMenuLevel::OnEvent(Event& e)
{

}
