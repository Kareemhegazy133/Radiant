# Implementation Plan — RAD-26: Event queue: buffer OS events, drain at frame start

| Field | Value |
|-------|-------|
| **Jira** | [RAD-26](https://hndredgames.atlassian.net/browse/RAD-26) |
| **Epic** | RAD-2 — Phase 2: Simulation Foundation |
| **Story status** | To Do |
| **Dependencies** | RAD-25 (landed 2026-07-06) — frame-start `PollEvents()`/`Present()` split is the foundation; this story completes playbook §1's event leg |
| **Planned** | 2026-07-08 |

Scope from the story: callbacks **enqueue** compact event structs; the application **drains** the queue at a single defined point at frame start. **Keep** the dispatcher/category design and top-down layer propagation with `Handled`. Out of scope: input recording/replay and editor input routing (the queue is their seam, not their implementation — RAD-53), ImGui input gating (RAD-53), the never-emitted `KeyTypedEvent`/char callback (noted in §10).

---

## 0. The Problem, Ground Up

Two terms first. An **event** is a small note saying "something happened" — *"the Escape key went down"*, *"the mouse moved to (400, 300)"*. A **callback** is a function you hand to someone else, and *they* decide when to call it — you don't call it, you get called.

The engine can't see the keyboard or mouse directly. Once per frame it calls `glfwPollEvents()` — "GLFW, ask Windows what happened since last frame" — and GLFW answers by calling **our callbacks**, once per thing that happened. The mistake we currently make: **inside those callbacks, we do all the work immediately.**

```text
glfwPollEvents()                        // "OS, anything happen?"
  └─ Windows: "key down: ESC" → GLFW calls OUR callback, right now:
       app.OnEvent(KeyPressedEvent)     // walks the ENTIRE layer stack...
         └─ GameLayer::OnKeyPressed     // ...running GAME code...
              └─ PushState<GamePausedState>()   // ...that rebuilds game state
       // only NOW does control return to GLFW — which is still holding
       // the REST of this frame's events, undelivered
```

Concrete failure: one poll can deliver a *batch* — say 17 mouse moves plus that Escape press. When the Escape handler rebuilds the game state (which pops and pushes layers), GLFW is standing mid-batch, about to deliver events #4–18 into a layer stack we just rearranged. The only thing saving us today is that `LayerStack` secretly defers all layer changes to end of frame — a guard rail bolted around this exact cliff. This is the bug class behind the DirectX-era alt-tab and drag-resize crashes: resize/focus handlers executing *inside* the OS's delivery call while a frame was mid-flight.

**The fix is a letterbox.** Today the mail carrier rings the doorbell and stands in the hallway while you act on each letter — and if a letter makes you renovate the hallway, he's still standing in it. Instead: the carrier drops everything through the slot and leaves; every morning, at the same time, at your desk, you go through the stack in arrival order. Nothing about the *letters* changes — only when and where you open them.

**UE does exactly this.** Its Windows callback writes each message onto a notepad — a tiny struct appended to an array (`DeferMessage`) — and returns immediately. Once per frame, at a point UE chose, it reads the notepad top to bottom and does the real work (`ProcessDeferredEvents`). See §2 for the actual source.

**What we gain — and honestly, what we don't.** The work still happens in the *same frame* (deliberately: processing next frame would add input lag). The gains are elsewhere: (1) **a clean call stack** — handlers run from a plain loop we own, not with a half-finished OS delivery frozen beneath them, so any handler can safely do anything; (2) **input becomes data** — each frame's input exists as an inspectable, serializable list, which is the seam recording/replay and headless testing hang off later; (3) **one line owns input policy** — future filtering (ImGui capture, editor routing) happens at one drain point, not in six callbacks. Not gained: no latency win, no performance win, no fix for the Windows title-bar-drag freeze (that stall lives inside the OS, below GLFW). We moved the work in *space*, not time — off the OS's call stack and onto ours.

**What we have:** OS callbacks that execute arbitrary game logic re-entrantly, at moments the OS chooses. **What we're building:** callbacks that only *record* what happened, plus one fixed point at the top of each frame where the engine *replays* the recordings through the exact same dispatch machinery we already have.

## 1. Architecture Decision

Today the GLFW callbacks (`WindowsWindow.cpp:106-185`) call `data.EventCallback(event)` — bound to `GameApplication::OnEvent` — synchronously from inside `glfwPollEvents()`. The entire layer stack, including arbitrary game logic (Reaper pushes game states from key handlers, `GameLayer.cpp:116-124`), executes re-entrantly inside an OS callback. This story inverts that: **callbacks only record; the application replays at one defined point.** The callbacks construct the same typed events they do today but push them into an app-owned `EventQueue`; `Run()` drains the queue immediately after `PollEvents()`, feeding each event to the existing, unchanged `GameApplication::OnEvent` (dispatcher + top→bottom layer walk + `Handled`). The *when* of handler execution becomes a property of the frame architecture (playbook §1), not of the OS.

**Storage is a `std::variant` of the concrete event types in a `std::vector` with persistent capacity, drained via swap-and-drain double buffering.** Three candidate representations were weighed:

- *Heap-allocated polymorphic events* (`std::vector<Scope<Event>>`, the Hazel-community shape) — rejected: one heap allocation per event in an every-frame path (mouse movement alone is dozens per frame) violates the performance-first rule for zero benefit.
- *POD struct + union + type enum, re-translated to typed events at drain* (UE's `FDeferredWindowsMessage` shape, see §2) — rejected: UE defers **raw OS messages** because translation itself must move off the callback; GLFW has already translated for us. This shape adds a second translation layer plus a hand-written switch ladder the compiler generates for free with a variant.
- *`std::variant` of concrete events* — chosen: events stay typed values (no slicing, no allocation — vector capacity persists across frames), `std::visit` recovers the concrete type so drain hands the existing `OnEvent(Event&)` a real derived object, and the type list is compile-enforced (`Push` of an unlisted event type fails to build).

Ownership (playbook §2): `GameApplication` owns the `EventQueue` by value; the window holds a **non-owning pointer**, injected via `Window::SetEventQueue(EventQueue*)`, which replaces the `SetEventCallback`/`std::function` seam. The dispatcher, categories, `Handled` walk, all layer code, and all Reaper code survive untouched — the story's "Keep" clause is honored by construction.

## 2. UE Reference

UE has this exact system: the Windows message pump **defers** messages as compact structs and processes them once per tick. Studied in `C:\dev\HNDREDGAMES\UE_5_7_4`:

**The compact deferred struct** — `Engine/Source/Runtime/ApplicationCore/Public/Windows/WindowsApplication.h:129`:

```cpp
struct FDeferredWindowsMessage
{
	FDeferredWindowsMessage( const TSharedPtr<FWindowsWindow>& InNativeWindow, HWND InHWnd,
		uint32 InMessage, WPARAM InWParam, LPARAM InLParam, ... )
	...
	/** Message code */
	uint32 Message;
	/** Message data */
	WPARAM wParam;
	LPARAM lParam;
	...
};
```

**Enqueue from the WndProc** — `Engine/Source/Runtime/ApplicationCore/Private/Windows/WindowsApplication.cpp:4111`:

```cpp
void FWindowsApplication::DeferMessage( TSharedPtr<FWindowsWindow>& NativeWindow, ... )
{
	if( GPumpingMessagesOutsideOfMainLoop && bAllowedToDeferMessageProcessing )
	{
		DeferredMessages.Add( FDeferredWindowsMessage( NativeWindow, InHWnd, InMessage, ... ) );
	}
	else
	{
		// When not deferring messages, process them immediately
		ProcessDeferredMessage( ... );
	}
}
```

**The single drain point** — `WindowsApplication.cpp:4138`, called once per frame from `FSlateApplication::TickPlatform` (`Engine/Source/Runtime/Slate/Private/Framework/Application/SlateApplication.cpp:1691`):

```cpp
void FWindowsApplication::ProcessDeferredEvents( const float TimeDelta )
{
	// This function can be reentered when entering a modal tick loop.
	// We need to make a copy of the events that need to be processed or we may end
	// up processing the same messages twice
	TArray<FDeferredWindowsMessage> EventsToProcess( DeferredMessages );
	DeferredMessages.Empty();
	for( int32 MessageIndex = 0; MessageIndex < EventsToProcess.Num(); ++MessageIndex )
	...
```

**We adopt:** defer-as-compact-values in a flat array; one drain point per frame owned by the application; isolating the drain buffer from the live buffer so enqueues during processing can't corrupt iteration (their copy, our swap).

**We simplify:** (1) we defer *translated engine events*, not raw OS messages — GLFW already did the WndProc layer for us; (2) we **swap two persistent buffers** instead of copying a `TArray` every tick — UE pays a per-tick copy (and potential allocation) here; at our scale we can afford to do it strictly better; (3) no immediate-processing fallback path — UE's `GPumpingMessagesOutsideOfMainLoop` exists because modal OS loops (window drag) re-enter their pump; GLFW hides that loop from us entirely (see §10).

## 3. File Plan

```text
Radiant/Source/Radiant/Events/
├── EventQueue.h/.cpp        (new)    — variant storage, templated Push, swap-and-drain
│   Event.h                  (modify) — protected defaulted copy/move on Event; retire "currently BLOCKING" header comment
Radiant/Source/Radiant/Core/
│   Window.h                 (modify) — SetEventCallback → SetEventQueue(EventQueue*); doc updates
│   GameApplication.h/.cpp   (modify) — owns EventQueue (declared BEFORE m_Window); drain call in Run()
Radiant/Source/Radiant/Platform/Windows/
│   WindowsWindow.h/.cpp     (modify) — WindowData holds EventQueue*; callbacks Push typed events; null guard
Docs/
│   Event-System.md          (modify) — dispatch flow, timing, rationale, Known Issues (major update)
│   Core-Application.md, Architecture-Overview.md (modify) — frame anatomy: drain step
.claude/references/radiant-playbook.md (modify) — §1 "still target" annotations → landed
CLAUDE.md                    (modify) — Events row + Core row reflect queued dispatch
```

| Action | Path | Description |
|--------|------|-------------|
| Create | `Events/EventQueue.h/.cpp` | `QueuedEvent` variant alias, `Push(T&&)`, `ProcessEvents(sink)`, two persistent buffers |
| Modify | `Events/Event.h` | Copy/move hygiene on the base (variant needs clean copies; protected blocks slicing) |
| Modify | `Core/Window.h`, `Platform/Windows/WindowsWindow.*` | Seam change: queue pointer in, `std::function` callback out; callbacks become enqueue-only |
| Modify | `Core/GameApplication.*` | Own the queue, wire the window, drain at frame start; `OnEvent` itself unchanged |
| Modify | `Docs/*`, playbook, CLAUDE.md | Update contract — RAD-25 left "queue arrives in RAD-26" notes in ~6 places; all die here |

Target loop shape (`GameApplication::Run()` — only the two marked lines change):

```text
while running:
    frameDelta = now - last
    window->PollEvents()               // callbacks ENQUEUE only (changed)
    eventQueue.ProcessEvents(OnEvent)  // handlers run HERE, on a plain call stack (new)
    if not minimized:
        fixed-step drain (unchanged)
        OnUpdate / ImGui (unchanged)
    window->Present()
    layerStack.ProcessPendingLayers()
```

`EventQueue` is engine-internal: not exported via `Radiant.h` — game code never touches it (games keep receiving `Event&` through `Layer::OnEvent`).

## 4. Type Design

### EventQueue
- **Kind:** class
- **Responsibility:** Buffer typed events between OS delivery and the frame-start drain point, preserving arrival order.
- **Ownership:** Value member of `GameApplication` — **declared before `m_Window`** so it is constructed before and destroyed after the window that holds a pointer to it.
- **Lifetime & threading:** Application lifetime; main-thread-only (GLFW's contract: callbacks fire on the thread calling `glfwPollEvents`). If a future platform delivers events off-thread, the queue gains synchronization at this seam — not now.
- **Key Members:**
  - `using QueuedEvent = std::variant<WindowResizeEvent, WindowCloseEvent, KeyPressedEvent, KeyReleasedEvent, KeyTypedEvent, MouseButtonPressedEvent, MouseButtonReleasedEvent, MouseMovedEvent, MouseScrolledEvent>` — one alternative per concrete event; adding an event type = one line here, compile-enforced
  - `template<typename T> void Push(T&& event)` — emplaces into the live buffer; no translation, no switch
  - `void ProcessEvents(const std::function<void(Event&)>& sink)` — swaps the pending buffer into the processing buffer, visits each variant (`std::visit` → concrete `T&` → binds to `Event&`), clears; events pushed *during* processing land in the (now-empty) pending buffer and process **next frame** (deterministic policy, documented). Named in walkthrough 2026-07-08: `Drain` rejected as queue-implementer jargon (fails first-glance readability); `DispatchEvents` rejected — "dispatch" already names the `EventDispatcher` type-switch one call-frame below, and one word must not mean two mechanisms in the same subsystem; `ProcessEvents` matches the member family and UE's `ProcessDeferredEvents`
  - `m_PendingEvents`, `m_ProcessingEvents` — two `std::vector<QueuedEvent>` (named in walkthrough 2026-07-08: self-describing at first glance; a gerund like `m_Draining` reads as a state flag, not a container); `reserve(64)` at construction; capacity persists across frames (zero steady-state allocation)
  - `m_IsProcessingEvents` + `RADIANT_ASSERT(!m_IsProcessingEvents)` at the top of `ProcessEvents` (added in walkthrough 2026-07-08) — the swap-into-member idiom is safe only because `ProcessEvents` never re-enters (one call site in `Run()`; handlers have no path to it). UE's per-tick array *copy* tolerates re-entry from Windows modal loops; we don't have that problem, so we assert the invariant instead of paying for it — a tripwire, not a tax
  - `static_assert(sizeof(QueuedEvent) <= 64)` — guards against someone adding a fat payload (e.g. a `std::string`) and silently bloating every queued event
- **Playbook Patterns:** §1 (frame architecture), §2 (value ownership), §9 (deliberately *not* a ring buffer — the queue fully drains every frame, so there is never producer/consumer overlap; vector-swap is strictly simpler and the §9 container gates aren't met anyway)

### Event (modification, not a new type)
- Add `protected` defaulted copy/move constructor and assignment. Two birds: (1) the implicit copy of a class with a user-declared virtual destructor is deprecated — the variant needs clean, non-deprecated copies of derived events; (2) *protected* means only derived classes can invoke the base copy — nobody can accidentally slice-copy through an `Event&`. Public API is otherwise untouched.

### Window (interface change)
- `SetEventCallback(const EventCallbackFn&)` → `SetEventQueue(EventQueue* queue)` (non-owning; asserted non-null). `EventCallbackFn` moves out of `Window.h` — its one remaining consumer is the drain sink, typed in `EventQueue.h`.
- `WindowsWindow::WindowData` swaps `EventCallbackFn EventCallback` for `EventQueue* Queue`; every GLFW callback becomes `data.Queue->Push(SomeEvent(...))` behind a null guard (events arriving before wiring — a boot-window edge — are dropped; today's code would call a null `std::function` and crash).

## 5. Implementation Steps

### Phase A — The queue type (mentorship: Kareem implements)
- [x] **Event.h copy hygiene** — protected defaulted copy/move on `Event`; retire the lines of the header comment (`Event.h:7-12`) that describe blocking dispatch. *Done 2026-07-09 (delegated to Claude): all five special members restated protected (+ `Event()` — declaring the copy ctor suppresses the implicit default ctor); header comment + class doc comment now describe the queued world; Debug build clean.*
- [x] **EventQueue** — `Events/EventQueue.h/.cpp` per §4: variant alias, `Push`, swap-and-drain `ProcessEvents`, reserve, static_assert. The one subtle bit is the processing loop's re-entrancy story — see §10. *Done 2026-07-09 (delegated to Claude): built exactly per §4 — `EventSink` alias lives on the class; both asserts (re-entrancy tripwire + null sink); solution regenerated via premake; Debug build clean, static_assert passing. Phase A complete.*

### Phase B — Rewiring the seam (mentorship: Kareem implements)
- [x] **Window seam** — `SetEventQueue` replaces `SetEventCallback` in `Window.h`; `WindowsWindow` stores the pointer in `WindowData`; all six GLFW callbacks converted to enqueue-only with the null guard. *Done 2026-07-09 (delegated to Claude): `EventCallbackFn` alias removed; EventQueue forward-declared in headers (full include only in the .cpp); `SetEventQueue` defined in the .cpp with the null assert; resize callback keeps its live-size mirror immediate.*
- [x] **GameApplication wiring** — `m_EventQueue` value member declared **before** `m_Window` (§6); constructor wires `m_Window->SetEventQueue(&m_EventQueue)`; `Run()` gains the `ProcessEvents` call immediately after `PollEvents()`. `OnEvent` body untouched. *Done 2026-07-09 (delegated to Claude): declaration-order comment in place; ProcessEvents runs before the minimized gate (restore/close arrive as events). Verified: Debug build clean; live Reaper run closed via WM_CLOSE → WindowCloseEvent through the queue → graceful shutdown. Phase B complete.*

### Phase C — Docs & annotations (chore: Claude implements)
- [x] **Doc-comment pass** — `Window.h` (`PollEvents`/`SetEventQueue` contracts), `WindowsWindow.h` (class comment: callbacks translate + enqueue; drain runs handlers), `GameApplication.h` (`OnEvent` doc: now called from the frame-start drain), `Event.h` (dispatch-timing comment reflects queued reality). *Done 2026-07-09 — landed inline with the Phase A/B code changes (a comment updates with the code it describes).*
- [x] **Docs/ update contract** — `Event-System.md` (flow diagram gains the queue box, "Timing" section rewritten, blocking-dispatch rationale becomes history, Known Issues updated), `Core-Application.md` + `Architecture-Overview.md` (frame anatomy gains the drain step), playbook §1 (drop both "still target — RAD-26" annotations), CLAUDE.md (Events row + Core row). *Done 2026-07-09: Event-System.md gained a "The queue" section with the Push/ProcessEvents idiom, the letterbox extension of the letters analogy, and the honest what-we-gained/didn't rationale; staleness sweep also caught Time-And-Simulation.md's Known Issues bullet (same lesson as RAD-25: the update contract outranks the plan's file list).*
- [x] **Build + run gate** — all three configs via CLI, Reaper run from `Reaper/`: menu → gameplay → Escape-pause → resize → minimize → close. *2026-07-09: Debug/Release/Dist all clean, zero warnings. Smoke run: Reaper booted, simulated (demo timer sim 4.02s ≡ real 4.02s), and shut down gracefully via WM_CLOSE → WindowCloseEvent through the queue. REMAINING for Kareem before /review: the §11 breakpoint check (call stack shows ProcessEvents, no glfwPollEvents) and the interactive flow sweep (Escape-pause, resize, minimize) — those need hands on the keyboard.*

## 6. Ownership & Lifetime Strategy

`GameApplication` owns the `EventQueue` by value. The window's pointer is a **non-owning observer**, and the member declaration order makes the lifetime sound by construction: `m_EventQueue` is declared before `m_Window`, so C++ guarantees it is constructed first and destroyed last — the window can never hold a dangling queue pointer, even during teardown. (This is the same reasoning as the existing "explicit `m_LayerStack.Clear()` before renderer shutdown" comment in the destructor, but achieved declaratively.)

**Smart pointer rejected (walkthrough 2026-07-08).** `Ref`/`Scope` would claim ownership the window must not have (and can't point at a value member without forcing the queue onto the heap); `weak_ptr`-style checking would pay a per-push liveness check against a death that declaration order makes structurally impossible. Raw pointer = the codebase's vocabulary for "observer; lifetime guaranteed elsewhere" (playbook §2). Instructive contrast: UE's `FDeferredWindowsMessage` holds `TWeakPtr<FWindowsWindow>` because in *their* direction (queued note → window) the window genuinely can die before drain; in ours (window → queue) the lifetime is certain. The pointer type is a claim about lifetime — pick the one whose claim is true.

Queued events are **values owned by the queue** — a copy is taken at enqueue, so nothing about GLFW's callback-local stack lifetime leaks into the frame. During drain, handlers receive `Event&` references into the drain buffer; the existing contract on `Event` ("valid only for the duration of dispatch; handlers must not store pointers or references") is unchanged and remains the rule that makes this safe. Everything is main-thread-only.

## 7. Performance Notes

- **Steady state: zero allocations.** Both buffers' capacity persists across frames; `reserve(64)` covers realistic frames (a 1 kHz gaming mouse at 60 FPS ≈ 17 move events/frame). Worst-case floods (8 kHz mouse ≈ 133/frame) grow capacity once, then never again.
- Per event: one variant copy at enqueue (≤ 64-byte memcpy, asserted), one `std::visit` (a jump table) at drain. The handler work itself is *moved*, not added — it ran inside the callbacks before.
- The queue is unbounded by design: dropping input on a hitch is worse than a few KB of growth (500 events ≈ 32 KB, reclaimed never, reused always).
- UE comparison: our swap-and-drain beats the `ProcessDeferredEvents` copy-then-drain — no per-frame `TArray` copy. Cited in §2.
- No GPU implications.

## 8. Logging & Diagnostics

- No per-event logging anywhere — mouse-move volume would make it unreadable and violate the hot-path rule even in Debug.
- `RADIANT_ASSERT(queue, ...)` in `SetEventQueue` — wiring a null queue is a programmer error.
- Null-queue guard in the GLFW callbacks silently drops pre-wiring events (boot-window edge; not a misconfiguration a user can cause, so no WARN — documented in the callback comment instead).
- The existing throttled accumulator-clamp WARN (RAD-25) is unaffected; no new warnings introduced.

## 9. Scalability Review

- **Adding an event type** = the concrete class + one line in the `QueuedEvent` alias, compile-enforced (unlisted `Push<T>` fails to build). O(1) sites; no switch ladders anywhere in the design. Already the scalable shape.
- **Multi-window (Phase 5 editor)** — events carry no source-window identity today. The queue seam is exactly where a window handle would be added (each window pushes into the shared app queue with a tag). Defer to RAD-53; zero rework required here.
- **Input recording/replay** — the story's "Why" names this: record = serialize the drained variants (they're plain values); replay = `Push` from a file instead of from GLFW. The seam exists after this story; the feature stays unbuilt until a story needs it.
- **Mouse-move coalescing** — Slate coalesces pointer moves; we deliberately don't (changes semantics for anything sampling per-move deltas, and our drain cost is nanoseconds at our volume). Revisit only if profiling ever says otherwise.
- **Ring buffer** (playbook §9 catalog) — not the right structure even ignoring the unmet gates: the buffer fully empties every frame, so there's no wrap-around case for a ring to optimize. Vector-swap is the end-state, not a placeholder.
- **Closed type set — deliberate, not a flaw** (raised in walkthrough 2026-07-08). The variant is a closed list: game code cannot define new event types without editing it. Correct for this domain — the OS's input vocabulary is inherently closed (UE closes the same set: `FDeferredWindowsMessage`, and Slate's fixed `FGenericApplicationMessageHandler` interface). Open-set *gameplay* events ("player died") are a different system at a different altitude — a future gameplay event bus (icebox), which must not ride the OS input queue. If an event ever needs a big payload (e.g. file-drop paths), it carries a handle to side storage — the `static_assert` enforces the discipline.

## 10. Risks & Edge Cases

- **Destruction/construction order** — the window observes the queue; declaration order (§6) makes the queue outlive it. A future member reorder could silently break this: the declaration gets a load-bearing comment.
- **Handlers assuming immediate dispatch** — audited: engine handlers (`OnWindowClose`, `OnWindowResize`/minimize gating) run at the drain, which precedes the simulation gate *in the same frame* — behavior is identical to today. Reaper's handlers gain safety (state pushes now happen on a plain call stack) and need no changes.
- **ImGui input** — unaffected: ImGui installs its own *chained* GLFW callbacks (`install_callbacks=true`, `OpenGLImGuiLayer.cpp:48`) and consumes input at poll time independently of our dispatch. Our queue changes when *engine* handlers run, not when ImGui sees input.
- **`WindowData.Width/Height`** stays updated inside the resize callback (state mirroring, not dispatch) — `GetWidth()` reflects the new size before the `WindowResizeEvent` drains. Same-frame, benign; documented in `WindowsWindow.h`.
- **Events pushed during drain** — processed next frame (swap happened; they land in the live buffer). Deterministic and allocation-safe; impossible today (only OS callbacks push, and polling finished), becomes relevant when programmatic events arrive. UE's copy-then-empty has the same policy (§2).
- **Modal window drag on Windows** still blocks `glfwPollEvents` (the OS confiscates the main thread inside `DefWindowProc`'s drag loop; GLFW hides it). Frames stall during the drag exactly as they do today — pre-existing behavior, out of scope, explicitly *not* a queue regression. No fix exists from our side without paying real architecture money: UE lives inside the hijack (`bInModalSizeLoop` + immediate message processing + `OnOSPaint` from the callback — all re-entrancy-dependent, the exact disease this story cures), AAA engines render on a second thread, indies go borderless with custom chrome. We accept the freeze until any of those is worth its cost.
- **The queue makes the freeze *harmless*, though** (raised in walkthrough 2026-07-08): Windows calls back into the WndProc from inside its drag loop for some messages, and GLFW forwards them — so our callbacks **can fire mid-drag, from inside the modal loop, while `PollEvents` is frozen mid-call**. Today that runs full game handlers at the deepest re-entrancy the platform can produce; after this story, a mid-drag callback merely appends to `m_PendingEvents`, and the notes are processed when `Run()` resumes after release. The freeze stays; its danger goes.
- **`KeyTypedEvent`** remains defined-but-never-emitted (no char callback registered). Kept in the variant so the type list is complete; wiring the callback is a two-line follow-up when text input matters (Phase 5).

## 11. Verification (AC → proof)

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| Handlers no longer execute inside OS callbacks | Breakpoint in `GameLayer::OnKeyPressed`: call stack shows `Run` → `ProcessEvents` → `OnEvent`, with **no `glfwPollEvents` frame** — the decisive check |
| Events drain at one defined frame-start point | Code inspection: exactly one `ProcessEvents` call sits between `PollEvents()` and the simulation gate in `Run()` |
| Arrival order preserved | TRACE-log a rapid press/release burst in a debug handler: sequence matches physical input order |
| Dispatcher/`Handled`/layer walk unchanged | Reaper behaves identically: Escape pauses gameplay (UILayer consumes before GameLayer), menu navigation works, resize re-viewports, minimize gates sim, close button exits |
| No steady-state allocation | Debugger watch on `m_PendingEvents` capacity after a minute of play: stable (reserve absorbed the load) |
| Engine still ships | All three configs build clean via CLI (Debug/Release/Dist — Dist catches assert-only code); Reaper runs from the project working directory |