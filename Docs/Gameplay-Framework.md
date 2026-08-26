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

### Recovering a concrete behaviour

**The problem this solves.** The engine creates your `CollisionLogger` but remembers it only as an `EntityBehaviour*` — the base-class pointer, because the engine cannot know your subclass. It can still call `OnUpdate()` on it (that is *virtual dispatch*: call whatever the real object's version is), but it cannot hand it back as the type you wrote. So game code used to take it by force:

```cpp
auto* nsc = platform.TryGetComponent<NativeScriptComponent>();
static_cast<CollisionLogger*>(nsc->Instance)->SetDestroyOnContact(true);
```

`static_cast` is C++ for *"trust me"* — the compiler checks nothing and emits nothing. Rebind that entity and the same line reinterprets a `CameraController` as a `CollisionLogger` and writes `true` at whatever offset a `CollisionLogger` keeps its flag. If that lands on a `float` member, `2.5f` becomes `2.5000002f` and **the program keeps running**: no log, no assert, no memory tool. Weeks later the camera zoom is subtly wrong and nothing connects it to this line. Land on a pointer instead and you crash in a system that never touched this code.

**The answer is a checked query on the entity:**

```cpp
if (CollisionLogger* logger = platform.GetBehaviour<CollisionLogger>())
    logger->SetDestroyOnContact(true);
```

`Entity::GetBehaviour<T>()` asks the object what it really is before handing it over. Any class with a virtual function carries a runtime tag describing its actual type — **RTTI**, "run-time type information" — and `dynamic_cast` reads it: the real pointer if the object is a `T`, `nullptr` if it is not. A wrong guess becomes a null check instead of silent corruption. Gameplay names neither the component nor a cast.

**It never asserts, because it is a query.** Four ways to get nothing, all silent: an invalid handle, no `NativeScriptComponent`, an instance not built yet (behaviours are created lazily on the first fixed step after `Bind<T>()`), or the wrong type. The invalid-handle case is the load-bearing one — `OnCollisionEnd`'s contract says the partner **may already be dead**, so `other.GetBehaviour<Door>()` on a corpse is the designed path, and a warning there would fire during correct gameplay. Contrast the physics resolvers (`Docs/Physics.md`), which *do* warn on a stale id: those callers asked for an **action** on something they believed existed, so a miss is news. This caller asked a **question**, and "no" is an answer. The only check that fires is compile-time — `T` must derive from `EntityBehaviour`.

**The pointer is transient**, with the lifetime of the handle it came from: it dangles when the entity dies, when the level dies, and when the behaviour is rebound (`AddOrReplaceComponent<NativeScriptComponent>` deletes the old instance). Re-query rather than store; keep the `Entity`, or its UUID.

**How Unreal does it, and where we differ.** Same shape: `AActor::FindComponentByClass<T>()` scans the owned components, asks each `IsA(TargetClass)`, and returns `nullptr` when nothing matches (`Engine/Source/Runtime/Engine/Private/Actor.cpp:3991`) — never an assert, with a `static_assert` constraining `T` in the templated half (`Actor.h:3823`). The **mechanism** differs. UE deliberately avoids `dynamic_cast`; it goes so far as to `#define` the keyword out of existence (`Templates/Casts.h:591`) and route `UObject` casts through its own reflection chain, where a type test is a 64-bit flag AND for hot types and an O(1) ancestry-array lookup for everything else (`UObject/Class.h:430`) — every class stores its full lineage as a flat array plus its depth, so "am I an X?" is an indexed load, not a walk up the parents.

We do not adopt that, and the reason is precise: **UE's speed comes from `UClass`, not from `Cast`.** Every tier of it is a thin read of a runtime class object holding a flag mask, an ancestry array and a depth. Radiant has no such object — RAD-98 declined the reflection trade deliberately — so a hand-written `Radiant::Cast<T>` would be either a rename of `dynamic_cast` or the start of hand-building `UClass`: one edit per behaviour type forever, and an *exact*-type test that cannot answer "is it a `Guard` **or derived from** `Guard`". UE keeps `ExactCast` as a separate function precisely because those are different questions. The trigger that reopens this is **RAD-72**: `entt::meta` is already vendored, and it supplies the class object for free — at which point `dynamic_cast` gets *replaced*, not *reworked*, and the signature above does not change.

Cost is one entity-validity check, one sparse-set lookup, and one `__RTDynamicCast` call — a real function with a loop in it, far more than the `static_cast` it replaces, which compiled to nothing. That is correctness bought with cycles, at call sites that are event-shaped rather than frame-shaped. Wanting it per frame per entity means caching the `Entity` and querying once in `OnCreate`, never caching the pointer.

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
- **Typed retrieval reshapes under composition (RAD-100 → RAD-101):** `Entity::GetBehaviour<T>()` exists (above) and today is one component lookup plus one cast. Once an entity may carry several behaviours it becomes a linear scan over that entity's list — UE's shape exactly — and RAD-101 must define what two behaviours of the same type mean. **Reject-at-attach is the answer that keeps this doc honest**, because it is what makes "*the* behaviour of type `T`" a total phrase rather than "whichever we hit first". The transient-pointer contract also gets more load-bearing there: a sibling's actions can invalidate a stored pointer, not only the entity dying.
- **No possession model (RAD-102):** `Pawn`/`Controller` do not exist yet. Relationship accessors for them may only be added once the relationship does.
- **Behaviour instances are owning raw pointers in a component (RAD-30):** the known plain-data violation. Instances move to a Level-owned side table.
- **Destroying your own entity from a hook** is safe from any hook and needs no positional discipline — destruction marks the entity dead and defers the free to the end of the fixed step, so the instance outlives the method that asked (RAD-97). `OnDestroy` fires immediately, before the calling method resumes. The entity reports `IsValid() == false` from that line on, so anything after it reads a dead handle.