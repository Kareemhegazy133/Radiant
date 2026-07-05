# Memory & Reference Counting

**Status:** Stable design — implementation hardening in progress (RAD-7, RAD-19).

## Architecture

Radiant has a four-word ownership vocabulary. Every type commits to exactly one:

| Model | Type | Use |
|-------|------|-----|
| Shared | `Ref<T>` | Renderer resources (buffers, shaders, textures, framebuffers), assets, levels, game states |
| Unique | `Scope<T>` (= `std::unique_ptr`) | Window, graphics context, subsystem data blocks |
| Value | plain member / component | ECS components, math types, specs |
| Observer | raw pointer / `WeakRef<T>` | Non-owning back-references (`Entity::m_Level`) |

### RefCounted + Ref<T> (`Core/Ref.h`)

`Ref<T>` is an **intrusive** reference-counted smart pointer: the count lives inside the object as `std::atomic<uint32_t> RefCounted::m_RefCount`, and `Ref<T>` statically requires `T : RefCounted`. Construction/copy call `IncRef()`; destruction/reassignment call `DecRef()`, which deletes the instance when the count reaches zero.

Key API: `Ref<T>::Create(args...)` (preferred construction), `.As<T2>()` (static-cast conversion), `.Raw()` (non-owning access), copy/move/nullptr semantics as expected.

### Live-reference tracking + WeakRef

Every `IncRef` registers the instance pointer in a global mutex-guarded set (`Ref.cpp`); final release removes it. `WeakRef<T>` holds a raw pointer and implements `IsValid()` by asking that set (`RefUtils::IsLive`). This is a **debug facility** — leak detection and dangling-weak-checks — but it is currently compiled into all builds and taxes every `Ref` construction with a mutex acquisition.

## Design Rationale

**Why intrusive rather than `std::shared_ptr`?** Three principal-level reasons, and they are the same reasons UE's `TRefCountPtr` is intrusive:

1. **No control block** — the count sits in the object's own cache line; `shared_ptr` allocates a separate control block (or forces `make_shared` discipline) and doubles pointer size for weak support.
2. **Recoverable from a raw pointer** — an intrusive `Ref` can be safely reconstructed from `T*` anywhere (the count travels with the object); reconstructing a `shared_ptr` from raw is a double-delete. This matters for C APIs and deferred callbacks that traffic in raw pointers.
3. **The engine controls the semantics** — asset unloading (Phase 4, RAD-44) will key off the manager holding the last reference; owning the refcount type makes that trivial to observe.

The cost is that only `RefCounted` types participate — enforced by `static_assert`, which is fine for an engine that owns its resource types.

## Rules (enforced in review — playbook §2)

- One ownership story per type, stated at the declaration. Never `RefCounted` inheritance *and* `Scope` ownership on the same type.
- No raw owning pointers in new code; a container of raw pointers must be the sole deleter.
- Types holding raw resource handles (GL ids, `Vk*`, `b2Body*`) delete copy or implement rule-of-5.
- The last-reference release decision comes from the decrement itself: `fetch_sub(1) == 1` — never decrement-then-check (see below).

## Known Issues & Evolution

- **Release race (RAD-7, in progress):** `DecRef` decrements atomically, then does a *separate* zero-check before `delete`. Two threads releasing the last two references can both observe zero → double free. The fix is the canonical `fetch_sub(1, memory_order_acq_rel) == 1` idiom. Latent today (single-threaded engine); must be fixed before Phase 3 async work and the job system.
- **Tracking overhead in all builds (RAD-7):** the live-reference set (global mutex + hash set per Ref construction) will be compiled down to Debug builds only; `RefUtils::IsLive` also reads the set without the lock — a data race.
- **`Ref::CopyWithoutIncrement` is broken and unused** — dereferences a null `Ref`; deleted in the Phase 1 dead-code sweep (RAD-23) together with the unused `WeakRef` (nothing in the engine currently holds one).
- **Ownership inconsistencies (RAD-19):** `GraphicsContext` inherits `RefCounted` but is owned by a `Scope`; `LayerStack` stores raw `Layer*` but `GameApplication` deletes them. Both get one owner each in Phase 1.
