# Physics

**Status:** Rework planned (Phase 2: RAD-25, RAD-27, RAD-28, RAD-29). This is the engine's weakest subsystem; this document describes both the current behavior and the target design so the rework has a written contract.

## Architecture (current)

Radiant integrates **Box2D 2.4.0** (submodule) behind `Physics/Physics2D.{h,cpp}` — a fully **static** class holding one process-wide data block: the `b2World` (gravity hardcoded to `{0, -9.8}`), a `Level*` back-pointer, and the `CollisionListener2D`.

### Body & fixture lifecycle

Driven by entt signals (see [ECS-And-Levels](ECS-And-Levels.md)):

- `on_construct<RigidBody2DComponent>` → `CreatePhysicsBody`: builds a `b2BodyDef` from the entity's transform (position = translation.xy, angle = rotation.z), stamps the entity's UUID into the body's user data, stores the `b2Body*` in the component's `RuntimeBody`.
- `on_construct<BoxCollider2DComponent>` → creates the box fixture (half-extents scaled by transform, density/friction/restitution from the component).
- `on_destroy<RigidBody2DComponent>` → destroys the body.

### Per-frame flow (current)

```text
Level::OnUpdate
 ├─ for each active rigidbody entity: SubmitEntitiesTransforms
 │    ECS transform → body->SetTransform, then DESTROY + RECREATE the fixture   ← defect
 └─ Physics2D::OnUpdate: world->Step(ts, 6, 2)                                  ← variable ts
Level::OnRender (sprite loop, only when a Primary camera exists)
 └─ UpdateEntitiesTransforms: body position/angle → ECS transform               ← wrong place
```

### Collision callbacks

`CollisionListener2D` implements `b2ContactListener`: on begin/end contact it resolves both bodies' UUIDs (from user data) to entities and invokes the `std::function` callbacks stored on their `RigidBody2DComponent`s, passing the *other* entity.

## Design Rationale (target — the Phase 2 contract, playbook §4)

- **One world per Level, owned by the Level.** Physics is world state; a process singleton means one Level maximum and dangling worlds on Level churn (RAD-27). Play-in-editor requires two live Levels.
- **Fixed timestep.** `Step(variable_dt)` makes simulation framerate-dependent and non-deterministic — unacceptable for gameplay consistency and a hard blocker for any future networking. The Phase 2 loop steps physics at a fixed rate from an accumulator, with render interpolation (RAD-25).
- **Physics owns dynamic transforms.** ECS→Box2D pushes happen only on explicit teleport/spawn/property change; Box2D→ECS readback is a dedicated sync pass over all rigidbody entities after each step — never inside a render path, never gated on cameras or sprites (RAD-28). Fixtures are rebuilt only when collider properties change: per-frame rebuild destroys contact persistence, sleeping, and warm-starting.
- **Collision events are queued, not called back.** The contact listener records `(uuidA, uuidB, begin/end)` during the step; the Level dispatches after the step with validity checks. Box2D forbids world mutation during callbacks, and per-component `std::function`s can't serialize (RAD-29).

## Known Issues & Evolution

All tracked under Phase 2; the headline defects the current design carries:

- **Process-singleton world (RAD-27)** — every `Level` constructor overwrites the shared world without freeing the previous one.
- **Per-frame fixture destroy/recreate (RAD-28)** — allocation storm + broken contact state, every frame, for every collider.
- **Readback coupled to rendering (RAD-28)** — a body without a sprite (or any body when no `Primary` camera exists) simulates but its ECS transform never moves.
- **Variable timestep (RAD-25)** — framerate-dependent simulation.
- **Unsafe callbacks (RAD-29)** — no validity checks on mid-destruction entities; components hold `std::function`s.
- **Creation-path bugs (RAD-12)** — collider creation crashes on sprite-less entities (fetches an unused `SpriteComponent`) and passes a translation where Box2D expects an angle.
