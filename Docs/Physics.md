# Physics

**Status:** Rework planned (Phase 2: RAD-27, RAD-28, RAD-29) — **on Box2D v3** (upgrade decided 2026-07-05, RAD-60). Fixed timestep + post-step readback landed 2026-07-05 (RAD-25). This is the engine's weakest subsystem; this document describes both the current behavior (Box2D 2.4) and the target design so the rework has a written contract.

## The Problem This Solves

Making things fall, slide, bounce, and never sink through the floor is a numerical-simulation problem — genuinely hard, thoroughly solved, and not this project's learning goal — so Radiant delegates the math to **Box2D** and keeps its own learning budget for architecture.

What *is* our problem: the engine now holds **two descriptions of the same world**. The ECS has a transform per entity ("draw the crate here"); Box2D has a body per physical object ("physics says the crate is here"). The physics subsystem is really the discipline of keeping those two truths in sync without them fighting — deciding who owns an entity's position (physics, for anything dynamic), when each side reads the other, and how "these two things touched" gets reported back to gameplay. Nearly every defect in this subsystem, current or historical, is a two-sources-of-truth failure: pushing stale transforms into the simulation, reading results back in the wrong place, or telling the simulation to rebuild things it was still using.

## Architecture (current)

Radiant integrates **Box2D 2.4.0** (submodule) behind `Physics/Physics2D.{h,cpp}` — a fully **static** class holding one process-wide data block: the `b2World` (gravity hardcoded to `{0, -9.8}`), a `Level*` back-pointer, and the `CollisionListener2D`.

### Body & fixture lifecycle

Driven by entt signals (see [ECS-And-Levels](ECS-And-Levels.md)):

- `on_construct<RigidBody2DComponent>` → `CreatePhysicsBody`: builds a `b2BodyDef` from the entity's transform (position = translation.xy, angle = rotation.z), stamps the entity's UUID into the body's user data, stores the `b2Body*` in the component's `RuntimeBody`.
- `on_construct<BoxCollider2DComponent>` → creates the box fixture (half-extents scaled by transform, density/friction/restitution from the component).
- `on_destroy<RigidBody2DComponent>` → destroys the body.

The wiring is three lines in the `Level` constructor — entt calls the handler whenever the component appears or disappears, so there is no separate "register with physics" step to forget:

```cpp
m_Registry.on_construct<RigidBody2DComponent>().connect<&Level::OnRigidBody2DComponentConstruct>(this);
m_Registry.on_destroy<RigidBody2DComponent>().connect<&Level::OnRigidBody2DComponentDestroy>(this);
m_Registry.on_construct<BoxCollider2DComponent>().connect<&Level::OnBoxCollider2DComponentConstruct>(this);
```

### Per-step flow (current)

Physics advances once per FIXED simulation step (`Level::OnFixedUpdate`, driven by the accumulator loop — see [Time-And-Simulation](Time-And-Simulation.md)), never per rendered frame:

```text
Level::OnFixedUpdate (0..N times per frame, fixed delta)
 ├─ snapshot pass: movable entities' transforms → TransformSnapshotComponent
 ├─ native scripts
 ├─ for each active rigidbody entity: SubmitEntitiesTransforms
 │    ECS transform → body->SetTransform, then DESTROY + RECREATE the fixture   ← defect (RAD-28)
 ├─ Physics2D::OnUpdate: world->Step(fixedDelta, 6, 2)
 └─ readback pass: every active body's position/angle → ECS transform
Level::OnRender(alpha) — draw-only: interpolates via snapshots, never mutates simulation
```

The post-step readback covers **every** active body — the old design read positions back inside the sprite render loop, so a body without a sprite (or any body when no `Primary` camera existed) simulated but its ECS transform never moved. That hole died with the move.

### Collision callbacks

`CollisionListener2D` implements `b2ContactListener`: on begin/end contact it resolves both bodies' UUIDs (from user data) to entities and invokes the `std::function` callbacks stored on their `RigidBody2DComponent`s, passing the *other* entity.

## Design Rationale (target — the Phase 2 contract, playbook §4)

- **Box2D v3, not 2.4 (RAD-60, decided 2026-07-05).** The rework implements against v3's rewritten API because it *is* our target architecture: bodies are `b2BodyId` value handles (legal in data-only components, unlike `b2Body*`), worlds are `b2WorldId` (trivially per-Level), and contact reporting is an **event buffer drained after the step** (`b2World_GetContactEvents`) — no listener callbacks exist to misuse. Costs accepted: C API migration (free during a seam rebuild), our own vendor premake script replacing the 2.4 fork, pinned to a v3 release tag.

- **One world per Level, owned by the Level.** Physics is world state; a process singleton means one Level maximum and dangling worlds on Level churn (RAD-27). Play-in-editor requires two live Levels.
- **Fixed timestep — landed (RAD-25).** `Step(variable_dt)` makes simulation framerate-dependent and non-deterministic — unacceptable for gameplay consistency and a hard blocker for any future networking. The loop now steps physics at a fixed rate from an accumulator, with render interpolation ([Time-And-Simulation](Time-And-Simulation.md)).
- **Physics owns dynamic transforms.** The readback half landed with RAD-25 (dedicated post-step pass, never inside a render path, never gated on cameras or sprites). Still owed to RAD-28: ECS→Box2D pushes only on explicit teleport/spawn/property change (today every active body is re-pushed each step), and fixtures rebuilt only when collider properties change — per-step rebuild destroys contact persistence, sleeping, and warm-starting.
- **Collision events are queued, not called back.** The contact listener records `(uuidA, uuidB, begin/end)` during the step; the Level dispatches after the step with validity checks. Box2D forbids world mutation during callbacks, and per-component `std::function`s can't serialize (RAD-29).

## Known Issues & Evolution

All tracked under Phase 2; the headline defects the current design carries:

- **Process-singleton world (RAD-27)** — every `Level` constructor overwrites the shared world without freeing the previous one.
- **Per-step fixture destroy/recreate (RAD-28)** — allocation storm + broken contact state, every step, for every collider. The explicit-teleport push path (replacing the unconditional per-step transform push) lands in the same story, and formalizes snapshot-reset-on-teleport so interpolation snaps instead of smearing.
- **Unsafe callbacks (RAD-29)** — no validity checks on mid-destruction entities; components hold `std::function`s.

Resolved 2026-07-05 (RAD-25): variable timestep (physics now steps at a fixed rate from the accumulator) and readback-coupled-to-rendering (dedicated post-step sync pass over all active bodies).
