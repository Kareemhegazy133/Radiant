# Event System

**Status:** Stable — queued dispatch landed 2026-07-09 (RAD-26).

## The Problem This Solves

The operating system reports things — a key went down, the mouse moved, the window resized — whenever it wants, in raw OS-specific form, by calling *your* code at moments you don't choose. Game code wants the opposite: those facts as typed, self-describing objects it can inspect and react to, delivered in an order and at a moment the engine controls.

The event system is that translation-and-routing layer. Think of events as **letters**: the OS delivers them to the door (GLFW callbacks), each is put in a typed envelope (`KeyPressedEvent`, `WindowResizeEvent`) and dropped through the slot into a **letterbox** (`EventQueue`). Every morning at the same time — the top of each frame — the application takes the stack out of the box, in arrival order, and carries each letter down the layer stack, where each layer opens only the envelope types it cares about. A layer that fully handles a letter marks it `Handled`, and it stops being passed on — that's how the pause menu eats the Escape key before the game sees it.

The letterbox is the load-bearing idea: the mail carrier (an OS callback) never stands in the hallway while you act on a letter. Callbacks only *record*; the engine *replays* at one point it owns. Before RAD-26, handlers executed synchronously inside the OS callbacks — arbitrary game logic running re-entrantly mid-`glfwPollEvents`, the bug class behind the DirectX-era alt-tab crashes.

## Architecture

### Event types

Events (`Events/`) form a small closed hierarchy over `Event` (`Event.h`): window events (`WindowClose`, `WindowResize`), key events, and mouse events. Two macros keep them RTTI-free:

- `EVENT_CLASS_TYPE(type)` — generates a static `GetStaticType()` and virtual `GetEventType()` pair, so dispatch can compare a runtime event against a compile-time type without `dynamic_cast`.
- `EVENT_CLASS_CATEGORY(bits)` — bitflag categories (`Application`, `Input`, `Keyboard`, `Mouse`, `MouseButton`) queryable via `IsInCategory`, letting handlers filter families without enumerating types.

Each event carries a `Handled` flag — the propagation short-circuit.

### The queue (`EventQueue`)

`EventQueue` (`Events/EventQueue.h`) is the letterbox: a `std::variant` of the nine concrete event types stored in a `std::vector` with persistent capacity — typed values, no heap allocation per event (rejected: Hazel-style heap-allocated polymorphic events), no switch-ladder re-translation (rejected: UE's raw-message POD shape, which exists because UE defers *untranslated* OS messages; GLFW already translated for us). Owned by value by `GameApplication`, declared before `m_Window` so the window's non-owning pointer (wired via `Window::SetEventQueue`) can never dangle. The two operations:

```cpp
data.Queue->Push(KeyPressedEvent(key, false));   // GLFW callback: enqueue only, return

m_EventQueue.ProcessEvents(RADIANT_BIND_EVENT_FN(GameApplication::OnEvent));  // Run(), frame start
```

`ProcessEvents` swap-and-drains two buffers: the pending buffer swaps into the processing buffer (O(1), no copy — UE's `ProcessDeferredEvents` copies its whole array every tick), events are visited in arrival order into the sink, and anything pushed *during* processing lands in the pending buffer for **next frame** — deterministic, and immune to iterator invalidation. A re-entrancy assert guards the swap idiom; a `static_assert(sizeof(QueuedEvent) <= 64)` keeps events compact (big payloads travel as handles, never by value). Adding an event type = one line in the variant alias, compile-enforced.

### Dispatch & propagation

```text
OS → GLFW callback (WindowsWindow) ── constructs the typed event
      └─ data.Queue->Push(event)    ── enqueue ONLY; the callback returns
⋯ frame start ⋯
GameApplication::Run
      └─ m_EventQueue.ProcessEvents(OnEvent)   ── the single defined point, once per frame
           ├─ EventDispatcher: WindowClose / WindowResize → app handlers
           └─ layers top→bottom: layer->OnEvent(e); stop when e.Handled
```

`EventDispatcher` (`Event.h`) wraps an event and offers `Dispatch<T>(handler)`: if the wrapped event's runtime type matches `T::GetStaticType()`, the handler runs and its `bool` return is OR-ed into `Handled`. Handlers are member functions bound with the `RADIANT_BIND_EVENT_FN` macro. The idiom every layer repeats (from Reaper's `UILayer`):

```cpp
void UILayer::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);   // opens only the envelope types it cares about
    dispatcher.Dispatch<KeyPressedEvent>(RADIANT_BIND_EVENT_FN(UILayer::OnKeyPressed));
}

bool UILayer::OnKeyPressed(KeyPressedEvent& e)
{
    if (e.GetKeyCode() == Key::Escape) { /* ... */ return true; }   // true → Handled: stops here
    return false;                                                    // false → falls through
}
```

Propagation is **top→bottom** through the layer stack (overlays first — UI consumes input before the world), stopping at the first layer that sets `Handled`.

**Timing:** handlers run at exactly one point per frame — the `ProcessEvents` call in `Run()`, immediately after `PollEvents()` and before the simulation gate — on the engine's own call stack, in arrival order, in the same frame the input arrived (zero added latency; RAD-25 put polling at frame start, RAD-26 moved the handlers out of the callbacks). `ProcessEvents` runs even while minimized: restore and close arrive *as events*, so gating it would build a window that can never wake up.

## Design Rationale

- **Static/virtual type-pair dispatch** is the right-sized alternative to RTTI or string comparison: zero allocation, one virtual call, and the dispatcher template reads naturally at call sites. Keep it — the Phase 2 rework changes *when* events are delivered, not *how* they're typed or dispatched.
- **`Handled` + top→bottom order** is a simple, predictable consumption model. It is *not* an input-focus system — that arrives with the editor (RAD-53) as a drain-time routing policy.
- **Why dispatch is queued (and what it doesn't buy):** the work moved in *space*, not time — off the OS's call stack and onto ours, same frame. Gains: handlers can safely do anything (Reaper pushes game states from key handlers — now on a plain stack); input is *data* (each frame's events exist as a serializable list, the seam for recording/replay and headless tests); one line owns input policy (future ImGui-capture/editor routing filters at the drain, not in six callbacks). Explicitly not gained: no latency win, no perf win, and the Windows title-bar-drag freeze remains (the OS confiscates the thread inside `DefWindowProc`'s modal loop — below GLFW, out of reach). The queue does make that freeze *harmless*: callbacks that fire from inside the modal loop now merely append, instead of running full handlers at the deepest re-entrancy the platform can produce.

## Known Issues & Evolution

- **No ImGui input gating** — ImGui and game layers both see every event (Hazel's `BlockEvents` was dropped in the fork; ImGui consumes input via its own chained GLFW callbacks, independent of the queue). Interim mitigation is Reaper's state checks; the real fix is capture-flag gating at the `ProcessEvents` drain (Phase 5, RAD-53) — the chokepoint now exists.
- **The variant is a closed set, deliberately.** The OS input vocabulary is inherently closed (UE closes the same set). Open-set *gameplay* events ("player died") are a different system at a different altitude — a future gameplay event bus (icebox) — and must not ride the OS input queue.
- **Events carry no source-window identity** — irrelevant single-window; the queue seam is where a window tag gets added when the editor's multi-window work lands (RAD-53).
- **`KeyTypedEvent` is defined but never emitted** (no GLFW char callback registered); kept in the variant for completeness. Text input is a two-line follow-up when the editor needs it (Phase 5).
