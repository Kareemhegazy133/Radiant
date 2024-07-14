#include "MainMenuLevel.h"

#include "Layers/GameLayer.h"

MainMenuLevel::MainMenuLevel()
{
	m_MainMenu.SetVisibility(true);
	std::cout << "MainMenuLevel Created!" << std::endl;
}

MainMenuLevel::~MainMenuLevel()
{
	std::cout << "MainMenuLevel Destroyed!" << std::endl;
}

void MainMenuLevel::OnUpdate(Timestep ts)
{
	
}

void MainMenuLevel::OnRender()
{
	m_MainMenu.OnRender();
}

void MainMenuLevel::OnEvent(Event& e)
{
	m_MainMenu.OnEvent(e);
}

