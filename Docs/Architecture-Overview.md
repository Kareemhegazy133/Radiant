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
 1. timestep = now - lastFrameTime          (variable delta)
 2. for each layer (bottom→top): OnUpdate(ts)   — game logic, physics, rendering
 3. ImGui Begin → each layer OnImGuiRender → End
 4. Window::OnUpdate()                       — glfwPollEvents + SwapBuffers
 5. LayerStack::ProcessPendingLayers()       — deferred layer push/pop applied
```

Two properties of this loop matter and both change in Phase 2 (RAD-25, RAD-26): the timestep is **variable** (simulation is framerate-dependent), and events fire at the **end** of the frame from inside `glfwPollEvents` via blocking callbacks. The target loop is: drain event queue → fixed-step simulation (accumulator) → interpolated render.

## Locked Strategy

Decisions locked 2026-07-04 (full rationale in `CLAUDE.md → Mission & Locked Strategy`):

1. **Incremental re-architecture** — no greenfield rewrite; the engine stays runnable at every milestone.
2. **2D-first, dimension-agnostic RHI** — 3D later is a renderer-module addition.
3. **Vulkan** via raw API + VMA + shaderc; the GL backend is deleted at parity.
4. **Phases:** 1 Triage → 2 Simulation Foundation → 3 RHI v2 + Vulkan → 4 Asset Pipeline v2 → 5 Editor. Tracked as Jira epics RAD-1…RAD-6.

## Reading Order

For a new engineer: this page → [Core-Application](Core-Application.md) → [Memory-And-Reference-Counting](Memory-And-Reference-Counting.md) → [Layer-System](Layer-System.md) → [Event-System](Event-System.md) → [ECS-And-Levels](ECS-And-Levels.md) → [Rendering](Rendering.md); then the rest as needed.
