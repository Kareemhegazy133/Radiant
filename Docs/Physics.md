# Physics

**Status:** Per-Level worlds on **Box2D v3.1.1** landed 2026-07-09 (RAD-27; the v3 upgrade was decided in RAD-60). Fixed timestep + post-step transform retrieval landed 2026-07-05 (RAD-25). Remaining Phase 2 rework: sync semantics (RAD-28) and collision events (RAD-29).

## The Problem This Solves

Making things fall, slide, bounce, and never sink through the floor is a numerical-simulation problem — genuinely hard, thoroughly solved, and not this project's learning goal — so Radiant delegates the math to **Box2D** and keeps its own learning budget for architecture.

What *is* our problem: the engine holds **two descriptions of the same world**. The ECS has a transform per entity ("draw the crate here"); Box2D has a body per physical object ("physics says the crate is here"). The physics subsystem is really the discipline of keeping those two truths in sync without them fighting — deciding who owns an entity's position (physics, for anything dynamic), when each side reads the other, and how "these two things touched" gets reported back to gameplay. Nearly every defect in this subsystem, current or historical, is a two-sources-of-truth failure: pushing stale transforms into the simulation, reading results back in the wrong place, or telling the simulation to rebuild things it was still using.

A second, subtler problem is **ownership**: a Box2D world is *world state*, and it must be owned like state, not like a global service. Until RAD-27 the engine had one process-wide static world that every Level overwrote on construction and destroyed on destruction — which capped the engine at one Level and made level switching a four-stage corruption (leak, cross-world body destruction, deletion of the successor's world, null-deref). Play-in-editor requires two coexisting Levels, which that design could never provide.

## Architecture (current)

Radiant integrates **Box2D v3.1.1** (submodule: Kareem's fork of `erincatto/box2d`, branch `radiant/v3.1.1` — the upstream tag plus our premake script; see [Build-System](Build-System.md)) behind `Physics/PhysicsWorld2D.{h,cpp}` — an instance class owning one world. **Each Level owns one:**

```cpp
// Level.h — created in the constructor (live levels only), destroyed with the level
Scope<PhysicsWorld2D> m_PhysicsWorld;
```

Worlds are fully independent simulations — bodies in one world never interact with another's — so any number of Levels coexist (verified: two worlds simulating simultaneously, plus scratch-level create/destroy leaving the live world untouched). Scratch levels (`initialize == false`) hold a null Scope: no world, no signals, and their destruction touches nothing global. In `~Level`, the entity loop in the destructor *body* destroys bodies via signals while the member world is still alive; the Scope destroys the world itself afterwards.

The dependency points one way — `Level → PhysicsWorld2D → Box2D` — with no back-pointer: v2's contact listener needed a `Level*` to resolve entities mid-step, but v3 reports contacts via post-step event buffers that the *Level* will drain (RAD-29), so the world never calls up. All Box2D includes stay inside `Physics/`; `Level.h` only forward-declares the class. UE's parallel: every `UWorld` owns its `FPhysScene` member (`Engine/Source/Runtime/Engine/Classes/Engine/World.h:1567`) — PIE works there for exactly this reason.

### Handles, not pointers

v3 hands out **generation handles**: `b2WorldId`/`b2BodyId` are slot+serial tickets, checkable with `b2World_IsValid`/`b2Body_IsValid`, never dangling silently — the same idiom as `TimerHandle` (playbook §2). The component stores its body ticket packed into an integer:

```cpp
// RigidBody2DComponent — packed b2BodyId (b2StoreBodyId/b2LoadBodyId), 0 = no body
uint64_t RuntimeBodyId = 0;
```

Packing keeps `Components.h` free of vendor includes and makes `RigidBody2DComponent` fully plain data (playbook §3). The ticket is a *claim check*, not ownership — the world owns bodies and zeroes the field on destroy. `Level::Copy` (Phase 5) must zero copied ids and recreate bodies: v3 ids embed their world index, so a shallow-copied id would silently address the original world's body.

### Body & shape lifecycle

Driven by entt signals, unchanged in pattern (see [ECS-And-Levels](ECS-And-Levels.md)) — component presence IS the physics binding:

- `on_construct<RigidBody2DComponent>` → `PhysicsWorld2D::CreateBody`: def from the transform (v3 rotations are cos/sin pairs — `b2MakeRot(Rotation.z)`; `fixedRotation` rides in the def), entity UUID stamped into `userData` (RAD-29 resolves contact events through it), packed id stored in the component.
- `on_construct<BoxCollider2DComponent>` → `CreateBoxShape`: `b2MakeOffsetBox` with half-extents × transform scale; friction/restitution live in v3.1's `b2SurfaceMaterial` (`shapeDef.material.*`), density stays top-level. The restitution *threshold* is world-level in v3 (we take the default; `b2World_SetRestitutionThreshold` exists if a Level ever needs tuning) — the old per-collider field is gone, and stale keys in old `.rdlvl` files are ignored on load.
- `on_destroy<RigidBody2DComponent>` → `DestroyBody`: validity-checked; a stale ticket is a warned no-op where v2's pointer would have been undefined behavior.

Signal handlers assert on a null world (programmer error — scratch levels never connect signals) and recover in Dist.

### Per-step flow

Physics advances once per FIXED simulation step (`Level::OnFixedUpdate`, accumulator-driven — see [Time-And-Simulation](Time-And-Simulation.md)), never per rendered frame:

```text
Level::OnFixedUpdate (0..N times per frame, fixed delta)
 ├─ snapshot pass: movable entities' transforms → TransformSnapshotComponent
 ├─ native scripts
 ├─ for each active rigidbody entity: SubmitTransform
 │    ECS transform → b2Body_SetTransform, then DESTROY + RECREATE the shape   ← defect (RAD-28)
 ├─ PhysicsWorld2D::Step: b2World_Step(world, fixedDelta, 4 sub-steps)
 └─ retrieve pass: every active body's position/rotation → ECS transform (RetrieveTransform)
Level::OnRender(alpha) — draw-only: interpolates via snapshots, never mutates simulation
```

v3's sub-step count (4, the library default) replaces v2's `(6, 2)` velocity/position iterations. The retrieve pass covers **every** active body, camera or no camera (landed with RAD-25).

### Collision reporting (deliberate gap until RAD-29)

v3 deleted the listener-callback API outright: contacts are recorded into **event buffers** drained after the step (`b2World_GetContactEvents`) — natively the record-then-dispatch design RAD-29 was scoped to build by hand. The v2 `CollisionListener2D` and the per-component `std::function` callbacks died with the port; nothing in the codebase consumed them (verified by grep). Until RAD-29 lands, gameplay has **no collision notifications** — stated here so nobody builds on the gap. Two facts RAD-29 inherits: the UUID in every body's `userData`, and `shapeDef.enableContactEvents` being **false by default** in v3.1 — the drain receives nothing unless shapes opt in.

### Diagnostics

`b2SetAssertFcn` routes Box2D's internal asserts into Radiant's log-then-break machinery (installed once, at first world creation). World create/destroy is TRACE-logged with the owning Level's name — the paired-lifetime evidence for RAD-27's AC. The Box2D vendor project keeps `B2_ASSERT` active in Debug and Release (`B2_ENABLE_ASSERT`) and strips it in Dist (`NDEBUG`), matching the engine's assert convention.

## Design Rationale (the Phase 2 contract, playbook §4)

- **One world per Level, owned by the Level — landed (RAD-27).** Physics is world state; a process singleton meant one Level maximum and corruption on Level churn. Rejected alternative: ref-counting the singleton's Init/Shutdown — patches the leak, still forbids coexistence.
- **Box2D v3, not 2.4 — landed (RAD-60 decision, RAD-27 execution).** v3 *is* the target architecture: id handles legal in plain-data components, per-Level worlds as values, event-buffer contacts. Migrating during the seam rebuild paid for the port once.
- **Fixed timestep — landed (RAD-25).** Simulation steps at a fixed rate from an accumulator; rendering interpolates.
- **Physics owns dynamic transforms — half landed.** The retrieve half is a dedicated post-step pass (RAD-25). Still owed to RAD-28: push ECS→Box2D only on explicit teleport/spawn/property change, rebuild shapes only when collider properties change.
- **Collision events are queued, not called back — RAD-29.** v3 enforces the queue shape at the API level; RAD-29 adds entity-validity checks and decides where gameplay callbacks live (script hooks / Level delegate — never `std::function`s on components).

## Known Issues & Evolution

- **Per-step transform push + shape rebuild (RAD-28)** — ported 1:1 by design (a migration is reviewable only with frozen behavior): every step teleports every body (`b2Body_SetTransform` — v3's docs call it "fairly expensive") and destroys/recreates its shape, wrecking contact persistence, sleeping, and warm-starting. Includes a pre-existing quirk: the box shape's local rotation repeats the body's world rotation, so a rotated entity's collider turns twice — invisible while everything spawns at rotation 0.
- **No collision notifications (RAD-29)** — see the gap note above.
- **Resolved 2026-07-09 (RAD-27):** process-singleton world (leak + cross-world corruption + scratch-level teardown landmine) and dangling `b2Body*` runtime pointers (now checkable generation handles).
- **Resolved 2026-07-05 (RAD-25):** variable timestep; transform retrieval coupled to rendering.