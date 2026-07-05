#include "Radiant/rdpch.h"
#include "GameApplication.h"

#include <glfw/glfw3.h>

#include "Radiant/Renderer/Renderer.h"
#include "Radiant/ImGui/UI/Font.h"

namespace Radiant {

	GameApplication* GameApplication::s_Instance = nullptr;

	GameApplication::GameApplication(const GameApplicationSpecification& specification)
		: m_Specification(specification)
	{
		RADIANT_PROFILE_FUNCTION();

		RADIANT_TRACE("GameApplication Constructor");
		RADIANT_ASSERT(!s_Instance, "GameApplication already exists!");
		s_Instance = this;
		
		WindowSpecification windowSpec;
		windowSpec.Title = specification.Name;
		windowSpec.Width = specification.WindowWidth;
		windowSpec.Height = specification.WindowHeight;
		windowSpec.VSync = specification.VSync;
		windowSpec.IconPath = specification.IconPath;

		m_Window = Window::Create(windowSpec);
		m_Window->SetEventCallback(RADIANT_BIND_EVENT_FN(GameApplication::OnEvent));

		Renderer::Init();

		m_ImGuiLayer = ImGuiLayer::Create();

		RADIANT_ASSERT(m_ImGuiLayer, "Failed to Create ImGuiLayer");
		if (!specification.FontPath.empty())
		{
			Font::Init(FontConfiguration(specification.FontPath, specification.FontSize));
		}
		else
		{
			Font::Init(FontConfiguration());
		}

		PushOverlay(m_ImGuiLayer);

		RADIANT_TRACE("GameApplication Constructed");
	}

	GameApplication::~GameApplication()
	{
		RADIANT_PROFILE_FUNCTION();

		RADIANT_TRACE("GameApplication Destructor");

		// Explicit clear (rather than relying on the member destructor) so layers
		// are detached and destroyed before the renderer and fonts shut down
		m_LayerStack.Clear();

		Font::Shutdown();
		
		Renderer::Shutdown();
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

	void GameApplication::PopLayer(Layer* layer)
	{
		RADIANT_PROFILE_FUNCTION();

		// OnDetach fires when the removal is actually applied (ProcessPendingLayers) —
		// the layer keeps receiving updates/events until the end of this frame
		m_LayerStack.PopLayer(layer);
	}

	void GameApplication::PopOverlay(Layer* layer)
	{
		RADIANT_PROFILE_FUNCTION();

		m_LayerStack.PopOverlay(layer);
	}

	void GameApplication::OnEvent(Event& e)
	{
		RADIANT_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(RADIANT_BIND_EVENT_FN(GameApplication::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(RADIANT_BIND_EVENT_FN(GameApplication::OnWindowResize));

		// Events propagate top→bottom so overlays (UI) consume input before the world
		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			if (e.Handled)
				break;
			(*--it)->OnEvent(e);
		}
	}

	void GameApplication::Close()
	{
		m_Running = false;
	}

	void GameApplication::Run()
	{
		RADIANT_PROFILE_FUNCTION();

		while (m_Running)
		{
			RADIANT_PROFILE_SCOPE("RunLoop");

			// Variable timestep — simulation is framerate-dependent until the
			// fixed-step rework (RAD-25). m_LastFrameTime starts at 0, so the first
			// frame receives the full time since GLFW init; dies in the same rework.
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

				m_ImGuiLayer->Begin();
				{
					RADIANT_PROFILE_SCOPE("LayerStack OnImGuiRender");

					for (Layer* layer : m_LayerStack)
						layer->OnImGuiRender();
				}
				m_ImGuiLayer->End();
			}

			// glfwPollEvents runs in here: all event handlers (OnEvent) execute now,
			// at end of frame, inside the OS callbacks (queued dispatch lands in RAD-26)
			m_Window->OnUpdate();

			// Applied only between frames: updates and event handlers are the very
			// code that queues pushes/pops, and mutating the stack while it is being
			// walked would invalidate the iteration
			m_LayerStack.ProcessPendingLayers();
		}
	}

	bool GameApplication::OnWindowClose(WindowCloseEvent& e)
	{
		Close();
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
		// Deliberately not consumed — layers may also need to react to the resize
		return false;
	}
}