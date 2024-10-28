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

	struct GameApplicationSpecification
	{
		std::string Name = "Game";
		uint32_t WindowWidth = 1280, WindowHeight = 720;
		bool VSync = true;
		std::filesystem::path IconPath;
	};

	class GameApplication
	{
	public:
		GameApplication(const GameApplicationSpecification& specification);
		virtual ~GameApplication();

		void OnEvent(Event& e);

		void Close();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* layer);

		inline static Window& GetWindow() { return *(s_Instance->m_Window); }
		inline static ImGuiLayer& GetImGuiLayer() { return *(s_Instance->m_ImGuiLayer); }
		inline static GameApplication& Get() { return *s_Instance; }

	private:
		void Run();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		GameApplicationSpecification m_Specification;
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

	// Implemented by Game
	GameApplication* CreateGameApplication();
}