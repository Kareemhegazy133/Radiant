#pragma once

#include "Core/Core.h"

#include "SFML/Window.hpp"

namespace Engine {

	class GameApplication
	{
	public:
		GameApplication(unsigned int windowWidth, unsigned int windowHeight, const std::string windowTitle);
		virtual ~GameApplication();

		virtual void Run() = 0;

		inline sf::Window& GetWindow() { return *m_Window; }

		inline static GameApplication& Get() { return *s_Instance; }
	protected:
		std::unique_ptr<sf::Window> m_Window;

	private:
		static GameApplication* s_Instance;
	};

	// To be defined in GAME
	GameApplication* CreateGameApplication();
}