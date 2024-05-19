#include "Enginepch.h"
#include "GameApplication.h"

namespace Engine {

	GameApplication* GameApplication::s_Instance = nullptr;

	GameApplication::GameApplication(unsigned int windowWidth, unsigned int windowHeight, const std::string windowTitle)
	{
		ENGINE_ASSERT(!s_Instance, "GameApplication already exists!");
		s_Instance = this;
		m_Window = std::make_unique<sf::RenderWindow>(sf::VideoMode(windowWidth, windowHeight), windowTitle);
	}

	GameApplication::~GameApplication()
	{

	}
	void GameApplication::Run()
	{
		while (m_Window->isOpen())
		{
			//GAME_INFO("Game Running..");
			float time = sfmlGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			// check all the window's events that were triggered since the last iteration of the loop
			sf::Event event;
			while (m_Window->pollEvent(event))
			{
				// "close requested" event: we close the window
				if (event.type == sf::Event::Closed)
				{
					m_Window->close();
				}
			}

			// clear the window with black color
			m_Window->clear(sf::Color::Black);

			// draw everything here...
			// m_Window->draw(...);

			// end the current frame
			m_Window->display();
		}
	}
}