#pragma once

#include "Radiant/Core/Base.h"
#include "Radiant/Core/Log.h"

#include "Window.h"
#include "Radiant/Core/LayerStack.h"
#include "Radiant/Events/Event.h"
#include "Radiant/Events/ApplicationEvent.h"

#include "Radiant/Core/Timestep.h"
#include "Radiant/Core/FrameClock.h"
#include "Radiant/Core/TimerManager.h"

#include "Radiant/ImGui/ImGuiLayer.h"

int main(int argc, char** argv);

namespace Radiant {

	/**
	 * Boot-time configuration a game passes to the GameApplication constructor.
	 * FontPath/FontSize (pixels) select the default ImGui font; an empty
	 * FontPath falls back to ImGui's built-in font. IconPath is loaded as the
	 * window icon and logged + skipped if the image cannot be loaded.
	 * SimulationRate is the fixed simulation step rate in Hz (steps per
	 * simulation second); zero is a programmer error (asserted in FrameClock).
	 */
	struct GameApplicationSpecification
	{
		std::string Name = "Game";
		uint32_t WindowWidth = 1280, WindowHeight = 720;
		bool VSync = true;
		std::filesystem::path IconPath;
		std::filesystem::path FontPath;
		float FontSize = 18.0f;
		uint32_t SimulationRate = 60;
	};

	/**
	 * The engine's root object: owns boot order (window → renderer → ImGui
	 * overlay), the main loop, and reverse-order teardown. A singleton —
	 * constructing a second instance asserts. Games subclass it, fill in a
	 * specification, and push their layers; the engine-owned main() (see
	 * EntryPoint.h) creates it via CreateGameApplication(), runs it, and
	 * deletes it. Pushed layers are owned by the layer stack.
	 */
	class GameApplication
	{
	public:
		GameApplication(const GameApplicationSpecification& specification);
		virtual ~GameApplication();

		/**
		 * Entry point for every window/input event. Currently BLOCKING: called
		 * synchronously from the GLFW callbacks during frame-start event
		 * polling — handlers run inside OS callbacks, so do not assume
		 * mid-frame safety (a queued dispatch replaces this in Phase 2,
		 * RAD-26). Dispatches window close/resize to the application, then
		 * walks the layers top→bottom until one sets Handled.
		 */
		void OnEvent(Event& e);

		/**
		 * Requests exit: the loop stops after finishing the current frame.
		 * Destroys nothing itself — safe to call from event handlers.
		 */
		void Close();

		/**
		 * Transfers ownership of a heap-allocated layer to the layer stack,
		 * which deletes it at pop or shutdown. OnAttach runs immediately, but
		 * insertion is deferred — the layer starts receiving updates and events
		 * next frame.
		 */
		void PushLayer(Layer* layer);
		/** PushLayer, but into the overlay partition: updates last (draws on top), sees events first. Same ownership and deferral. */
		void PushOverlay(Layer* layer);
		/**
		 * Requests removal. Deferred: the layer keeps updating and receiving
		 * events until end of frame, when OnDetach fires and the stack deletes
		 * it. No-op if the layer is not currently in the stack (including one
		 * whose push is still pending from this same frame).
		 */
		void PopLayer(Layer* layer);
		/** PopLayer for overlays — identical deferred detach/delete semantics. */
		void PopOverlay(Layer* layer);

		// These accessors reach through the singleton: valid only while the
		// application exists (asserted in Debug/Release; unchecked in Dist)
		inline static Window& GetWindow() { RADIANT_ASSERT(s_Instance, "GetWindow() before GameApplication construction"); return *(s_Instance->m_Window); }
		inline static ImGuiLayer& GetImGuiLayer() { RADIANT_ASSERT(s_Instance, "GetImGuiLayer() before GameApplication construction"); return *(s_Instance->m_ImGuiLayer); }
		inline static GameApplication& Get() { RADIANT_ASSERT(s_Instance, "Get() before GameApplication construction"); return *s_Instance; }
		/** Engine-global timer service (simulation-time; see TimerManager). Per-Level migration planned with RAD-52. */
		inline static TimerManager& GetTimerManager() { RADIANT_ASSERT(s_Instance, "GetTimerManager() before GameApplication construction"); return s_Instance->m_TimerManager; }

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
		// Both doubles: GLFW hands out double seconds, and the app keeps full
		// precision — narrowing to float Timestep happens only at the layer boundary
		double m_LastFrameTime = 0.0;
		FrameClock m_Clock;
		TimerManager m_TimerManager;

	private:
		static GameApplication* s_Instance;
		friend int ::main(int argc, char** argv);
		// The Time facade is the public face of m_Clock — friendship keeps the
		// loop-driving methods (BeginFrame/ConsumeStep) unreachable from game code
		friend class Time;
	};

	/**
	 * Implemented by the game: constructs its GameApplication subclass. The
	 * engine-owned main() calls this exactly once, takes ownership of the
	 * returned instance, and deletes it at shutdown.
	 */
	GameApplication* CreateGameApplication();
}