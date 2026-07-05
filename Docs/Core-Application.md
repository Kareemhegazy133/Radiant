# Core: Application Lifecycle

**Status:** Stable — loop rework planned (Phase 2: RAD-25, RAD-26).

## The Problem This Solves

Every program needs an answer to three questions: what starts first (and in what order), what happens repeatedly while running, and what shuts down last. In a game engine the ordering is unforgiving — the renderer cannot exist before the window that owns the graphics context, the window cannot come before logging (or you'll never see why it failed), and teardown must run in reverse or you destroy things others still depend on. `GameApplication` is the engine's **spine**: the one object that owns boot order, the frame heartbeat, and teardown, so no other system ever has to guess whether its dependencies exist yet.

The "frame heartbeat" is the second idea. A game is not a program that runs once — it's a loop that repeats ~60+ times per second: measure how much time passed, let every part of the app update by that much, draw everything, show the finished image, and collect whatever the OS reported (keys, clicks, resizes). Everything the player ever experiences happens inside one beat of this loop; the entire engine exists to make each beat fast and predictable.

## Architecture

### GameApplication

`Core/GameApplication.{h,cpp}` is the engine's root object and a **singleton** (`s_Instance`, asserted unique, reachable via `GameApplication::Get()`). It owns, in construction order:

- the **window** (`Scope<Window>`, created from a `WindowSpecification` derived from the app spec),
- the **renderer** (`Renderer::Init()` — static subsystem, not a member),
- the **ImGui overlay** (`ImGuiLayer::Create()` factory → backend-specific layer, pushed as an overlay),
- the **layer stack** (see [Layer-System](Layer-System.md)).

`GameApplicationSpecification` carries name, window size, VSync, working directory, plus Radiant-specific extensions: `IconPath` (window icon) and `FontPath`/`FontSize` (default ImGui font). A game subclasses `GameApplication`, configures the spec, and pushes its layers (see `Reaper/Source/Core/Game.cpp`).

`main()` lives in the **engine** (`Core/EntryPoint.h`, compiled only on Windows): it initializes logging, calls the game-provided `CreateGameApplication()`, runs, and deletes. `Run()` is private; `main` is befriended — the loop cannot be invoked from game code.

### The Main Loop

`GameApplication::Run()` per frame:

1. Compute `Timestep` from `glfwGetTime()` minus the previous frame's time (variable delta).
2. If not minimized: iterate layers bottom→top calling `OnUpdate(ts)` — all game logic, physics stepping, and rendering happen here.
3. ImGui pass: `ImGuiLayer::Begin()` → every layer's `OnImGuiRender()` → `End()`.
4. `Window::OnUpdate()` — `glfwPollEvents()` (event callbacks fire *here*, end of frame) then `SwapBuffers()`.
5. `LayerStack::ProcessPendingLayers()` — deferred layer pushes/pops are applied between frames.

Shutdown: `Close()` merely sets `m_Running = false`; the destructor detaches and deletes every layer, then shuts down fonts and the renderer.

### Window & Input

`Window` is an abstract seam with one implementation, `Platform/Windows/WindowsWindow` (GLFW). The window owns the `GraphicsContext` (GL context today). GLFW callbacks translate OS events into Radiant `Event` objects and invoke the application's `OnEvent` callback synchronously (see [Event-System](Event-System.md)).

`Input` (`Core/Input.h`) is a static polling API (`IsKeyPressed`, `GetMousePosition`) implemented in `Platform/Windows/WindowsInput.cpp` by querying GLFW directly through `GameApplication::Get().GetWindow()`.

## Design Rationale

- **Singleton application** is the pragmatic industry norm for a single-app engine (UE has `GEngine`); the cost is that `Input` and the ImGui layers reach through the global. Acceptable at this scale; module boundaries — not the singleton — are what keep it from rotting.
- **Engine-owned `main()`** guarantees the engine controls init/shutdown ordering — games cannot boot subsystems in the wrong order.
- **ImGui behind a factory** (`ImGuiLayer::Create()` switching on `RendererAPI::GetAPI()`) keeps the app code backend-agnostic — this seam is exactly where the Vulkan ImGui layer slots in (RAD-41).

## Known Issues & Evolution

- **Variable timestep** — simulation is framerate-dependent. Phase 2 (RAD-25) restructures the loop into: drain event queue → fixed-step simulation with accumulator → interpolated render. The first-frame timestep bug (`m_LastFrameTime` starts at 0, so frame one receives the full time since GLFW init) dies in the same rework.
- **Events fire at end-of-frame from inside OS callbacks** — replaced by a frame-start event queue (RAD-26).
- **VSync spec ignored** — `WindowsWindow::Init` hardcodes `SetVSync(true)` (RAD-18).
- **Windows-only** — `PlatformDetection.h` hard-errors on other platforms; the `Window`/`Input` seams exist, but no other implementations do. Not on any phase roadmap; deliberate.
