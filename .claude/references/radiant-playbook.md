# Radiant Engine Playbook

Established patterns and hard-won rules for this codebase. Cite sections by number (e.g. "playbook §3"). Seeded from the 2026-07-04 audit; grows via `/audit-standards` as patterns get established. Rules marked **(target)** describe the post-rework architecture — enforce them in all new code; legacy code migrates by phase.

## §1 — Frame Architecture (target, Phase 2)

Frame order: **pump event queue → fixed-step simulation (accumulator) → variable-rate render (interpolated)**.
- Simulation steps at a fixed rate; never pass raw frame delta into physics or gameplay-critical logic.
- Events are enqueued by OS callbacks and drained at a single defined point at frame start. Handlers never execute inside OS callbacks (re-entrancy).
- Rendering reads simulation state; it never mutates it. Reference: Glenn Fiedler, "Fix Your Timestep!".

## §2 — Ownership Contract

- One ownership story per type, stated at the declaration. `Ref<T>` = shared (intrusive refcount), `Scope<T>` = unique, value = value.
- `LayerStack` owns its layers. Containers that own raw pointers must be the *only* deleter.
- Types owning raw API handles (`m_RendererID`, `VkBuffer`, `b2Body*`) delete copy or implement rule-of-5.
- The last-reference release idiom is `if (count.fetch_sub(1) == 1) delete` — decrement-then-separately-check is a race (the RAD-7 bug class).

## §3 — Data-Only Components (target, Phase 2)

- Components are plain serializable data: no owning raw pointers, no `std::function`, no `type_index`. Must survive shallow copy — `Level::Copy()` (play-in-editor) depends on it.
- Runtime state lives in Level-owned side tables keyed by entity: `b2Body*` handles, script instances.
- Adding a component = declare in `Components.h` + serializer entry + (Phase 5) inspector entry. If you add one and skip a site, the review flags it.

## §4 — Physics Integration Rules

- One physics world **per Level**, owned by the Level (target; the static singleton is the RAD-27 bug).
- Bodies/fixtures are created via entt `on_construct`/`on_destroy` signals — keep this pattern.
- **Never** destroy/recreate fixtures per frame (kills contact persistence, sleeping, warm-starting). Rebuild fixtures only when collider properties change.
- Physics owns the transform of dynamic bodies. ECS→Box2D push happens only on explicit teleport/spawn; Box2D→ECS readback is a dedicated sync pass after each step — never inside a render loop.
- Collision contacts are *recorded* during the step and *dispatched* after it, with entity-validity checks. Never mutate the world from inside a Box2D callback.

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

## §8 — Known Bug Patterns (watch for these in review)

1. **Decrement-then-check** on atomics (see §2) — any check-then-act on a shared counter.
2. **String-as-condition asserts** — `RADIANT_ASSERT("msg")` is always truthy; the macro takes (condition, message).
3. **Interior-pointer delete** — deleting a bump-allocator cursor instead of the base allocation.
4. **Cache poisoning** — storing failure results (null) in a cache so retries can never succeed.
5. **Side effects in asserts/logs** — vanish in Dist; the expression must be evaluable-free.
6. **Working-directory-relative paths** — break the moment the exe runs outside VS.
7. **Copyable types holding raw resource handles** — shallow copy → double free (rule-of-5, §2).
8. **entt view invalidation** — destroying entities or sorting the iterated pool mid-iteration.
