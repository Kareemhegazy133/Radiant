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
- Collision contacts are *recorded* during the step and *dispatched* after it, with entity-validity checks (landed: RAD-29 2026-08-01). Never mutate the world from inside a Box2D callback. Three checks, three moments: shape validity at translation (an **end** event may name a shape destroyed a step earlier — v3 double-buffers end events); UUID→entity at resolution; and **re-resolution of each side immediately before its callback**, because an earlier callback in the same batch may have destroyed it (UE does the same per-entry `Actor.Get()`). Dispatch order is a guarantee, not an accident: begins before ends (so overlap counters never cross zero spuriously), and Level-wide observers before per-entity script hooks. A callback list owned by a Level uses a `{Index, Generation}` slot handle (§2), and releases during dispatch are deferred — a subscriber may remove itself.

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
8. **entt view invalidation** — destroying entities or sorting the iterated pool mid-iteration.

## §9 — Containers & Custom Data Structures (decided 2026-07-05)

- Default to the C++20 standard library. **No wholesale UE-style container clones** (TArray/TMap/TSet): UE's containers exist for 1998-era portability plus GC/reflection integration — forcing functions Radiant doesn't have. Revisit only if reflection (RAD-72) ever demands container introspection, and even then prefer traits/adapters over replacements.
- Build **specialized** structures only where std has no answer and the win is concrete: generation-handle pools (TimerManager), sparse sets (entt provides ours), per-frame ring buffers (Phase 3 frames-in-flight).
- When allocation strategy matters, reach for `std::pmr` / custom allocators on std containers before writing containers.
- **Gated learning containers (decided 2026-07-05):** targeted structures may be built where std is genuinely weak. The catalog, each with a named customer: `InlineArray<T,N>` (small-buffer array — std::vector has no SBO; check C++26 `std::inplace_vector` first), an open-addressing hash map (`std::unordered_map` is standard-mandated pointer-chasing; replace in *measured* hot paths only), `SlotMap<T>` (generalize TimerManager's pool at the rule-of-three moment — RAD-30 side tables, Phase 3 GPU resource pools), `RingBuffer<T>` (frames-in-flight §5, event queue — likely built inside those stories), `StringID` (FName-alike; Phase 4 asset names, GAS tags). Hard gates: only after RAD-67 (doctest) + RAD-83 (ASan) are Done; DoD includes benchmarks on Radiant's own workloads; adoption is per-call-site by measurement; never blocks phase work. API sugar (Contains/AddUnique-style helpers over std) is a separate cheap utilities chore, not a container.
