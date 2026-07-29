# Implementation Plan — RAD-28: Fix physics sync: no per-frame fixture rebuild, correct transform ownership

| Field | Value |
|-------|-------|
| **Jira** | [RAD-28](https://hndredgames.atlassian.net/browse/RAD-28) |
| **Epic** | RAD-2 — Phase 2: Simulation Foundation |
| **Story status** | To Do |
| **Dependencies** | RAD-60 Box2D v3 decision (Done), RAD-27 per-Level worlds (Done) |
| **Planned** | 2026-07-09 |

**Scope note.** The story text predates RAD-27. Half of its second defect — "results are read back only inside the sprite render loop and only when a primary camera exists" — was already fixed when RAD-27 landed: readback is now a dedicated post-step pass over all rigidbodies, camera-independent (`Level.cpp` `OnFixedUpdate`). What this story still owes: kill the per-step ECS→Box2D force-push, kill the per-step shape destroy/recreate, and build the explicit verbs (teleport, collider refresh) that replace them.

---

## 0. The Problem, Ground Up

**What the code literally does today.** Radiant keeps two descriptions of every physical object: the ECS transform ("draw the crate here") and the Box2D body ("physics says the crate is here"). Box2D — the physics library that computes falling, sliding, and bouncing — is supposed to be the authority for anything dynamic. But every fixed simulation step, `Level::OnFixedUpdate` currently does this to **every** physics entity:

```text
every fixed step (60×/sec), for every physics entity:
    1. b2Body_SetTransform(body, ecs_transform)      // teleport the body to where the ECS thinks it is
    2. b2DestroyShape(body's collision shape)        // rip the collider off the body
    3. b2CreatePolygonShape(body, rebuilt collider)  // build a brand-new one, in case something changed
then:
    4. b2World_Step(...)                             // let physics simulate
    5. read every body's position back into the ECS  // (this half is already correct — RAD-27)
```

Steps 1–3 exist "just in case" gameplay moved an entity or changed its collider since last step. Almost every step, nothing changed — the code pays the full price anyway.

**Why that's not just wasteful but *wrong*.** Three terms of art, defined:

- A **contact** is Box2D's record that two shapes are touching: where, how deep, along which direction.
- **Warm-starting** means seeding this step's contact forces with last step's answer. A crate resting on the floor pushes back with the same force every step — starting from last step's solution, the solver converges almost instantly. Starting from zero, it visibly jitters before settling.
- **Sleeping** means a body that hasn't moved for a while is taken out of the solver entirely — a resting crate costs nothing until something touches it.

All three live *inside the shape's contacts*. Destroying the shape every step (step 2) destroys its contacts every step — so Box2D develops amnesia at 60 Hz. Warm-starting has nothing to warm-start from. Stacks of boxes wobble and creep instead of settling. Restitution (bounciness) misbehaves because it's computed from contact state that keeps being erased. Sleeping never engages because the contact graph never stabilizes. And the teleport in step 1 (Box2D's own docs: *"This acts as a teleport and is fairly expensive"* — `box2d.h:247`) overwrites whatever the simulation computed with the ECS's possibly-stale copy — the two truths fight, and the wrong one wins.

**Concrete numbers.** A level with 50 crates stacked into towers, simulating at 60 steps/sec: 3,000 teleports + 3,000 shape destructions + 3,000 shape creations *per second* — 9,000 mutations of a simulation in which, most seconds, **nothing moved**. Meanwhile a correctly-integrated Box2D would have all 50 crates asleep, costing ~zero. This defect class ships in real games: it's the classic "our stacks of physics props never stop trembling" bug.

**The analogy: an autopilot.** Box2D is the autopilot; the ECS transform is the flight display. Today's code is a pilot who grabs the yoke *every second* and yanks the plane back to where his paper flight plan says it should be — and, while he's at it, rebuilds the wings in case their shape changed. The plane never actually flies; the autopilot never gets to do its job. The fix is how real cockpits work: **let the autopilot fly, read the instruments, and take the controls only for a deliberate maneuver.** Concretely:

- **Physics owns the transform of dynamic bodies.** The per-step ECS→Box2D push is deleted outright. The ECS transform becomes a *display*, updated from the simulation — never the other way around, except:
- **Taking the controls is an explicit verb.** `Teleport(entity, position)` — for spawn points, respawns, level scripting — writes the ECS transform *and* pushes it into Box2D, once, deliberately.
- **Rebuilding the wings is an explicit verb.** `RefreshCollider(entity)` — called only when gameplay actually changes collider size, offset, or material — *mutates* the existing shape in place (Box2D v3 can do this; v2 couldn't) instead of destroy/recreate.

**How UE does it, in plain words.** Unreal has the identical contract: a component with "Simulate Physics" on is *owned by physics* — you cannot move it by writing its transform field; you call an explicit move function that carries a teleport flag (`ETeleportType`), and a dedicated sync pass copies simulated poses back into components after each physics tick. We're adopting exactly that shape at 2D scale (§2).

**What we gain and what we deliberately don't gain.** Gained: contact persistence, warm-starting, stable stacks, working restitution, working sleeping, and the removal of ~9,000 no-op mutations/sec — correctness *and* performance from the same deletion. **Not gained:** no new gameplay powers. Nothing in Reaper can push a body around yet (forces/impulses/velocity verbs are a filed follow-up), and one implicit convenience is deliberately *removed*: writing `transform.Translation` on a dynamic-body entity used to "work" by accident — after this story it does nothing physical, by design. That's a footgun we accept and document loudly, because the alternative (silent per-step pushes) is the bug this story exists to kill.

**What we have / what we're building:** we have a simulation that gets teleported and rebuilt every step by its own engine. We're building a simulation that runs undisturbed, reports what moved, and accepts commands only through two explicit verbs.

---

## 1. Architecture Decision

The decision that shapes everything: **transform ownership flips to physics, and every ECS→physics write becomes an explicit, named verb funneled through `Level`.** `Level` is the right funnel because it already owns both sides of the sync (the registry and the `Scope<PhysicsWorld2D>`, playbook §4) and already mediates all entity lifecycle. Gameplay never touches `PhysicsWorld2D` directly; `Physics/` never includes renderer or game headers. The verbs also maintain the *render* contract: a teleport resets the entity's `TransformSnapshotComponent` so interpolation (playbook §1) doesn't smear the jump across a frame — closing a documented gap in `Components.h`.

Two decisions were open at planning time; both were **locked in the walkthrough (2026-07-10)**:

- **D1 — Readback mechanism: LOCKED → drain Box2D v3's move events.** Replace the retrieve-all view loop with `b2World_GetBodyEvents` — Box2D hands us a contiguous array of *only the bodies that moved this step*. O(moved) instead of O(all bodies): readback cost scales with *activity*, not *population*, which is exactly what sleeping exists to exploit. It's also the same post-step event-drain idiom RAD-29 will use for contacts — learning it here pays twice. UE receipt: Chaos's readback entry point is `PullPhysicsStateForEachDirtyProxy_External` (`Engine/Source/Runtime/Experimental/Chaos/Public/Chaos/Framework/PhysicsSolverBase.h:671`) — "for each **dirty** proxy," never "for each body"; PhysX's `PxActiveTransforms` was the same design a generation earlier. Honest caveat: at Reaper's current entity count the perf win is unmeasurable; we're buying the architectural shape. Rejected: keep the view loop (simpler, already works) — reads every static and sleeping body forever and teaches nothing new.
- **D2 — Gameplay verb surface: LOCKED → `Entity::Teleport` convenience forwarding to the `Level` implementation.** The implementation lives only in `Level` (it owns all three touched things: registry, snapshot, physics world); `Entity::Teleport` is a zero-logic one-line forwarder — the **one operation, one implementation** rule with an ergonomic face, the same shape as `AddComponent`/`GetComponent` (entity-surface verbs over Level-owned storage) and UE's `AActor::SetActorLocation` forwarding to `MoveComponent`. Rejected: Level-only API — every script call becomes `GetLevel()->Teleport(GetEntity(), ...)` boilerplate through a Level accessor scripts don't currently have.

Playbook compliance: §1 (events/sim/render separation — readback stays inside the fixed step), §2 (ids are generation handles; shape ticket mirrors the body ticket), §3 (new component field is plain data, runtime-only), §4 (this story *completes* three of its bullets: never rebuild fixtures per frame, push only on teleport/spawn, dedicated readback), §7 (three-config DoD).

## 2. UE Reference

UE's contract for "you don't move simulated bodies by writing transforms" is expressed in three places we studied:

**The teleport semantics enum** — moving a physics body always states *how*:

```cpp
// Engine/Source/Runtime/Engine/Classes/Engine/EngineTypes.h:2400
enum class ETeleportType : uint8
{
    /** Do not teleport physics body. This means velocity will reflect the movement between initial and final position, and collisions along the way will occur */
    None,
    /** Teleport physics body so that velocity remains the same and no collision occurs */
    TeleportPhysics,
    /** Teleport physics body and reset physics state completely */
    ResetPhysics,
};
```

**The single explicit push point** — one funnel, wake-by-default:

```cpp
// Engine/Source/Runtime/Engine/Classes/PhysicsEngine/BodyInstance.h:1165
/**
 *	Move the physics body to a new pose.
 *	@param	bTeleport	If true, no velocity is inferred on the kinematic body from this movement, but it moves right away.
 */
ENGINE_API void SetBodyTransform(const FTransform& NewTransform, ETeleportType Teleport, bool bAutoWake = true);
```

**The one-way readback pass** — `UPrimitiveComponent::SyncComponentToRBPhysics()` (`Engine/Source/Runtime/Engine/Private/PrimitiveComponentPhysics.cpp:915`) copies simulated poses back into components after the physics tick; simulation results flow component-ward through this pass and nowhere else.

**We adopt:** the ownership rule; one explicit push verb with teleport-and-wake semantics (`bAutoWake = true` is UE's default and ours — see Risks); a dedicated one-way readback pass.
**We deliberately simplify:** no `ETeleportType` enum — we ship only the `TeleportPhysics` behavior (keep velocity, move instantly). UE's `None` mode is a swept move that infers velocity — it requires the whole `MoveComponent`/sweep machinery we don't have and 2D Reaper doesn't need; `ResetPhysics` (zero velocity too) waits for a customer, and adding a `resetVelocity` flag later is trivial. No deferred/scoped movement updates (`FScopedMovementUpdate`) — that's an optimization for deep attachment hierarchies; we have no attachment hierarchy.

## 3. File Plan

No new files — this story reshapes the seam built by RAD-25/27.

```text
Radiant/Source/Radiant/
├── Physics/
│   ├── PhysicsWorld2D.h     (modify) — SubmitTransform/RetrieveTransform deleted; Teleport,
│   │                                    UpdateBoxShape, DestroyBoxShape, BodyMove + GetMoveEvents added
│   └── PhysicsWorld2D.cpp   (modify) — CreateBoxShape stores shape id + rotation fix; Step drains move events
├── ECS/
│   ├── Components.h         (modify) — BoxCollider2DComponent gains RuntimeShapeId (packed claim ticket)
│   ├── Level.h / Level.cpp  (modify) — submit loop deleted; Teleport + RefreshCollider verbs;
│   │                                    collider on_destroy signal; move-event drain replaces retrieve loop
│   └── Entity.h / Entity.cpp(modify) — Teleport convenience forwarding to Level (D2)
Reaper/Source/Layers/GameLayer.cpp (modify) — debug cheats: T teleport, G grow collider; rotated test entity
Docs/Physics.md                    (modify) — per-step flow, ownership contract, known-issues updates
```

| Action | Path | Description |
|--------|------|-------------|
| Modify | `Physics/PhysicsWorld2D.{h,cpp}` | Delete the per-step push/rebuild; add the two explicit verbs + shape destroy; drain `b2World_GetBodyEvents` in `Step` |
| Modify | `ECS/Components.h` | `RuntimeShapeId` on the collider — plain-data ticket, never serialized (deserializer builds the component fresh, field defaults to 0 — verified, no serializer change needed) |
| Modify | `ECS/Level.{h,cpp}` | Ownership flip in `OnFixedUpdate`; `Teleport`/`RefreshCollider`; `on_destroy<BoxCollider2DComponent>`; removal-order fix in `DestroyEntity` |
| Modify | `ECS/Entity.{h,cpp}` | `Entity::Teleport` forwarder |
| Modify | `Reaper/.../GameLayer.cpp` | Verification cheats + one rotated static entity in `CreateDEBUG` (proves the collider-rotation fix) |
| Modify | `Docs/Physics.md` | Update contract (same-change rule) |

## 4. Type Design

No new classes — one POD struct, new members on existing types.

### PhysicsWorld2D::BodyMoveEvent
*(planned as `BodyMove`; renamed during implementation 2026-07-10 — the accessor (`GetMoveEvents`), the buffer (`m_MoveEvents`), and the vendor type (`b2BodyMoveEvent`) all say "event"; the element type should too)*
- **Kind:** nested POD struct (public)
- **Responsibility:** one "this body moved" record, translated out of Box2D types: `{ UUID EntityId; glm::vec2 Position; float Rotation; }`
- **Ownership:** value elements of a world-owned reusable buffer (`std::vector<BodyMove> m_MoveEvents`), refilled by `Step`, valid until the next `Step`
- **Lifetime & threading:** main-thread only; consumers must not hold references across steps
- **Playbook Patterns:** §1 (queued results drained at a defined point), §5-adjacent (reused buffer, no per-step allocation after warmup)

### PhysicsWorld2D (modified)
- `Teleport(Entity&, const glm::vec2& position, float rotation)` — `b2Body_SetTransform` + `b2Body_SetAwake(body, true)`; validity-checked like every body access
- `UpdateBoxShape(Entity&, BoxCollider2DComponent&)` — `b2Shape_SetPolygon` (recomputed half-extents × scale, offset, **identity local rotation**) + `b2Shape_SetDensity(id, d, false)` + `b2Shape_SetFriction`/`SetRestitution`, then one `b2Body_ApplyMassFromShapes` (v3's geometry setters deliberately don't touch mass — `box2d.h:637`)
- `DestroyBoxShape(Entity&, BoxCollider2DComponent&)` — signal-driven mirror of `CreateBoxShape`; zero/stale ticket is a warned no-op
- `GetMoveEvents() const` → `const std::vector<BodyMove>&` — filled by `Step`; keeps Box2D types out of `Level` (the boundary rule)
- **Deleted:** `SubmitTransform`, `RetrieveTransform`

### BoxCollider2DComponent (modified)
- `uint64_t RuntimeShapeId = 0` — packed `b2ShapeId` (`b2StoreShapeId`/`b2LoadShapeId`, `id.h:119`), same claim-ticket pattern as `RuntimeBodyId` (playbook §2/§3): the world owns the shape, zeroes the ticket on destroy, never serialized

### Level / Entity (modified)
- `Level::Teleport(Entity, const glm::vec3& translation, float rotationZ)` (+ translation-only overload keeping current rotation) — writes ECS transform, `emplace_or_replace`s the snapshot to the new pose (kills the interpolation smear), pushes to the body if one exists; works on non-physics entities too (ECS + snapshot only) — the universal "move discontinuously" verb
- `Level::RefreshCollider(Entity)` — re-applies collider component fields + current transform scale to the live shape via `UpdateBoxShape`
- `Entity::Teleport(...)` — thin forwarders (Entity.cpp; Entity already friends Level)
- New signal wiring: `on_destroy<BoxCollider2DComponent>` → `OnBoxCollider2DComponentDestroy`

## 5. Implementation Steps

Mentorship split: Kareem implements Phases A–D core; Claude does doc comments, `Docs/Physics.md`, the Reaper cheats, and build verification (chores).

### Phase A — Shape identity & lifecycle symmetry
- [x] **A1. Shape ticket on the collider** — add `RuntimeShapeId` to `BoxCollider2DComponent` with the claim-ticket doc comment; confirm deserializer leaves it 0 (it builds the component fresh — no serializer edit). *(Done 2026-07-10, Claude at Kareem's request)*
- [x] **A2. `CreateBoxShape` stores the ticket and stops double-rotating** — store the packed id; local rotation becomes identity (`b2MakeOffsetBox` with `b2Rot_identity`) — the shape is body-local, the body already carries world rotation. This fixes the documented rotated-collider quirk. *(Done 2026-07-10)*
- [x] **A3. Shape lifecycle symmetry** — `DestroyBoxShape` + `on_destroy<BoxCollider2DComponent>` signal; `DestroyEntity` removes the collider **before** the rigidbody (a body destroy takes its shapes with it — collider-after-body means a stale ticket warn); `DestroyBody` zeroes a sibling collider's `RuntimeShapeId` (the shapes died with the body — both the normal and stale-id paths zero it). *(Done 2026-07-10; all 3 configs built, Reaper smoke run clean)*

### Phase B — The ownership flip
- [x] **B1. Delete the push** — remove `SubmitTransform` and its loop in `OnFixedUpdate`. This is the story's core deletion: after it, nothing pushes ECS state into Box2D implicitly, ever. Reaper must behave identically (its entities never move via ECS writes after spawn — audited). *(Done 2026-07-10; `Level.h` OnFixedUpdate doc updated same-change; all 3 configs built, smoke run clean)*

### Phase C — The explicit verbs
- [x] **C1. `PhysicsWorld2D::Teleport`** — SetTransform + **SetAwake(true)**: v3's SetTransform does not wake; without the wake, teleporting a sleeping body leaves it frozen mid-air (see Risks). *(Done 2026-07-10; TRACE audit line per teleport)*
- [x] **C2. `Level::Teleport` + `Entity::Teleport`** — ECS write + snapshot reset + physics push. Snapshot reset is the subtle line: without it the renderer lerps from the old position and the teleport smears across one frame. *(Done 2026-07-10; +null-level guard in the Entity forwarders — the one invalid state Level's own warn can't reach)*
- [x] **C3. `UpdateBoxShape` + `Level::RefreshCollider`** — in-place mutation, never destroy/create; one `b2Body_ApplyMassFromShapes` after the setters. *(Done 2026-07-10; all 3 configs built, smoke run clean — verbs get their first callers in E1)*

### Phase D — The move-event drain (D1)
- [x] **D1. Drain in `Step`, consume in `Level`** — after `b2World_Step`, copy `b2World_GetBodyEvents` into `m_MoveEvents` (UUID from `userData`, position, angle; `reserve` + `clear` keeps capacity; the Box2D array is transient — copy, never store). `Level::OnFixedUpdate` replaces the retrieve view loop with: for each `BodyMove`, resolve UUID via `m_EntityMap`, write `Translation.xy`/`Rotation.z`. TRACE on `fellAsleep` events (the sleeping-works evidence). Delete `RetrieveTransform`. *(Done 2026-07-10; IsActive gating preserved from the old readback; smoke run showed two `fell asleep` TRACE lines — sleeping works, first time ever)*

### Phase E — Proof, docs, handoff
- [x] **E1. Reaper verification cheats (chore)** — `T` teleports the Reaper entity to (4, 2) mid-air (`Entity::Teleport` customer; also proves wake-from-sleep); `G` grows the green square's collider `Size` ×1.25 (wrapping at 1.5) and calls `RefreshCollider`; one static "RotatedColliderTest" entity spawned at 45° in `OnAttach` on **both** load paths (collider debug-draw must match the sprite — proves A2; runtime-spawned so the authored level is never polluted). *(Done 2026-07-10)*
- [x] **E2. Docs & DoD (chore)** — `Docs/Physics.md` per-step flow + ownership/verbs section + known-issues (incl. new: no dynamics verbs, IsActive doesn't disable bodies); playbook §4 landed markers; doc comments trued up (`Components.h` smear note, `Level.h` OnFixedUpdate contract, `PhysicsWorld2D.h`); all 3 configs clean; smoke run graceful with 3 `fell asleep` events and no new warnings. *(Done 2026-07-10; visual cheat verification is Kareem's run)*

## 6. Ownership & Lifetime Strategy

Nothing changes hands — this story tightens existing stories. The Level owns the world (`Scope`, RAD-27); the world owns bodies and shapes; components carry **claim tickets** (packed generation handles), zeroed by the world when the underlying object dies. New rules this story adds: the shape ticket follows the exact body-ticket lifecycle (created by signal, zeroed by `DestroyBoxShape` or by `DestroyBody` when the body takes its shapes down); `m_MoveEvents` elements are valid only between a `Step` and the next — `Level` consumes them inside the same `OnFixedUpdate`, and nothing destroys entities between the step and the drain (scripts run *before* the step), so UUID resolution during the drain cannot race entity destruction. A drain lookup miss is therefore a programmer error: assert, skip in Dist.

## 7. Performance Notes

- **Removed, per step, per body:** one `b2Body_SetTransform` (broadphase update), one shape destroy + create (contact destruction, BVH remove/insert, allocation), one implicit mass recompute. At 50 bodies / 60 Hz that's ~9,000 world mutations/sec gone; steady-state per-step ECS→Box2D cost is now **zero**.
- **Restored, for free:** warm-starting (solver convergence), sleeping (resting bodies leave the solver), contact persistence (stable stacks, correct restitution).
- **Readback:** O(moved bodies) instead of O(all rigidbodies); a fully-settled level drains an empty array. Per-moved-body cost: one hash lookup (`m_EntityMap`) + two float writes.
- **Allocations:** `m_MoveEvents` reserves once and reuses capacity; teleports and refreshes are gameplay-rare by contract. No new per-frame heap activity anywhere.

## 8. Logging & Diagnostics

- `Teleport` / `RefreshCollider`: TRACE (rare, deliberate acts — the log line is the audit trail of every explicit push).
- Drain: no per-event logging (60 Hz spam); TRACE on `fellAsleep` — doubles as the AC evidence that sleeping works.
- Programmer errors (assert + recover in Dist): teleport/refresh on an entity missing the required component; stale ticket in the drain or `DestroyBoxShape` path follows the existing warn-and-zero convention; drain UUID miss (violates the no-destroy-between-step-and-drain invariant).
- Content mistakes (WARN + recover): cheat targets missing from the loaded level (`FindEntityByName` miss).

## 9. Scalability Review

- **Per-shape-type functions** (`CreateBoxShape`/`UpdateBoxShape`/`DestroyBoxShape`): at 10+ collider types this smells like a switch ladder, but Box2D v3's own API is per-geometry (`b2Shape_SetPolygon`/`SetCircle`/`SetCapsule` — `box2d.h:626-639`), so per-component-type functions dispatched by per-component signals is the *native* shape, not a workaround. When circle/capsule colliders arrive (no story yet — file when needed), each adds one component + three small functions; no cross-cutting rewrite. Already the most scalable option at our altitude.
- **The verb funnel scales:** future physics verbs (forces, impulses, velocity — follow-up below) join `Teleport`/`RefreshCollider` on the same Level funnel; the readback drain shape is exactly RAD-29's contact drain. This story establishes the pattern the rest of Phase 2 reuses.
- **O(moved) readback** is the right asymptotic for the 1,000-body levels Phase 5's editor will host; the view-loop alternative would have been the flagged pattern.

## 10. Risks & Edge Cases

- **Sleeping teleport freeze:** `b2Body_SetTransform` does not wake a sleeping body — teleport one into mid-air and it hangs there until touched. Mitigation: `Teleport` always wakes (C1), matching UE's `bAutoWake` default.
- **Transient event array:** `b2World_GetBodyEvents` data is invalidated by body destruction and the next step (`box2d.h:44`); the drain copies into `m_MoveEvents` immediately, inside `Step`.
- **The removed safety net:** code that wrote `transform.Translation` on a dynamic body and relied on the implicit push now silently does nothing physical. Audited: no such code exists in Radiant or Reaper today (spawns set the transform *before* `AddComponent<RigidBody2DComponent>`, which is the supported spawn path — both `CreateDEBUG` and the deserializer, verified `LevelSerializer.cpp:307→379`). The contract lands in doc comments + `Docs/Physics.md`.
- **Stale collider after scale writes:** changing `Scale` without `RefreshCollider` leaves the physics shape at the old size. Deliberate (scale is not simulated; implicit detection is the antipattern this story kills); the debug collider draw reads ECS data, so a mismatch is *visible* — sprite and collider outline grow while the body's behavior doesn't change until refresh.
- **Teleport into overlap:** legal; Box2D resolves the penetration over the following steps (standard solver behavior). No special handling.
- **Kinematic bodies:** no mover verb this story — v3's `b2Body_SetTargetTransform` (`box2d.h:280`) is the right tool and waits for a customer (Reaper has zero kinematic bodies). Filed with the follow-up.

**Follow-up: filed as [RAD-90](https://hndredgames.atlassian.net/browse/RAD-90)** (2026-07-10) — *Gameplay dynamics verbs: forces, impulses, velocity, kinematic mover, teleport velocity reset*. Includes the `TeleportType::ResetVelocity` mode motivated by the T-spam tumble observed during this story's verification (velocity-kept teleport carries transient angular velocity into free fall — correct physics, wrong tool for respawns).

## 11. Verification (AC → proof)

The story has no explicit AC section; these criteria operationalize its Fix paragraph and will be written back to the story per the plan-approved-AC convention.

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| No per-step ECS→Box2D push; physics owns dynamic transforms | `SubmitTransform` no longer exists (compile proves no caller); Reaper run: crates fall/land/settle identically to pre-change baseline |
| No per-step shape destroy/recreate; rebuild only on property change | Per-step path contains no shape calls (review); `G` cheat grows the collider via `RefreshCollider` and behavior changes only then |
| Explicit teleport works end-to-end | `T` cheat: Reaper entity jumps to spawn instantly — no interpolation smear — and resumes falling (velocity kept, body awake) |
| Sleeping & contact persistence restored | After the stack settles, `fellAsleep` TRACE lines appear; no idle jitter on resting bodies |
| Rotated collider matches sprite (quirk fix) | 45° static entity in `CreateDEBUG`: collider debug-draw aligns with the sprite (previously doubled to 90°) |
| Readback independent of rendering/cameras (RAD-27 regression guard) | Sprite-less rigidbody entity still gets ECS transform updates (drain is camera-blind by construction) |
| Engine stays shippable | All three configs build clean via CLI; full Reaper run from `Reaper/` working dir |