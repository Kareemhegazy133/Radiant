# Event System

**Status:** Stable — queue rework planned (Phase 2: RAD-26).

## The Problem This Solves

The operating system reports things — a key went down, the mouse moved, the window resized — whenever it wants, in raw OS-specific form, by calling *your* code at moments you don't choose. Game code wants the opposite: those facts as typed, self-describing objects it can inspect and react to, delivered in an order and at a moment the engine controls.

The event system is that translation-and-routing layer. Think of events as **letters**: the OS drops them at the door (GLFW callbacks), each is put in a typed envelope (`KeyPressedEvent`, `WindowResizeEvent`), and the dispatcher carries them down the layer stack, where each layer opens only the envelope types it cares about. A layer that fully handles a letter marks it `Handled`, and it stops being passed on — that's how the pause menu eats the Escape key before the game sees it.

## Architecture

### Event types

Events (`Events/`) form a small closed hierarchy over `Event` (`Event.h`): window events (`WindowClose`, `WindowResize`), key events, and mouse events. Two macros keep them RTTI-free:

- `EVENT_CLASS_TYPE(type)` — generates a static `GetStaticType()` and virtual `GetEventType()` pair, so dispatch can compare a runtime event against a compile-time type without `dynamic_cast`.
- `EVENT_CLASS_CATEGORY(bits)` — bitflag categories (`Application`, `Input`, `Keyboard`, `Mouse`, `MouseButton`) queryable via `IsInCategory`, letting handlers filter families without enumerating types.

Each event carries a `Handled` flag — the propagation short-circuit.

### Dispatch & propagation

```text
OS → GLFW callback (WindowsWindow) ── constructs stack event
      └─ data.EventCallback(event)  ── bound to GameApplication::OnEvent
           ├─ EventDispatcher: WindowClose / WindowResize → app handlers
           └─ layers top→bottom: layer->OnEvent(e); stop when e.Handled
```

`EventDispatcher` (`Event.h`) wraps an event and offers `Dispatch<T>(handler)`: if the wrapped event's runtime type matches `T::GetStaticType()`, the handler runs and its `bool` return is OR-ed into `Handled`. Handlers are member functions bound with the `RADIANT_BIND_EVENT_FN` macro.

Propagation is **top→bottom** through the layer stack (overlays first — UI consumes input before the world), stopping at the first layer that sets `Handled`.

**Timing:** dispatch is currently *blocking* — handlers execute synchronously inside the GLFW C callback, which itself runs inside `glfwPollEvents()` at the **end** of the frame (`Window::OnUpdate`). The header itself documents this as interim ("a better strategy might be to buffer events in an event bus").

## Design Rationale

- **Static/virtual type-pair dispatch** is the right-sized alternative to RTTI or string comparison: zero allocation, one virtual call, and the dispatcher template reads naturally at call sites. Keep it — the Phase 2 rework changes *when* events are delivered, not *how* they're typed or dispatched.
- **`Handled` + top→bottom order** is a simple, predictable consumption model. It is *not* an input-focus system — that arrives with the editor (RAD-53) as a drain-time routing policy.
- **Why blocking dispatch must go:** handlers running inside an OS callback means arbitrary game logic executes re-entrantly mid-`glfwPollEvents` — Reaper already pushes game states from key handlers, and the deferred layer stack exists precisely to paper over one instance of this hazard. Events also arrive a frame late (polled after update/render).

## Known Issues & Evolution

- **Phase 2 (RAD-26):** callbacks will *enqueue* compact event structs; the application drains the queue at a single defined point at frame start. Deterministic timing, no re-entrancy, and the queue becomes the seam for input recording/replay and editor input routing. The dispatcher, categories, and `Handled` walk survive unchanged.
- **No ImGui input gating** — ImGui and game layers both see every event (Hazel's `BlockEvents` was dropped in the fork). Interim mitigation is Reaper's state checks; the real fix is capture-flag gating at queue-drain time (Phase 5, RAD-53).
