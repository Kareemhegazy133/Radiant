#pragma once

#include "Core/Base.h"
#include "Core/Log.h"

#include "Window.h"
#include "Core/LayerStack.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "Core/Timestep.h"

#include "ImGui/ImGuiLayer.h"

int main(int argc, char** argv);

namespace Radiant {

	struct GameApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const
		{
			RADIANT_ASSERT(index < Count);
			return Args[index];
		}
	};

	class GameApplication
	{
	public:
		GameApplication(
			const std::string& name = "Game App",
			const uint32_t width = 1280,
			const uint32_t height = 720,
			GameApplicationCommandLineArgs args = GameApplicationCommandLineArgs()
		);
		virtual ~GameApplication();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* layer);

		inline static Window& GetWindow() { return *(s_Instance->m_Window); }
		inline static ImGuiLayer& GetImGuiLayer() { return *(s_Instance->m_ImGuiLayer); }
		inline static GameApplication& Get() { return *s_Instance; }

		GameApplicationCommandLineArgs GetCommandLineArgs() const { return m_CommandLineArgs; }
	private:
		void Run();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		GameApplicationCommandLineArgs m_CommandLineArgs;
		Scope<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;
	private:
		static GameApplication* s_Instance;
		friend int ::main(int argc, char** argv);
	};

	// To be defined in GAME
	GameApplication* CreateGameApplication(GameApplicationCommandLineArgs args);
}