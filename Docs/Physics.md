# Physics

**Status:** Collision reporting landed 2026-08-01 (RAD-29): contacts are drained into an engine-typed event queue after the step and dispatched to gameplay with validity checks. Sync semantics landed 2026-07-10 (RAD-28): physics owns dynamic transforms, ECS→Box2D pushes are explicit verbs only, readback drains v3 move events, shapes are never rebuilt per step. Per-Level worlds on **Box2D v3.1.1** landed 2026-07-09 (RAD-27; the v3 upgrade was decided in RAD-60). Fixed timestep landed 2026-07-05 (RAD-25). Phase 2's physics rework is complete; remaining physics work is follow-on (RAD-90 dynamics verbs, RAD-91 verb-surface cleanup).

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
 │    ├─ drain b2World_GetBodyEvents    → m_MoveEvents    (engine-typed copies)
 │    └─ drain b2World_GetContactEvents → m_ContactEvents (engine-typed copies)
 ├─ move-event drain: for each BodyMoveEvent, UUID → entity → Translation.xy/Rotation.z
 └─ contact dispatch: for each ContactEvent → observers, then per-side script hooks
Level::OnRender(alpha) — draw-only: interpolates via snapshots, never mutates simulation
```

Contact dispatch runs **after** the move drain deliberately: a handler asking "where am I?" must read this step's transforms, not the previous step's.

v3's sub-step count (4, the library default) replaces v2's `(6, 2)` velocity/position iterations.

**Readback drains move events, not entities (RAD-28):** Box2D reports *only the bodies that moved* — the same "for each dirty proxy" shape as UE Chaos's `PullPhysicsStateForEachDirtyProxy_External` and PhysX's `PxActiveTransforms`. Cost scales with **activity, not population**: a settled level drains an empty array; sleeping and static bodies cost nothing. Mechanics: the event array is transient, so `Step` copies it immediately into a reusable buffer of engine-typed `BodyMoveEvent` PODs (`{UUID, position, rotation}` — Box2D types never leave `Physics/`); the UUID rides in each body's `userData`. `Level` resolves UUIDs via its entity map — nothing destroys entities between the step and the drain (scripts run *before* the step), so a lookup miss is an asserted invariant break, not a survivable case. Bodies falling asleep TRACE — the visible proof that contact persistence works. The drain is camera-blind and sprite-blind by construction, and its shape is a deliberate rehearsal of RAD-29's contact-event drain.

### Collision reporting (RAD-29)

**The problem this solves.** Physics knows two things touched; gameplay needs to hear about it. The obvious mechanism — hand physics a callback and let it call you the instant the solver notices — is the wrong one, because it fires *inside* the step, while Box2D is walking its own contact arrays. The natural gameplay reaction ("bullet hit wall → delete bullet") then mutates the very structure being iterated. Box2D's manual forbids it outright (*"You cannot create/destroy Box2D entities inside these callbacks"*, `docs/simulation.md`); Unity defers `Destroy()` to end-of-frame for exactly this reason. v2's `CollisionListener2D` did it anyway, with no validity checks, and died with the v3 port.

v3 removes the temptation: there is no listener API. The library records touches into arrays during the step and hands them over afterwards, because it simulates on multiple threads where a mid-step callback would be meaningless. **Radiant's job is the part Box2D cannot do: translate shape ids to entities, verify those entities still exist, and choose where gameplay hears about it.**

**The drain.** `PhysicsWorld2D::Step` copies `b2World_GetContactEvents` into a reusable buffer of engine-typed `ContactEvent{EntityA, EntityB, Phase}` records the instant the step returns. That copy is load-bearing: once the records are engine types, gameplay may destroy bodies, spawn entities, and add or remove components freely, because nothing touches Box2D's arrays again that step.

`ContactPhase`/`ContactEvent` live in `Physics/ContactEvent.h` rather than inside `PhysicsWorld2D` — the ECS layer names `ContactPhase`, and `PhysicsWorld2D.h` carries `<box2d/id.h>` for its `b2WorldId` member, so nesting them would have put a vendor header in a public engine header.

**Begins are drained before ends**, and that ordering is a guarantee gameplay may rely on, not an accident. Box2D provides no cross-array timestamp, so consider an entity standing on platform A (overlap count 1) when A is destroyed and it lands on B in the same step: begins-first gives 1 → 2 → 1, ends-first gives 1 → **0** → 1 and every "I left the ground" reaction fires spuriously for one step.

**Three validity checks, at three moments, for three different reasons:**

| When | Check | Why it can fail |
|------|-------|-----------------|
| Translation (`Step`) | `b2Shape_IsValid` before following any shape id | An **end** event may name a shape destroyed a step ago (below) |
| Resolution (`Level`) | UUID present in the entity map | The entity died since the step |
| Dispatch (per side, per callback) | the side is re-resolved immediately before its hook runs | An **earlier callback in this same batch** destroyed it |

The third is the one that matters, and it mirrors UE: `FPhysScene_Chaos::DispatchPendingCollisionNotifies` calls `Actor.Get()` *inside* its loop, per entry, for the identical reason (`PhysScene_Chaos.cpp:1150`). Verified 2026-08-01: with a script destroying its partner from `OnCollisionBegin`, the partner's own hook never fires — it is re-resolved, found dead, and skipped.

Note the asymmetry with the move drain: there, a missing entity is an **asserted invariant break** (nothing can destroy entities between step and drain, since scripts run before the step). Here it is **routine** — dispatch *is* gameplay code. Same lookup, opposite verdict, because the surrounding invariants differ. Likewise, an invalid shape in a *begin* event asserts (nothing can run between `b2World_Step` returning and the drain), while an invalid shape in an *end* event is survived silently.

**Why end events name dead shapes.** v3 keeps two end-event buffers and swaps them at the end of each step (`src/world.c:807`); `b2World_GetContactEvents` reads the one *not* being written (`world.c:1538`). So an end event caused by a `b2DestroyBody` between steps surfaces after the **next** step, with the shape provably gone — the library's own header says *"this shape may have been destroyed … @see b2Shape_IsValid"*. An unresolvable side becomes `UUID(0)`, and the **surviving** side is still notified: an overlap counter that never decrements is worse than one told its partner vanished.

**Two dispatch channels**, in this order:

- **Observers** — `Level::AddCollisionObserver` / `RemoveCollisionObserver`, called once per event with **both** participants. The channel for level-wide systems (abilities, damage, audio, VFX) belonging to no single entity. Keyed by a `{Index, Generation}` handle over a slot pool with a free list — the `TimerHandle` idiom (playbook §2), so a stale handle is a no-op rather than a removal of whoever inherited the slot.
- **Script hooks** — `ScriptableEntity::OnCollisionBegin/OnCollisionEnd(Entity other)`, called once per **live side**, because "something hit me" is inherently one-sided. `other` may be invalid; the hook is skipped for inactive entities, matching `OnUpdate`.

Observers run first, matching UE's world-handler-before-per-actor order: global systems see the fullest picture before per-entity gameplay starts destroying things. Deliberately rejected: UE's *batch* signature (`HandlePhysicsCollisions_AssumesLocked(TArray<…>&)`) — it hands subscribers a list that goes stale as they walk it, making re-validation everyone's problem.

Two re-entrancy hazards, both closed: an observer that **removes itself** mid-callback would free the `std::function` currently executing, so removal during dispatch defers the release to the end of the batch; an observer that **registers another** mid-dispatch could reallocate the vector, so iteration is index-based with the count captured at entry, and the newcomer joins the next batch (the rule `EventQueue` already uses).

The observer callback lifetime contract is `TimerManager`'s, word for word: **the Level owns the callback by value, and therefore its captures.** A subscriber that dies without removing its handle leaves the Level calling into a dead object, and the Level cannot detect that.

**Opting in.** `b2DefaultShapeDef()` leaves `enableContactEvents` **false**, so `BoxCollider2DComponent::EnableContactEvents` surfaces it — defaulting to `true`. Events are *transitions, not states*: a settled stack emits nothing per step, so defaulting off buys no measurable performance while recreating UE's classic "why isn't my hit event firing" trap. Box2D ORs the flag — a contact reports if **either** shape opted in (`src/contact.c:253`) — and a contact caches the flag when it is **created**, so toggling mid-touch yields an unpaired event. Treat it as spawn-time configuration.

**Cost.** One `b2World_GetContactEvents` (three pointers, no copy), one pass over begin+end counts, and per dispatched event `2 × (observers + 2)` entity-map lookups from the re-resolution. No steady-state allocation — `clear()` keeps capacity, `reserve()` precedes the fill. Everything scales with how much the world is *changing*, not how much is in it.

### Diagnostics

`b2SetAssertFcn` routes Box2D's internal asserts into Radiant's log-then-break machinery (installed once, at first world creation). World create/destroy is TRACE-logged with the owning Level's name — the paired-lifetime evidence for RAD-27's AC. The Box2D vendor project keeps `B2_ASSERT` active in Debug and Release (`B2_ENABLE_ASSERT`) and strips it in Dist (`NDEBUG`), matching the engine's assert convention.

## Design Rationale (the Phase 2 contract, playbook §4)

- **One world per Level, owned by the Level — landed (RAD-27).** Physics is world state; a process singleton meant one Level maximum and corruption on Level churn. Rejected alternative: ref-counting the singleton's Init/Shutdown — patches the leak, still forbids coexistence.
- **Box2D v3, not 2.4 — landed (RAD-60 decision, RAD-27 execution).** v3 *is* the target architecture: id handles legal in plain-data components, per-Level worlds as values, event-buffer contacts. Migrating during the seam rebuild paid for the port once.
- **Fixed timestep — landed (RAD-25).** Simulation steps at a fixed rate from an accumulator; rendering interpolates.
- **Physics owns dynamic transforms — landed (RAD-28).** Push ECS→Box2D only at spawn and through the explicit verbs (`Teleport`/`RefreshCollider`); readback drains v3 move events; shapes mutate in place and are never destroyed per step. Rejected alternative: implicit change detection (dirty flags or per-step compares) — the per-step push this replaced *was* the implicit design, and it cost contact persistence, sleeping, and warm-starting.
- **Collision events are queued, not called back — landed (RAD-29).** v3 enforces the queue shape at the API level; RAD-29 added the three entity-validity checks and two dispatch channels (Level-wide observers, then per-side script hooks — never `std::function`s on components). The move-event drain (RAD-28) was the rehearsal of exactly this idiom. Rejected alternatives: reusing `EventQueue` (frame-scoped, closed variant list, propagation semantics that mean nothing for a physics fact); dispatching inside `PhysicsWorld2D` (needs the `Level*` back-pointer RAD-27 deleted); and a batch observer signature (exports re-validation to every subscriber).

## Known Issues & Evolution

- **Contacts report entities, not shapes.** One collider per entity today, so entity identity suffices. The moment an entity carries several shapes ("hurtbox vs feet sensor"), gameplay needs to know *which* shape touched and `ContactEvent` must be extended — never duplicated.
- **No contact detail and no sensors yet.** `ContactEvent` carries the pair and the phase, not the manifold: no contact normal, no impact point, no `approachSpeed` (v3's hit events, `enableHitEvents`, are untouched), and no sensor events. Nothing needs them yet; the normal's A/B ordering semantics deserve their own design pass.
- **No collision filtering.** `b2Filter` is left at default — no channels, no filtering matrix. `EnableContactEvents` answers "report or not," not "report to whom."
- **No gameplay dynamics verbs yet (RAD-90)** — nothing can apply forces, impulses, or velocities to a body; kinematic bodies have no mover (`b2Body_SetTargetTransform`); and `Teleport` is keep-velocity only — a teleport during landing carries transient angular velocity into free fall (observed 2026-07-10; `TeleportType::ResetVelocity` arrives with RAD-90).
- **IsActive does not disable bodies** — an inactive entity's body keeps simulating and colliding; only its ECS transform stops following (pre-existing behavior, preserved by the RAD-28 drain). A real disable would use `b2Body_Disable` when a customer appears.
- **Resolved 2026-08-01 (RAD-29):** no collision notifications at all (gameplay could not detect a landing, a hit, or a pickup); the v2 design's mid-step callbacks that made world mutation from a handler undefined behaviour; and per-component `std::function` callbacks that blocked `Level::Copy` and could never serialize.
- **Resolved 2026-07-10 (RAD-28):** per-step transform push + shape destroy/recreate (killed contact persistence, sleeping, warm-starting); collider double-rotation quirk (shape local rotation repeated the body's world rotation); teleports smearing across one rendered frame (snapshot now stamped by `Level::Teleport`); missing `on_destroy` for colliders (removing the component left the shape colliding forever).
- **Resolved 2026-07-09 (RAD-27):** process-singleton world (leak + cross-world corruption + scratch-level teardown landmine) and dangling `b2Body*` runtime pointers (now checkable generation handles).
- **Resolved 2026-07-05 (RAD-25):** variable timestep; transform retrieval coupled to rendering.