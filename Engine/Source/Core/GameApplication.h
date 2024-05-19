#pragma once

#include "Core/Log.h"
#include "Core/Assert.h"

#include "Window.h"
#include "Core/LayerStack.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "Core/Timestep.h"

#include "SFML/Graphics.hpp"

int main(int argc, char** argv);

namespace Engine {

	class GameApplication
	{
	public:
		GameApplication();
		virtual ~GameApplication();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }
		inline float sfmlGetTime() { return clock.getElapsedTime().asSeconds(); }

		inline static GameApplication& Get() { return *s_Instance; }

	private:
		void Run();
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		std::unique_ptr<Window> m_Window;
		LayerStack m_LayerStack;
		sf::Clock clock;
		float m_LastFrameTime = 0.0f;
	private:
		static GameApplication* s_Instance;
		friend int ::main(int argc, char** argv);
	};

	// To be defined in GAME
	GameApplication* CreateGameApplication();
}