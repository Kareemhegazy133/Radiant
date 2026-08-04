# Gameplay Framework

**Status:** Stable — the base class, the named verb surface and the `Gameplay/` module landed 2026-08-04 (RAD-99). Behaviour composition (RAD-101), typed retrieval (RAD-100) and possession (RAD-102) are planned; the hierarchy grows `Pawn`/`Controller` on top of what is here.

## The Problem This Solves

An engine has to answer one question before a game can be written in it: **what does a gameplay programmer actually type?**

Radiant stores the world as an ECS — entities are ids, components are plain data, and systems are loops (see [ECS-And-Levels](ECS-And-Levels.md)). That is an excellent way to *store* a world and a poor way to *talk about* one. Storage vocabulary leaks into every sentence:

```cpp
// "where am I?" — three concepts to say one thing
glm::vec3 here = GetOwner().GetComponent<TransformComponent>().Translation;
```

You had to know that position lives in a type called `TransformComponent`, that the field is spelled `Translation` and not `Position`, and that a template lookup sits between you and it.

Reading is merely verbose. **Writing is a trap.** Here is a behaviour respawning its entity at the origin:

```cpp
GetOwner().GetComponent<TransformComponent>().Translation = { 0.0f, 0.0f, 0.0f };
```

Compiles. Runs. Does nothing — if the entity has a physics body. Physics owns the transform of a body-backed entity, so every fixed step Box2D writes its own pose back over that field. At a 60 Hz step the value survives **16.6 ms**, is never rendered (the render pass reads the transform *after* the readback), and produces no error, no warning, no assert. The crate simply stays where it was. Unity ships the identical trap — `transform.position` on a `Rigidbody` object versus `Rigidbody.MovePosition` — and it is the single most-answered question in its physics forum.

**The fix is a front desk.** Picture a hotel lobby with a hundred numbered pigeonholes behind it: keys, mail, billing slips. You *can* walk behind the desk and take what you want out of pigeonhole 47 — and if you take the key without telling the register, the room shows as vacant while you are asleep in it. The answer is not to lock the pigeonholes. It is to put someone at the desk who knows the few things guests actually ask for and who does the *whole* operation each time, register included. The pigeonholes stay open behind them for the rare request nobody anticipated.

The desk is a small set of **named gameplay verbs**. The pigeonholes are `GetComponent<T>()` — still there, one hop further away, for everything the desk does not cover.

## Architecture

### The object model, in one sentence

> **`Entity` is Radiant's `AActor`. `EntityBehaviour` is Radiant's `UActorComponent`.**

Once that is accepted the rest follows mechanically. The **entity** is the world-thing: it has a location, it is destroyed, it belongs to a level — and it exists whether or not any C++ behaviour is attached. Most entities in Reaper have none. A **behaviour** is one unit of C++ gameplay attached to an entity, receiving lifecycle callbacks from the engine.

That last fact is why the base class is *not* called `Actor`: a type that is optional on the things it would claim to be cannot be those things. Naming it `Actor` would name the *script on the crate* as the crate. (The predecessor name, `ScriptableEntity`, came from Hazel and described a mechanism rather than a role.)

| Unreal | Radiant | Role |
|--------|---------|------|
| `AActor` | `Entity` | the world-thing: has a location, is destroyed, belongs to a world |
| `UActorComponent` | `EntityBehaviour` | a behaviour unit: lifecycle callbacks, attached to a world-thing |
| `UWorld` | `Level` / `GameplayLevel` | the world |
| `GetOwner()` | `EntityBehaviour::GetOwner()` | the behaviour's entity |
| `FindComponentByClass<T>()` | `Entity::GetComponent<T>()` | the escape hatch |
| `GetActorLocation()` | `Entity::GetLocation()` | a named gameplay verb |

**One difference is structural, not stylistic, and it explains the whole shape of this framework.** In Unreal you get unqualified verbs — `SetActorLocation()` with no receiver — by *inheriting the world-thing*: `AMyCharacter : ACharacter : APawn : AActor`, so you **are** the actor. That path is closed to us. Radiant's world-thing is an `entt::entity` id in a registry private to `Level` — it is *data*, not a class, so nothing can inherit it. Unreal buys that ergonomic shortcut with actor-IS-storage and pays for it with a reflection system; Radiant declined that trade deliberately (RAD-98), which places us in `UActorComponent`'s seat by construction. And `UActorComponent`'s answer is `GetOwner()->` — which is our `GetOwner().`.

Worth knowing, because it is easy to assume UE has a shorthand we are refusing: `UActorComponent` has **no transform verbs at all**. `ActorComponent.h` contains zero occurrences of "Location". A component asks its owner.

### `EntityBehaviour` — the behaviour base

```cpp
class EntityBehaviour        // two accessors and five hooks — the whole class
{
public:
    Entity        GetOwner() const;    // my entity
    GameplayLevel GetLevel() const;    // the world it is in
protected:
    virtual void OnCreate() {}
    virtual void OnUpdate(Timestep ts) {}
    virtual void OnDestroy() {}
    virtual void OnCollisionBegin(Entity other) {}
    virtual void OnCollisionEnd(Entity other) {}
};
```

Everything a behaviour can reach chains off those two accessors:

```cpp
GetOwner().GetLocation()                      // named gameplay verbs
GetOwner().GetComponent<SpriteComponent>()    // the escape hatch
GetOwner().GetPhysicsBody().ApplyForce(...)   // my physics
GetLevel().CreateEntity("Bullet")             // the world
```

`GetOwner()` is named for a fact, not for parity: the entity's `NativeScriptComponent` holds the instance by an owning pointer and deletes it, so the entity genuinely *owns* the behaviour in the sense [Memory-And-Reference-Counting](Memory-And-Reference-Counting.md) uses the word. UE's `UActorComponent::GetOwner()` happens to agree.

**The inclusion rule — what may ever be added here.** Exactly two kinds of member:

1. A **lifecycle hook** — a method the engine calls on you at a defined point in the frame.
2. A **relationship accessor** — navigation to something the behaviour genuinely has a relationship with. That set is *bounded*, and a behaviour has two: its entity, and that entity's level. A third requires a third **relationship** to exist first.

And two kinds that never may:

- An **entity-scope verb** (`GetLocation`, `Teleport`, `Destroy`). Those live on `Entity`. Once an entity may carry several behaviours (RAD-101), an unqualified `SetLocation()` here would claim to be *the* entity while being one of many — and would duplicate the entity's whole verb surface once per attached behaviour.
- A **subsystem facade** (`GetPhysicsBody`, a future `GetAbilitySystem`). That set is *unbounded* — one per subsystem, forever — which is the O(N)-edits-per-feature pattern the accessor conventions exist to forbid. Reach subsystems through `GetOwner()`.

### The named verbs, and the rule for adding one

A verb that names **one entity** lives on `Entity`; a verb about the **level as a whole** lives on `GameplayLevel`. That dividing rule is mechanical so it does not drift.

| Verb | Meaning |
|------|---------|
| `GetLocation()` / `GetRotation()` | where it is; rotation is radians about Z |
| `SetLocation()` / `SetRotation()` | move it continuously — *"it moved"* |
| `SetTransform(loc, rot)` | both at once, one lookup |
| `Teleport(loc, rot, TeleportType)` | move it discontinuously — *"it jumped"* |
| `Destroy()`, `RefreshCollider()` | structural verbs |
| `GetPhysicsBody()`, `GetLevel()` | subsystem facade, relationship |

A verb is admitted only when **both** gates pass:

- **Semantic gate** — it names exactly one entity, is expressible without the caller naming a component type, and *either* the equivalent raw component write is **silently wrong**, *or* it is the read half of such a write.
- **Customer gate** — something needs it now.

That bounds the surface by the number of silent-failure traps in the engine — small, and shrinking — rather than by the number of component types, which grows forever. If it ever passes roughly a dozen verbs the rule has stopped working, and the answer is a general mechanism rather than a thirteenth verb.

Deliberately **excluded**, recorded so the next addition meets a decision rather than a precedent: `SetScale`/`GetScale` (passes semantic — a scale change leaves the physics shape stale until `RefreshCollider` — but fails customer: nothing scales anything yet); `SetActive`, `SetColor`, `SetTexture` and every other read/write-this-field (fails semantic: the raw write is correct, and `GetComponent<T>()` is right there); `GetVelocity` and the physics surface (subsystem state — lives on the `PhysicsBody` facade).

This does **not** reopen the rejection of typed component accessors (`entity.GetSprite()`). That rejection is about handing back a *component*, which would make `Entity.h` grow per component type. `GetLocation()` returns a `glm::vec3` and hides which component stores it: a verb, not an accessor.

**`SetTransform` is the rule working, and is worth reading as such.** The set was designed as four verbs. The first real consumer — Reaper's `CameraController`, which sets both halves of its pose every fixed step — showed that calling `SetLocation` then `SetRotation` costs roughly four transform lookups where one would do, because each single-axis verb must read the half it is preserving. It passes the semantic gate like its siblings and passes the customer gate on that evidence, so it was admitted **by** the rule rather than argued in around it.

### `SetLocation` is not `Teleport` with a shorter name

The distinction is one sentence — *"it moved"* versus *"it jumped"* — and it is visible on screen.

| | `SetLocation` / `SetTransform` | `Teleport` |
|---|---|---|
| Render snapshot | **preserved** → motion interpolates | **stamped** to destination → no smear |
| Velocity | always kept | `TeleportType` chooses |
| Logs | **silent** | TRACEs every call |

Rendering runs at display rate while simulation runs at a fixed rate, so `Level::OnRender(alpha)` draws `lerp(snapshot, current, alpha)` — where the entity *was* at the start of the step, blended toward where it *is*. For a jump, blending is wrong: you would see the entity smear across the room over one frame, so `Teleport` stamps the snapshot to the destination and the lerp becomes a no-op.

For **continuous** movement that same stamp is the bug. At a 60 Hz step on a 144 Hz display there are ~2.4 rendered frames per simulation step; if every step stamps the snapshot then `snapshot == current` always, the lerp returns `current` for every alpha, and the entity renders at fixed-step granularity — visible judder. The logging follows the same split: a rare, deliberate act is worth an audit line, and a verb a follow-camera calls every step would emit 60 lines a second.

Both push the pose into Box2D, and **that is the half that closes the trap** the problem statement opened with.

One honest limitation: `SetLocation`'s interpolation promise holds only for entities the snapshot pass classifies as movers (has physics, a camera, or a behaviour). A manager moving a plain decorative entity gets no smoothing. Stamping a snapshot to compensate would fight the same pass, which strips snapshots from non-movers every step; the real fix is the explicit mover marker planned with RAD-30.

### Layering

```
Gameplay/  ──►  ECS/  ──►  Physics/  ──►  Core/
 facades       storage      vendor
     └──────────────────►──────┘
```

`Gameplay/` holds `EntityBehaviour`, `GameplayLevel` and `PhysicsBody`. The dependency runs one way and never back. `Entity` stays in `ECS/` despite being the most gameplay-facing type there is, because `Level.h` includes it — moving it would invert the direction the split exists to establish.

`PhysicsBody` cannot live in `Physics/` either, for a reason worth keeping written down: it stores an `Entity` **by value**, and a member requires a *complete* type, so its header must include `ECS/Entity.h` — which drags `Renderer/SceneCamera.h` and `Renderer/Texture.h` into the physics module. `PhysicsWorld2D` escapes only because it takes `Entity` as a *parameter* and forward-declares it. **Members require complete types; parameters do not.**

### A whole behaviour

```cpp
class Turret : public EntityBehaviour
{
    void OnCreate() override
    {
        m_TargetID = GetLevel().FindEntityByName("Player").GetUUID();
    }

    void OnUpdate(Timestep ts) override
    {
        Entity target = GetLevel().GetEntityByUUID(m_TargetID);
        if (!target)
            return;

        glm::vec3 toTarget = target.GetLocation() - GetOwner().GetLocation();
        GetOwner().SetRotation(std::atan2(toTarget.y, toTarget.x));
    }
};
```

Every line reads in one vocabulary: level-scope off `GetLevel()`, entity-scope off an `Entity` — its own or anyone else's, identically — and subsystem verbs off a facade.

## Design Rationale

- **An authoring layer over ECS storage, not a storage change.** entt stays as storage and the registry stays private to `Level`. Unreal couples its actor model to its storage; we deliberately do not, because that model is only affordable with reflection — without it, actor-model storage means hand-written serialization and a hand-written reference graph per class.
- **Verbs on the world-thing, not on the behaviour.** A level-wide collision callback holds a bare `Entity`, not a behaviour. Verbs on `Entity` mean `other.GetLocation()` in a handler and `GetOwner().GetLocation()` in a behaviour are the same API; verbs on the base would leave everything that is not a behaviour reaching through `GetComponent`.
- **The direction was chosen for reversibility.** Adding forwarders to the base later is a handful of one-liners that break nothing; removing them later breaks every call site in the game. With a cost curve that asymmetric, start narrow and widen on evidence. The cost accepted is `GetOwner().` at every call site, forever. A game wanting the shorthand can add its own base-class shim — that is the game's business, and it is self-measuring: if it grows, that is the evidence for promoting it.
- **Rules are written where the compiler or the next reader will meet them.** The inclusion rules live in the class headers, not only here, because a surface defined by exclusion fails silently — the failure mode is under-exposure, and the doc is the mitigation.

## Known Issues & Evolution

- **One behaviour per entity (RAD-101):** `NativeScriptComponent` holds a single `Instance`, so composing behaviour is only possible by inheritance. The decision is recorded — an entity **may** carry several — and the shape of that change is a Level-owned side table holding a list, with defined update and destruction ordering.
- **No typed retrieval (RAD-100):** recovering a concrete behaviour from an `Entity` still means reaching into `NativeScriptComponent` and hand-casting. `Entity::GetBehaviour<T>()` returning nullptr on mismatch is the planned answer.
- **No possession model (RAD-102):** `Pawn`/`Controller` do not exist yet. Relationship accessors for them may only be added once the relationship does.
- **Behaviour instances are owning raw pointers in a component (RAD-30):** the known plain-data violation. Instances move to a Level-owned side table.
- **Destroying your own entity from a hook** deletes the instance whose method is executing, so it must be the last statement. RAD-97 removes the wart by deferring the reap.