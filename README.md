# Radiant

[![Build](https://github.com/Kareemhegazy133/Radiant/actions/workflows/build.yml/badge.svg?branch=dev)](https://github.com/Kareemhegazy133/Radiant/actions/workflows/build.yml)

A 2D game engine in C++20, being re-architected system by system to principal-engineering standards — with a Vulkan renderer as the next major milestone. **Reaper**, the game built on it, must run at every step: the engine is never allowed to go dark.

Radiant began as a fork of [TheCherno's Hazel](https://github.com/TheCherno/Hazel) and is being deliberately rebuilt from that foundation: every system gets audited, documented ground-up, and re-architected with its design rationale written down. The goal is not shipping speed — it's an engine where every architectural decision can be explained and defended.

## Status

| Phase | Scope | State |
|-------|-------|-------|
| 1 — Triage & Foundation | Crash/leak fixes, ownership contracts, build system, CI | ✅ Complete |
| 2 — Simulation Foundation | Fixed-timestep loop, event queue, per-Level physics, data-only components | Next |
| 3 — RHI v2 + Vulkan | Command-buffer RHI, raw Vulkan + VMA + shaderc, delete OpenGL | Planned |
| 4 — Asset Pipeline v2 | Editor/runtime split, binary asset pack, MSDF text | Planned |
| 5 — Editor | Docking editor, hierarchy/inspector, play-in-editor | Planned |

## What's in the engine today

- **Batched 2D renderer** — quads, sprites, subtextures, and lines through a CPU-side batcher (one draw per primitive family), on OpenGL 4.5 behind an API-agnostic layer
- **Entity Component System** — [entt](https://github.com/skypjack/entt)-backed `Level`/`Entity` model with UUID-stable identity and YAML level serialization (`.rdlvl`)
- **2D physics** — Box2D integration driven by ECS component lifecycle signals
- **Handle-based assets** — content referenced by 64-bit handles, never paths; YAML registry (`.rdar`), lazy loading
- **Application core** — layer stack with deferred mutation, typed event dispatch, intrusive thread-safe reference counting, Debug-only leak tracking
- **Tooling** — ImGui (docking) integration, Chrome-trace instrumentation, three build configurations with checks active in Debug *and* Release

## Getting started

Requirements: Windows, Visual Studio 2022 (or Rider using the same toolchain).

```
git clone --recursive https://github.com/Kareemhegazy133/Radiant.git
cd Radiant
.\Scripts\Setup-Windows.bat
```

The setup script runs the vendored premake (no installs needed) and generates `Radiant.sln`. Build and run **Reaper** (the start-up project) — main menu → gameplay with a small physics scene → pause menu.

## Project structure

```text
Radiant/Source/Radiant/   the engine (namespaced includes: #include "Radiant/Core/...")
Radiant/Vendor/           library dependencies (submodules + vendored single-headers)
Reaper/                   the proving-ground game
Docs/                     per-system architecture documentation
Tools/                    build tooling (vendored premake binaries)
Build.lua                 premake workspace root; each project has its own premake5.lua
```

## Documentation

[`Docs/`](Docs/README.md) holds a document per engine system, each written ground-up for a reader with no engine experience: *the problem this solves* → *architecture* → *design rationale* → *known issues*. Start with the [Architecture Overview](Docs/Architecture-Overview.md).

## License

MIT — see [LICENSE.txt](LICENSE.txt).
