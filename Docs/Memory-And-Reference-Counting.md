# Memory & Reference Counting

**Status:** Stable — release-race fix and Debug-only tracking landed 2026-07-05 (RAD-7); ownership fixes (RAD-19) pending.

## The Problem This Solves

C++ has no garbage collector: every object created with `new` must be deleted by someone, exactly once. With a single owner that's easy. But engine resources are **shared** — one texture might be in use by five sprites, the asset manager's cache, and a level mid-load, and those users come and go in any order. Delete too early and every remaining user holds a pointer to freed memory (a crash); never delete and memory leaks until the app dies. The question is always: *who deletes, and when?*

Reference counting answers it with a **sign-in sheet attached to each shared object**. Anyone who starts using the object signs in (+1); anyone who stops signs out (−1). Whoever signs out and finds the sheet empty is, by definition, the last user — *they* turn off the lights and delete the object. No coordination is needed; the count **is** the coordination.

What makes it automatic is C++'s most reliable feature: constructors and destructors run deterministically. `Ref<T>`'s constructor signs in; its destructor signs out. You never call inc/dec yourself — you cannot forget:

```cpp
{
    Ref<Texture2D> a = Texture2D::Create("player.png");  // count: 1
    Ref<Texture2D> b = a;                                 // copy → count: 2
}   // b destructs (count: 1), a destructs (count: 0) → texture deleted here
```

(Copying signs in a new user; *moving* just hands over an existing signature — which is why a move doesn't touch the count.)

## Architecture

Radiant has a four-word ownership vocabulary. Every type commits to exactly one:

| Model | Type | Use |
|-------|------|-----|
| Shared | `Ref<T>` | Renderer resources (buffers, shaders, textures, framebuffers), assets, levels, game states |
| Unique | `Scope<T>` (= `std::unique_ptr`) | Window, graphics context, subsystem data blocks |
| Value | plain member / component | ECS components, math types, specs |
| Observer | raw pointer | Non-owning back-references (`Entity::m_Level`); weak references are deliberately absent (see below) |

### RefCounted + Ref<T> (`Core/Ref.h`)

`Ref<T>` is an **intrusive** reference-counted smart pointer: the count lives inside the object as `std::atomic<uint32_t> RefCounted::m_RefCount`, and `Ref<T>` statically requires `T : RefCounted`. Construction/copy call `IncRef()`; destruction/reassignment call `DecRef()`.

**The release contract:** the decision "did I release the last reference?" comes from the decrement itself. `RefCounted::DecRefCount()` performs `fetch_sub(1, std::memory_order_acq_rel)` and returns `true` only to the caller whose decrement took the count from 1 to 0 — that caller (and no other) deletes. This is the same idiom as UE's `TTransactionalAtomicRefCount::ImmediatelyRelease`. `GetRefCount()` exists for diagnostics only — never build release logic on an observed count, because it can change between the load and any decision made from it. An underflow (releasing a dead object) triggers `RADIANT_DEBUGBREAK` in Debug.

Key API: `Ref<T>::Create(args...)` (preferred construction), `.As<T2>()` (static-cast conversion), `.Raw()` (non-owning access), copy/move/nullptr semantics as expected.

### Live-reference tracking (Debug only)

Behind `RADIANT_TRACK_REFERENCES` (`Base.h` — on under `RD_DEBUG`, flippable to 1 manually for leak-hunting optimized builds): every `IncRef` registers the instance pointer in a global mutex-guarded set (`Ref.cpp`); the final release removes it **before** the delete, so a destroyed object is never still reported as tracked. In Release/Dist the tracking compiles out entirely and `IncRef`/`DecRef` are bare atomic operations.

**Weak references are deliberately absent.** The fork's unused `WeakRef<T>` was removed 2026-07-05 (its validity check depended on tracking that no longer exists outside Debug — a lying API in Release). When a real consumer appears, a weak reference gets designed properly (control block or generation counters) as its own piece of work.

**Include-order note:** `Base.h` and `Ref.h` form a deliberate cycle — `Base.h` defines `RADIANT_DEBUGBREAK`/`RADIANT_TRACK_REFERENCES` *before* including `Ref.h` at its bottom, so `Ref.h` can use those macros from any include entry point. Don't move `#include "Core/Ref.h"` above the macro definitions.

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

- **Ownership inconsistencies (RAD-19):** `GraphicsContext` inherits `RefCounted` but is owned by a `Scope`; `LayerStack` stores raw `Layer*` but `GameApplication` deletes them. Both get one owner each in Phase 1.
- **Over-release detection compiles out of Dist:** the underflow debug-break is active in Debug and Release (RAD-9), out of Dist — a Dist over-release corrupts silently. Acceptable: all iteration happens in checked configs; Dist is the player build.
