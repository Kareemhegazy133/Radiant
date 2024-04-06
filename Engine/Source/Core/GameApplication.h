#pragma once

namespace Engine {

	class GameApplication
	{
	public:
		GameApplication();
		virtual ~GameApplication();

		virtual void Run() = 0;

	protected:
		bool m_Running = true;
	};

	// To be defined in GAME
	GameApplication* CreateGameApplication();
}