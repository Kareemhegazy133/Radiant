# Core: Application Lifecycle

**Status:** Stable — event queue planned (Phase 2: RAD-26). Fixed-timestep loop landed 2026-07-05 (RAD-25; see [Time-And-Simulation](Time-And-Simulation.md)).

## The Problem This Solves

Every program needs an answer to three questions: what starts first (and in what order), what happens repeatedly while running, and what shuts down last. In a game engine the ordering is unforgiving — the renderer cannot exist before the window that owns the graphics context, the window cannot come before logging (or you'll never see why it failed), and teardown must run in reverse or you destroy things others still depend on. `GameApplication` is the engine's **spine**: the one object that owns boot order, the frame heartbeat, and teardown, so no other system ever has to guess whether its dependencies exist yet.

The "frame heartbeat" is the second idea. A game is not a program that runs once — it's a loop that repeats ~60+ times per second: collect whatever the OS reported (keys, clicks, resizes), advance the simulation, draw everything, and show the finished image. Everything the player ever experiences happens inside one beat of this loop; the entire engine exists to make each beat fast and predictable.

## Architecture

### GameApplication

`Core/GameApplication.{h,cpp}` is the engine's root object and a **singleton** (`s_Instance`, asserted unique, reachable via `GameApplication::Get()`). It owns, in construction order:

- the **window** (`Scope<Window>`, created from a `WindowSpecification` derived from the app spec),
- the **renderer** (`Renderer::Init()` — static subsystem, not a member),
- the **ImGui overlay** (`ImGuiLayer::Create()` factory → backend-specific layer, pushed as an overlay),
- the **layer stack** (see [Layer-System](Layer-System.md)).

`GameApplicationSpecification` carries name, window size, VSync, working directory, plus Radiant-specific extensions: `IconPath` (window icon), `FontPath`/`FontSize` (default ImGui font), and `SimulationRate` (fixed simulation step rate in Hz, default 60). A game subclasses `GameApplication`, configures the spec, and pushes its layers (see `Reaper/Source/Core/Game.cpp`).

`main()` lives in the **engine** (`Core/EntryPoint.h`, compiled only on Windows): it initializes logging, calls the game-provided `CreateGameApplication()`, runs, and deletes. `Run()` is private; `main` is befriended — the loop cannot be invoked from game code.

### The Main Loop

`GameApplication::Run()` primes `m_LastFrameTime` immediately before the loop (so frame one measures loop-start → frame-1, not GLFW-init → frame-1), then per frame:

1. Compute the frame delta from `glfwGetTime()` minus the previous frame's time (`double` precision).
2. `Window::PollEvents()` — frame **start**, so simulation sees this frame's input. Event callbacks fire here (handlers still run inside the OS callbacks until the RAD-26 queue).
3. If not minimized:
   - **Fixed-step drain:** `FrameClock::BeginFrame(frameDelta)` deposits scaled time; `while (ConsumeStep())` runs `TimerManager::Tick` then every layer's `OnFixedUpdate(fixedDelta)` bottom→top — 0..N simulation steps per frame.
   - **Render-rate update:** every layer's `OnUpdate(frameDelta)` bottom→top, exactly once.
   - ImGui pass: `ImGuiLayer::Begin()` → every layer's `OnImGuiRender()` → `End()`.
4. `Window::Present()` — swap buffers at frame end (runs even while minimized; only simulation and render are gated).
5. `LayerStack::ProcessPendingLayers()` — deferred layer pushes/pops are applied between frames.

The heart of it, condensed from `Run()` to the load-bearing lines:

```cpp
m_Window->PollEvents();                        // frame START — simulation sees this frame's input
m_Clock.BeginFrame(frameDelta);                // deposit real time (scaled by the time scale)
while (m_Clock.ConsumeStep())                  // 0..N whole fixed steps banked this frame
{
    m_TimerManager.Tick(m_Clock.GetFixedDeltaTime());
    for (Layer* layer : m_LayerStack)
        layer->OnFixedUpdate(fixedTimestep);   // simulation, always the same delta
}
for (Layer* layer : m_LayerStack)
    layer->OnUpdate(timestep);                 // render-rate, exactly once
m_Window->Present();                           // swap at frame end
```

The time accounting behind step 3 — accumulator, dilation/pause, interpolation alpha, timers — is owned by `FrameClock`/`TimerManager` and documented in [Time-And-Simulation](Time-And-Simulation.md).

Shutdown: `Close()` merely sets `m_Running = false`; the destructor detaches and deletes every layer, then shuts down fonts and the renderer.

### Window & Input

`Window` is an abstract seam with one implementation, `Platform/Windows/WindowsWindow` (GLFW). The window owns the `GraphicsContext` (GL context today). GLFW callbacks translate OS events into Radiant `Event` objects and invoke the application's `OnEvent` callback synchronously (see [Event-System](Event-System.md)).

`Input` (`Core/Input.h`) is a static polling API (`IsKeyPressed`, `GetMousePosition`) implemented in `Platform/Windows/WindowsInput.cpp` by querying GLFW directly through `GameApplication::Get().GetWindow()`.

## Design Rationale

- **Singleton application** is the pragmatic industry norm for a single-app engine (UE has `GEngine`); the cost is that `Input` and the ImGui layers reach through the global. Acceptable at this scale; module boundaries — not the singleton — are what keep it from rotting.
- **Engine-owned `main()`** guarantees the engine controls init/shutdown ordering — games cannot boot subsystems in the wrong order.
- **ImGui behind a factory** (`ImGuiLayer::Create()` switching on `RendererAPI::GetAPI()`) keeps the app code backend-agnostic — this seam is exactly where the Vulkan ImGui layer slots in (RAD-41).

## Known Issues & Evolution

- **Event handlers still run inside OS callbacks** — polling moved to frame start with the RAD-25 loop, but dispatch remains blocking until the event queue lands (RAD-26).
- **Windows-only** — `PlatformDetection.h` hard-errors on other platforms; the `Window`/`Input` seams exist, but no other implementations do. Not on any phase roadmap; deliberate.
