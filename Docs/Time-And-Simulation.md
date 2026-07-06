# Time & Simulation

**Status:** Stable (landed 2026-07-05, RAD-25). Timer service migrates from engine-global to per-Level with play-in-editor (RAD-52).

## The Problem This Solves

Before this system, the engine ran the obvious loop: measure how much real time the last frame took, hand that number to everything — scripts, physics, rendering all advanced by "however long the last frame happened to take." It feels natural, and it is quietly wrong. Physics integrators are only stable and repeatable when fed steps of consistent size: with a variable step, the same jump reaches different heights at 40 FPS and 200 FPS, a one-frame hitch (window drag, level load) becomes one giant step that fires objects through walls, and no two runs of the game ever compute quite the same world. The simulation's *correctness* was coupled to the display's *speed* — two things that have no business being coupled.

The fix is the industry-standard **fixed timestep with an accumulator** (canonical write-up: Glenn Fiedler's "Fix Your Timestep"). Think of the accumulator as a coin jar. Every rendered frame deposits the real time that just passed into the jar. The simulation only ever advances by withdrawing **fixed-size coins** — exactly 1/60th of a second each, no other denomination exists. A fast 300 FPS frame deposits a sliver of time and usually withdraws nothing; a slow frame deposits enough for two or three coins and the simulation steps two or three times to catch up. Physics now always sees identical step sizes regardless of framerate, so behavior is stable and repeatable on any machine speed.

The leftover balance in the jar — too small to buy a full step — is not waste, it's information: it says *"the moment being displayed sits 40% of the way between the last simulation state and the next one."* That fraction is the **interpolation alpha**, and rendering uses it to draw entities blended between their last two simulated positions. Without it, a 60 Hz simulation on a 144 Hz monitor visibly stutters (many rendered frames show the same unmoved world); with it, motion is smooth at any display rate.

Owning all time in one place buys two features almost for free. **Time dilation:** scale the *deposit* (`accumulator += frameDelta × timeScale`) and the whole world runs in slow motion — fewer coins per real second, each coin still the same size, so physics stability is untouched. **Pause:** time scale 0 deposits nothing, so zero steps run — while rendering, ImGui, and input continue at full rate, which is exactly why a paused game's menus stay responsive.

## Architecture

### The frame order (`GameApplication::Run`)

```text
prime m_LastFrameTime                       (before the loop — frame one measures
while running:                               loop-start → frame-1, not GLFW-init → frame-1)
    frameDelta = now - last                 (double precision, glfwGetTime)
    Window::PollEvents()                    — frame START: simulation sees this frame's input
    if not minimized:
        clock.BeginFrame(frameDelta)        — deposit scaled time, clamp
        while clock.ConsumeStep():          — 0..N fixed steps
            timerManager.Tick(fixedDelta)   — timers fire first (consistent pre-update world)
            each layer: OnFixedUpdate(fixedDelta)
        each layer: OnUpdate(frameDelta)    — render-rate, exactly once
        ImGui Begin → OnImGuiRender → End
    Window::Present()                       — swap at frame end
    LayerStack::ProcessPendingLayers()
```

While minimized, polling and presenting continue but nothing is deposited — the simulation freezes, and (thanks to the clamp) restoring the window does not trigger a catch-up burst.

### FrameClock — the single owner of time

`Core/FrameClock.{h,cpp}` is a plain value type owned by `GameApplication`; every number is seconds stored as `double` (float accumulators visibly drift within hours). Per frame, `BeginFrame(realDelta)` deposits `realDelta × timeScale` and clamps the accumulator to 0.25 s — the **spiral-of-death valve**. Without it, a long hitch would demand many catch-up steps, which take time, which demands more steps, locking the game up; with it, the excess is dropped (throttled `RADIANT_WARN`) and the simulation briefly runs slower than real time instead. `ConsumeStep()` withdraws one fixed delta per call and drives the `while` drain. `GetAlpha()` returns the leftover fraction in [0, 1) — forced to 1.0 while paused so rendering draws the settled current state rather than a frozen mid-blend pose.

The whole mechanism is a dozen lines (`FrameClock.cpp`, condensed):

```cpp
void FrameClock::BeginFrame(double realFrameDelta)
{
    m_RealTime += realFrameDelta;
    m_Accumulator += realFrameDelta * m_TimeScale;   // dilation scales the DEPOSIT...
    if (m_Accumulator > m_MaxAccumulation) { /* clamp + throttled WARN */ }
}

bool FrameClock::ConsumeStep()
{
    if (m_Accumulator < m_FixedDeltaTime)
        return false;
    m_Accumulator -= m_FixedDeltaTime;               // ...never the size of the coin
    m_SimulationTime += m_FixedDeltaTime;
    return true;
}
```

The clock also keeps two odometers: **simulation time** (exactly steps-consumed × fixed delta — freezes under pause, halves under 0.5 dilation) and **real time** (unscaled deposits). Logging one against the other is the quickest way to *see* dilation working.

### The two hooks (`Layer`)

| Hook | Calls per frame | Delta received | Contract |
|------|-----------------|----------------|----------|
| `OnFixedUpdate(ts)` | 0..N | fixed (1/SimulationRate) | all simulation mutations live here |
| `OnUpdate(ts)` | exactly 1 | real frame delta | render-rate work; never mutates simulation state (playbook §1) |

This is the Unity `FixedUpdate`/`Update` (and Godot `_physics_process`/`_process`) split: the **engine** owns frame structure, and a layer opts into simulation semantics just by overriding the right hook. Pause-immunity falls out of the split — anything in `OnUpdate`/`OnImGuiRender` (UI, editor camera someday) simply never notices the time scale.

### Interpolated rendering

`Level::OnFixedUpdate` opens each step by writing a `TransformSnapshotComponent` (translation + rotation; scale is not simulated) for every entity that can move in simulation — today's heuristic: it has a `RigidBody2DComponent`, `CameraComponent`, or `NativeScriptComponent`. `Level::OnRender(alpha)` then draws movable entities at `lerp(snapshot, current, alpha)`; entities without a snapshot draw their current transform, which makes freshly spawned entities snap into place instead of smearing in from a stale position. The primary camera is interpolated too — a script-driven camera left uninterpolated would judder against a smooth world.

The snapshot is a **runtime-only POD component**: the serializer never writes it, it costs one sparse-set write per movable entity per step and one lerp per movable entity per frame — no maps, no allocations. Interpolation displays the world up to one fixed step in the past; that latency (≤ 16.7 ms at 60 Hz) is the standard price of never extrapolating into states the simulation hasn't computed.

### The `Time` facade

`Core/Time.{h,cpp}` is the public face of the clock: stateless statics (`Time::SetTimeScale`, `GetTimeScale`, `GetAlpha`, `GetFixedDeltaTime`, `GetSimulationTime`, `GetRealTime`) forwarding to the application's `FrameClock`. The loop-driving methods (`BeginFrame`/`ConsumeStep`) are deliberately unreachable from game code — `Time` is a friend of `GameApplication`, so game code can observe and scale time but never advance it. The header is include-free so gameplay code can ask the time without paying for the application's headers. A negative time scale is treated as a configuration mistake: clamped to 0 with a `RADIANT_WARN`, never asserted.

### TimerManager — callbacks in simulation time

`Core/TimerManager.{h,cpp}` (reached via `GameApplication::GetTimerManager()`) schedules callbacks N **simulation** seconds out, one-shot or looping. It is ticked once per fixed step — before layer `OnFixedUpdate`, mirroring UE's tick placement — so timers dilate and pause with the world with no special casing: a 2-second timer under 0.5× dilation fires after 4 real seconds, and never fires while paused.

Using it is two calls — schedule with a delay in simulation seconds, clear when the owner tears down:

```cpp
TimerHandle handle = GameApplication::GetTimerManager().SetTimer(2.0f, []()
{
    GAME_INFO("sim {:.2f}s / real {:.2f}s", Time::GetSimulationTime(), Time::GetRealTime());
}, /*looping*/ true);

GameApplication::GetTimerManager().ClearTimer(handle);   // safe even if already fired
```

Internals mirror UE's `FTimerManager`: slot storage with a free list and **generation counters**, plus a min-heap of (expiry, handle) with lazy invalidation. `SetTimer` returns a `TimerHandle` (index + generation); a stale handle is always safe — `ClearTimer` on it is a no-op, `IsActive` returns false. Looping timers re-arm at `expiry += period`, so lateness never compounds. Callbacks may re-entrantly set or clear timers during `Tick`. The one real hazard is documented on `SetTimer`: the manager owns the callback by value, so a lambda capturing an `Entity` or `Level*` outlives its target unless the owner clears the handle in its teardown path.

### Diagnostics

Boot logs the simulation rate (`"Simulation: 60 Hz (16.67 ms fixed step)"`); the accumulator clamp warns (throttled to once per second) when the simulation can't keep up. Non-Dist Reaper builds carry RAD-25 debug scaffolding in `UILayer`: F1/F2/F3 set time scale 1 / 0.5 / 0, and a looping 2-sim-second demo timer logs simulation vs real time.

## Design Rationale

- **Fully fixed step, not UE's variable tick.** UE's game thread is variable-timestep with physics protected by substepping — complexity it carries because a hard fixed step would death-spiral a 10,000-actor world. Our simulation is small, and determinism-by-default is worth more to a learning engine headed toward networking (UE itself re-introduced fixed ticking in its Network Prediction plugin for exactly this reason).
- **Dilation scales the deposit, never the step.** Shrinking the step size for slow motion would change integrator behavior — breaking both stability and repeatability. Keeping the coin size constant means slow motion is *fewer steps per real second*, physically identical per step.
- **`FrameClock` is an extracted value type** so the trickiest logic in the system (clamping, dilation, alpha) is unit-testable without a window: feed it deltas, assert step counts. Doctest coverage lands with the test infrastructure (RAD-67).
- **The engine owns frame structure.** The alternative — each layer running its own accumulator — lets every game reinvent and desynchronize the simulation contract. One drain in `Run()` means new layers get correct semantics with zero engine edits.
- **The render-rate hook kept the name `OnUpdate`, not `OnRender`.** Per-frame work is broader than draw submission (editor fly-cam, pause-immune visual motion, frame bookkeeping); `Level::OnRender(alpha)`, which genuinely is draw-only, keeps the render name. The "rendering never mutates simulation" rule is enforced by review against playbook §1, not by hook naming.
- **Snapshots are a component, not a map,** because the render loop touches every movable entity every frame: sparse-set iteration is contiguous and O(1) per entity where a hash map pays a lookup per entity — and a POD component survives the shallow copy `Level::Copy()` (Phase 5) will perform, honoring playbook §3's actual rule.
- **Determinism honesty:** the fixed step buys *per-machine* reproducibility. Cross-machine determinism (float FP differences, ordering) is a networking-era concern (RAD-59) and explicitly not claimed.

## Known Issues & Evolution

- **Engine-global TimerManager (flagged at design time).** Gameplay timers capturing Level entities couple a global container to Level-lifetime objects — fine while exactly one Level exists, wrong the moment play-in-editor does. The UE answer (per-world manager) becomes per-Level here when RAD-52 lands; the API shape is identical, so the migration is mechanical.
- **Movable-entity heuristic is implicit.** "Has rigidbody/camera/script" is correct and cheap today, but a future animation or tween system (RAD-64, RAD-70) adds movers this list doesn't know about. The end-state is an explicit opt-in marker or the component-hygiene rework (RAD-30) deciding movability.
- **Event handlers still run inside OS callbacks** — polling moved to frame start (this system), but the queue that makes delivery deterministic is RAD-26.
- **The clock is not reset across level loads:** a load hitch triggers the clamp WARN (observed: ~651 ms dropped at the MainMenu→Gameplay transition — the valve working as designed). A future loading-screen system should reset the clock across loads.
- **Teleports must snap, not smear:** an explicit transform push should also refresh the snapshot or interpolation draws a one-frame streak. RAD-28's explicit-teleport path formalizes this.