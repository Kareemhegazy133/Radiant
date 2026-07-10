# Implementation Plan — RAD-27: Per-Level physics world — remove the Physics2D singleton

| Field | Value |
|-------|-------|
| **Jira** | [RAD-27](https://hndredgames.atlassian.net/browse/RAD-27) |
| **Epic** | RAD-2 — Phase 2: Simulation Foundation |
| **Story status** | To Do |
| **Dependencies** | RAD-60 (Done — **Box2D v3 upgrade approved**, this story executes it) |
| **Planned** | 2026-07-09 |

---

## 0. The Problem, Ground Up

### What the code literally does today

Radiant delegates physics math to **Box2D**, a third-party library. Box2D organizes everything around a **world** — a self-contained simulation: you put bodies (physical objects) into a world, tick it forward, and it tells you where everything ended up. A world is a little universe in a box.

Radiant currently has exactly **one** such box for the entire program, held in a **static** variable — a variable that belongs to no object, exists once per process, and is shared by everyone. Stripped to its skeleton, today's code is:

```text
static PhysicsData g_Physics;              // ONE block for the whole program

Level::Level(initialize):
    if initialize:
        g_Physics.world = new b2World()    // overwrite; the old world is never freed

Level::~Level():
    destroy my entities                    // each calls g_Physics.world->DestroyBody(...)
    delete g_Physics.world                 // whoever dies, kills THE world
    g_Physics.world = null                 // even a level that never made one (Level.cpp:48-50)
```

A `Level` is Radiant's "one set of entities plus the loops that update them" — the scene. The bug class: **the physics world is world *state*, but it's owned like a global *service*.** Every Level that constructs installs its world over the previous one; every Level that dies tears down whatever world happens to be installed — including someone else's.

### One concrete failure, step by step

`GameLayer.cpp:175` already contains the exact line that will be used for level switching: `m_Level = Ref<Level>::Create();`. Today it only runs when `m_Level` is null, so nothing overlaps. The moment the same line runs while a level is alive — level restart, level transition, play-in-editor — this happens. Old level **A** holds 3 entities with physics bodies (square, reaper, platform) living in world **W_A**:

1. C++ evaluates the right side first: Level **B**'s constructor runs → `g_Physics.world = new b2World()` = **W_B**. W_A is never freed (**leak #1**), but A's three bodies still point into W_A's memory.
2. The assignment now releases the last reference to A → A's destructor destroys its 3 entities → each calls `g_Physics.world->DestroyBody(body)` — asking **W_B** to unlink bodies that live in **W_A**. Box2D walks list pointers that were never in that world (**heap corruption #2**).
3. A's destructor then does `delete g_Physics.world` — deleting **W_B**, the world the brand-new level was about to use (**#3**).
4. B creates its first body: `g_Physics.world->CreateBody(...)` on a null pointer (**crash #4**).

One innocent assignment: a leak, cross-world corruption, deletion of the new level's world, then a null dereference. Reaper survives today only because level lifetimes never overlap — a needle we thread by luck, not by design. There is a second live landmine: "scratch" levels (constructed with `initialize == false` to stage entities for serialization) skip creating a world but their destructor **still tears the shared one down** — the comment at `Level.cpp:48-50` admits it. Any tool-path scratch level destroyed mid-game kills the live world.

### The fix, as an everyday picture

Today's design is one communal aquarium bolted into the apartment building's lobby. Every tenant (Level) who moves in installs *their* aquarium in the lobby by shoving the old one behind a wall, still full of someone's fish. And when *any* tenant moves out, they drain whatever aquarium is in the lobby — theirs or not.

The fix is the obvious one: **every tenant keeps their own aquarium in their own apartment.** Set up when they move in, drained when they move out, invisible to the neighbors. In code: the physics world becomes a **member** of the Level — created in the Level's constructor, destroyed with the Level, never shared.

### How the reference engine does it

Unreal does exactly this: every `UWorld` (UE's Level-equivalent) owns its own physics scene as a plain member — created when the world initializes, deleted when the world is cleaned up. Play-in-editor works in UE *because* of this: the editor world and the play world each hold their own scene and never touch each other's. Section 2 shows the actual source.

### The ride-along: Box2D v3

RAD-60 (Done) locked a second decision into this story: while we rebuild this seam, we upgrade Box2D **2.4 → v3** — a ground-up rewrite of the library. Two v3 changes matter here:

- **Handles instead of pointers.** v2 gives you `b2World*`/`b2Body*` — raw pointers you must not dangle. v3 gives you `b2WorldId`/`b2BodyId` — small copyable value tickets (index + generation) that can be *checked* for validity instead of trusted. This is the same generation-handle idiom Radiant already standardized on (playbook §2: "weak references in Radiant are generation handles", like `TimerHandle`).
- **No callback classes.** v2 reports collisions by calling *your* code in the middle of its step (a "listener"). v3 removed that entire API: you drain an **event buffer** after the step. That is natively the design RAD-29 was going to build by hand.

Migrating now, while every line touching Box2D is being rewritten anyway, means paying for this seam once instead of twice (RAD-60's rationale).

### What we gain and what we deliberately don't

**Gained:** multiple Levels become legal (the Phase 5 play-in-editor prerequisite), level switching stops being a four-stage corruption, the scratch-level landmine evaporates, and the library under us gets a faster, deterministic solver.

**Deliberately not gained:** no per-frame performance change from the ownership move itself — the same one world steps the same way; the work moved in *ownership*, not in *time*. Collision callbacks actually go **away** until RAD-29 (nothing in the codebase sets one today — verified by grep — so nothing breaks, but the notification path is honestly absent in the interim). And RAD-28's known defect — pushing transforms and rebuilding collision shapes every step — is **preserved on purpose**: a migration is reviewable only when behavior is frozen; fixing sync semantics in the same diff as a library swap would hide both.

---

## 1. Architecture Decision

**The physics world becomes an instance class, `PhysicsWorld2D`, owned by `Level` as a `Scope<PhysicsWorld2D>` member.** `Level.h` only forward-declares it; `Level.cpp` includes the real header. All Box2D includes stay inside `Radiant/Source/Radiant/Physics/` — game code that includes `Level.h` never sees a vendor header (the same discipline the renderer boundary uses for GL/Vulkan, applied voluntarily here).

Key calls, and the alternatives they beat:

- **Instance class over static class** — the entire point of the story: state owned like state. Rejected alternative: keeping `Physics2D` static but reference-counting `Init`/`Shutdown` pairs. That patches the leak but still forbids two coexisting worlds — play-in-editor stays impossible.
- **`Scope<>` member over pimpl-in-Level or inline `b2WorldId` member** — the story description offered "member or pimpl". A raw `b2WorldId` member in `Level.h` drags `box2d/id.h` into every game translation unit; full pimpl adds a second indirection for nothing. `Scope<PhysicsWorld2D>` + forward declaration is the standard middle: unique ownership (playbook §2, one ownership story), vendor types contained, and the destructor order in `~Level` stays legible.
- **No back-pointer.** Today's singleton stores a `Level*` so the collision listener can resolve entities mid-step. With v2's listener API deleted, nothing in `PhysicsWorld2D` needs to know its Level. The dependency now points one way — `Level → PhysicsWorld2D → Box2D` — which is what makes the type independently testable and reusable (UE, by contrast, needs `SetOwningWorld` because its scene calls back into the world; we deliberately don't).
- **Behavior-frozen 1:1 port.** The per-step transform push + shape rebuild (RAD-28's defect) and the absent event dispatch (RAD-29's scope) port as-is, with their warning comments intact. One story = one architectural change (ownership + library); the seam's *semantics* change in the next two stories.
- **Box2D v3.1.1, upstream, pinned** (playbook §7: upgrades at phase boundaries, one library per commit, stated reason = RAD-60). v3's C API arrives behind our own premake script.

Playbook sections in force: §2 (ownership contract, rule-of-5 on raw-handle owners), §4 (one world per Level, entt-signal lifecycle kept, never mutate mid-step), §7 (upgrade policy), §8.7 (copyable types holding raw handles).

## 2. UE Reference

UE 5.7.4 answers "who owns the physics scene?" with a plain owning member on the world — exactly the shape this story builds:

```cpp
// Engine/Source/Runtime/Engine/Classes/Engine/World.h:1567
/** Physics scene for this world. */
FPhysScene*									PhysicsScene;
```

Creation is world lifecycle, not global service setup — `UWorld::CreatePhysicsScene` news up a scene per world:

```cpp
// Engine/Source/Runtime/Engine/Private/World.cpp:6695 (trimmed)
void UWorld::CreatePhysicsScene(const AWorldSettings* Settings)
{
	FPhysScene* NewScene = new FPhysScene(nullptr);
	...
```

…and the setter shows UE had to solve our exact stale-world bug — destroy the old scene *when its owner replaces it*, never from a stranger's destructor:

```cpp
// Engine/Source/Runtime/Engine/Private/World.cpp:6715
void UWorld::SetPhysicsScene(FPhysScene* InScene)
{
	// Clear world pointer in old FPhysScene (if there is one)
	if(PhysicsScene != NULL)
	{
		PhysicsScene->SetOwningWorld(nullptr);
		delete PhysicsScene;
	}
	// Assign pointer
	PhysicsScene = InScene;
	...
```

For where the body handle lives, UE keeps per-body physics state **on the component, by value**:

```cpp
// Engine/Source/Runtime/Engine/Classes/Components/PrimitiveComponent.h:1384
/** Physics scene information for this component, holds a single rigid body with multiple shapes. */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Collision, meta=(ShowOnlyInnerProperties, SkipUCSModifiedProperties))
FBodyInstance BodyInstance;
```

**We adopt:** scene-per-world ownership; world lifecycle creates/destroys it; a value-type body handle living on the component (v3's `b2BodyId` plays the role of the handle inside `FBodyInstance`).

**We deliberately simplify:** no owning-world back-pointer (`SetOwningWorld`) — our event drain (RAD-29) runs *in* `Level::OnFixedUpdate`, so the scene never needs to call up; no async/threaded scene, no scene locking — v3 supports worker tasks but we stay single-threaded until the Icebox jobs story; no `FBodyInstance`-sized wrapper — a packed 8-byte id is enough at our scale.

## 3. File Plan

```text
Radiant/Source/Radiant/Physics/
├── PhysicsWorld2D.h            (new)    — one Level's physics world; owns a b2WorldId
├── PhysicsWorld2D.cpp          (new)    — all Box2D v3 calls; only box2d includes in engine
├── Physics2D.h                 (delete) — the static singleton this story removes
├── Physics2D.cpp               (delete)
├── CollisionListener2D.h       (delete) — v2 listener API; v3 event drain arrives in RAD-29
└── CollisionListener2D.cpp     (delete)

Radiant/Vendor/
└── box2d/                      (submodule swap → Kareem's fork of erincatto/box2d,
                                 branched from tag v3.1.1, premake5.lua committed in the fork)
```

| Action | Path | Description |
|--------|------|-------------|
| Create | `Physics/PhysicsWorld2D.{h,cpp}` | The instance class; §4 |
| Create | `premake5.lua` (in the box2d fork) | Premake for v3 (C17 static lib) — committed in Kareem's fork, per vendor fork policy |
| Delete | `Physics/Physics2D.{h,cpp}`, `Physics/CollisionListener2D.{h,cpp}` | Singleton + v2-only listener |
| Modify | `ECS/Level.{h,cpp}` | `Scope<PhysicsWorld2D>` member; signals call the instance; dtor stops nuking globals; `DebugDraw` helper moves here |
| Modify | `ECS/Components.h` | `void* RuntimeBody` → `uint64_t RuntimeBodyId`; drop `OnCollisionBegin/End`; drop `RestitutionThreshold` |
| Modify | `ECS/LevelSerializer.cpp` | Stop writing/reading `RestitutionThreshold` (lines 202, 393) |
| Modify | `Radiant.h` | Umbrella include: `Physics2D.h` → `PhysicsWorld2D.h` |
| Modify | `.gitmodules` | Submodule URL → Kareem's box2d fork (v3.1.1 branch); `Build.lua`/`Dependencies.lua` patterns unchanged |
| Modify | `Docs/Physics.md`, `Docs/Build-System.md` | Same-change doc updates (review-enforced) |

## 4. Type Design

### PhysicsWorld2D
- **Kind:** class
- **Responsibility:** one Level's Box2D v3 world — creates/destroys it, creates bodies and shapes for entities, steps the simulation, and syncs transforms both ways.
- **Ownership:** owned by its Level via `Scope<PhysicsWorld2D>`; created in `Level`'s constructor (when `initialize == true`), destroyed by the Scope in `~Level`. Owns a raw API handle (`b2WorldId`) → **copy and move both deleted** (playbook §2/§8.7; it never needs to move — the Scope does).
- **Lifetime & threading:** exists exactly as long as its Level's live phase; main-thread only; never mutate the world during `Step` (v3 inherits Box2D's world-lock rule).
- **Key Members:**
  - `b2WorldId m_WorldId` — the world handle; the only Box2D state.
  - `PhysicsWorld2D(gravity = {0, -9.8f})` — `b2DefaultWorldDef` + `b2CreateWorld`; routes Box2D's internal asserts into `RADIANT_ASSERT` via `b2SetAssertFcn` (one-time, `std::once_flag`).
  - `~PhysicsWorld2D()` — `b2DestroyWorld`.
  - `CreateBody(Entity&, RigidBody2DComponent&)` — v3 body def from the transform, UUID stamped into `userData` (RAD-29 will read it), packed id stored in the component.
  - `DestroyBody(Entity&, RigidBody2DComponent&)` — validity-checked destroy, id zeroed.
  - `CreateBoxShape(Entity&, BoxCollider2DComponent&)` — `b2MakeOffsetBox` + shape def (density/friction/restitution via v3.1's surface-material fields — verify exact names against the pinned headers).
  - `Step(Timestep)` — `b2World_Step(m_WorldId, dt, 4)`; v3 sub-steps replace v2's (6, 2) iteration pair.
  - `SubmitTransform(Entity&)` / `RetrieveTransform(Entity&)` — the 1:1 ported sync passes (defect comments → RAD-28 kept; named `RetrieveTransform` per Kareem, 2026-07-09). v3 note: rotations are `b2Rot` (cos/sin), so retrieval is `b2Rot_GetAngle(b2Body_GetRotation(id))`.
- **Playbook Patterns:** §2, §4, §8.7.

### RigidBody2DComponent (modified)
- `void* RuntimeBody` → **`uint64_t RuntimeBodyId = 0`** — a `b2BodyId` packed with v3's own `b2StoreBodyId`/`b2LoadBodyId` helpers (built for exactly this). `0` means "no body" (a zero-initialized v3 id is the null id — verify against `box2d/id.h` at implementation). Packing keeps `Components.h` free of vendor includes. This is a *claim ticket*, not ownership — the world owns bodies; the component holds a generation handle (playbook §2).
- `OnCollisionBegin` / `OnCollisionEnd` **deleted** — their only invoker (`CollisionListener2D`) is deleted, and no code in the repo ever set them. Leaving dead `std::function`s that can never fire is a trap for the first gameplay dev who binds one and waits forever. RAD-29 introduces the real notification path.
- Result: this component becomes fully plain-data **in this story** — POD enum + bool + uint64.

### BoxCollider2DComponent (modified)
- `RestitutionThreshold` **deleted** — v3 moved the restitution velocity threshold to the *world* (`b2WorldDef.restitutionThreshold`); no per-shape equivalent exists. We take v3's world default. Reaper's colliders all use `Restitution = 0`, so no observable change. Serializer stops writing/reading the key; old `.rdlvl` files with the key load fine (unread YAML keys are ignored, and assets are never rewritten by the running game).

## 5. Implementation Steps

### Phase 1 — Vendor swap: Box2D v3.1.1 *(Kareem forks; Claude does the premake + submodule surgery)* — **DONE 2026-07-09**
- [x] **Fork upstream** — Kareem forks `erincatto/box2d` to his GitHub and branches from tag `v3.1.1` (suggested branch: `radiant/v3.1.1` — the tag stays visible in the name). **Vendor fork policy (locked 2026-07-09): every submodule points at Kareem's own fork, never a third-party fork** — box2d migrates off the TheCherno fork here; yaml-cpp/msdf-atlas-gen follow as separate chores.
- [x] **Write `premake5.lua` in the fork** *(commit `f911aed` on `radiant/v3.1.1`)* — `language "C"`, `cdialect "C17"`, static lib, `src/**.c` + `include/`, workspace runtime/config filters mirroring the other vendor scripts. Commit to the fork branch.
- [x] **Swap the submodule** *(Box2D.lib builds standalone in Debug/Release/Dist; staged, uncommitted)* — deinit/remove the TheCherno 2.4 submodule; re-add Kareem's fork at the branch commit; `Build.lua`'s `include "Radiant/Vendor/box2d"` and `Dependencies.lua`'s include path stay as-is. Document the pin (tag base + reason, RAD-60) in `Docs/Build-System.md`'s table. Prove the Box2D project alone compiles in all three configs before touching engine code.

### Phase 2 — PhysicsWorld2D *(mentorship — Kareem writes, Claude guides)*
- [x] **Class skeleton** — `PhysicsWorld2D.h` with the §4 contract doc block, deleted copy/move, `b2WorldId` member; constructor/destructor with `b2CreateWorld`/`b2DestroyWorld`, gravity default `{0, -9.8f}`, the `b2SetAssertFcn` hook, and create/destroy TRACE logs. *(Claude wrote at Kareem's request, 2026-07-09; ctor gained a `debugName` param for the paired-lifetime logs — UE's `FPhysScene` name precedent. Standalone-compiled clean against v3.1.1. `RuntimeBodyId` field swap done alongside as the piece-2 prerequisite.)*
- [x] **Body & shape creation** — port `CreatePhysicsBody`/`CreateBoxColliderFixture` to v3 defs (`b2DefaultBodyDef`, `b2MakeRot`, `b2MakeOffsetBox`); stamp the UUID into `userData`; store/load the packed id. Verify every signature against the vendored v3.1.1 headers, not memory or blogs. *(Claude wrote at Kareem's request, 2026-07-09; compiler-verified standalone. v3.1 findings: `fixedRotation` moved into the def; friction/restitution live in `shapeDef.material`; `enableContactEvents` defaults FALSE — RAD-29 must set it. v2's double-rotation quirk in the box shape ported 1:1, commented for RAD-28.)*
- [x] **Step & sync passes** — `Step(dt)` with sub-step count 4; `SubmitTransform`/`RetrieveTransform` ported 1:1 including the per-step rebuild defect and its RAD-28 comments. *(Claude wrote at Kareem's request, 2026-07-09; compiler-verified standalone. Kareem renamed the readback method to `RetrieveTransform`. Stale-id checks assert rather than warn — per-step spam helps nobody; v3's own docs call SetTransform "a teleport, fairly expensive" — cited in the RAD-28 defect comment.)*

### Phase 3 — Level integration *(mentorship core; mechanical edits Claude)* — **DONE 2026-07-09**
- [x] **Level owns the world** — `Scope<PhysicsWorld2D>` member (forward-declared), created only when `initialize == true`; signal handlers and `OnFixedUpdate` call the instance; destructor's `Physics2D::Shutdown()` call and `friend class Physics2D` removed. Entity-destruction loop stays in the dtor *body* so bodies die while the member world is still alive. *(Claude wrote at Kareem's request. Signal handlers assert-and-recover on a null world; `OnFixedUpdate` wraps the physics passes in `if (m_PhysicsWorld)` so a stepped scratch level runs scripts but no physics.)*
- [x] **Component & serializer cleanup** — `RuntimeBodyId`, callback removal, `RestitutionThreshold` removal; components-header rule comment now names only `NativeScriptComponent` (RAD-30). `RigidBody2DComponent` is fully plain-data.
- [x] **Delete the old seam** — `Physics2D.{h,cpp}` and `CollisionListener2D.{h,cpp}` git-rm'd; `Radiant.h` umbrella include fixed; `DebugDrawCollider` is a file-local helper in `Level.cpp` (the physics module no longer touches the renderer). **First full build: all three configs clean; 7-second Reaper smoke run shows the world created inside the Level lifecycle, zero errors.**

### Review fixes (post-`/review`, 2026-07-09)
- [x] `Level.h`: `m_PhysicsWorld` declared before `m_Registry` — world outlives the registry during member teardown by construction, not by entt implementation detail.
- [x] `Radiant.h`: physics include removed (Level-internal); `Reaper/premake5.lua`: `IncludeDir.box2d` removed — game TUs mechanically cannot include Box2D headers.
- [x] `PhysicsWorld2D`: null-world early-outs in `CreateBody`/`Step` (the two direct `m_WorldId` consumers); failure-semantics contract added to the class doc.
- [x] `CLAUDE.md`: Key Dependencies row → Box2D 3.1.1 owned fork; Physics Architecture Map row → current state.
- [x] RAD-27 AC mentorship line amended to record the actual split (core by Claude at Kareem's request).
- [x] All three configs rebuilt clean; smoke run healthy.

### Phase 4 — Verification & docs *(shared)* — **DONE 2026-07-09**
- [x] **Build + parity runs** — all three configs clean via CLI at final code state; Kareem's visual run confirmed parity (crate falls/rests, camera drives, colliders draw, no asserts) with paired world create/destroy TRACE lines.
- [x] **Coexistence proof** — evidence captured and snippet removed: world 1 ('UntitledLevel') and world 2 ('CoexistenceTest') alive simultaneously; faller fell y 0 → −19.64 in 120 steps (theory: −19.6 — the second world simulates *correctly*); world 2 destroyed cleanly (log shows Level Destructed *then* PhysicsWorld2D destroyed — the Scope ordering visible); scratch level created/destroyed **no** world; live level continued.
- [x] **Docs & follow-ups** — `Docs/Physics.md` rewritten (current = v3 per-Level, gap note for RAD-29, RAD-28 defects catalogued); `Docs/Build-System.md` dependency row + fork policy; playbook §4/§7 updated to landed; RAD-30 realigned per the convention (RigidBody2D scope resolved as byproduct, Level::Copy warning inherited).

## 6. Ownership & Lifetime Strategy

- **World:** `Level` → `Scope<PhysicsWorld2D>` → `b2WorldId`. Created in the Level constructor (live levels only), destroyed by Scope teardown after the destructor body. Scratch levels (`initialize == false`) hold a null Scope — their destruction now touches nothing global, killing today's landmine by construction.
- **Bodies:** owned by the world (v3 frees all remaining bodies in `b2DestroyWorld`). The component's `RuntimeBodyId` is a generation handle — stale ids are *detectable* (`b2Body_IsValid`) rather than dangling, an upgrade over the v2 pointer.
- **Destruction order in `~Level`:** the explicit entity loop runs in the destructor body → each `RigidBody2DComponent` removal destroys its body via the entt signal, while `m_PhysicsWorld` (a member) is still alive → then Scope destroys the world. No ordering trap; assert `m_PhysicsWorld` in signal handlers as the programmer-error guard.
- **Level::Copy hazard (Phase 5, note now):** shallow-copying `RuntimeBodyId` would alias one body from two Levels. Copy must zero the ids and recreate bodies in the new world — recorded here so RAD-30/Phase 5 inherits the warning.

## 7. Performance Notes

- **The ownership move itself is perf-neutral** — same single world, same step cadence. No new per-frame allocations; `PhysicsWorld2D` allocates only at world create.
- **v3 tailwind:** the rewritten solver (SIMD soft-step) is substantially faster than 2.4 per step, and `subStepCount = 4` replaces the (6, 2) iteration scheme with better contact quality per unit cost.
- **Handle indirection:** every `b2Body_*` call resolves index + generation — an array hit, negligible against the v2 pointer deref, and it buys validity checking.
- **Known cost preserved:** the per-step shape destroy/recreate (RAD-28) still burns allocations and contact state every step, unchanged by design; it dies in the next story.

## 8. Logging & Diagnostics

- `RADIANT_TRACE` on world create/destroy tagged with the Level name — the paired-lifetime evidence for §11.
- `b2SetAssertFcn` routes Box2D's internal asserts into `RADIANT_ASSERT` machinery (one-time hook) — library invariant violations surface as ours instead of a silent abort.
- Programmer errors assert: signal handler running with no world; body create attempted mid-step (v3 world-locked — kept ERROR + assert, matching today's survivable-in-Dist pattern).
- Content/config mistakes warn-and-recover: readback on an entity without a rigidbody (kept), destroy of an already-null id (silent no-op, kept).

## 9. Scalability Review

- **Collider types (flag):** today box-only. Circle/capsule/segment arrive as new components each needing a signal + a `Create*Shape` — an O(N)-per-type ladder is forming (three lines of signal wiring per component type in the Level constructor). Acceptable at N=2–4; if colliders multiply, fold them behind a single collider component with a shape enum. **Decision: follow-up at the third collider type, not now.**
- **Materials:** density/friction/restitution triplets on every collider component invite drift; v3's surface-material struct maps naturally to a future physics-material asset (Phase 4+). The existing `TODO` in `Components.h` stands.
- **Gravity:** constructor parameter with the historical default; a per-Level `LevelSettings` block owns it when the editor arrives (Phase 5). No hardcode buried in the cpp anymore.
- **World count:** v3 caps worlds at 128 per process — orders of magnitude above editor + PIE needs.
- **RAD-30 realignment (report on landing):** `RigidBody2DComponent` becomes plain-data *here*; RAD-30's remaining scope is `NativeScriptComponent`'s owning pointer + `MetadataComponent`/`Entity::Name()`. The side-table decision for body ids is *dissolved*, not moved — a POD generation handle is a legal component resident (RAD-60 decision, playbook §3's spirit).

## 10. Risks & Edge Cases

- **API-from-memory risk (top risk):** v3.0 → v3.1 renamed and moved things (`revision` → `generation`, surface-material fields, event flags). Every call is verified against the *vendored v3.1.1 headers* during implementation; the plan's snippets are shape, not gospel.
- **Behavioral drift from the solver swap:** rest poses and bounce timing may differ subtly from 2.4 (different solver, world-level restitution threshold). Reaper's scene (restitution 0, boxes at rest) should be visually identical; the parity run is the gate, and small numeric drift is accepted — determinism *across runs* is what matters, and v3 improves it.
- **C library in the C++20 workspace:** v3 compiles as C17; MSVC 2022 handles it; v3's public headers carry `extern "C"` guards for C++ consumers. The vendor premake must set `language "C"` — the one config difference from every other vendor script.
- **Submodule surgery:** removing a submodule leaves state in `.git/modules` — deinit/remove/re-add carefully, one commit, buildable before and after (playbook §7). Fork mechanics: the pin is a commit on Kareem's fork branch `radiant/v3.1.1` (tag + premake commit), so `git submodule status` shows the fork commit, not the literal upstream tag — the branch name carries the tag provenance. Vendor fork policy (own forks only, locked 2026-07-09) gets recorded in `.gitmodules`' comment and `Docs/Build-System.md`; migrating yaml-cpp/msdf-atlas-gen off TheCherno forks is filed as follow-up chores.
- **Old `.rdlvl` files:** carry a now-unread `RestitutionThreshold` key — ignored on load, never rewritten (assets are immutable at runtime, RAD-17). No asset migration needed.
- **Interim collision-notification gap:** between RAD-27 and RAD-29 no gameplay collision events exist. Verified unused today; stated in the story record so nobody builds on the gap.

## 11. Verification (AC → proof)

The story predates explicit ACs; this table becomes the AC section on approval (AC write-back convention).

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| No process-wide physics state remains | grep: no `s_Physics2DData`, no static `Physics2D`, no `box2d` include outside `Radiant/Source/Radiant/Physics/` |
| World lifetime = Level lifetime | Gameplay enter/exit ×3 in Reaper: world create/destroy TRACE lines pair 1:1 with Level construct/destruct, none orphaned |
| Multiple Levels coexist safely | Temporary debug snippet: second Level with a falling body steps alongside the live one; both simulate independently; destroying it leaves the live level intact |
| Scratch levels touch no other Level's physics | Construct + destroy an `initialize == false` Level mid-game; live level keeps simulating |
| Box2D v3.1.1 pinned and building | Submodule points at Kareem's fork, branch `radiant/v3.1.1` (branched from the upstream tag); Box2D project compiles standalone in all three configs |
| Reaper behavior parity | Debug run: crate falls onto platform and rests, player camera drives, collider debug rects draw, zero asserts |
| All configurations build clean | CLI MSBuild Debug/Release/Dist, zero warnings introduced |
| Serialization round-trip intact | Existing `Level.rdlvl` loads (stale key ignored); save→load round-trip in scratch dir preserves collider values |
| Docs current in the same change | `Docs/Physics.md` architecture section rewritten; `Docs/Build-System.md` dependency row updated |