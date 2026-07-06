# Radiant Engine Documentation

Per-system architecture documentation for the Radiant engine. Written for an engineer who has never seen the codebase: each document explains how the system works today, why it is built that way, and where it is headed.

## Document Map

| Document | System |
|----------|--------|
| [Architecture-Overview](Architecture-Overview.md) | The whole engine: module map, boot sequence, frame anatomy, strategy |
| [Core-Application](Core-Application.md) | Application lifecycle, main loop, window, input |
| [Time-And-Simulation](Time-And-Simulation.md) | Fixed-timestep loop: FrameClock, dilation/pause, render interpolation, timers |
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

- **Structure:** every document has four sections — *The Problem This Solves* (a ground-up explanation for a reader with **no** engine experience: the problem in plain language, the core idea, an analogy where it genuinely clarifies — written the way you'd teach it at a whiteboard, before any implementation detail), *Architecture* (what exists and how it works), *Design Rationale* (why it is built that way, what was traded off), and *Known Issues & Evolution* (defects and planned rework, referencing Jira `RAD-*` cards).
- **Teach, don't just describe:** a reader should leave a document able to *explain* the system, not just navigate it. Prefer plain language, concrete examples, and one strong analogy over exhaustive enumeration.
- **Show the code where code teaches best:** each document carries at least one SHORT snippet of the load-bearing lines — the idiom a reader would actually write (composing an entity, dispatching an event, the release decrement), verbatim from the source or faithfully condensed (and marked "condensed" when it is). Never full files; a snippet that needs scrolling is describing, not teaching.
- **Status line:** each document opens with a status: `Stable`, `Stable — hardening planned`, or `Rework planned (Phase N)`, so a reader immediately knows how much to trust the design as final.
- **References:** docs cite files and symbols (`GameApplication::Run`, `Renderer2D.cpp`), not line numbers — line numbers rot.
- **Honesty rule:** docs describe the system as it *is*, including known defects. Aspirational design lives clearly marked under *Evolution*, never silently blended into *Architecture*.

## The Update Contract

**A change that alters a system's behavior or architecture updates that system's document in the same change.** Doc staleness is flagged as an ERROR by `/review`. When a Jira phase lands (e.g. the Phase 2 simulation rework), the affected docs' *Architecture* sections are rewritten and the corresponding *Evolution* entries removed.

`/sync-docs` pushes this folder to Confluence — keep documents self-contained (no relative links outside `Docs/`).
