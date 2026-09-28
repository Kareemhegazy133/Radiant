# Implementation Plan — RAD-90: Gameplay dynamics verbs — forces, impulses, velocity, kinematic mover, teleport velocity reset

| Field | Value |
|-------|-------|
| **Jira** | [RAD-90](https://hndredgames.atlassian.net/browse/RAD-90) |
| **Epic** | RAD-2 — Phase 2, Simulation Foundation |
| **Story status** | To Do |
| **Dependencies** | RAD-28 (Done), RAD-91 (In Review, committed to `dev` as `f646faa`) |
| **Planned** | 2026-08-01 |

> **Architecture realignment (2026-08-01).** The story's Context section says the verbs arrive as
> "one implementation per verb in `Level`, zero-logic `Entity` forwarders." That is **superseded** by
> decision **D1** below: the physics-only verbs land on a new `PhysicsBody` handle instead, and
> `Level`/`Entity` gain no new verbs at all. Reasoning in §1; the story description gets a matching
> realignment note when this plan is posted.

---

## 0. The Problem, Ground Up

### What gameplay can do to a physics object today

Two things. That is the entire list:

```text
level.Teleport(entity, position)     // "be at these coordinates now"
level.RefreshCollider(entity)        // "your collision shape changed, re-read it"
```

That's it. Physics runs, and gameplay *watches*. Nothing in the engine can **push** anything. A
character that cannot be pushed cannot be controlled, so Reaper has no player movement — not because
the movement code is unwritten, but because there is no engine call for it to make.

### The failures, concretely

**No jump.** Jumping means "add roughly 5 m/s of upward velocity, right now, once." There is no verb
that says that. Teleporting the entity 1 unit up instead produces a body hanging in mid-air with zero
upward velocity, which then falls immediately — a hop that looks like a glitch, not a jump.

**No walking.** Holding a movement key means "keep pushing right while the key is down" — a sustained
push, re-applied every simulation step. No verb says that either.

**No moving platforms.** Say a platform must slide from x = 0 to x = 3 with a crate resting on it. The
only tool available is Teleport, which sets the platform's transform without giving it any velocity.
To the solver the platform did not *move* — it *appeared* somewhere else. Two things follow, both bad:
the friction that would drag the crate along never happens (friction acts between two surfaces with
relative velocity, and the platform's velocity is zero), so the crate stays behind and the platform
slides out from under it; and because the platform never travelled through the space between the two
poses, anything standing in that space is now inside it. Unity ships a whole separate API for this
reason — `Rigidbody.MovePosition` exists precisely because writing `transform.position` on a physics
body teleports it, and their docs say so.

**And the observed bug that named this story.** From RAD-28's verification run on 2026-07-10: spam the
`T` teleport cheat while the Reaper is landing. A landing body picks up a small angular velocity when
one corner touches first — say 1.4 rad/s. `Teleport` keeps velocity, deliberately, so the body arrives
at (4, 2) in open air still spinning at 1.4 rad/s and tumbles all the way down. That is *correct
physics and the wrong tool*: a portal should preserve your momentum, a respawn should not. One verb is
currently being asked to be both.

### The fix, in everyday terms

Think of the simulation as a swimming pool full of floating objects. Right now we can only lift an
object out and set it down somewhere else. We cannot push it. And there are four distinct ways to
push, which is the whole lesson of this story — mixing them up is the classic beginner physics bug:

| Everyday version | Physics name | Use it for | Applied |
|---|---|---|---|
| A hand pressed against the object, pushing steadily | **Force** (newtons, N) | wind, thrust, a held movement key | every step, while it lasts |
| A single hammer tap | **Impulse** (N·s) | a jump, an explosion, a bullet hit | once |
| Picking it up and throwing it at exactly this speed | **Set velocity** (m/s) | character controllers, conveyor belts | once, overwrites |
| A hand under it moving it along a track | **Kinematic target** | platforms, elevators, doors | every step |

The distinction that matters most: a **force** is spread over time, so its effect depends on how long
you push and how heavy the thing is. An **impulse** is instantaneous — it changes velocity *now*, by
`impulse / mass`. Setting **velocity** ignores mass entirely and throws away whatever the solver had
just computed; honest and blunt, which is exactly right for a character controller and wrong for
almost everything else.

The fourth is the platform case. A **kinematic** body is one the simulation moves but never pushes
back on — infinitely heavy, unstoppable, immune to gravity. The correct way to move one is not to set
its transform each step but to tell the solver **where you want it by next step** and let the solver
work out the velocity that gets it there. Now the platform genuinely *has* a velocity, so friction
carries the rider and contacts are generated along the way. Box2D v3 gives us
`b2Body_SetTargetTransform` for exactly this; Unreal's Chaos solver does the identical thing
internally — it moves the body to the target and derives the velocity from the move (§2).

And the teleport gets a second flavour: `KeepVelocity` (a portal — what we have) versus
`ResetVelocity` (a respawn — arrives still).

### What we gain, and what we deliberately do not

**Gain:** gameplay can push bodies, with four verbs whose semantics are distinct and documented;
kinematic platforms that carry their riders; respawn-style teleports that arrive without spin; and one
obvious place to *find* all of it (`entity.GetPhysicsBody()`).

**Not gained, on purpose:**

- **No character controller.** Jumping, coyote time, air control, ground checks — those are a *system*
  built on top of these verbs, not the verbs themselves. This story is the alphabet, not the sentence.
- **No un-doing a force.** Box2D v3.1 has no `ClearForces`; forces accumulate until the next step
  consumes them, so a `ResetVelocity` teleport zeroes velocity but does not cancel a force applied
  earlier in the same step.
- **No warning when you push a static body.** Box2D silently ignores it, and so will we (§10).
- **No new performance.** Every verb is O(1) and nothing is added to the step loop; this story adds
  *capability*, not speed.

**What we have:** a simulation gameplay can only watch and teleport things inside of. **What we're
building:** the write side of that boundary — the four ways to push, reached through one entity-scoped
handle.

---

## 1. Architecture Decision

### D1 — The verbs live on a `PhysicsBody` handle, not on `Level` and `Entity` (locked 2026-08-01)

The story assumed the RAD-28 shape: one implementation per verb in `Level`, plus a zero-logic `Entity`
forwarder. For `Teleport` that is right — teleporting writes the ECS transform *and* the render
snapshot *and* the physics pose, so `Level` (the only type owning all three) must own it. But eleven
dynamics verbs would ride along on that pattern while touching **no ECS state at all**, and the tax is
22 trivial functions plus doc comments on the two most widely-included headers in the engine.

Instead, `Entity` gains **one** accessor and the verbs live on the handle it returns:

```text
Entity::GetPhysicsBody()  ->  PhysicsBody { Entity }        // 16 bytes, owns nothing
     PhysicsBody::ApplyImpulse(v)
       └─ Level::ResolvePhysics(entity, "ApplyImpulse")     // private; friend access
            └─ PhysicsWorld2D::ApplyImpulse(entity, v)      // Box2D stops here
                 └─ b2Body_ApplyLinearImpulseToCenter(...)
```

This sharpens a rule that was previously accidental:

> **`Level` owns verbs that touch ECS state. `PhysicsBody` owns verbs that touch only physics state.**

`Teleport` and `RefreshCollider` stay on `Level` (+ the existing `Entity` forwarders) because they
write `TransformComponent` and `TransformSnapshotComponent`. Forces, impulses, velocity and kinematic
targets never touch the registry, so they have no business on the type that owns it.

The handle is a **value, not a cache**: `PhysicsBody` stores an `Entity` and nothing else, and
re-resolves the body on every call. Caching a `b2BodyId` would be faster by one array read and would
forfeit RAD-91's stale-recover-and-clear policy the moment a body died under it — a bad trade at that
exchange rate.

`operator bool()` answers "is this entity physics-capable *right now*" — live handle, live Level with a
world, `RigidBody2DComponent` present. It deliberately does **not** check whether the packed id names a
live Box2D body; that check belongs to the verb, because it is the one that can warn, clear the stale
field and recover (playbook §4). So the bool is cheap, vendor-free, and honest about what it promises.

**What this is not.** It is not UE's `FBodyInstance`, despite the resemblance (§2). `FBodyInstance` is a
heavyweight record that *owns* a body's setup data and lives as a member of a component. `PhysicsBody`
owns nothing, stores nothing, and is constructed on demand — it is the *accessor* half of
`FBodyInstance`, not the record half. Radiant's record half is still the plain-data component holding
a packed id (playbook §3), and this story does not change that.

### D2 — One resolution helper per layer, never one per verb

RAD-91's lesson, applied at every boundary this story crosses. Each layer has exactly one way to fail,
so each layer gets exactly one helper that owns that failure:

| Layer | Its one question | Its one helper | Verb body |
|---|---|---|---|
| `PhysicsBody` | is the entity live, and does its Level have a world? | `Level::ResolvePhysics(entity, verb)` | 2 lines |
| `PhysicsWorld2D` | is there a component, and is its id live? | `ResolveBody(entity, verb)` | 3 lines |

`ResolveBody` is **not** a second copy of RAD-91's policy — it is a component-fetching wrapper *over*
`ResolveBodyId`, which stays exactly as RAD-91 left it for the callers that already hold a component
reference (the entt signal handlers, `Teleport`). One policy, two entry points, chosen by what the
caller already has.

### D3 — Teleport becomes a two-value enum, not a bool

`Teleport(pos, 0.f, true)` is unreadable at the call site; `Teleport(pos, 0.f, TeleportType::ResetVelocity)`
reads itself. This is UE's `ETeleportType` minus its third mode (§2). The enum needs a home both
`Level.h` and `PhysicsWorld2D.h` can include — `PhysicsWorld2D.h` carries `<box2d/id.h>` and `Level.h`
deliberately does not, so it cannot live inside `PhysicsWorld2D`. New header `Physics/TeleportType.h`,
the same solution `ContactEvent.h` already is for `ContactPhase` (RAD-29).

### D4 — Units are 2D everywhere except Teleport

Dynamics verbs take `glm::vec2` — force, impulse and velocity are physics quantities and the physics is
two-dimensional. `Teleport` keeps its `glm::vec3` because it writes the ECS `TransformComponent`, whose
z the renderer uses for layering.

### D5 — Every verb wakes, and there is no `wake` parameter

Box2D's force and impulse functions take a `bool wake`; passing `false` means "apply this to a sleeping
body", and a sleeping body ignores forces entirely (`box2d.h:290`) — so `wake = false` is a silent
no-op with no customer. We pass `true` and do not export the flag. UE's `SetLinearVelocity` shows the
alternative: it accepts a `bAutoWake` parameter and never reads it (§2) — a parameter that lies.

**Playbook sections that apply:** §4 (physics integration rules — verb-surface shape, guard semantics),
§3 (components stay plain data — verbs live on the handle, never on components), §2 (ownership —
`PhysicsBody` is a value handle owning nothing, the third such type after `Entity` and `TimerHandle`),
§1 (fixed timestep — `MoveKinematic` needs the fixed delta and must never see a frame delta).

---

## 2. UE Reference

**Teleport semantics.** UE has had exactly this two-flavour distinction since forever, plus a third
mode we do not need (`None` = a *swept* move that collides along the way — Radiant has no sweep API
until RAD-76):

```cpp
// Engine/Source/Runtime/Engine/Classes/Engine/EngineTypes.h:2398
/** Whether to teleport physics body or not */
UENUM()
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

We adopt the two-value core (`KeepVelocity` = `TeleportPhysics`, `ResetVelocity` = `ResetPhysics`) and
deliberately skip `None`.

**The handle, and what we take from it.** `FBodyInstance` is UE's per-body record, and every verb on it
reaches the solver object through one accessor — `GetPhysicsActor()`:

```cpp
// Engine/Source/Runtime/Engine/Private/PhysicsEngine/BodyInstance.cpp:3695
void FBodyInstance::AddForce(const FVector& Force, bool bAllowSubstepping, bool bAccelChange, ...)
{
    const bool bIsInternal = TimeStamp.IsValid();
    ApplyAsyncPhysicsCommand(TimeStamp, bIsInternal, PlayerController, [&, Force, ...]()
    {
        if (FPhysicsInterface::IsInScene(GetPhysicsActor()) && IsBodyDynamic(GetPhysicsActor(), bIsInternal))
        {
            FPhysicsInterface::AddForce_AssumesLocked(GetPhysicsActor(), Force, bAllowSubstepping, bAccelChange, bIsInternal);
        }
    });
}
```

Two things to take from this. First, **UE repeats its guard in every verb too**, and it costs one line,
because resolution and validity are already factored into an accessor plus a predicate — the goal was
never "never repeat the guard", it is that the repeated part is one legible line. Ours is
`ResolveBody(entity, "ApplyForce")` plus a null check. Second, **the verbs hang off a per-body handle
rather than off the world or the actor**, which is precisely the shape `PhysicsBody` adopts (D1).

Both directions of the entity↔body mapping already exist in Radiant, and both stay private:

| UE | Direction | Radiant | Where |
|---|---|---|---|
| `FBodyInstance::GetPhysicsActor()` | engine record → backend object | `ResolveBody(entity, verb)` → `b2BodyId` | file-local, `PhysicsWorld2D.cpp` |
| `FBodyInstance::GetOwner()` | backend object → gameplay object | UUID in `b2Body userData` → `Level::GetEntityByUUID` | `ResolveEntityFromShape`, RAD-29 |

Ours never become public API: `GetPhysicsActor()` hands out a Chaos handle, and the Radiant equivalent
would hand out a `b2BodyId` — a vendor type the boundary rule keeps inside `Physics/`.

**What we deliberately decline.** `ApplyAsyncPhysicsCommand` and `FPhysicsCommand::ExecuteWrite` exist
to marshal a write onto the physics thread or bracket a scene lock; Radiant is main-thread-only with
nothing to marshal or lock. And note `FBodyInstance::SetLinearVelocity(const FVector& NewVel, bool
bAddToCurrent, bool bAutoWake)` (`BodyInstance.cpp:3557`) — `bAutoWake` never appears in the function
body. A parameter kept for source compatibility that silently does nothing is exactly the trap D5
avoids by not exposing a wake flag at all.

**The kinematic mover.** Chaos does internally what `b2Body_SetTargetTransform` does for us — moves the
body to the target and *derives* the velocity from the move, so the rest of the solver sees a body that
genuinely moved:

```cpp
// Engine/Source/Runtime/Experimental/Chaos/Private/Chaos/PBDRigidsEvolutionGBF.cpp:1180
case EKinematicTargetMode::Position:
{
    // Move to kinematic target and update velocities to match
    ...
    if (Dt > MinDt)
    {
        if (bPositionChanged)
        {
            NewV = FVec3::CalculateVelocity(CurrentX, NewX, Dt);
        }
```

Box2D's version is the same arithmetic, four lines of C (`src/body.c`, `b2Body_SetTargetTransform`):
`linearVelocity = (1/timeStep) * (targetCenter - currentCenter)`.

**One difference we adopt deliberately.** Chaos flips the target to `EKinematicTargetMode::Reset` once
reached, and `Reset` zeroes the velocities on the following step (`PBDRigidsEvolutionGBF.cpp:1170`) — so
a UE kinematic body that stops receiving targets stops moving. Box2D has no such auto-stop: it sets the
velocity and walks away, so a Radiant kinematic body that stops receiving `MoveKinematic` calls **keeps
sailing at its last velocity**. We document that contract loudly rather than building UE's target-mode
state machine, which needs per-body bookkeeping and has no customer yet (§9).

**Where UE warns and we do not.** `UPrimitiveComponent::AddImpulse` (`PrimitiveComponentPhysics.cpp:200`)
calls `WarnInvalidPhysicsOperations` before forwarding — UE's answer to "why isn't my force doing
anything", and it is **editor-only**. Radiant has no editor yet, so we take the silent path and record
the idea in Known Issues (§10).

---

## 3. File Plan

```text
Radiant/Source/Radiant/Physics/
├── TeleportType.h        (new)    — KeepVelocity/ResetVelocity; the one header both layers can include
├── PhysicsWorld2D.h      (modify) — 11 verb declarations; Teleport gains TeleportType
└── PhysicsWorld2D.cpp    (modify) — ResolveBody helper over RAD-91's ResolveBodyId; one 3-line verb body each

Radiant/Source/Radiant/ECS/
├── PhysicsBody.h         (new)    — the entity-scoped verb handle; no vendor types
├── PhysicsBody.cpp       (new)    — 2-line bodies through Level::ResolvePhysics
├── Entity.h              (modify) — one accessor: GetPhysicsBody(); Teleport gains TeleportType
├── Entity.cpp            (modify) — GetPhysicsBody + Teleport pass-through
├── Level.h               (modify) — private ResolvePhysics; friend PhysicsBody; Teleport gains TeleportType
└── Level.cpp             (modify) — ResolvePhysics; Teleport pass-through

Reaper/Source/                      ← all scaffolding: separate commit, retires with RAD-92
├── KinematicPlatform.h   (new)    — oscillating platform script driven by MoveKinematic
└── Layers/GameLayer.cpp  (modify) — Space (impulse) / R (reset-teleport) cheats, platform + crate spawn

Docs/Physics.md           (modify) — dynamics-verb section, Status line, Known Issues bullet retired
.claude/references/radiant-playbook.md (modify) — §4: the PhysicsBody split rule, one resolver per layer
```

| Action | Path | Description |
|--------|------|-------------|
| Create | `Radiant/Source/Radiant/Physics/TeleportType.h` | Two-value enum, no includes — the `ContactEvent.h` precedent for a type both `Level.h` and `PhysicsWorld2D.h` must name |
| Create | `Radiant/Source/Radiant/ECS/PhysicsBody.{h,cpp}` | The gameplay verb surface (D1). Lives in `ECS/`, not `Physics/`, so the one-way ECS→Physics header dependency survives — it holds an `Entity` by value, which would otherwise force `Physics/` to include `ECS/Entity.h` |
| Modify | `Radiant/Source/Radiant/Physics/PhysicsWorld2D.{h,cpp}` | 11 new verbs + `TeleportType` on `Teleport`; `ResolveBody` helper |
| Modify | `Radiant/Source/Radiant/ECS/Level.{h,cpp}` | Private `ResolvePhysics`, `friend class PhysicsBody`, `Teleport` overloads take `TeleportType`. **No new public verbs** |
| Modify | `Radiant/Source/Radiant/ECS/Entity.{h,cpp}` | `GetPhysicsBody()` (forward-declares `PhysicsBody`; defined in the .cpp, so no include cycle) and `Teleport`'s new parameter |
| Create | `Reaper/Source/KinematicPlatform.h` | RAD-90 verification scaffolding — an oscillating platform that carries a crate |
| Modify | `Reaper/Source/Layers/GameLayer.cpp` | Cheat keys + scaffolding entities (retires with RAD-92) |
| Modify | `Docs/Physics.md`, `.claude/references/radiant-playbook.md` | The doc-update contract (CLAUDE.md) |

---

## 4. Type Design

### TeleportType
- **Kind:** `enum class` (`uint8_t`)
- **Responsibility:** say which of the two teleport semantics the caller wants.
- **Ownership / lifetime:** a value; no state.
- **Key members:**
  - `KeepVelocity` — default. Momentum survives the jump. Portal semantics; what `Teleport` does today.
  - `ResetVelocity` — linear and angular velocity zeroed on arrival. Respawn semantics.
- **Playbook patterns:** §4 (physics types stay in `Physics/`; no vendor include in this header).

### PhysicsBody
- **Kind:** class — a **value handle**, copied freely, 16 bytes (one `Entity`).
- **Responsibility:** the gameplay-facing verbs that touch only physics state.
- **Ownership:** owns nothing. The Level owns the world; the world owns the body; the component holds
  the claim ticket. This type holds an entity handle and a phone number.
- **Lifetime & threading:** transient, like `Entity` — valid for the scope you obtained it in; never
  store one across a fixed update. Main-thread only.
- **Key members:**
  - `operator bool()` / `IsValid()` — live entity + live Level with a world + `RigidBody2DComponent`
    present. Does **not** probe the packed id (D1).
  - The eleven verbs below.
- **Playbook patterns:** §2 (one ownership story, stated at the declaration — this one owns nothing),
  §4, §3.

### PhysicsWorld2D — the new verb surface

| Verb | Box2D call | Units |
|---|---|---|
| `ApplyForce(e, force)` | `b2Body_ApplyForceToCenter(body, f, true)` | N |
| `ApplyForceAtPoint(e, force, worldPoint)` | `b2Body_ApplyForce(body, f, p, true)` | N, world units |
| `ApplyTorque(e, torque)` | `b2Body_ApplyTorque(body, t, true)` | N·m |
| `ApplyLinearImpulse(e, impulse)` | `b2Body_ApplyLinearImpulseToCenter(body, i, true)` | N·s |
| `ApplyLinearImpulseAtPoint(e, impulse, worldPoint)` | `b2Body_ApplyLinearImpulse(body, i, p, true)` | N·s, world units |
| `ApplyAngularImpulse(e, impulse)` | `b2Body_ApplyAngularImpulse(body, i, true)` | kg·m²/s |
| `SetLinearVelocity(e, velocity)` | `b2Body_SetLinearVelocity` | m/s |
| `SetAngularVelocity(e, angularVelocity)` | `b2Body_SetAngularVelocity` | rad/s |
| `GetLinearVelocity(e)` → `glm::vec2` | `b2Body_GetLinearVelocity` | m/s |
| `GetAngularVelocity(e)` → `float` | `b2Body_GetAngularVelocity` | rad/s |
| `MoveKinematic(e, position, rotation, fixedDelta)` | `b2Body_SetTargetTransform` | world units, rad, s |
| `Teleport(e, position, rotation, teleportType)` | `b2Body_SetTransform` + wake (+ zero velocities) | world units, rad |

- **Getters are non-`const`** on both `PhysicsWorld2D` and `PhysicsBody` (locked during RAD-91,
  2026-08-01). Resolution *clears* a stale id as part of recovering from it, which a `const` method
  cannot do. A `const` overload that warned without clearing would leave the dead id in place to warn
  again on every subsequent call — log spam plus a permanently broken field. `const` here would be
  claiming "this call only observes", and the call does not only observe. `Entity::GetUUID()` is
  non-`const` for the same family of reason. Both getters return zero when there is no live body.

### The two resolvers

| Helper | Home | Returns | Failure policy |
|---|---|---|---|
| `Level::ResolvePhysics(Entity, const char* verb)` | private member; `PhysicsBody` is a friend | `PhysicsWorld2D*` | invalid handle → WARN naming the verb + null; no world (scratch level) → null, silent |
| `ResolveBody(Entity, const char* verb)` | file-local, `PhysicsWorld2D.cpp` | `b2BodyId` | no component → formatted WARN + assert + null; then defers to `ResolveBodyId` |

---

## 5. Implementation Steps

### Phase 1 — Teleport semantics

- [x] **`Physics/TeleportType.h`** — the two-value enum with a doc comment stating exactly what each
      mode does to velocity. No includes. Modelled on `ContactEvent.h`'s reason for existing.
      *(Landed with `<cstdint>` only — needed for the explicit `uint8_t` underlying type, which the
      header must have to be self-contained; no engine or vendor includes.)*
- [x] **`PhysicsWorld2D::Teleport` takes `TeleportType`** — order inside matters and needs an inline
      comment: `b2Body_SetTransform` → `b2Body_SetAwake(true)` → *then* zero the velocities.
      Backwards, the zeroing is silently dropped: v3 only wakes on a **nonzero** velocity
      (`body.c`, `b2Body_SetLinearVelocity`), and a sleeping body has no `b2BodyState` to write into.
      Extend the existing TRACE to name the mode.
- [x] **`Level::Teleport` + `Entity::Teleport` gain the parameter**, defaulted to
      `TeleportType::KeepVelocity` so every existing call site keeps its meaning. Both overloads
      (with and without `rotationZ`) on both types. *(No ambiguity: the 3-arg `float` overload has no
      default for `rotationZ`, and `TeleportType` is an enum class so it never converts to `float`.)*

### Phase 2 — The handle and the resolvers

- [x] **`ResolveBody(entity, verb)`** in `PhysicsWorld2D.cpp`'s anonymous namespace, above the verbs.
      Missing `RigidBody2DComponent` → `RADIANT_WARN` naming the verb and entity (formatted; survives
      Dist) followed by `RADIANT_ASSERT(false, "…")` with a literal message — `RADIANT_ASSERT` takes no
      format arguments (`Assert.h`), the same split `CreateBody` already uses.
- [x] **`Level::ResolvePhysics` + `friend class PhysicsBody`**, and `PhysicsBody` itself with its
      `operator bool` and one verb (`ApplyLinearImpulse`) to prove the chain end to end before the
      other ten are mechanical.
      *(Deviation: `PhysicsBody` also needed `friend class PhysicsBody` on **`Entity`**, to read
      `m_Level`. The alternative — storing a `Level*` in `PhysicsBody` alongside the `Entity` that
      already holds one — keeps the same pointer in two places to avoid one friend declaration
      between two types in the same module. Both friendships are documented at their declarations.)*
      *(Deviation: the plan put the null-level guard on `Entity::LevelOrWarn`. With no new `Entity`
      forwarders it has no home there, so it folded into a private `PhysicsBody::Resolve(verb)` —
      which is the better place anyway: it makes this layer's guard one function, matching the other
      two layers, instead of a repeated ternary in eleven verbs.)*
- [x] **`Entity::GetPhysicsBody()`** — `Entity.h` forward-declares `class PhysicsBody;` and returns it
      by value; the definition lives in `Entity.cpp`, which includes `PhysicsBody.h`. That ordering is
      what keeps the two headers from including each other.

### Phase 3 — The verbs

- [x] **The eight force/impulse/velocity verbs on `PhysicsWorld2D`** — each is resolve, `B2_IS_NULL`
      skip, one Box2D call, `wake = true`. No per-verb guard block; no TRACE (§8).
- [x] **`MoveKinematic`** — takes the fixed delta as a parameter; `PhysicsBody` supplies it from
      `Time::GetFixedDeltaTime()`. Asserts the body is kinematic (`b2Body_GetType`, a
      pure read, so the expression is side-effect-free when it compiles out in Dist) and continues.
- [x] **Mirror all eleven onto `PhysicsBody`** — two lines each. Doc comments state units, wake
      behaviour, and *when the effect is observed*: a verb called from a script's `OnUpdate` (before the
      step) lands in the step about to run; one called from a collision handler (after the step) lands
      in the next one.
- [x] **Three-config build clean** (2026-08-01): Debug, Release and Dist all link. Zero errors and zero
      new warnings — the six `C4267` in `Platform/OpenGL/OpenGLFrameBuffer.cpp` are pre-existing, in a
      file this story does not touch (confirmed against `git status`).

### Phase 4 — Docs and verification scaffolding

- [x] **`Docs/Physics.md` + playbook §4** — new "Dynamics verbs (RAD-90)" section covering the four
      push kinds, the `Level`-vs-`PhysicsBody` split rule, the wake rule, the kinematic every-step
      contract, and the non-`const` getter decision; Status line updated; the "No gameplay dynamics
      verbs yet (RAD-90)" Known Issues bullet replaced by the honest residue (no auto-stop for
      kinematics, no static-body warning, no ClearForces, no character controller, no body-state
      accessors). Playbook §4 gained five rules: the verb split, one-resolver-per-layer, the
      log-rare-not-routine rule, physics-takes-application-state-as-parameters, and enums-not-bools.
      *(Added beyond plan: `Docs/ECS-And-Levels.md` — `Entity` grew a public accessor and the ECS
      layer gained a new public type, so the ECS doc owed a subsystem-facade paragraph under the
      same update contract.)*
- [x] **Reaper scaffolding — its own commit, and RAD-92's scope amended first.** `Space` = impulse hop;
      `V` = the same impulse at a corner point (the torque proof); `R` = `ResetVelocity` teleport (kept
      alongside `T`, which stays `KeepVelocity`, so the two are comparable live); `KinematicPlatform`
      script + a `PlatformRider` crate. Every piece carries the "retires with RAD-92" comment.
      *(A separate kinematic platform was added rather than converting the existing static "Platform" —
      that one is the landing surface for the RAD-28/RAD-29 cheats, and making it move would have
      quietly changed what they test.)*
- [x] **Deviation — `ScriptableEntity::GetPhysicsBody()` added (engine change beyond the plan).**
      `ScriptableEntity::m_Entity` is private with only `friend class Level`, so a script could not
      reach its own `Entity` and therefore could not reach **any** RAD-90 verb — the story's whole
      surface was unreachable from the only gameplay code the engine has. Added as a fourth forwarder
      alongside the existing `AddComponent`/`GetComponent`/`RemoveComponent`. This is the physics half
      of the gap filed as **RAD-95** and deliberately does not pre-empt that card's larger question
      (what else a script may reach, including its Level).

---

## 6. Ownership & Lifetime Strategy

Nothing in this story allocates, owns, or frees anything. `PhysicsBody` is the third value handle in
the engine after `Entity` and `TimerHandle`, and like them it owns nothing: it stores one `Entity` and
re-resolves through the Level on every call. Every verb is a stateless translation into one Box2D call.
The only state touched already had an owner:

- **Bodies and shapes** — owned by the world, as before. The verbs read ids and never create or destroy.
- **The packed ids in components** — still claim tickets, still zeroed only by the resolver's stale path
  and the destroy verbs (playbook §4).
- **The fixed delta** — read from `Time` at the moment of the call and passed down by value.
  `PhysicsWorld2D` stores no clock and holds no reference to one, which keeps it constructible in a
  unit test with no `GameApplication` (RAD-67).

Two hazards worth naming, both inherited rather than introduced:

- **A stored `PhysicsBody` can outlive its entity**, exactly as a stored `Entity` can. The doc comment
  states the same transient contract Entity's does: obtain it, use it, drop it — never cache it across
  a fixed update. Misuse is caught and warned, not crashed (`ResolvePhysics` → `IsValid`).
- **`friend class PhysicsBody` on `Level`** widens the friend list to two (`Entity` was already there).
  The friendship buys exactly one thing — `ResolvePhysics` stays private, so the physics world is not
  exposed to the world at large just to make the verbs reachable.

---

## 7. Performance Notes

Each verb is a fixed, allocation-free sequence: one entt sparse-set lookup (`TryGetComponent`), one
`b2Load*Id` decode plus a validity check, one Box2D call. Tens of nanoseconds, no branching on
collection size, nothing cached.

- **Nothing is added to the step loop.** `Step`, the move drain and the contact drain are untouched.
- **`PhysicsBody` costs nothing to create** — a 16-byte copy of an `Entity`, constructed in a register.
  It is not an allocation, not a lookup, and not a cache that can go stale.
- **`GetUUID()` stays in the cold branch.** RAD-91's reason holds and now applies eleven more times:
  `RADIANT_WARN` survives Dist, so its arguments are evaluated in every config — passing `Entity`
  rather than a UUID keeps that component lookup off every verb's hot path.
- **The only per-step-per-entity caller is `MoveKinematic`** (one per moving platform) and whatever a
  character controller does with forces (one or two per character per step). Both are far below the
  noise floor next to the step itself.
- **`b2Body_GetType` in `MoveKinematic`'s assert** compiles out in Dist and is a single array read in
  the other configs.
- **Time dilation is free.** Forces are applied per fixed step and the fixed delta never changes at any
  time scale (`FrameClock` scales the deposit, not the step), so a force applied every step stays
  physically correct in slow motion without anything in this story knowing about it.

---

## 8. Logging & Diagnostics

The rule this story establishes: **verbs that are rare and discontinuous log; verbs that are routine
and continuous do not.**

- **`Teleport` keeps its TRACE** and extends it to name the mode — a teleport is a deliberate act and
  the log is the audit trail of every explicit ECS→Box2D push (RAD-28).
- **No force/impulse/velocity/kinematic TRACE.** A character controller applies a force every step at
  60 Hz; logging it would drown every other line in the file and slow Debug measurably.
- **Missing `RigidBody2DComponent`** → formatted `RADIANT_WARN` naming the verb and entity, then
  `RADIANT_ASSERT(false, …)`. Programmer error (the call site believes it is pushing a physics object
  that is not one), so it breaks in Debug/Release and recovers everywhere.
- **Stale id** → the RAD-91 resolver's existing WARN + clear + recover, now naming eleven more verbs.
- **Invalid entity handle** → `Level::ResolvePhysics` warns naming the verb, matching
  `DestroyEntity`/`Teleport`/`RefreshCollider`.
- **Silent by design:** a force on a static or sleeping-and-not-woken body, and a scratch level with no
  world. Rationale in §10.

---

## 9. Scalability Review

**The handle is the scalability answer, and it was adopted rather than deferred.** The alternative
shape — a verb on `Level` plus a forwarder on `Entity` for each operation — cost three edits per new
physics operation and 22 trivial functions on the two most widely-included headers in the engine.
`PhysicsBody` makes it two edits and one gameplay-facing surface. Remaining flags:

1. **A data-driven alternative would be worse, not better.** The obvious "scalable" refactor —
   `ApplyPhysicsCommand(CommandType, vec2, float)` behind a switch — throws away compile-time argument
   checking, makes units ambiguous at the call site (is that scalar a torque or an angular impulse?),
   and replaces legible functions with a switch ladder that grows just as fast. Explicitly rejected.
2. **`PhysicsBody` is the template for the next two surfaces, and that is deliberate.** RAD-76's query
   API is **not** a per-entity verb surface — a raycast belongs to the world, not to an entity — so it
   should arrive as its own facade (`Level::GetPhysicsQuery()` or similar), never as `Level::Raycast`.
   The precedent this story sets is *what kind of thing gets a handle*, which makes that decision
   easier rather than pre-empting it.
3. **Body-state accessors are the obvious next growth** — `GetMass`, `IsAwake`, `GetBodyType`,
   `SetGravityScale`, damping. All one-liners on `PhysicsBody` when a customer appears; none has one
   today, so none are in scope. The handle means adding them later costs one edit each.
4. **Kinematic auto-stop, flagged not built.** UE's `EKinematicTargetMode::Reset` needs per-body
   bookkeeping — "which bodies received a target this step, zero the rest." That is a kinematic-mover
   *system*, and systems keyed by entity belong in the Level-owned side tables RAD-30 introduces.
   Document the every-step contract now; build the system when a game needs it.

---

## 10. Risks & Edge Cases

- **Forces on a static body vanish silently.** Box2D checks `setIndex == b2_awakeSet` and returns; a
  static body is never in that set. We match UE's runtime behaviour (`IsBodyDynamic` guard, silent
  return) rather than warning, because the only call site that could produce it — a per-step force
  loop — would emit 60 warnings a second. UE's editor-only `WarnInvalidPhysicsOperations` is the right
  eventual answer and lands with the Phase 5 editor; recorded in Known Issues.
- **`SetLinearVelocity({0,0})` does not wake.** v3 wakes only on a nonzero velocity, and a sleeping body
  has no `b2BodyState` to write into — so "stop this body" is a no-op on a sleeping body. Harmless (it
  is already stopped) but it is exactly why `Teleport`'s ordering is wake-then-zero (§5, Phase 1).
- **`ResetVelocity` does not clear accumulated forces.** v3.1 exposes no `ClearForces`; forces sit in
  `bodySim->force` until the next step consumes them. A script that applies a force and then
  reset-teleports in the same step gets a body that arrives still and is immediately pushed.
  Contradictory instructions, documented rather than silently reconciled.
- **A kinematic body that stops receiving `MoveKinematic` keeps moving forever** at its last computed
  velocity (§2). The doc comment must say this in the first sentence; the Reaper platform script is the
  worked example of the every-step contract.
- **`b2Body_SetTargetTransform` silently declines a sub-sleep-threshold move** (`body.c`: "Return if
  velocity would be sleepy"). A platform asked to move a very small distance per step will not move at
  all rather than moving slowly — surprising, vendor behaviour, documented.
- **`MoveKinematic` before the first fixed update.** `Time::GetFixedDeltaTime()` is valid as soon as the
  `GameApplication` exists, so there is no zero-delta window — this is precisely why the delta comes
  from `Time` rather than from a "last step's ts" member, which *would* have one.
- **A stored `PhysicsBody` outliving its entity** — see §6. Same contract and same failure mode as a
  stored `Entity`; warned, not crashed.
- **Verb called from a collision handler.** Legal and safe — the world is idle during dispatch (RAD-29)
  — but the effect lands in the *next* step, one step later than the same call from `OnUpdate`. A
  documented asymmetry, not a bug.
- **Include-cycle risk between `Entity.h` and `PhysicsBody.h`.** Resolved by declaration order (§5,
  Phase 2): `Entity.h` forward-declares `PhysicsBody`, `PhysicsBody.h` includes `Entity.h`, and
  `Entity::GetPhysicsBody` is defined in the .cpp. Getting this backwards produces a wall of
  incomplete-type errors, so it is called out as a step rather than left to discovery.
- **New Reaper scaffolding becomes permanent debt** unless RAD-92 is amended before it lands. Mitigation
  is procedural and in the steps: amend RAD-92 first, scaffolding in its own commit, "retires with
  RAD-92" comment on every piece.

---

## 11. Verification (AC → proof)

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| `Teleport` takes `TeleportType` (default `KeepVelocity`); `ResetVelocity` arrives with zero linear and angular velocity — the RAD-28 mid-landing tumble is dead under it | Reaper: spam `T` during a landing → body tumbles (unchanged, `KeepVelocity`). Spam `R` at the same moment → body arrives motionless every time. Both keys exist so the difference is observed, not asserted |
| Force/impulse/velocity verbs are reachable from gameplay through one entity-scoped handle; at-point overloads generate torque | Reaper: `Space` applies a centre impulse via `entity.GetPhysicsBody().ApplyLinearImpulse(...)` → the Reaper hops straight up and gravity returns it. A second cheat applies the same impulse at a corner point → the body hops **and spins**, proving the torque term |
| `MoveKinematic` uses `b2Body_SetTargetTransform` with the fixed delta; a kinematic platform pushes and carries dynamic bodies | Reaper: `KinematicPlatform` oscillates horizontally with a crate resting on it → the crate rides along instead of being left behind or passed through |
| All verbs wake the body; guard conventions match RAD-28/RAD-91 — each verb resolves through the shared helper rather than carrying its own guard block | Let the square settle until the "fell asleep" TRACE appears, then `Space` → it wakes and hops. Code review: every `PhysicsWorld2D` verb body is resolve → `B2_IS_NULL` skip → one Box2D call, no per-verb guard |
| Getters are non-`const` so stale-id recovery still clears | Signature review; §4's rationale is the record. Remove a rigidbody at runtime and read velocity → one WARN, field cleared, second read silent |
| Box2D types never leave `Physics/`; components remain plain data | `PhysicsBody.h`, `Level.h` and `Entity.h` name only `glm`, `Entity`, `PhysicsBody` and `TeleportType`; no `b2*` outside `Physics/`. `Components.h` unchanged by this story |
| `Level` and `Entity` gain no dynamics verbs — the ECS-touching / physics-only split holds | Header review: `Level`'s public surface grows by zero verbs; `Entity`'s by one accessor |
| Compiles clean in Debug/Release/Dist, no new warnings | CLI MSBuild all three configurations (CLAUDE.md Build & Run) — Dist especially, since the new asserts and WARNs straddle the config boundary |
| Reaper runs and is visually verified | Interactive run exercising `Space`, `T`, `R`, the platform, and the sleeping-body wake; log scanned for unexpected `[W]`/`[E]` |
| Ownership/units documented on every new verb | Doc-comment review: every verb states units (N / N·s / N·m / kg·m²/s / m/s / rad/s), wake behaviour, when the effect is observed, and its failure semantics; `PhysicsBody`'s declaration states its owns-nothing/transient contract; `Docs/Physics.md` and playbook §4 updated in the same change |