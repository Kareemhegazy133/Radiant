#include "Engine.h"

class TheReaper : public Engine::GameApplication
{
public:
	TheReaper()
	{

	}

	~TheReaper()
	{

	}

	void Run() override
	{
		while (m_Running)
		{
			//GAME_INFO("Game Running..");
		}
	}
};

Engine::GameApplication* Engine::CreateGameApplication()
{
	return new TheReaper();
}