#include "Enginepch.h"
#include "GameApplication.h"

namespace Engine {

	GameApplication* GameApplication::s_Instance = nullptr;

	GameApplication::GameApplication(unsigned int windowWidth, unsigned int windowHeight, const std::string windowTitle)
	{
		ENGINE_ASSERT(!s_Instance, "GameApplication already exists!");
		s_Instance = this;
		m_Window = std::make_unique<sf::Window>(sf::VideoMode(windowWidth, windowHeight), windowTitle);
	}

	GameApplication::~GameApplication()
	{

	}
}