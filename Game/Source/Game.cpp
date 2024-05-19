#include "Engine.h"
#include "Core/EntryPoint.h"
#include "GameConfig.h"

namespace Engine {

	class TheReaper : public GameApplication
	{

	public:
		TheReaper() : GameApplication(GAME_WIDTH, GAME_HEIGHT, GAME_NAME)
		{
		}


	};

	GameApplication* CreateGameApplication()
	{
		return new TheReaper();
	}
}
