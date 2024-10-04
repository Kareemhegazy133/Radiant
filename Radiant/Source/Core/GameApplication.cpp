#include "rdpch.h"
#include "GameApplication.h"

#include <glfw/glfw3.h>

#include "Renderer/Renderer.h"

namespace Radiant {

	GameApplication* GameApplication::s_Instance = nullptr;

	GameApplication::GameApplication(const std::string& name, const uint32_t width, const uint32_t height, GameApplicationCommandLineArgs args)
		: m_CommandLineArgs(args)
	{
		RADIANT_PROFILE_FUNCTION();

		RADIANT_TRACE("GameApplication Constructor");
		RADIANT_ASSERT(!s_Instance, "GameApplication already exists!");
		s_Instance = this;
		
		m_Window = Window::Create(WindowProps(name, width, height));
		m_Window->SetEventCallback(RADIANT_BIND_EVENT_FN(GameApplication::OnEvent));

		Renderer::Init();
		RADIANT_TRACE("GameApplication Constructed");
	}

	GameApplication::~GameApplication()
	{
		RADIANT_PROFILE_FUNCTION();

		Renderer::Shutdown();
		RADIANT_TRACE("GameApplication Destructed");
	}

	void GameApplication::PushLayer(Layer* layer)
	{
		RADIANT_PROFILE_FUNCTION();

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void GameApplication::PushOverlay(Layer* layer)
	{
		RADIANT_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void GameApplication::OnEvent(Event& e)
	{
		RADIANT_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(RADIANT_BIND_EVENT_FN(GameApplication::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(RADIANT_BIND_EVENT_FN(GameApplication::OnWindowResize));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			if (e.Handled)
				break;
			(*--it)->OnEvent(e);
		}
	}

	void GameApplication::Run()
	{
		RADIANT_PROFILE_FUNCTION();

		while (m_Running)
		{
			RADIANT_PROFILE_SCOPE("RunLoop");

			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				{
					RADIANT_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(timestep);
				}
			}

			m_Window->OnUpdate();
		}
	}

	bool GameApplication::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool GameApplication::OnWindowResize(WindowResizeEvent& e)
	{
		RADIANT_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
		return false;
	}
}