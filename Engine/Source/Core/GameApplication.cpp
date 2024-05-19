#include "Enginepch.h"
#include "GameApplication.h"

namespace Engine {

	GameApplication* GameApplication::s_Instance = nullptr;

	GameApplication::GameApplication()
	{
		ENGINE_ASSERT(!s_Instance, "GameApplication already exists!");
		s_Instance = this;
		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
	}

	GameApplication::~GameApplication()
	{

	}

	void GameApplication::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void GameApplication::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void GameApplication::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void GameApplication::Run()
	{
		while (m_Window->IsRunning())
		{
			//GAME_INFO("Game Running..");
			float time = sfmlGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			m_Window->OnUpdate();

			// draw everything here...
			for (Layer* layer : m_LayerStack)
				layer->OnUpdate(timestep);

			// end the current frame
			m_Window->Display();
		}
	}

	bool GameApplication::OnWindowClose(WindowCloseEvent& e)
	{
		m_Window->Shutdown();
		return true;
	}
}