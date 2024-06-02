#include "Enginepch.h"
#include "GameApplication.h"

namespace Engine {

	GameApplication* GameApplication::s_Instance = nullptr;

	GameApplication::GameApplication(const std::string& name, GameApplicationCommandLineArgs args)
		: m_CommandLineArgs(args)
	{
		ENGINE_ASSERT(!s_Instance, "GameApplication already exists!");
		s_Instance = this;
		m_Window = Window::Create(WindowProps(name));
		m_Window->SetEventCallback(ENGINE_BIND_EVENT_FN(GameApplication::OnEvent));

		m_TextureManager = new TextureManager();
	}

	GameApplication::~GameApplication()
	{
		delete m_TextureManager;
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
		dispatcher.Dispatch<WindowCloseEvent>(ENGINE_BIND_EVENT_FN(GameApplication::OnWindowClose));

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
			float time = SFMLGetTime();
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