# Radiant Engine Playbook

Established patterns and hard-won rules for this codebase. Cite sections by number (e.g. "playbook §3"). Seeded from the 2026-07-04 audit; grows via `/audit-standards` as patterns get established. Rules marked **(target)** describe the post-rework architecture — enforce them in all new code; legacy code migrates by phase.

## §1 — Frame Architecture (landed: RAD-25 2026-07-05, RAD-26 2026-07-09)

Frame order: **pump + process events at frame start → fixed-step simulation (accumulator) → variable-rate render (interpolated)**.
- Simulation steps at a fixed rate; never pass raw frame delta into physics or gameplay-critical logic.
- Events are enqueued by OS callbacks and processed at a single defined point at frame start (`EventQueue::ProcessEvents` in `Run()`, unconditional even while minimized); handlers never execute inside OS callbacks. Platform callbacks translate + `Push` only.
- Rendering reads simulation state; it never mutates it. Reference: Glenn Fiedler, "Fix Your Timestep!".

## §2 — Ownership Contract

- One ownership story per type, stated at the declaration. `Ref<T>` = shared (intrusive refcount), `Scope<T>` = unique, value = value.
- `LayerStack` owns its layers. Containers that own raw pointers must be the *only* deleter.
- Types owning raw API handles (`m_RendererID`, `VkBuffer`, `b2Body*`) delete copy or implement rule-of-5.
- The last-reference release idiom is `if (count.fetch_sub(1) == 1) delete` — decrement-then-separately-check is a race (the RAD-7 bug class).
- **Weak references in Radiant are generation handles** (`TimerHandle`, `entt::entity`, asset UUIDs) — there is no `WeakRef` type, deliberately (decided 2026-07-09). An intrusive refcount cannot support weak semantics: the count dies with the object, and adding a control block forfeits intrusive's single-allocation benefit (UE agrees — `TRefCountPtr` has no weak counterpart; `TWeakObjectPtr` is index + serial, i.e. a generation handle). Domain handles also carry the right recovery semantics (placeholder asset, timer no-op, entity validity check) where a generic `Lock()` can only return null. Revisit only if the Phase 4 asset cache produces a concrete customer.

## §3 — Data-Only Components (target, Phase 2)

- Components are plain serializable data: no owning raw pointers, no `std::function`, no `type_index`. Must survive shallow copy — `Level::Copy()` (play-in-editor) depends on it.
- Runtime state lives in Level-owned side tables keyed by entity: `b2Body*` handles, script instances.
- Adding a component = declare in `Components.h` + serializer entry + (Phase 5) inspector entry. If you add one and skip a site, the review flags it.

## §4 — Physics Integration Rules

- One physics world **per Level**, owned by the Level as `Scope<PhysicsWorld2D>` (landed: RAD-27 2026-07-09, on Box2D v3.1.1 — body handles are packed `b2BodyId` generation handles, per §2).
- Bodies/fixtures are created via entt `on_construct`/`on_destroy` signals — keep this pattern.
- **Never** destroy/recreate fixtures per frame (kills contact persistence, sleeping, warm-starting). Shapes mutate in place on explicit refresh only (landed: RAD-28 2026-07-10 — `Level::RefreshCollider` → v3 setters + one mass recompute).
- Physics owns the transform of dynamic bodies. ECS→Box2D push happens only at spawn and via `Level::Teleport` (which also stamps the render snapshot and wakes the body); Box2D→ECS readback drains the world's move events after each step — cost scales with activity, not population — never inside a render loop (landed: RAD-28 2026-07-10).
- **The verb surface has one shape** (landed: RAD-91 2026-08-01). Runtime-id resolution lives in **one helper per id kind**, never a copy per verb: zero id = silent null (the failure was logged where it happened); stale id = WARN naming the verb, zero the component field, null. A caller needing a *different* zero-id policy keeps that branch at its own call site rather than adding a flag to the shared helper. The helper takes the `Entity`, not its UUID — `RADIANT_WARN` survives Dist, so a UUID parameter would hoist a component lookup onto every verb's hot path (§8.5 wearing its performance hat). Verbs take `Entity` **by value**; a component reference is a parameter only where the caller already holds one (entt signal handlers, a precondition assert).
- **Gameplay verbs split by what they touch** (landed: RAD-90 2026-08-01). `Level` owns the verbs that touch **ECS state** — `Teleport` and `RefreshCollider` write `TransformComponent` and the render snapshot, so only the type owning the registry can perform them. `PhysicsBody` (a 16-byte value handle over an `Entity`, from `Entity::GetPhysicsBody()`) owns the verbs that touch **only physics state** — forces, impulses, velocities, kinematic targets. The handle owns nothing and caches nothing: it re-resolves per call, so the stale-id recovery above still applies; `operator bool` asks whether the *entity* is physics-capable and deliberately does not probe the id, which is the verb's job because the verb is what can recover. This is the first instance of the accessor convention in RAD-94.
- **One resolution helper per layer, never one per verb** — the rule above generalised (RAD-90). Each layer has exactly one way to fail, so each gets exactly one helper: `PhysicsBody::Resolve` (level-less handle) → `Level::ResolvePhysics` (dead entity, world-less scratch level) → `ResolveBody` (missing component, dead id, wrapping `ResolveBodyId` rather than copying it). Result: three-line vendor verbs and two-line gameplay verbs, with no preamble anywhere to get subtly wrong.
- **Rare and discontinuous verbs log; routine and continuous verbs do not** (RAD-90). `Teleport` TRACEs — it is a deliberate act and the log is the audit trail of every explicit ECS→Box2D push. Force/impulse/velocity/kinematic verbs log nothing: a character controller calls them at 60 Hz and would drown every other line.
- **Physics modules take application state as parameters, never read it** (RAD-90). `MoveKinematic`'s fixed delta is a parameter supplied by `PhysicsBody` from `Time::GetFixedDeltaTime()`, so `PhysicsWorld2D` holds no clock and stays constructible in a test with no `GameApplication` (RAD-67). A cached "last step's delta" was rejected: scripts run before the first `Step`, so it would silently drop the first call.
- **Semantic flags are enums, not bools** (RAD-90). `Teleport(pos, 0.f, TeleportType::ResetVelocity)` reads itself; `Teleport(pos, 0.f, true)` does not. A type both `Level.h` and `PhysicsWorld2D.h` must name gets its own vendor-free header in `Physics/` — the `ContactEvent.h` precedent.
- Collision contacts are *recorded* during the step and *dispatched* after it, with entity-validity checks (landed: RAD-29 2026-08-01). Never mutate the world from inside a Box2D callback. Three checks, three moments: shape validity at translation (an **end** event may name a shape destroyed a step earlier — v3 double-buffers end events); UUID→entity at resolution; and **re-resolution of each side immediately before its callback**, because an earlier callback in the same batch may have destroyed it (UE does the same per-entry `Actor.Get()`). Dispatch order is a guarantee, not an accident: begins before ends (so overlap counters never cross zero spuriously), and Level-wide callbacks before per-entity script hooks. A callback list owned by a Level uses a `{Index, Generation}` slot handle (§2), and releases during dispatch are deferred — a subscriber may remove itself.

## §5 — RHI v2 Principles (target, Phase 3)

- Interface models Vulkan's shape, not GL's: command-buffer recording, pipeline state objects (vertex layout lives in the pipeline — no VertexArray concept), descriptor-based binding, explicit per-frame-in-flight resource lifetimes.
- Shaders are SPIR-V, compiled by shaderc at build/cook time with an on-disk cache; reflection (SPIRV-Cross) derives layouts. No runtime GLSL text, no string-name uniform sets.
- API-specific types appear only under `Platform/<API>/`. If a public engine header needs a graphics type, the abstraction is wrong.
- CPU-written per-frame buffers (batch vertices, camera UBOs) are ringed per frame-in-flight — never reused while the GPU may still read them.

## §6 — Asset Rules

- Runtime references assets by **handle** (UUID), never by path. Paths exist only in registry/import code.
- Import (mints handles, mutates registry) and load (resolves handles) are different operations — editor-side vs runtime-side (target, Phase 4).
- Failed loads return placeholder assets (magenta checkerboard, default font) — never null, never a cached null.
- Running the game never writes assets. Saving is an explicit action.

## §7 — Build System Rules

- Workspace root aggregates; each project has its own premake file declaring its own dependencies.
- Generated files (`.sln`, `.vcxproj`) are never committed. Submodules pin to deliberate tags/commits; each fork's reason is documented.
- Configs: Debug (asserts, symbols), Release (optimized + asserts), Dist (shipping: WindowedApp, LTO, no asserts). Code must compile in **all three** — beware Dist-only breaks from code that exists only inside assert/log macros.
- **Upgrades (toolchain + vendors) happen at phase boundaries with a stated reason** — never mid-story, one library per commit, verified by all three configs + a Reaper run (+ ASan once available). "Newest" is not a reason; unowned drift is how the spdlog-1.14/fmt formatter breakage happened (2026-07-05). Planned: C++23 at Phase 4 start (`std::expected` for asset/serialization errors); ImGui refreshed before Phase 5. Done: Box2D v3.1.1 (RAD-27, 2026-07-09 — first owned-fork vendor per the fork policy; yaml-cpp/msdf follow via RAD-88/89).

## §8 — Known Bug Patterns (watch for these in review)

1. **Decrement-then-check** on atomics (see §2) — any check-then-act on a shared counter.
2. **String-as-condition asserts** — `RADIANT_ASSERT("msg")` is always truthy; the macro takes (condition, message).
3. **Interior-pointer delete** — deleting a bump-allocator cursor instead of the base allocation.
4. **Cache poisoning** — storing failure results (null) in a cache so retries can never succeed.
5. **Side effects in asserts/logs** — vanish in Dist; the expression must be evaluable-free.
6. **Working-directory-relative paths** — break the moment the exe runs outside VS.
7. **Copyable types holding raw resource handles** — shallow copy → double free (rule-of-5, §2).
8. **entt view invalidation** — destroying entities or sorting the iterated pool mid-iteration. **Any loop that hands control to gameplay must not be riding a view** (landed: RAD-95 2026-08-02): snapshot the entity ids into a reusable member vector, then walk those, re-validating each. And note the *second*, distinct hazard in the same loop — a component **reference** obtained before the callback dangles if the callback emplaces into that pool. Iterator invalidation and reference invalidation need separate fixes; the snapshot cures only the first, so re-fetch the component after anything that can run gameplay.

## §9 — Containers & Custom Data Structures (decided 2026-07-05)

- Default to the C++20 standard library. **No wholesale UE-style container clones** (TArray/TMap/TSet): UE's containers exist for 1998-era portability plus GC/reflection integration — forcing functions Radiant doesn't have. Revisit only if reflection (RAD-72) ever demands container introspection, and even then prefer traits/adapters over replacements.
- Build **specialized** structures only where std has no answer and the win is concrete: generation-handle pools (TimerManager), sparse sets (entt provides ours), per-frame ring buffers (Phase 3 frames-in-flight).
- When allocation strategy matters, reach for `std::pmr` / custom allocators on std containers before writing containers.
- **Gated learning containers (decided 2026-07-05):** targeted structures may be built where std is genuinely weak. The catalog, each with a named customer: `InlineArray<T,N>` (small-buffer array — std::vector has no SBO; check C++26 `std::inplace_vector` first), an open-addressing hash map (`std::unordered_map` is standard-mandated pointer-chasing; replace in *measured* hot paths only), `SlotMap<T>` (generalize TimerManager's pool at the rule-of-three moment — RAD-30 side tables, Phase 3 GPU resource pools), `RingBuffer<T>` (frames-in-flight §5, event queue — likely built inside those stories), `StringID` (FName-alike; Phase 4 asset names, GAS tags). Hard gates: only after RAD-67 (doctest) + RAD-83 (ASan) are Done; DoD includes benchmarks on Radiant's own workloads; adoption is per-call-site by measurement; never blocks phase work. API sugar (Contains/AddUnique-style helpers over std) is a separate cheap utilities chore, not a container.

## §10 — The Gameplay Seam (landed: RAD-95 2026-08-02)

What gameplay may reach, and through what. RAD-94 owns the wider accessor convention and folds this in; this section is the seam itself.

- **Two accessors on a script, and everything chains off them:** `GetEntity()` and `GetLevel()`. No component forwarders, no subsystem forwarders. The distinction that decides whether an accessor may be added is **bounded vs unbounded**, not a count:
  - *Relationship navigation* (RAD-94 category B) is **bounded** — sized by relationships that actually exist. A script has two: its entity, and that entity's level. A third accessor requires a third *relationship* to exist first; RAD-94 forbids fabricating one.
  - *Subsystem facades* (category C — `GetPhysicsBody`, future `GetAbilitySystem`, `GetAnimation`) are **unbounded**: one per subsystem, forever. Never forward these. Reach them through `GetEntity()`, which costs the seam zero edits per subsystem. UE agrees on both halves — `UActorComponent` carries `GetOwner()` **and** `GetWorld()`, and no per-subsystem forwarder.
- **Verbosity at the call site is a feature on the component path.** `GetEntity().GetComponent<T>()` says *whose* component and looks like the sparse-set lookup it is; the forwarder it replaced read like a member access, which is how two lookups for one component went unnoticed in `CameraController` for a year. Named gameplay verbs (`GetLocation`) are the designed answer and belong to RAD-99 — never accreted ad hoc to relieve a call site.
- **The dividing rule:** a verb that names one entity lives on `Entity` (`Teleport`, `Destroy`, `RefreshCollider`); a verb about the level as a whole lives on `GameplayLevel`. Mechanical, so it does not drift. A rule with a hole in it on day one is a rule nobody trusts — `Entity::RefreshCollider` was added in the same change for exactly that reason.
- **Narrow by type, because we cannot narrow by annotation.** `Level` is also the frame driver, so gameplay gets `GameplayLevel` — a value handle over `Level*` exposing the safe subset. UE narrows the same surface with `UFUNCTION` tags enforced by a reflection system; with RAD-72 iceboxed, a type is the only place our compiler can read the decision. **Never add an accessor returning the underlying pointer, or a conversion to it** — that hands back exactly what the type exists to withhold, and no other rule here survives it.
- **A facade documents its exclusions, not just its inclusions.** `GameplayLevel`'s header carries a row per withheld verb with the reason, so the next addition meets a decision rather than a precedent. The failure mode of a narrowing facade is silent under-exposure, and the doc is the mitigation.
- **The umbrella header exports game-facing types; other headers must not do it by accident.** Reaper reached `PhysicsBody` transitively through `ScriptableEntity.h` purely because a forwarder happened to include it; deleting the forwarder broke game code that never named the wrong header. Facades belong in `Radiant.h`.
- **A header that is never compiled alone is never proven self-contained.** `ScriptableEntity.h` named `Timestep` without including it and compiled for years inside `Level.cpp`. Giving a header its own TU is the cheapest way to find that class of bug.
