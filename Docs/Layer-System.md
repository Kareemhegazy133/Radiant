# Layer System

**Status:** Stable (ownership + detach ordering fixed in RAD-19, 2026-07-05).

## The Problem This Solves

An app is more than the game world. There's the world itself, UI drawn over it, and someday an editor wrapped around both — several coarse parts that each want a slice of every frame. Two orderings matter, and they're **opposite**: drawing wants world-first, UI-last (paint the background before the foreground), while input wants UI-first, world-last (clicking a menu button must not also fire the player's weapon underneath it).

Layers solve both with one structure: a stack of transparent sheets. Updating and drawing walk the stack bottom→top; input walks it top→bottom, and any sheet can say *"that was mine"* (`Handled`) to stop an event falling through to the sheets below. A layer is a coarse slice of the *application* — Reaper runs exactly two (the game view, and the UI overlay on top) — not a home for individual game systems; those live inside the Level.

## Architecture

Layers are the engine's **application-level composition mechanism**: coarse slices of the app (game view, UI overlay, future editor shell) with a defined update order and event priority. A `Layer` (`Core/Layer.h`) has five hooks:

| Hook | Called | Purpose |
|------|--------|---------|
| `OnAttach` / `OnDetach` | on add/remove from the stack | acquire/release resources |
| `OnUpdate(Timestep)` | every frame, bottom→top | logic + rendering |
| `OnImGuiRender` | every frame inside the ImGui pass | debug/tool/UI widgets |
| `OnEvent(Event&)` | on each event, **top→bottom** | input handling; set `Handled` to consume |

`LayerStack` (`Core/LayerStack.h`) is a single vector partitioned by an insert index: **layers** occupy the front half (world content), **overlays** the back half (UI, ImGui) — so overlays always update last (draw on top) and receive events first (consume input before the world).

```text
update order  ──────────────────────────►
[ layer, layer, layer | overlay, overlay ]
◄──────────────────────────  event order
```

### Deferred mutation

`PushLayer`/`PopLayer`/`PushOverlay`/`PopOverlay` do **not** mutate the stack immediately — they queue into pending lists that `ProcessPendingLayers()` applies at the end of the frame. This is a deliberate Radiant deviation from stock Hazel (which mutates immediately): layers are frequently pushed/popped from inside event handlers and state transitions (Reaper's `GameplayState::OnEnter` pushes `GameLayer`), and immediate mutation during the update/event iteration would invalidate the very vector being iterated.

Usage in practice: Reaper pushes `UILayer` as an overlay at boot and pushes/pops `GameLayer` as its state machine enters/leaves gameplay.

## Design Rationale

- **Layers vs "systems":** layers answer *"in what order do the app's coarse parts update, and who sees input first"* — nothing more. Game systems (physics, animation, rendering order inside the world) belong to the `Level` update, not to new layers. Engines rot when everything becomes a layer; keep the stack shallow (Reaper runs two).
- **Deferred push/pop** trades one frame of latency for structural safety — the same reasoning behind deferring entity destruction in ECS designs. The frame delay is invisible at app-composition granularity.
- **Overlay partition** is a two-tier z-order without a full priority system — sufficient until the editor, which will reuse the same mechanism (editor shell as overlay).

## Known Issues & Evolution

- **Input routing** is currently just the top→bottom `Handled` walk; ImGui does not gate input (the fork dropped Hazel's `BlockEvents`). A real focus/routing model arrives with the editor (Phase 5, RAD-53) on top of the Phase 2 event queue.

Resolved 2026-07-05 (RAD-19): the stack **owns** its layers (`Clear()` detaches + deletes, called by the destructor and explicitly by `GameApplication` for shutdown ordering), and `OnDetach` fires at actual removal inside `ProcessPendingLayers()` — a popped layer stays attached (and updating) until end of frame, by design. Consequence for consumers: a layer's `OnDetach` runs *deferred*, so it must not clear global state the next frame already depends on (asset scope belongs to Reaper's state machine, not layers).
