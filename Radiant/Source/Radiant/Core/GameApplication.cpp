#include "Radiant/rdpch.h"
#include "GameApplication.h"

#include <glfw/glfw3.h>

#include "Radiant/Renderer/Renderer.h"
#include "Radiant/ImGui/UI/Font.h"

namespace Radiant {

	GameApplication* GameApplication::s_Instance = nullptr;

	GameApplication::GameApplication(const GameApplicationSpecification& specification)
		: m_Specification(specification), m_Clock(specification.SimulationRate)
	{
		RADIANT_PROFILE_FUNCTION();

		RADIANT_TRACE("GameApplication Constructor");
		RADIANT_TRACE("Simulation: {} Hz ({:.2f} ms fixed step)", specification.SimulationRate, m_Clock.GetFixedDeltaTime() * 1000.0);
		RADIANT_ASSERT(!s_Instance, "GameApplication already exists!");
		s_Instance = this;
		
		WindowSpecification windowSpec;
		windowSpec.Title = specification.Name;
		windowSpec.Width = specification.WindowWidth;
		windowSpec.Height = specification.WindowHeight;
		windowSpec.VSync = specification.VSync;
		windowSpec.IconPath = specification.IconPath;

		m_Window = Window::Create(windowSpec);
		m_Window->SetEventQueue(&m_EventQueue);

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

		RADIANT_ASSERT(layer, "PushLayer: null layer");
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void GameApplication::PushOverlay(Layer* layer)
	{
		RADIANT_PROFILE_FUNCTION();

		RADIANT_ASSERT(layer, "PushOverlay: null layer");
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

		// Priming here means frame one measures loop-start → frame-1 instead of
		// GLFW-init → frame-1 (which delivered all of boot as one giant timestep)
		m_LastFrameTime = glfwGetTime();

		while (m_Running)
		{
			RADIANT_PROFILE_SCOPE("RunLoop");

			// The variable frame delta feeds the clock (which converts it to fixed
			// steps) and the render-rate OnUpdate hook — never simulation directly
			double time = glfwGetTime();
			double frameDelta = time - m_LastFrameTime;
			m_LastFrameTime = time;
			Timestep timestep = (float)frameDelta;

			// Frame START: simulation must see this frame's input, not last
			// frame's. Polling only enqueues; handlers run in ProcessEvents
			// below, on this call stack — never inside the OS callbacks.
			// Unconditional even while minimized: restore/close arrive as
			// events, so gating this would make a window that can't wake up.
			m_Window->PollEvents();
			m_EventQueue.ProcessEvents(RADIANT_BIND_EVENT_FN(GameApplication::OnEvent));

			if (!m_Minimized)
			{
				{
					RADIANT_PROFILE_SCOPE("LayerStack OnFixedUpdate");

					m_Clock.BeginFrame(frameDelta);
					Timestep fixedTimestep = (float)m_Clock.GetFixedDeltaTime();

					while (m_Clock.ConsumeStep())
					{
						// Timers fire first so layers observe a consistent
						// post-timer world (mirrors UE's tick placement)
						m_TimerManager.Tick(m_Clock.GetFixedDeltaTime());

						for (Layer* layer : m_LayerStack)
							layer->OnFixedUpdate(fixedTimestep);
					}
				}

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

			// Present runs even while minimized (GLFW tolerates it) — polling and
			// presenting are unconditional; only simulation and render are gated
			m_Window->Present();

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