#pragma once

#include "Core/Log.h"
#include "Core/Assert.h"

#include "Core/Timestep.h"

#include "SFML/Graphics.hpp"

int main(int argc, char** argv);

namespace Engine {

	class GameApplication
	{
	public:
		GameApplication(unsigned int windowWidth, unsigned int windowHeight, const std::string windowTitle);
		virtual ~GameApplication();

		inline sf::RenderWindow& GetWindow() { return *m_Window; }
		inline float sfmlGetTime() { return clock.getElapsedTime().asSeconds(); }

		inline static GameApplication& Get() { return *s_Instance; }

	private:
		void Run();
	private:
		std::unique_ptr<sf::RenderWindow> m_Window;
		sf::Clock clock;
		float m_LastFrameTime = 0.0f;
	private:
		static GameApplication* s_Instance;
		friend int ::main(int argc, char** argv);
	};

	// To be defined in GAME
	GameApplication* CreateGameApplication();
}