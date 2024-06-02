#pragma once

#include "SFML/Graphics.hpp"

#include "Core/Log.h"
#include "Core/Assert.h"

#include "Window.h"
#include "Core/LayerStack.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "Core/Timestep.h"

#include "Utils/TextureManager.h"

int main(int argc, char** argv);

namespace Engine {

	struct GameApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const
		{
			ENGINE_ASSERT(index < Count);
			return Args[index];
		}
	};

	class GameApplication
	{
	public:
		GameApplication(const std::string& name = "Game App", GameApplicationCommandLineArgs args = GameApplicationCommandLineArgs());
		virtual ~GameApplication();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }
		inline float SFMLGetTime() { return clock.getElapsedTime().asSeconds(); }

		inline static GameApplication& Get() { return *s_Instance; }

		GameApplicationCommandLineArgs GetCommandLineArgs() const { return m_CommandLineArgs; }
	private:
		void Run();
	protected:
		TextureManager* m_TextureManager;
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		GameApplicationCommandLineArgs m_CommandLineArgs;
		Scope<Window> m_Window;
		LayerStack m_LayerStack;
		sf::Clock clock;
		float m_LastFrameTime = 0.0f;
	private:
		static GameApplication* s_Instance;
		friend int ::main(int argc, char** argv);
	};

	// To be defined in GAME
	GameApplication* CreateGameApplication(GameApplicationCommandLineArgs args);
}