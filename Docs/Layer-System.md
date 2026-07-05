# Layer System

**Status:** Stable — ownership fixes planned (Phase 1: RAD-19).

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

- **Ownership split (RAD-19):** `LayerStack` stores raw `Layer*` but its destructor deletes nothing — `GameApplication`'s destructor does the deleting. Fix: the stack owns and deletes its layers, full stop.
- **Detach-ordering bug (RAD-19):** `GameApplication::PopLayer` calls `OnDetach()` immediately while removal is deferred — a detached layer keeps receiving `OnUpdate`/`OnEvent` until the frame ends. Fix: `OnDetach` fires at actual removal inside `ProcessPendingLayers()`.
- **Input routing** is currently just the top→bottom `Handled` walk; ImGui does not gate input (the fork dropped Hazel's `BlockEvents`). A real focus/routing model arrives with the editor (Phase 5, RAD-53) on top of the Phase 2 event queue.
