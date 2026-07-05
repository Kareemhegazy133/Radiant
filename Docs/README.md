# Radiant Engine Documentation

Per-system architecture documentation for the Radiant engine. Written for an engineer who has never seen the codebase: each document explains how the system works today, why it is built that way, and where it is headed.

## Document Map

| Document | System |
|----------|--------|
| [Architecture-Overview](Architecture-Overview.md) | The whole engine: module map, boot sequence, frame anatomy, strategy |
| [Core-Application](Core-Application.md) | Application lifecycle, main loop, window, input |
| [Memory-And-Reference-Counting](Memory-And-Reference-Counting.md) | Ownership model: `Ref`, `Scope`, `RefCounted` |
| [Layer-System](Layer-System.md) | `Layer` / `LayerStack` composition and update ordering |
| [Event-System](Event-System.md) | Event types, dispatcher, propagation |
| [Rendering](Rendering.md) | RHI abstraction, Renderer2D batching, OpenGL backend, Vulkan direction |
| [ECS-And-Levels](ECS-And-Levels.md) | `Level`, `Entity`, components, native scripts, level serialization |
| [Physics](Physics.md) | Box2D integration, body lifecycle, transform sync |
| [Asset-System](Asset-System.md) | Handles, registry, AssetManager, asset pack |
| [Serialization](Serialization.md) | Stream I/O, wire format rules, binary pack format |
| [Build-System](Build-System.md) | Premake structure, configurations, dependencies |

## Conventions

- **Structure:** every document has three sections — *Architecture* (what exists and how it works), *Design Rationale* (why it is built that way, what was traded off), and *Known Issues & Evolution* (defects and planned rework, referencing Jira `RAD-*` cards).
- **Status line:** each document opens with a status: `Stable`, `Stable — hardening planned`, or `Rework planned (Phase N)`, so a reader immediately knows how much to trust the design as final.
- **References:** docs cite files and symbols (`GameApplication::Run`, `Renderer2D.cpp`), not line numbers — line numbers rot.
- **Honesty rule:** docs describe the system as it *is*, including known defects. Aspirational design lives clearly marked under *Evolution*, never silently blended into *Architecture*.

## The Update Contract

**A change that alters a system's behavior or architecture updates that system's document in the same change.** Doc staleness is flagged as an ERROR by `/review`. When a Jira phase lands (e.g. the Phase 2 simulation rework), the affected docs' *Architecture* sections are rewritten and the corresponding *Evolution* entries removed.

`/sync-docs` pushes this folder to Confluence — keep documents self-contained (no relative links outside `Docs/`).
