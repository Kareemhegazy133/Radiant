# Implementation Plan — RAD-7: Fix Ref<T> release race: decrement-then-check is not atomic

| Field | Value |
|-------|-------|
| **Jira** | [RAD-7](https://hndredgames.atlassian.net/browse/RAD-7) |
| **Epic** | Phase 1 — Triage & Foundation Cleanup (RAD-1) |
| **Story status** | In Progress |
| **Dependencies** | None |
| **Planned** | 2026-07-04 |
| **Implemented** | 2026-07-05 — delegated to Claude by Kareem (all steps) |

---

## 1. Architecture Decision

The defect: `Ref<T>::DecRef` (`Core/Ref.h`) decrements the atomic refcount, then performs a *separate* atomic load to test for zero before deleting. Each operation is individually atomic, but the sequence is not — two threads releasing the last two references can interleave between the decrement and the load so that **both** observe zero and both delete (playbook §2, §8.1). The fix is the canonical release idiom: the decision "did I release the last reference?" must come from the decrement itself — `fetch_sub` atomically decrements **and returns the previous value**, and exactly one thread can receive `1`.

Design choice that shapes the change: `RefCounted::DecRefCount()` changes signature from `void` to `bool` — *"returns true if this call released the last reference."* This moves the release decision into the one place that can answer it correctly, and makes the wrong pattern (`DecRefCount(); if (GetRefCount() == 0)`) impossible to write through the public API. `GetRefCount()` remains for diagnostics only — notably, UE 5.6 *deprecated* refcount inspection entirely (`FReturnedRefCountValue`) with the comment "it is unsafe to rely on the value of a refcount for any logic" — which is precisely the bug we are fixing.

Two secondary hardenings ride along (same card, same files): the live-reference tracking becomes Debug-only (it currently taxes every `Ref` construction with a global mutex in all builds), and `RefUtils::IsLive` gets the lock it is missing.

## 2. UE Reference

Studied: `Engine/Source/Runtime/Core/Public/Templates/RefCounting.h` (UE 5.7.4 source build), specifically `UE::Private::TTransactionalAtomicRefCount::ImmediatelyRelease` and `FRefCountBase`.

**What UE does — and what we adopt:**
- `ImmediatelyRelease` is exactly the target idiom: `AtomicType RefsBeforeRelease = RefCount.fetch_sub(1, std::memory_order_acq_rel); if (RefsBeforeRelease == 1) { DeleteFn(this); }`. Epic's own comment explains the ordering choice: *"std::memory_order_acq_rel is used so that, if we do end up executing the destructor, it's not possible for side effects from executing the destructor to end up being visible before we've determined that the reference count is actually zero."* We adopt the idiom and the ordering verbatim.
- **Over-release detection:** UE checks `RefsBeforeRelease == 0` (an underflow — someone released a dead object) and raises a check. We adopt this as a `RADIANT_ASSERT`.
- **AddRef is a plain `++`** — no ordering decision hangs on an increment, so UE doesn't pay for one. We keep our `++m_RefCount` unchanged.

**What UE has that we deliberately skip:**
- **AutoRTFM transactional memory** (deferred releases, on-abort compensation) — UE-scale infrastructure for their transactional runtime; irrelevant to Radiant.
- **`FReturnedRefCountValue` deprecation wrapper** — UE's migration tooling for a 25-year-old codebase with thousands of call sites. Radiant has a handful; we just don't build logic on `GetRefCount()`.
- UE splits thread-safe (`FRefCountBase`) and non-thread-safe (`FRefCountedObject`, documented "should not be used for new code") variants. Radiant keeps **one** thread-safe type — a second variant is a footgun with no measurable win at our scale.

## 3. File Plan

```text
Radiant/Source/Core/
    Base.h                       (modify) — RADIANT_TRACK_REFERENCES define (on in Debug)
    Ref.h                        (modify) — DecRefCount → bool (fetch_sub idiom); DecRef branches on it;
                                            bookkeeping-before-delete; tracking calls gated;
                                            delete CopyWithoutIncrement + WeakRef (decision below)
    Ref.cpp                      (modify) — IsLive takes the lock; tracking bodies gated
Docs/
    Memory-And-Reference-Counting.md (modify) — Known Issues entries resolved; Architecture updated
```

| Action | Path | Description |
|--------|------|-------------|
| Modify | `Radiant/Source/Core/Ref.h` | The core fix + API removal |
| Modify | `Radiant/Source/Core/Ref.cpp` | `IsLive` lock; `#if RADIANT_TRACK_REFERENCES` gating |
| Modify | `Radiant/Source/Core/Base.h` | Tracking switch definition |
| Modify | `Docs/Memory-And-Reference-Counting.md` | Docs update contract (Docs/README.md) |

## 4. Type Design

No new types. One signature change:

### RefCounted (modified)
- **Kind:** base class (existing)
- **Responsibility:** owns the intrusive atomic refcount; now also owns the release decision.
- **Key change:**
  - `bool DecRefCount() const` — decrements via `fetch_sub(1, std::memory_order_acq_rel)`; returns `true` iff the previous value was `1` (this call released the last reference); asserts on underflow (previous value `0`).
- **Playbook Patterns:** §2 (ownership contract), §8.1 (decrement-then-check ban).

### Removed API (pulled forward from RAD-23 — pending approval)
- `Ref<T>::CopyWithoutIncrement` — unused and broken (dereferences a null `Ref`).
- `WeakRef<T>` — unused; its `IsValid()` depends on live-tracking, which becomes Debug-only, leaving it broken-by-design in Release. Deleting beats shipping a lying API. A real weak-reference design (control block or generation counters) is a deliberate future decision when a consumer exists.

## 5. Implementation Steps

### Phase 1 — The release race (the core)
- [x] **`RefCounted::DecRefCount` → `bool`** — `fetch_sub(1, std::memory_order_acq_rel)`, return `previous == 1`; over-release triggers `RADIANT_DEBUGBREAK()` (plain macro instead of `RADIANT_ASSERT`: `Ref.h` parses before the assert/log macros exist in the `Base.h` include cycle — see implementation note below).
- [x] **`Ref<T>::DecRef` branches on the return** — no `GetRefCount()` call; bookkeeping (`RemoveFromLiveReferences`) moves **before** `delete` so a deleted object is never still "live".

### Phase 2 — Debug-only tracking + the missing lock
- [x] **`RADIANT_TRACK_REFERENCES` switch in `Base.h`** — defined to 1 under `RD_DEBUG`, 0 otherwise (a dedicated switch, not raw `RD_DEBUG`, so it can be force-enabled in Release when leak-hunting).
- [x] **Gate tracking; `IsLive` removed** — `#if RADIANT_TRACK_REFERENCES` around the set/mutex/functions and their call sites in `IncRef`/`DecRef`. `IsLive` (whose only consumer was `WeakRef`) was deleted with `WeakRef` rather than locked — no consumer remained to protect.

### Phase 3 — Dead API removal
- [x] **Delete `CopyWithoutIncrement` and `WeakRef`** — approved via full-card delegation; pull-forward noted on RAD-23.

### Phase 4 — Verify & close
- [x] **Build Debug, Release, Dist** — all clean, zero new warnings. (Fixing a warning required `RADIANT_DEBUGBREAK()`'s no-op form to become `((void)0)` in `Base.h`.) Sandbox fails to compile in *all* trees including untouched `dev` — pre-existing breakage from the 2024-12 MSDF removal, verified by stash test; not caused by this change.
- [x] **Run Reaper** — Debug build with tracking + asserts active boots to main menu and runs stably (10s soak, clean shutdown).
- [x] **Docs + board** — `Docs/Memory-And-Reference-Counting.md` rewritten (release contract, Debug-only tracking, weak-refs-absent note, include-order note); plan posted to RAD-7; RAD-23 pull-forward comment; RAD-7 → In Review.

**Implementation notes (deviations from plan):**
1. **`Base.h` include-order fix (unplanned prerequisite):** `Base.h` included `Ref.h` at its top, *before* defining `RADIANT_DEBUGBREAK`/`RADIANT_ENABLE_ASSERTS` — meaning `Ref.h` could never reliably use engine macros. `#include "Core/Ref.h"` moved below the macro definitions (commented), and `Ref.h` now includes `Core/Base.h` itself, making it self-contained from any entry point.
2. **Over-release check uses `RADIANT_DEBUGBREAK()` directly**, not `RADIANT_ASSERT` — the assert machinery expands to `RADIANT_ERROR` logging macros that aren't yet defined when `Ref.h` parses in some include orders. A debug-break with a code comment is dependency-free and equally loud under a debugger.
3. **`RADIANT_DEBUGBREAK()` no-op form fixed to `((void)0)`** — the empty expansion produced warning C4390 (`if (x) ;`) in Release/Dist.

## 6. Ownership & Lifetime Strategy

No ownership model changes — this hardens the existing `Ref` contract. The one lifetime-semantics change: the live-reference set now loses the instance *before* destruction instead of after, closing the window where a destroyed object was still reported live. `Reset()`, all assignment operators, and the destructor funnel through the single fixed `DecRef`, so every release path inherits the fix.

## 7. Performance Notes

- **Release/Dist win:** every `Ref` copy/destruction currently acquires a global mutex and touches a hash set; with tracking compiled out, `IncRef`/`DecRef` become a bare atomic op — measurable in Ref-heavy paths (asset churn, state transitions).
- The `fetch_sub` change is cost-neutral: `--m_RefCount` was already an atomic RMW; we now simply keep its return value instead of issuing a second load.
- Debug builds keep the tracking cost knowingly — that's what Debug is for.

## 8. Logging & Diagnostics

- Over-release check: `if (previous == 0) RADIANT_DEBUGBREAK();` in `DecRefCount` — programmer error, fail-fast under a debugger (see implementation note 2 for why not `RADIANT_ASSERT`).
- Tracking asserts in `Ref.cpp` (`instance` non-null, present-on-remove) stay, gated with the feature.
- No logging changes.

## 9. Scalability Review

The single thread-safe `RefCounted` scales as-is: one atomic RMW per inc/dec is the floor for shared ownership, and the type is already the engine-wide currency. Two consciously deferred items: (a) contended-refcount optimization (e.g. UE's deliberate avoidance of ordering on increment) is already matched; (b) weak references — deleting `WeakRef` now means re-designing properly (control block or generation handles) *when a consumer exists*, rather than maintaining speculative broken API. No O(N)-per-feature patterns introduced.

## 10. Risks & Edge Cases

- **Underflow masking:** with asserts off in Dist, an over-release silently corrupts. Mitigated by asserts-on-in-Release (RAD-9) catching it during all real iteration.
- **Tracking asymmetry preserved:** `AddToLiveReferences` fires on every `IncRef` (set dedupes), removal only on final release — semantics unchanged, just Debug-gated; behavior identical within Debug.
- **Hidden `WeakRef`/`CopyWithoutIncrement` usage:** grep says none exist; compiler confirms at delete time.
- **Move constructor doesn't touch tracking** (pre-existing): correct — a move transfers the reference without changing the count; noted here so nobody "fixes" it.

## 11. Verification (AC → proof)

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| Release decision is a single atomic op | Code review of `DecRefCount`: one `fetch_sub`, decision on its return value, no second load |
| No double-delete window remains | Reasoning review in walkthrough (interleaving argument); idiom matches UE `ImmediatelyRelease` |
| Tracking compiled out of non-Debug | Build Release/Dist; verify `s_LiveReferences` symbols absent (map file / linker), no mutex acquisition in `IncRef` disassembly if desired |
| `IsLive` is race-free | Superseded: `IsLive` deleted with `WeakRef` (no consumer remained); remaining tracking functions all take the lock |
| Engine still works | Reaper full loop (menu → gameplay → pause → menu → quit) in Debug and Release, no asserts, no crashes |
| All configs build clean | Build Debug + Release + Dist, zero new warnings |
