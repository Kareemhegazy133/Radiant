#include "Engine.h"
#include "GameConfig.h"

class TheReaper : public Engine::GameApplication
{

public:
	TheReaper() : Engine::GameApplication(GAME_WIDTH, GAME_HEIGHT, GAME_NAME)
	{

	}

	~TheReaper()
	{

	}

	void Run() override
	{
		while (m_Window->isOpen())
		{
			//GAME_INFO("Game Running..");
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
		}
	}
};

Engine::GameApplication* Engine::CreateGameApplication()
{
	return new TheReaper();
}