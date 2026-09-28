# Implementation Plan — RAD-25: Fixed-timestep game loop with accumulator + render interpolation

| Field | Value |
|-------|-------|
| **Jira** | [RAD-25](https://hndredgames.atlassian.net/browse/RAD-25) |
| **Epic** | RAD-2 — Phase 2: Simulation Foundation |
| **Story status** | To Do |
| **Dependencies** | None — RAD-26/27/28/31 build on this. Recommended (non-blocking) precursor: RAD-67 doctest infra, so `FrameClock`/`TimerManager` land with unit tests |
| **Planned** | 2026-07-05 |

Scope note (from the 2026-07-05 Jira comment): this story also owns **time services** — a global time-dilation factor (pause = scale 0; RAD-31 consumes it) and a **timer manager** ticking in simulation time.

---

## 1. Architecture Decision

The loop moves from *"measure a variable delta, hand it to everything"* to the playbook §1 frame order: **pump events at frame start → fixed-step simulation via an accumulator → variable-rate render with interpolation**. The engine — not layers, not the game — owns frame structure: `GameApplication::Run()` drains the accumulator and invokes a new `Layer::OnFixedUpdate(Timestep)` hook zero-or-more times per frame, while the existing `Layer::OnUpdate(Timestep)` becomes the once-per-frame render-rate hook. This is the Unity `FixedUpdate`/`Update` split, chosen over "each layer accumulates internally" because frame architecture defined in one place is the entire point of §1 — a per-layer accumulator would let every game reinvent (and desynchronize) the simulation contract.

The core accounting lives in a new **`FrameClock`** value type rather than loose members on `GameApplication`: it owns the accumulator, fixed delta, time scale, simulation/real clocks, and the interpolation alpha. Two reasons a principal engineer insists on the extraction: (1) it makes the trickiest logic in the story (clamping, dilation, alpha) **unit-testable without a window** — feed it deltas, assert step counts and alpha; (2) it gives time dilation a single owner. Dilation scales the *accumulated* time (`accumulator += frameDelta * timeScale`), never the delta handed to physics — the fixed step stays a constant 1/60 s, so slow-mo means *fewer steps per real second*, not smaller (stability-breaking, determinism-breaking) steps. Pause is timescale 0: zero steps run, while render, ImGui, and events continue at full rate — exactly the seam RAD-31 needs.

**`TimerManager`** (UE `FTimerManager` analogue) ticks once per fixed step, so timers advance in simulation seconds and pause/dilate with the world for free. Deliberately *not* in scope: the event queue (RAD-26 — though polling moves to frame start here), per-Level physics world (RAD-27), the full physics sync pass and fixture-rebuild fix (RAD-28 — this story only moves the readback *call* out of the render loop so rendering stops mutating simulation state), and Reaper's pause-state glue (RAD-31).

Decisions locked during the walkthrough (2026-07-05): **(D1)** time is exposed via a thin `Time::` static facade (`Time::SetTimeScale(0.0f)`, `Time::GetAlpha()`) forwarding to the app-owned `FrameClock`, which holds all state — chosen over `GameApplication` accessors for call-site ergonomics; testability is unaffected because the facade is stateless. **(D2)** `TimerManager` is engine-global (app-owned) — one Level exists and global pause already comes from the timescale; per-Level migration is flagged in §9 and lands when RAD-52 (play-in-editor) forces it, with an identical API shape making the move mechanical. **(D3)** interpolation snapshots are a runtime-only POD component, never serialized — O(1) sparse-set access beats per-entity hash lookups in the render loop, and being trivially copyable it honors playbook §3's actual rule ("must survive shallow copy").

**Locked during the walkthrough (2026-07-05):** the per-frame hook keeps the name `OnUpdate` (not `OnRender`). Per-frame work is broader than draw submission — the Phase 5 editor fly-cam, pause-immune visual motion, and frame bookkeeping all need display-rate updates but are not rendering (Godot's `_process`/`_physics_process` split makes the same call). `Level::OnRender(alpha)`, which genuinely is draw-only, keeps the render name; the "rendering never mutates simulation" rule is enforced by review against playbook §1, not by hook naming.

## 2. UE Reference

Studied in `C:\dev\HNDREDGAMES\UE_5_7_4`:

- **`FEngineLoop::Tick` (`LaunchEngineLoop.cpp`) — what we deliberately do differently.** UE's game thread is *variable*-timestep: `FApp::DeltaTime` flows into `UWorld::Tick`, and only physics is protected via substepping (semi-fixed: `MaxSubstepDeltaTime`/`MaxSubsteps`) or Chaos' fixed-dt solver mode. UE carries this complexity because it must serve 10,000-actor worlds where a hard fixed step would death-spiral. We choose the **fully fixed simulation step** (Fiedler's canonical loop) instead: our simulation is small, and determinism-by-default is worth more to a learning engine headed toward networking (UE itself re-introduced fixed ticking for exactly that in the Network Prediction plugin).
- **`AWorldSettings::TimeDilation` — same shape as ours.** UE multiplies the world's delta by the dilation factor while the render/RHI threads run at real rate; pause routes through `SetGamePaused` with per-actor `bTickEvenWhenPaused` opt-outs. Our timescale-on-the-accumulator achieves the same player-facing behavior with none of the per-actor machinery — layers that must ignore pause (UI) simply live in `OnUpdate`, not `OnFixedUpdate`.
- **`FTimerManager` (`TimerManager.h`) — adopted almost wholesale.** UE stores timers in a `TSparseArray<FTimerData>` (slot storage), keeps an `ActiveTimerHeap` (min-heap of handles ordered by expiration against a `double InternalTime`), and hands out opaque `FTimerHandle`s; `SetTimer(handle, delegate, rate, bLoop, firstDelay)`, `ClearTimer(handle)`. We adopt: handle-based identity (index + generation, so stale handles are harmless no-ops), min-heap on expiry with lazy invalidation, `double` internal clock, looping timers re-armed by `expiry += period` (no drift). We simplify: `std::function` instead of the five delegate flavors, no per-timer pause, no `FTimerManagerTimerParameters` — none earn their complexity at our scale yet.

  ```cpp
  // Engine/Source/Runtime/Engine/Public/TimerManager.h (UE 5.7.4) — the shape we adopt:
  inline void SetTimer(FTimerHandle& InOutHandle, TFunction<void(void)>&& Callback,
                       float InRate, bool InbLoop, float InFirstDelay = -1.f);
  ...
  TSparseArray<FTimerData> Timers;        // slot storage (our: vector + free list + generations)
  /** Heap of actively running timers. */
  TArray<FTimerHandle> ActiveTimerHeap;   // min-heap ordered by expiry
  double InternalTime;                    // the sim-time clock expirations compare against
  ```

## 3. File Plan

```text
Radiant/Source/Radiant/Core/
├── FrameClock.h/.cpp        (new)    — accumulator, fixed dt, timescale, alpha; pure logic
├── TimerManager.h/.cpp      (new)    — sim-time timers: handles, min-heap, Tick(fixedDt)
├── Time.h/.cpp              (new,D1) — thin static facade over the app's FrameClock
│   GameApplication.h/.cpp   (modify) — loop rework; owns FrameClock + TimerManager; spec gains SimulationRate
│   Layer.h                  (modify) — add OnFixedUpdate(Timestep); re-document OnUpdate contract
│   Window.h                 (modify) — split OnUpdate() into PollEvents() + Present()
│   Timestep.h               (modify) — doc: fixed delta in OnFixedUpdate, frame delta in OnUpdate
Radiant/Source/Radiant/Platform/Windows/
│   WindowsWindow.h/.cpp     (modify) — implement the PollEvents/Present split
Radiant/Source/Radiant/ECS/
│   Level.h/.cpp             (modify) — OnUpdate→OnFixedUpdate; readback post-step; interpolated OnRender(alpha)
│   Components.h             (modify,D3) — TransformSnapshotComponent (runtime-only POD, never serialized)
Reaper/Source/
│   Layers/GameLayer.h/.cpp  (modify) — simulation → OnFixedUpdate; render → OnUpdate
│   Layers/UILayer.h/.cpp    (modify) — audit hooks; debug time-scale keys + demo timer (chore)
Docs/
├── Time-And-Simulation.md   (new)    — the time system, problem-first
│   Core-Application.md, Layer-System.md, Physics.md, Event-System.md, README.md (modify) — update contract
```

| Action | Path | Description |
|--------|------|-------------|
| Create | `Core/FrameClock.h/.cpp` | Testable time accounting: `BeginFrame(delta)`, `ConsumeStep()`, `Alpha()`, timescale, clamp |
| Create | `Core/TimerManager.h/.cpp` | `SetTimer`/`ClearTimer` with generation handles; ticked per fixed step |
| Create | `Core/Time.h/.cpp` (D1) | `Time::GetTimeScale()`, `SetTimeScale()`, `GetAlpha()`, `GetFixedDelta()`, `GetSimulationTime()` |
| Modify | `Core/GameApplication.*` | New loop (below); prime `m_LastFrameTime` pre-loop (kills first-frame bug); `SimulationRate` in spec |
| Modify | `Core/Layer.h`, `Core/Window.h` + `Platform/Windows/WindowsWindow.*` | `OnFixedUpdate` hook; poll/present split so events pump at frame *start* |
| Modify | `ECS/Level.*`, `ECS/Components.h` | Fixed-update path with snapshot → step → readback; render lerps snapshots by alpha |
| Modify | `Reaper/Source/Layers/*` | Hook split + debug scaffolding (chore) |
| Create/Modify | `Docs/*` | New Time doc + four stale-doc updates in the same change (update contract) |

Target loop shape (`GameApplication::Run()`):

```text
prime m_LastFrameTime                     // first-frame bug dies here
while running:
    frameDelta = now - last; last = now   // double precision
    window->PollEvents()                  // frame START (queue itself = RAD-26)
    if not minimized:                     // sim freezes while minimized: no deposit,
        clock.BeginFrame(frameDelta)      //   no catch-up burst on restore (§10)
        while clock.ConsumeStep():        // 0..N times
            timerManager.Tick(fixedDelta) // timers fire in sim time
            for layer: layer->OnFixedUpdate(fixedDelta)
        for layer: layer->OnUpdate(frameDelta)   // render-rate; alpha via Time
        ImGui Begin / OnImGuiRender / End
    window->Present()                     // swap at frame end
    layerStack.ProcessPendingLayers()
```

## 4. Type Design

### FrameClock
- **Kind:** class (value semantics, no statics)
- **Responsibility:** Convert real frame time into a whole number of fixed simulation steps plus a leftover interpolation alpha, under a time-scale factor and a spiral-of-death clamp.
- **Ownership:** Value member of `GameApplication`. Copyable (plain data) — copies are independent clocks, useful in tests.
- **Lifetime & threading:** Lives as long as the application; main-thread-only.
- **Key Members:**
  - `m_FixedDelta`, `m_Accumulator`, `m_SimulationTime`, `m_RealTime` — all `double` (float accumulators visibly drift after ~hours; GLFW hands us double already)
  - `m_TimeScale` — ≥ 0; `m_MaxAccumulation` (default 0.25 s, Fiedler's clamp)
  - `BeginFrame(double realDelta)` — `accumulator += realDelta * timeScale`, then clamp + throttled WARN
  - `bool ConsumeStep()` — one step's worth consumed per true; drives the `while` drain
  - `float Alpha()` — `accumulator / fixedDelta` ∈ [0,1); forced to 1.0 when timescale == 0 (render the settled current state while paused)
- **Playbook Patterns:** §1 (frame architecture), §2 (value ownership)

### TimerManager
- **Kind:** class
- **Responsibility:** Schedule callbacks N *simulation* seconds out (one-shot or looping); fire them during the fixed-step drain so they dilate and pause with the world.
- **Ownership:** Value member of `GameApplication` (D2). Owns its callbacks (`std::function` by value — fine outside components; §3 bans them *in components* only). Non-copyable (rule-of-5, callbacks are identity).
- **Lifetime & threading:** Application lifetime; main-thread-only. **Callback lifetime contract:** a callback capturing an object must be cleared (via its handle) before that object dies — documented on `SetTimer`, revisited in §9.
- **Key Members:**
  - `TimerHandle SetTimer(float delaySeconds, std::function<void()> callback, bool looping = false)`
  - `void ClearTimer(TimerHandle&)` — stale/invalid handles are benign no-ops (generation check)
  - `bool IsActive(TimerHandle) const`
  - `void Tick(double fixedDelta)` — advances internal sim clock; pops due timers off the min-heap *before* invoking (callbacks may legally Set/Clear timers re-entrantly); loops re-arm at `expiry += period` (no drift)
  - Storage: slot vector + free list + generation counters, min-heap of (expiry, handle) with lazy invalidation — UE's exact shape, no per-frame allocations
- **Playbook Patterns:** §2 (rule-of-5), §8.5 (no side effects vanish — callbacks never fire from asserts/logs)

### TimerHandle
- **Kind:** struct (POD: `uint32 Index`, `uint32 Generation`)
- **Responsibility:** Opaque, copyable identity for a scheduled timer; stale use is safe, never UB.
- **Ownership/Lifetime:** Value; meaningless after the manager that minted it is gone (app lifetime — practically always valid to *hold*).

### TransformSnapshotComponent (D3)
- **Kind:** struct (POD component, runtime-only — the serializer never writes it)
- **Responsibility:** The entity's transform as of the *start of the last fixed step*, so `OnRender(alpha)` can draw `lerp(previous, current, alpha)`.
- **Ownership:** Registry-owned like any component; trivially copyable, so `Level::Copy()` (Phase 5) shallow-copies it safely — honors §3's real rule.
- **Key Members:** `Translation`, `Rotation` (no scale — nothing simulates scale). Written each fixed step for entities that can move in simulation (today's heuristic: has `RigidBody2DComponent`, `CameraComponent`, or `NativeScriptComponent`); refreshed-to-current on spawn and on explicit teleport (snap, don't smear).

### Layer (interface change, not a new type)
- `OnFixedUpdate(Timestep)` — 0..N calls/frame, fixed delta, simulation mutations belong here.
- `OnUpdate(Timestep)` — exactly 1 call/frame, real frame delta, render-rate work; **never mutates simulation state** (§1).

## 5. Implementation Steps

### Phase A — Clock core & loop skeleton
- [x] **FrameClock** — new pure type per §4; the accumulator/dilation/clamp/alpha logic and its doc contract. (doctest target the moment RAD-67 lands.) *Done 2026-07-05; drive-by: Assert.h macro hygiene (do-while(0) + ((void)0)) and the one no-semicolon call site it exposed (OpenGLContext.cpp:13).*
- [x] **Spec + wiring** — `GameApplicationSpecification.SimulationRate` (default 60, asserted > 0); `GameApplication` owns a `FrameClock`; prime `m_LastFrameTime` immediately before the loop (first-frame bug dies). *Done 2026-07-05; `m_LastFrameTime` is now double, boot TRACE logs the fixed step.*
- [x] **Poll/present split** — `Window::OnUpdate()` → `PollEvents()` + `Present()`; `WindowsWindow` implements; loop pumps at frame start, swaps at frame end. (Handlers still run inside OS callbacks — the *queue* is RAD-26; only the *when* moves.) *Done 2026-07-05; four stale "end-of-frame" doc comments updated (Window.h, WindowsWindow.h, GameApplication.h, Event.h); minimize now takes effect same-frame.*
- [x] **Fixed-step drain** — `Layer::OnFixedUpdate` hook; `Run()` reworked to the target shape in §3. Minimized behavior preserved: poll + present always, simulation and render skipped. *Done 2026-07-05. Renames during implementation: `m_FixedDelta`→`m_FixedDeltaTime`, `Alpha()`→`GetAlpha()` (term-of-art + Get-convention). Note: Reaper still simulates in OnUpdate until Phase C.*

### Phase B — Time services
- [x] **Time scale + facade (D1)** — `SetTimeScale`/`GetTimeScale` (clamp negatives to 0 with a WARN — config mistake, recover); `Time::` facade forwarding to the app's clock. *Done 2026-07-05: stateless statics in Core/Time.h/.cpp, `friend class Time` on GameApplication (keeps BeginFrame/ConsumeStep unreachable from game code); header is include-free by design.*
- [x] **TimerManager (D2)** — per §4; ticked first inside each fixed step (timers observe a consistent pre-update world, mirroring UE's tick placement). *Done 2026-07-05: slot pool + free list + generation handles + lazy-invalidated min-heap; game access via `GameApplication::GetTimerManager()`. Phase B complete.*

### Phase C — Level integration & interpolation
- [x] **Level::OnFixedUpdate** — rename from `OnUpdate`; order: scripts → physics submit → `Physics2D::OnUpdate(fixedDelta)` → **readback pass** (moved out of `OnRender`; rendering stops mutating simulation — the *timing* fix only; the fixture-rebuild/ownership fix stays RAD-28). *Done 2026-07-05 — GameLayer hook split pulled forward from Phase D (simulation → OnFixedUpdate); Reaper is framerate-independent as of this step. Snapshot write moves to the next step with the component. Fixes the old no-sprite/no-camera readback hole as a side effect.*
- [x] **Interpolated render** — `Level::OnRender(float alpha)`: entities with a snapshot draw `lerp(snapshot, current, alpha)`; everything else draws current. Spawned-this-frame entities snap (snapshot = current at creation). *Done 2026-07-05: TransformSnapshotComponent (runtime-only POD), snapshot pass at top of OnFixedUpdate (any_of movable heuristic), camera interpolated too, Time.h exported via Radiant.h umbrella. Phase C complete.*

### Phase D — Game, diagnostics, docs
- [x] **Reaper hook split** — `GameLayer::OnFixedUpdate` → `Level::OnFixedUpdate`; `GameLayer::OnUpdate` → framebuffer bind + `Level::OnRender(Time::GetAlpha())`; UILayer audited (pure ImGui — stays render-rate). **Chore (Claude):** debug keys for timescale 1 / 0.5 / 0 + a looping demo timer logging sim vs real time. *Done 2026-07-06: hook split had landed with Phase C; audit removed UILayer's dead empty `OnUpdate` override. Scaffolding lives in UILayer (whole-run lifetime — keys work in every state), `#ifndef RD_DIST`-guarded (time cheats don't ship; matches Log.h stripping idiom): F1/F2/F3 → timescale 1/0.5/0, looping 2-sim-second timer logging sim vs real time, handle cleared in OnDetach per the TimerManager lifetime contract.*
- [x] **Docs (chore, update contract)** — new `Docs/Time-And-Simulation.md` (problem-first); update Core-Application (loop section), Layer-System (hook contract), Physics (readback timing), Event-System (frame-start polling note), README index. *Done 2026-07-06. Beyond the planned list, the staleness sweep also caught Architecture-Overview (Frame Anatomy + reading order) and ECS-And-Levels (Level loop paragraph, component table gains TransformSnapshotComponent) — the update contract outranks the plan's file list.*

## 6. Ownership & Lifetime Strategy

`GameApplication` owns `FrameClock` and `TimerManager` by value — created with the app, destroyed with it, no new heap ownership anywhere. The `Time` facade holds no state; it forwards through the app singleton (same pattern and caveat as the existing accessors: valid only while the app exists). `TimerManager` owns callbacks by value; the **capture hazard** is the one real lifetime risk this story introduces — a `std::function` capturing an `Entity` or `Level*` outlives its target unless cleared. Contract (documented on `SetTimer`, enforced by review): the capturing owner clears its handles in its teardown path. Stale `TimerHandle`s themselves are always safe (generation-checked no-ops). Snapshot components live in the registry with normal component lifetime; they are POD, so `Level::Copy()` and level teardown need no special handling. Everything in this story is main-thread-only.

## 7. Performance Notes

- **Net win at high framerates:** uncapped 300 FPS no longer runs scripts + physics 300×/s — simulation cost is fixed at 60 Hz regardless of render rate.
- Loop bookkeeping is a handful of double ops; the drain is bounded by the 0.25 s clamp (max 15 steps/frame @ 60 Hz) — no spiral of death.
- Interpolation: one POD snapshot write per movable entity per *step*, one lerp per movable entity per *frame* — contiguous sparse-set iteration, no allocations, no hash lookups (why D3 recommends the component over a map).
- TimerManager: O(log n) heap ops on set/fire; a no-timers-due tick is one comparison. `SetTimer` may allocate inside `std::function` — acceptable, it's not per-frame; `Tick` itself allocates nothing, with one caveat (found in review): a looping fire invokes a COPY of its callback for re-entrancy safety, which allocates when captures outgrow the small buffer.
- No GPU implications; render submission is unchanged.

## 8. Logging & Diagnostics

- `RADIANT_TRACE` at init: fixed rate and delta ("Simulation: 60 Hz (16.67 ms)").
- `RADIANT_WARN`, throttled (once/second), when the accumulator clamp triggers — the "simulation can't keep up" signal.
- `RADIANT_ASSERT(spec.SimulationRate > 0, ...)` — programmer error. Negative `SetTimeScale` → clamp to 0 + `RADIANT_WARN` (config mistake: warn and recover). `ClearTimer` on a stale handle: silent no-op by design (documented), not a warn — it's the correct idiom for "clear if still pending".
- Debug scaffolding logs via `GAME_*` (it lives in Reaper): demo timer prints simulation time vs real time so dilation is visible in the log.
- Nothing with side effects inside asserts/logs (§8.5); all new code must build in Dist where asserts vanish (§7 build rule).

## 9. Scalability Review

- **Engine-global TimerManager (D2) — flagged.** Gameplay timers captured against Level entities couple a global container to Level-lifetime objects. Fine at one-Level scale; becomes wrong the moment play-in-editor (RAD-52, `Level::Copy`) or multiple live Levels exist — the UE answer (per-world manager, `UWorld::GetTimerManager`) is per-*Level* here. **Proposal:** engine-global now, and when RAD-52 is planned, move gameplay timers into a Level-owned manager ticked from `Level::OnFixedUpdate`; the API shape is identical, so the migration is mechanical. Filed as a note on this plan rather than a new issue — RAD-52's plan must pick it up.
- **Movable-entity heuristic (D3) — flagged.** "Has rb2d/camera/script" is correct today and O(changed archetypes) cheap, but it's an implicit contract; a future animation/tween system (RAD-64, RAD-70) adds movers this list doesn't know. The scalable end-state is an explicit opt-in marker (or the Phase 2 component-hygiene rework, RAD-30, deciding movability). Heuristic now; RAD-30 review revisits.
- **Hook design scales:** new layers/games get correct simulation semantics with zero engine edits — no per-game switch ladders anywhere.
- **Determinism honesty:** fixed dt buys per-machine reproducibility; *cross*-machine determinism (float FP, ordering) is a Phase-icebox networking concern (RAD-59) and explicitly not claimed here.

## 10. Risks & Edge Cases

- **Spiral of death** — a hitch (window drag, alt-tab, debugger break) produces a huge delta → clamp to 0.25 s + throttled WARN; simulation slows instead of locking up.
- **Pause alpha** — with timescale 0 no steps run and the accumulator freezes mid-interval; forcing alpha to 1.0 renders the settled current state (no frozen mid-lerp pose).
- **VSync beat jitter** — 60 Hz render against 60 Hz sim makes alpha oscillate near 0/1; known Fiedler artifact, cosmetically invisible with interpolation on. Verify empirically at 10 Hz sim (below).
- **Teleports must snap, not smear** — an explicit transform push (spawn/teleport) also resets the snapshot; otherwise interpolation draws a one-frame streak across the teleport. (RAD-28's explicit-teleport path formalizes this.)
- **Timer re-entrancy** — callbacks that Set/Clear timers during `Tick`: pop-before-invoke ordering makes this safe; a doctest case pins it.
- **Scripts move to fixed rate** — `CameraController` input sampling now happens at 60 Hz; camera entities get snapshots so rendering stays smooth. Any game code assuming per-render-frame `OnUpdate` semantics must migrate knowingly (only Reaper exists; audited in Phase D).
- **Minimized** — simulation freezes while minimized (current behavior, kept deliberately); the clamp prevents a catch-up burst on restore.

## Implementation Log (source material for the Story Implementation Report)

Session-only lessons from Phases A–C (2026-07-05), recorded so the report can be written from any session:

- **`std::clamp` UB (FrameClock::BeginFrame):** first draft used `std::clamp(v, v, max)` — `clamp` has the precondition `lo <= hi`, so it was undefined behavior exactly when the clamp mattered (accumulator over cap). Lesson: one-sided bounds are `std::min`/`std::max`; `clamp` is for genuine two-sided ranges. The final `if` shape was forced anyway by the WARN needing the dropped amount.
- **Assert.h macro fixes:** bare `{ }` expansion → `do{}while(0)` (dangling-else hazard) and empty Dist form → `((void)0)` (orphan-semicolon). First build after exposed the one no-semicolon call site in the repo (`OpenGLContext.cpp:13`). Later discovered the 2-slot dispatcher can't take format args — `RADIANT_ASSERT(cond, "msg {}", x)` breaks the build despite the (now-corrected) usage comment claiming support. Two-arg messages only; macro upgrade deliberately deferred.
- **Rename lessons:** `m_FixedDelta` → `m_FixedDeltaTime` (industry term of art: Unity/UE) left a straggler call site (boot TRACE); `Alpha()` → `GetAlpha()` for the codebase's Get-accessor convention. Rule reinforced twice: a rename is done when grep for the old name returns nothing.
- **`friend class Time` (D1):** facade befriends GameApplication so `BeginFrame`/`ConsumeStep` stay unreachable from game code; forwarders in the .cpp keep `Time.h` include-free (compile-time firewall). Precedent: `main()` friended for `Run()`.
- **TimerManager subtleties:** copy/move callback to a local before invoking (re-entrant `SetTimer` can reallocate `m_Timers` and dangle references); pop-before-invoke (retire slot fully before user code); loopers re-arm at `expiry += period` so lateness never compounds. Internals mirror UE `FTimerManager` (slot storage + expiry min-heap + lazy invalidation via generation check).
- **Readback move fixed a real bug:** old in-render readback skipped bodies without sprites and did nothing without a primary camera; the post-step pass covers every active body. GameLayer's hook split was pulled forward from Phase D so the code never lied about which clock drives simulation.
- **Piece-7 → piece-8 arc:** after Level adopted the fixed step, 60 Hz motion visibly stepped on high-refresh displays — deliberately shown before interpolation cured it. Camera is interpolated too (script-driven mover), or the world would be smooth while the view judders.
- **Clamp valve fired in production:** first post-interpolation Reaper run logged `dropped 651.4 ms` at the MainMenu→Gameplay transition (level load hitch) — the spiral-of-death guard working as designed. Future refinement noted: a loading-screen system should reset the clock across loads.
- **Scope boundary for the report:** the engine-wide guard/logging sweep, Log-system rework (Dist stripping, fmt formatters), and Assert.h fixes happened alongside RAD-25 in the same working tree but are NOT part of this story — the report should mention them only as adjacent work.
- **Umbrella export:** `Time.h` added to `Radiant/Radiant.h` during piece 8 (missed in piece 5 — a wrong glob concluded no umbrella header existed).
- **/review findings, all fixed 2026-07-06:** (1) the review's one ERROR was *inside a guard added by the hardening sweep* — the unmapped-entry sort fallback returned true for two unmapped entries, violating strict weak ordering (UB); lesson: recovery paths need the same rigor as happy paths. (2) Stale-snapshot landmine: an entity that lost its last mover component kept its old snapshot and would render lerped against a stale pose forever — snapshot pass now removes orphaned snapshots, and iterates the three mover views instead of scanning every entity (perf). (3) Null-RuntimeBody skips in both physics sync passes make CreateBody failure actually survivable. (4) `LoadDEBUG` falls back to the debug level on a failed load (game honors the engine's warn-and-recover contract). (5) Zero-delay looping timers degrade to one-shot with a WARN. (6) Debug keys gate on `IsRepeat`. (7) Collider debug draw documented as deliberately uninterpolated. (8) Playbook §1 annotation and CLAUDE.md Core row refreshed to landed reality.

## 11. Verification (AC → proof)

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| Simulation is framerate-independent | Run Reaper VSync on vs off (uncapped): falling box comes to rest at the same position after the same *simulation* time (log sim-time at rest); trajectory identical |
| Fixed-step accumulator is correct | doctest (once RAD-67 lands): scripted delta sequences → exact step counts, alpha bounds, clamp engagement, timescale-0 → zero steps |
| Events pump at frame start | Log ordering: `PollEvents` → fixed steps → render within one frame (breakpoint or TRACE) |
| First-frame bug dead | TRACE the first frame's delta: milliseconds, not seconds-since-GLFW-init |
| Render interpolation works | Set `SimulationRate = 10` temporarily: motion stays smooth at display rate (the decisive visual proof), then restore 60 |
| Time dilation + pause | Debug keys: 0.5 → slow motion, 0 → world frozen while ImGui/UI stay fully interactive |
| Timers tick in sim time | Demo looping timer: fires every 2 sim-seconds; under 0.5 dilation fires every 4 real seconds; never fires while paused |
| Engine still ships | All three configs build (Debug/Release/Dist — Dist catches assert-only code, §7); Reaper runs from the project working directory |
