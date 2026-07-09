# Architecture Overview

**Status:** Living document — updated as phases land.

Radiant is a 2D game engine in C++20, born as a fork of TheCherno's Hazel and being incrementally re-architected to principal-engineering standards. It ships as a static library consumed by game executables; **Reaper** is the proving-ground game and must run at every milestone. (A legacy scratch project, Sandbox, was retired 2026-07-05 — it lives in git history.)

## Module Map

```text
Radiant/Source/Radiant/    (engine includes are namespaced: #include "Radiant/Core/...")
├── Core/          Application lifecycle, window, layers, Ref/Scope, log, assert, input
├── Events/        Event types + dispatcher (used by Core and layers)
├── Renderer/      API-agnostic rendering: Renderer2D batching, resource abstractions
├── Platform/
│   ├── OpenGL/    GL 4.5 backend (legacy — deleted when Vulkan reaches parity)
│   └── Windows/   Window, input, filesystem implementations (GLFW-based)
├── ECS/           Level (world), Entity, components, native scripts, level serialization
├── Physics/       Box2D integration
├── Asset/         Handle-based asset management + YAML registry
├── Serialization/ Stream I/O + binary asset-pack format (parked)
├── ImGui/         ImGui layer + font handling
├── Debug/         Chrome-trace instrumentation (compiled out by default)
└── Utilities/     Filesystem helpers, YAML converters
```

Dependency direction is strictly downward: games → engine public API → platform backends. Engine code never includes game code; graphics-API types never appear outside `Platform/<API>/`.

## Boot Sequence

```text
main()                            (EntryPoint.h — engine-owned)
 └─ Log::Init()
 └─ CreateGameApplication()       (implemented by the game)
     └─ GameApplication ctor
         ├─ Window::Create() ──── GLFW window + GL context
         ├─ Renderer::Init() ──── RenderCommand + Renderer2D batch resources
         ├─ ImGuiLayer::Create()─ backend-specific ImGui layer, pushed as overlay
         └─ Font::Init()
 └─ gameApp->Run()                (the main loop, below)
 └─ delete gameApp                (layers detached + destroyed, Renderer::Shutdown)
```

The engine owns `main()`; a game provides `CreateGameApplication()` returning its `GameApplication` subclass (see `Reaper/Source/Core/Game.cpp`). This inversion keeps lifecycle control in the engine.

## Frame Anatomy (current)

```text
per frame (GameApplication::Run):
 1. frameDelta = now - lastFrameTime         (variable frame delta, double)
 2. Window::PollEvents()                     — frame START; callbacks enqueue typed events only
 3. EventQueue::ProcessEvents(OnEvent)       — handlers run here, on the engine's call stack
 4. fixed-step drain (0..N steps):           — accumulator converts frame time to fixed steps
      TimerManager::Tick(fixedDelta)
      for each layer (bottom→top): OnFixedUpdate(fixedDelta)   — simulation
 5. for each layer (bottom→top): OnUpdate(frameDelta)          — render-rate, once
 6. ImGui Begin → each layer OnImGuiRender → End
 7. Window::Present()                        — swap buffers
 8. LayerStack::ProcessPendingLayers()       — deferred layer push/pop applied
```

Simulation is **framerate-independent** (fixed timestep + accumulator, with render interpolation — RAD-25, see [Time-And-Simulation](Time-And-Simulation.md)), and event delivery is **deterministic**: OS callbacks only enqueue; handlers run at the single `ProcessEvents` point at frame start (RAD-26, see [Event-System](Event-System.md)).

## Locked Strategy

Decisions locked 2026-07-04 (full rationale in `CLAUDE.md → Mission & Locked Strategy`):

1. **Incremental re-architecture** — no greenfield rewrite; the engine stays runnable at every milestone.
2. **2D-first, dimension-agnostic RHI** — 3D later is a renderer-module addition.
3. **Vulkan** via raw API + VMA + shaderc; the GL backend is deleted at parity.
4. **Phases:** 1 Triage → 2 Simulation Foundation → 3 RHI v2 + Vulkan → 4 Asset Pipeline v2 → 5 Editor. Tracked as Jira epics RAD-1…RAD-6.

## Reading Order

For a new engineer: this page → [Core-Application](Core-Application.md) → [Time-And-Simulation](Time-And-Simulation.md) → [Memory-And-Reference-Counting](Memory-And-Reference-Counting.md) → [Layer-System](Layer-System.md) → [Event-System](Event-System.md) → [ECS-And-Levels](ECS-And-Levels.md) → [Rendering](Rendering.md); then the rest as needed.
