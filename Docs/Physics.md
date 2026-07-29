# Physics

**Status:** Sync semantics landed 2026-07-10 (RAD-28): physics owns dynamic transforms, ECS→Box2D pushes are explicit verbs only, readback drains v3 move events, shapes are never rebuilt per step. Per-Level worlds on **Box2D v3.1.1** landed 2026-07-09 (RAD-27; the v3 upgrade was decided in RAD-60). Fixed timestep landed 2026-07-05 (RAD-25). Remaining Phase 2 rework: collision events (RAD-29).

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

v3 hands out **generation handles**: `b2WorldId`/`b2BodyId`/`b2ShapeId` are slot+serial tickets, checkable with `b2World_IsValid`/`b2Body_IsValid`/`b2Shape_IsValid`, never dangling silently — the same idiom as `TimerHandle` (playbook §2). The components store their tickets packed into integers:

```cpp
// RigidBody2DComponent — packed b2BodyId (b2StoreBodyId/b2LoadBodyId), 0 = no body
uint64_t RuntimeBodyId = 0;
// BoxCollider2DComponent — packed b2ShapeId, same pattern (RAD-28)
uint64_t RuntimeShapeId = 0;
```

Packing keeps `Components.h` free of vendor includes and keeps both components fully plain data (playbook §3). A ticket is a *claim check*, not ownership — the world owns bodies and shapes and zeroes the fields on destroy; when a body dies it takes its shapes with it, so `DestroyBody` also zeroes a surviving collider component's shape ticket. `Level::Copy` (Phase 5) must zero copied ids and recreate bodies: v3 ids embed their world index, so a shallow-copied id would silently address the original world's body.

### Body & shape lifecycle

Driven by entt signals, unchanged in pattern (see [ECS-And-Levels](ECS-And-Levels.md)) — component presence IS the physics binding, in both directions for both components (RAD-28 closed the collider's destroy half):

- `on_construct<RigidBody2DComponent>` → `PhysicsWorld2D::CreateBody`: def from the transform (v3 rotations are cos/sin pairs — `b2MakeRot(Rotation.z)`; `fixedRotation` rides in the def), entity UUID stamped into `userData` (the move-event drain and RAD-29's contact drain resolve entities through it), packed id stored in the component.
- `on_construct<BoxCollider2DComponent>` → `CreateBoxShape`: `b2MakeOffsetBox` with half-extents × transform scale and **identity local rotation** — the shape is body-local; the body already carries the world rotation (passing it here pre-RAD-28 doubled a rotated collider's rotation). Friction/restitution live in v3.1's `b2SurfaceMaterial` (`shapeDef.material.*`), density stays top-level; the restitution *threshold* is world-level in v3 (we take the default). Packed shape id stored in the component.
- `on_destroy<RigidBody2DComponent>` → `DestroyBody`; `on_destroy<BoxCollider2DComponent>` → `DestroyBoxShape`: validity-checked; a stale ticket is a warned no-op where v2's pointer would have been undefined behavior. `Level::DestroyEntity` removes the collider **before** the rigidbody — a body destroy takes its shapes with it, so body-first would hand the collider handler a stale ticket.

Signal handlers assert on a null world (programmer error — scratch levels never connect signals) and recover in Dist.

### Transform ownership & the explicit verbs (RAD-28)

**Physics owns the transform of dynamic bodies.** Nothing pushes ECS transforms into Box2D implicitly — writing `TransformComponent` on a body-owning entity has no physical effect. ECS→Box2D writes exist in exactly two forms, both explicit, both funneled through `Level` (the only type owning all the touched state — registry, render snapshot, physics world):

- **`Level::Teleport(entity, translation[, rotationZ])`** (+ zero-logic `Entity::Teleport` forwarders): writes the ECS transform, stamps the `TransformSnapshotComponent` to the destination (so the jump renders instantly instead of smearing through interpolation), and — if the entity has a body — `b2Body_SetTransform` + `b2Body_SetAwake(true)`. The wake matters: SetTransform alone leaves a *sleeping* body teleported into mid-air hanging there until touched (UE's `SetBodyTransform` defaults `bAutoWake` for the same reason). Velocity is kept — UE's `TeleportPhysics` semantics. Works on body-less entities too (ECS + snapshot only): it is *the* universal discontinuous-move verb.
- **`Level::RefreshCollider(entity)`** → `UpdateBoxShape`: re-applies collider fields + transform scale to the **existing** shape in place — `b2Shape_SetPolygon` + density/friction/restitution setters + one `b2Body_ApplyMassFromShapes` (v3's setters deliberately leave mass untouched). The shape id never changes; contacts survive a refresh. Changing collider fields or `Scale` without calling this leaves the physics shape stale *by design* — implicit change detection is the antipattern RAD-28 killed; the debug collider draw reads ECS data, so a stale shape is visible as outline/behavior mismatch.

Both verbs TRACE — the log is the audit trail of every explicit push.

### Per-step flow

Physics advances once per FIXED simulation step (`Level::OnFixedUpdate`, accumulator-driven — see [Time-And-Simulation](Time-And-Simulation.md)), never per rendered frame:

```text
Level::OnFixedUpdate (0..N times per frame, fixed delta)
 ├─ snapshot pass: movable entities' transforms → TransformSnapshotComponent
 ├─ native scripts (may Teleport / RefreshCollider — the only ECS→Box2D writes)
 ├─ PhysicsWorld2D::Step
 │    ├─ b2World_Step(world, fixedDelta, 4 sub-steps)
 │    └─ drain b2World_GetBodyEvents → m_MoveEvents (engine-typed copies)
 └─ move-event drain: for each BodyMoveEvent, UUID → entity → Translation.xy/Rotation.z
Level::OnRender(alpha) — draw-only: interpolates via snapshots, never mutates simulation
```

v3's sub-step count (4, the library default) replaces v2's `(6, 2)` velocity/position iterations.

**Readback drains move events, not entities (RAD-28):** Box2D reports *only the bodies that moved* — the same "for each dirty proxy" shape as UE Chaos's `PullPhysicsStateForEachDirtyProxy_External` and PhysX's `PxActiveTransforms`. Cost scales with **activity, not population**: a settled level drains an empty array; sleeping and static bodies cost nothing. Mechanics: the event array is transient, so `Step` copies it immediately into a reusable buffer of engine-typed `BodyMoveEvent` PODs (`{UUID, position, rotation}` — Box2D types never leave `Physics/`); the UUID rides in each body's `userData`. `Level` resolves UUIDs via its entity map — nothing destroys entities between the step and the drain (scripts run *before* the step), so a lookup miss is an asserted invariant break, not a survivable case. Bodies falling asleep TRACE — the visible proof that contact persistence works. The drain is camera-blind and sprite-blind by construction, and its shape is a deliberate rehearsal of RAD-29's contact-event drain.

### Collision reporting (deliberate gap until RAD-29)

v3 deleted the listener-callback API outright: contacts are recorded into **event buffers** drained after the step (`b2World_GetContactEvents`) — natively the record-then-dispatch design RAD-29 was scoped to build by hand. The v2 `CollisionListener2D` and the per-component `std::function` callbacks died with the port; nothing in the codebase consumed them (verified by grep). Until RAD-29 lands, gameplay has **no collision notifications** — stated here so nobody builds on the gap. Two facts RAD-29 inherits: the UUID in every body's `userData`, and `shapeDef.enableContactEvents` being **false by default** in v3.1 — the drain receives nothing unless shapes opt in.

### Diagnostics

`b2SetAssertFcn` routes Box2D's internal asserts into Radiant's log-then-break machinery (installed once, at first world creation). World create/destroy is TRACE-logged with the owning Level's name — the paired-lifetime evidence for RAD-27's AC. The Box2D vendor project keeps `B2_ASSERT` active in Debug and Release (`B2_ENABLE_ASSERT`) and strips it in Dist (`NDEBUG`), matching the engine's assert convention.

## Design Rationale (the Phase 2 contract, playbook §4)

- **One world per Level, owned by the Level — landed (RAD-27).** Physics is world state; a process singleton meant one Level maximum and corruption on Level churn. Rejected alternative: ref-counting the singleton's Init/Shutdown — patches the leak, still forbids coexistence.
- **Box2D v3, not 2.4 — landed (RAD-60 decision, RAD-27 execution).** v3 *is* the target architecture: id handles legal in plain-data components, per-Level worlds as values, event-buffer contacts. Migrating during the seam rebuild paid for the port once.
- **Fixed timestep — landed (RAD-25).** Simulation steps at a fixed rate from an accumulator; rendering interpolates.
- **Physics owns dynamic transforms — landed (RAD-28).** Push ECS→Box2D only at spawn and through the explicit verbs (`Teleport`/`RefreshCollider`); readback drains v3 move events; shapes mutate in place and are never destroyed per step. Rejected alternative: implicit change detection (dirty flags or per-step compares) — the per-step push this replaced *was* the implicit design, and it cost contact persistence, sleeping, and warm-starting.
- **Collision events are queued, not called back — RAD-29.** v3 enforces the queue shape at the API level; RAD-29 adds entity-validity checks and decides where gameplay callbacks live (script hooks / Level delegate — never `std::function`s on components). The move-event drain (RAD-28) is the rehearsal of exactly this idiom.

## Known Issues & Evolution

- **No collision notifications (RAD-29)** — see the gap note above.
- **No gameplay dynamics verbs yet (RAD-90)** — nothing can apply forces, impulses, or velocities to a body; kinematic bodies have no mover (`b2Body_SetTargetTransform`); and `Teleport` is keep-velocity only — a teleport during landing carries transient angular velocity into free fall (observed 2026-07-10; `TeleportType::ResetVelocity` arrives with RAD-90).
- **IsActive does not disable bodies** — an inactive entity's body keeps simulating and colliding; only its ECS transform stops following (pre-existing behavior, preserved by the RAD-28 drain). A real disable would use `b2Body_Disable` when a customer appears.
- **Resolved 2026-07-10 (RAD-28):** per-step transform push + shape destroy/recreate (killed contact persistence, sleeping, warm-starting); collider double-rotation quirk (shape local rotation repeated the body's world rotation); teleports smearing across one rendered frame (snapshot now stamped by `Level::Teleport`); missing `on_destroy` for colliders (removing the component left the shape colliding forever).
- **Resolved 2026-07-09 (RAD-27):** process-singleton world (leak + cross-world corruption + scratch-level teardown landmine) and dangling `b2Body*` runtime pointers (now checkable generation handles).
- **Resolved 2026-07-05 (RAD-25):** variable timestep; transform retrieval coupled to rendering.