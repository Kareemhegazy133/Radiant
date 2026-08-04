# Implementation Plan — RAD-99: Gameplay base class: designed verb surface, and the ScriptableEntity rename

| Field | Value |
|-------|-------|
| **Jira** | [RAD-99](https://hndredgames.atlassian.net/browse/RAD-99) |
| **Epic** | RAD-98 — Gameplay Framework (engine base classes for game code) |
| **Story status** | To Do |
| **Dependencies** | RAD-95 (In Review) blocks — landed on `dev`, so implementable now |
| **Planned** | 2026-08-03 |

---

## 0. The Problem, Ground Up

### What a script is, and what it can say today

A **native script** is a C++ class the engine calls once per simulation step so that gameplay can happen. You write one by subclassing an engine base class and overriding `OnUpdate`. The engine attaches it to an **entity** — one thing in the world: a camera, a crate, a moving platform — and drives it.

After RAD-95 the base class exposes exactly two things:

```cpp
class ScriptableEntity
{
    Entity        GetEntity() const;   // my entity
    GameplayLevel GetLevel()  const;   // the world my entity is in
    // ...plus the lifecycle hooks the engine calls: OnCreate, OnUpdate, OnDestroy, OnCollision*
};
```

That is a correct **seam** — the narrow doorway between engine and game. It is not yet a **framework**, and the difference shows up in the very first line a gameplay programmer writes. Asking "where am I?" reads like this:

```cpp
glm::vec3 here = GetEntity().GetComponent<TransformComponent>().Translation;
```

Three concepts to say one thing. The programmer had to know that position lives in a type called `TransformComponent`, that the field inside it is spelled `Translation` and not `Position`, and that there is a template lookup between them.

### The failure this actually causes — with numbers

Reading is merely verbose. **Writing is a trap.** Here is the whole of it, in a script that wants to respawn its entity at the origin:

```cpp
GetEntity().GetComponent<TransformComponent>().Translation = { 0.0f, 0.0f, 0.0f };
```

Compiles. Runs. Does nothing — if the entity has a physics body.

Radiant decided in RAD-28 that **physics owns the transform of a body-backed entity**. Every fixed step, Box2D simulates the bodies and then writes their poses back into `TransformComponent`. So the write above survives for exactly one step, and is then overwritten by the physics readback. At a 60 Hz fixed step, the value the programmer wrote lives for **16.6 milliseconds** and is never rendered — the render pass reads the transform *after* the readback. There is no error, no warning, no assert. The crate simply stays where it was.

The engine already has the correct operation: `Entity::Teleport(...)`, which writes the ECS transform *and* pushes the pose into Box2D. But nothing at the call site tells you that the obvious thing is wrong and the non-obvious thing is right. This is a well-known class of bug — Unity ships the identical trap (`transform.position` on a `Rigidbody` object versus `Rigidbody.MovePosition`), and it is the single most-answered question in its physics forum.

### The fix, as an everyday thing

Think of a **hotel front desk**. Behind it are a hundred numbered pigeonholes: keys, mail, billing slips, maintenance tickets. You *can* walk behind the desk and take what you want out of pigeonhole 47 — and if you take the key from 47 without also telling the register, the room shows as vacant while you are asleep in it.

The fix is not to lock the pigeonholes. It is to put a person at the desk who knows the small number of things guests actually ask for — "my key", "check me out", "where is my room" — and who does the *whole* operation each time, register included. The pigeonholes stay open behind them for the rare request nobody anticipated.

That person is a **named verb surface**: a small, deliberately-chosen set of methods that speak in gameplay words (`GetLocation`, `SetLocation`) and quietly do the complete operation. The pigeonholes are `GetComponent<T>()`, still there, one hop further away, for everything the desk doesn't cover.

### How Unreal does it

Unreal's world-thing is `AActor`, and it fronts exactly this surface — `GetActorLocation()`, `SetActorLocation()`, `SetActorRotation()`. Crucially, `SetActorLocation` does **not** poke the transform:

```cpp
// Engine/Source/Runtime/Engine/Private/Actor.cpp:4932
bool AActor::SetActorLocation(const FVector& NewLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport)
{
    if (RootComponent)
    {
        const FVector Delta = NewLocation - GetActorLocation();
        return RootComponent->MoveComponent(Delta, GetActorQuat(), bSweep, OutSweepHitResult, MOVECOMP_NoFlags, Teleport);
    }
    // ...
}
```

It routes through the movement machinery, which is what makes the physics state come along. And component access is still there — `FindComponentByClass<T>()` — as the escape hatch, not the idiom.

### The second problem: the class is named after a Hazel implementation detail

`ScriptableEntity` came from Hazel, the engine Radiant forked. The name describes a mechanism ("this entity is scriptable"), not a role. As the root of a gameplay hierarchy that will grow `Pawn` and `Controller` (RAD-102), it is actively misleading: it reads as though it *is* an entity, which is the one thing it is not.

The right name is not a matter of taste — it is a **consequence** of one unanswered question: *may an entity carry one behaviour, or several?* If one, the behaviour is the world-thing and `Actor` is right. If several, the behaviour is one of many attached to a world-thing and `Actor` is wrong. That question is RAD-101's to implement and this card's to answer, which is why the rename is sequenced here.

### The third problem: `ECS/` has become a drawer

One folder currently holds storage (`Level`), a handle (`Entity`), components, the serializer, the script base class, and two gameplay facades — one of which (`PhysicsBody`) is named for a different subsystem entirely. RAD-98 is about to add `Pawn` and `Controller` on top. The folder needs a seam before it gets more contents, not after.

### What we have / what we're building

**What we have:** a correct but bare seam — a script can reach its entity and its level, and from there everything, but every gameplay sentence is spelled in ECS storage vocabulary, and the obvious way to move something is silently wrong on exactly the objects that move.

**What we're building:** a small designed set of named gameplay verbs on the thing that actually has a location, a base class renamed to the role it plays rather than the mechanism it came from, and a `Gameplay/` module to hold the framework — each with a written rule for what may be added next, so the surface stays designed instead of accumulating.

**What we are deliberately NOT gaining.** No performance win — `GetLocation()` is the same sparse-set lookup `GetComponent<TransformComponent>()` was, and `SetLocation()` on a body-backed entity does strictly more work than the (broken) direct write. No new capability: everything expressible after this card was expressible before it, more verbosely and more dangerously. What we buy is that **the pleasant path and the correct path become the same path**, and that the next person to add to this surface meets a decision instead of a precedent.

---

## 1. Architecture Decision

### The thesis: `Entity` is Radiant's `AActor`; the base class is Radiant's `UActorComponent`

One sentence organises this entire card, and once it is accepted every other question answers itself mechanically.

Unreal's model is **`AActor` plus N `UActorComponent`s**. The actor is the world-thing — it has the location, it is destroyed, it belongs to a world. The components are the behaviour units — they have lifecycle callbacks, they are attached and detached, and there can be many on one actor. `ACharacter` is not deep inheritance; it is a capsule, a mesh and a `UCharacterMovementComponent` **assembled** onto one actor.

Radiant already has the actor. It is called `Entity`. It is the thing that exists in the world, that has a transform, that can be destroyed, that belongs to a `Level`. It exists whether or not any C++ behaviour is attached — most entities in Reaper have none.

That last fact is decisive and it settles the naming question outright: **a class that is optional on the things it claims to be cannot be the world-thing.** `ScriptableEntity` is present on 3 of Reaper's ~8 entities. Calling it `Actor` would name the *script on the crate* as the crate.

So the map is:

| Unreal | Radiant | Role |
|--------|---------|------|
| `AActor` | `Entity` | the world-thing: has a location, is destroyed, belongs to a world |
| `UActorComponent` | the renamed base class | a behaviour unit: lifecycle callbacks, attached to a world-thing |
| `UWorld` | `Level` / `GameplayLevel` | the world |
| `FindComponentByClass<T>()` | `Entity::GetComponent<T>()` | the escape hatch |

### Decision A — behaviour composition, answered in principle: **many per entity**

Recorded here, implemented in RAD-101, as the card requires.

An entity may carry several behaviours, each with its own lifecycle. The rejected alternative is one-behaviour-per-entity-forever, and rejecting it costs us a simpler side table in RAD-101 and buys us the thing UE spent two decades acquiring: reusable gameplay pieces that **compose** instead of stacking into an inheritance tree. With one behaviour per entity, a health system, a movement system and an interaction system on one crate must become one class, three entities, or three `Level` loops. None of those is a framework.

Note what this does *not* claim: entt already gives us composition of **data**. What it does not give us is composition of **behaviour** — a script is imperative glue and has no home but a class. That gap is the one RAD-101 closes, and answering it "many" is what makes the base class a behaviour rather than an actor.

### Decision B — the name follows: a *behaviour*, not an *actor*

**LOCKED 2026-08-03: `EntityBehaviour`.** It states exactly what an instance is — a behaviour attached to an entity, which is the same information `UActorComponent`'s name carries with the one colliding word swapped — and it survives the collision that plain `Behaviour` would hit when RAD-78's AI framework brings `BehaviourTree` / `BehaviourNode` into the same namespace meaning something entirely different.

Rejected, with reasons:

| Candidate | Verdict |
|-----------|---------|
| `EntityComponent` | The most UE-faithful name, and the one word we cannot borrow. "Component" already means the opposite thing here: plain, trivially copyable, serializable data (playbook §3), whereas this is a heap-allocated polymorphic object. The `*Component` suffix is *already* this codebase's convention for ECS components, so `EntityComponent` would read as a sibling of `SpriteComponent`. UE escapes this only because it has no ECS — the word is free in its namespace and the most-taken word in ours. UE also fuses data and behaviour into one `UActorComponent`; Radiant deliberately splits them (data → ECS components, behaviour → here), and one word for both halves would erase the distinction. |
| `Actor` | The world-thing is `Entity`, and this class is optional on entities — present on 3 of Reaper's ~8. Naming it `Actor` claims a role it does not have. |
| `Behaviour` | Correct shape, but `Radiant::Behaviour` collides conceptually with the behaviour-tree vocabulary RAD-78 will need. |
| `GameplayBehaviour` | Weaker than the chosen name: everything in `Gameplay/` is gameplay, so the prefix carries no information where `Entity` tells you what it attaches to. |
| `Script` / `EntityScript` | Names the mechanism, not the role — and RAD-58 may bring a real scripting host, where "script" would mean Lua or C#. |

**Spelling — LOCKED 2026-08-03: British (`-our`).** The codebase was genuinely mixed (comments 11 × "behaviour" to 2 × "behavior"; identifiers US, as in `SpriteComponent::Color`), so this needed a call rather than a default. British wins on the dominant usage in prose and Jira, and it becomes the **project standard for gameplay-framework types** from here — recorded in the playbook so `Pawn`/`Controller`/GAS work does not have to re-decide it.

The UE legibility this trades away is recovered in the docs, not the identifiers: `Docs/Gameplay-Framework.md` carries the mapping table (`AActor` → `Entity`, `UActorComponent` → `EntityBehaviour`, `UWorld` → `Level`/`GameplayLevel`, `FindComponentByClass<T>()` → `Entity::GetComponent<T>()`), read once and oriented forever.

### Decision C — the named verbs go on `Entity`, **not** on the base class

**LOCKED 2026-08-03. This amends the story's stated approach**, and it is the single most consequential decision in the plan. The call site becomes `GetOwner().GetLocation()` (see Decision C2 for the accessor rename).

The card's AC reads *"The base exposes named gameplay verbs."* A principal review says: put them on `Entity`, for four reasons, any one of which is sufficient.

1. **The dividing rule already decided this.** Playbook §10, locked one day ago: *"a verb that names one entity lives on `Entity`; a verb about the level as a whole lives on `GameplayLevel`."* `GetLocation` names one entity. Making an exception for six verbs on day two is precisely the "rule with a hole in it on day one" that §10 was written to prevent.
2. **UE agrees, once the mapping is right.** `AActor` has `GetActorLocation`; `UActorComponent` — the behaviour unit, our base class's true analog — has **no transform forwarders at all**. A component asks its owner: `GetOwner()->GetActorLocation()`. That is one hop, exactly like `GetOwner().GetLocation()`.
3. **With N behaviours per entity, "mine" stops meaning anything.** An unqualified `SetLocation()` inside behaviour #2 of 3 reads as "set my location" when there is no "my" — and it would be *three* copies of the same entity verb surface, one per attached behaviour. This is the same lie the component forwarders deleted in RAD-95 told ("silently means mine"), made worse by composition.
4. **Only entity-shaped code gets it otherwise.** A level-wide collision callback holds a bare `Entity`, not a script. If the verbs live on the base class, that callback is back to `GetComponent<TransformComponent>().Translation` — the exact asymmetry RAD-95 removed.

The honest cost, stated plainly: **every call site pays `GetOwner().` forever** — eleven characters, on every gameplay verb, in every script. What it buys is that `other.GetLocation()` in a collision handler and `GetOwner().GetLocation()` in a behaviour are the *same* API, and that the surface has one rule with no exceptions.

**Why this is the reversible direction, which is the real argument.** Adding forwarders to `EntityBehaviour` later is six one-liners and breaks nothing; every existing call site keeps compiling. *Removing* them later breaks every call site in the game — RAD-95 just performed exactly that removal, and it broke Reaper's build. With a cost curve that asymmetric, the principal move is to start narrow and widen **on evidence**. Today's evidence is one script (`CameraController`) where the shorthand reads better; that is not enough to spend an irreversible decision on.

**And the escape valve already exists, in the right place.** RAD-94 states it: *"Game-side helpers are unaffected — that is Reaper's business, not the engine's."* A ten-line `ReaperBehaviour : EntityBehaviour` with four protected forwarders buys the unqualified call in game code whenever the prefix starts to annoy, and it is **self-measuring**: if it grows to eight forwarders every script uses, that is hard evidence the engine should host them and promoting them is a one-hour card. Deliberately **not** written in this card — write scripts first, let the annoyance decide.

Three ways to get both *in the engine* were considered and rejected:

| Approach | Why it fails |
|----------|--------------|
| `EntityBehaviour : public Entity` | The most appealing idea in the room — the unqualified call *and* genuinely one vocabulary, since it is literally `Entity`'s method. Dies on the argument that killed `Actor`: public inheritance asserts a behaviour **is** an entity. Under composition three behaviours would each *be* the same entity, and `behaviourA == behaviourB` would return true through `Entity::operator==`. Mechanically it is also a polymorphic type deriving from a copied-by-value handle with no virtual destructor — a slicing hazard by construction. |
| `private Entity` + `using Entity::SetLocation;` | Gets the unqualified call without a public IS-A, but costs one `using` per verb — the O(N)-edits-per-feature pattern RAD-94 forbids — and leaves two spellings for one operation. |
| Forward every entity verb, and state the rule as "all of them" | At least an answerable rule, and still loses on `player.GetLocation() - GetLocation()`: two spellings of one question in a single expression, where the bare one silently means "the entity of whichever behaviour you are reading". A shorthand with one behaviour per entity; a fiction with several. |

What the base class delivers instead is its **inclusion rule**, written into the class doc so the surface cannot accumulate: only *lifecycle hooks* (methods the engine calls on you at a defined point in the frame) and *relationship accessors* (the bounded set — its entity, its level). Never an entity verb, never a subsystem facade. That is a designed surface defined by exclusion, exactly as `GameplayLevel`'s is.

### Decision C2 — `GetEntity()` → `GetOwner()`, and the verbs stay unprefixed

**LOCKED 2026-08-03.** The accessor RAD-95 added is renamed to match UE's `UActorComponent::GetOwner()` (`Classes/Components/ActorComponent.h:521`), and the reason is stronger than parity: **the entity literally owns the behaviour.** `NativeScriptComponent::Instance` is an owning raw pointer living on the entity, so `GetOwner()` is true in UE's sense *and* in Radiant's own ownership vocabulary (playbook §2) — unlike `Actor` or `EntityComponent`, which were false. It costs nothing, landing inside the commit that already renames this class.

Two further steps toward UE spelling were considered and declined:

- **`GetOwner()` returning `Entity*` so the call reads `GetOwner()->`.** `Entity` is a 16-byte value handle and playbook §4 is explicit that verbs take it *by value*; this would be the one place in the codebase that does not, and it would hand out a mutable pointer into the behaviour's private member. More importantly a pointer implies nullability, and `Entity` already has a **better** null: `IsValid()` consults entt's generation counter, so it distinguishes "no entity" from "dead entity" where a raw pointer can only say null. UE returns `AActor*` because UE actors are heap objects with identity — the `->` is a consequence of its object model, not a portable style.
- **`GetEntityLocation()` mirroring `GetActorLocation()`.** UE prefixes because in an `AActor` subclass the call is *unqualified* — `SetActorLocation()` with no receiver — so the name must carry what it acts on, and because `AActor` and `USceneComponent` both have location verbs meaning different things. We always have a receiver, so `entity.GetEntityLocation()` stutters. The Blueprint argument does not apply either: UE renames for reflection via metadata rather than the C++ name (`K2_GetComponentLocation` carries `meta=(DisplayName = "Get World Location")`, `SceneComponent.h:656`). If an attachment hierarchy ever introduces a second location concept, renaming a getter is mechanical — short now, prefixed when a second concept actually exists.

### Decision D — `SetLocation` and `Teleport` are genuinely different operations

The card says `SetLocation` "must go through `Entity::Teleport` or it silently does nothing." The first half is right and the second is subtly wrong — routing straight through `Teleport` would introduce two regressions, both real:

`Level::Teleport` does three things: writes the ECS transform, **stamps `TransformSnapshotComponent` to the destination**, and pushes the pose into Box2D. `PhysicsWorld2D::Teleport` additionally emits a `RADIANT_TRACE` on every call (`PhysicsWorld2D.cpp:262`).

- The snapshot stamp exists so a teleport does not **smear** across a rendered frame — the renderer draws `lerp(snapshot, current, alpha)`, and a jump must not be interpolated. But for a **continuous** move — a camera following the player, a lift rising — stamping the snapshot every step destroys the interpolation entirely: the entity renders at fixed-step granularity, which at a 60 Hz step on a 144 Hz display is visible judder. `CameraController` writes its transform directly today and is smooth *because* nothing stamps its snapshot.
- The TRACE is correct for a rare, deliberate act (playbook §4: *rare and discontinuous verbs log; routine and continuous verbs do not*). A follow-camera calling `SetLocation` every step would emit 60 trace lines a second.

So the two verbs split on a statable, testable line:

| Verb | Snapshot | Velocity policy | Logs | Meaning |
|------|----------|-----------------|------|---------|
| `SetLocation` / `SetRotation` | **preserved** → renders interpolated | keeps velocity | no | "I moved" |
| `Teleport` | **stamped to destination** → no smear | `TeleportType` chooses | TRACE | "I jumped" |

Both push the pose into Box2D, which is the half that makes them correct where the raw write is not. Implementation-wise this needs one small addition in the physics module — a quiet `PhysicsWorld2D::SetTransform`, with `Teleport` becoming `SetTransform` + velocity policy + TRACE. A "quiet" boolean flag on the existing verb was rejected outright: playbook §4 says a caller needing a different policy keeps that branch at its own call site rather than adding a flag to a shared helper.

### Decision E — the verb-inclusion rule for `Entity`

Two gates, both must pass:

> **Semantic gate.** The verb names exactly one entity, is expressible without the caller naming a component type, and either (a) the equivalent raw component write is *silently wrong*, or (b) it is the read half of such a write.
>
> **Customer gate.** Something in the engine or in Reaper needs it now (RAD-94: adopt when the need exists, never before).

Applied:

| Verb | Semantic gate | Customer gate | Verdict |
|------|---------------|---------------|---------|
| `SetLocation` | (a) — raw write silently no-ops on bodies (RAD-28) | yes | **include** |
| `GetLocation` | (b) | yes | **include** |
| `SetRotation` | (a) — same | yes | **include** |
| `GetRotation` | (b) | yes | **include** |
| `SetScale` / `GetScale` | (a) — scale change desyncs the collider until `RefreshCollider` | **no** — nothing scales anything | exclude, recorded |
| `SetActive` / `IsActive` | fails — the raw write is correct | — | exclude, recorded |
| `SetColor`, `SetTexture`, … | fails — the raw write is correct | — | exclude; that is what `GetComponent<T>()` is for |

Note this does **not** reopen RAD-94's category-A rejection (`entity.GetSprite()`). That rejection is about handing back a *component type* — it would force `Entity.h` to learn about every component and grow per component. `GetLocation()` returns a `glm::vec3` and hides which component stores it: it is a verb, not a typed accessor. `Entity.h` already includes `Components.h` (`Entity.h:5`), so no new coupling is introduced either way.

### Decision F — what moves into `Gameplay/`, and what stays

Moves: the renamed base class, `PhysicsBody` (RAD-90), `GameplayLevel` (RAD-95) — together, per RAD-95 decision F, because splitting two facades of the same category across two folders is a worse state than leaving both where they are.

Stays in `ECS/`: `Entity`, `Level`, `Components`, `LevelSerializer`. `Entity` is tempting to move — it is the most gameplay-facing type there is — but it is the handle over entt storage, and `Level.h` includes it. Moving it would make `ECS/` depend on `Gameplay/`, inverting the dependency the split exists to establish. `Gameplay/` depends on `ECS/`; never the reverse.

`PhysicsBody` cannot move *down* into `Physics/` either: it stores an `Entity` **by value**, so its header needs the complete type, which transitively pulls `Renderer/SceneCamera.h` and `Renderer/Texture.h` into the physics module. `PhysicsWorld2D` escapes this only because it takes `Entity` as a *parameter* and forward-declares it (`PhysicsWorld2D.h:17`). Members require complete types; parameters do not.

`Physics/` headers currently include nothing from `ECS/` (verified: `ContactEvent.h`, `PhysicsWorld2D.h`, `TeleportType.h`). This card must keep it that way.

**No premake change is needed.** `Radiant/premake5.lua:15-17` globs `Source/**.h` / `Source/**.cpp` recursively, so a new folder is picked up automatically. Worth stating because a reader will look for the edit.

---

## 2. UE Reference

### The world-thing carries the named verbs

```cpp
// Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:2494
/** Returns the location of the RootComponent of this Actor*/
inline FVector GetActorLocation() const
{
    return TemplateGetActorLocation(ToRawPtr(RootComponent));
}
```

Two things to notice. First, it is a **verb on the actor**, not on any behaviour class. Second, it does not expose the component — it reaches through `RootComponent` and hands back a value. Our `Entity::GetLocation()` is the same shape: reach through `TransformComponent`, hand back a `glm::vec3`.

### The setter routes through the movement machinery, never a raw write

```cpp
// Engine/Source/Runtime/Engine/Private/Actor.cpp:4932
bool AActor::SetActorLocation(const FVector& NewLocation, bool bSweep, FHitResult* OutSweepHitResult, ETeleportType Teleport)
{
    if (RootComponent)
    {
        const FVector Delta = NewLocation - GetActorLocation();
        return RootComponent->MoveComponent(Delta, GetActorQuat(), bSweep, OutSweepHitResult, MOVECOMP_NoFlags, Teleport);
    }
    else if (OutSweepHitResult)
    {
        *OutSweepHitResult = FHitResult();
    }

    return false;
}
```

`ETeleportType` is UE's answer to the same question our `TeleportType` answers: whether the physics state is carried along or recomputed from the delta. **What we deliberately simplify:** UE's `bSweep` performs a swept collision query along the path and stops short on a blocking hit. We have no query API until RAD-76, and Box2D has no equivalent one-call primitive, so our verbs always place — never sweep. Recorded in the header so nobody assumes otherwise.

### The behaviour unit has relationship accessors and no transform forwarders

```cpp
// Engine/Source/Runtime/Engine/Classes/Components/ActorComponent.h:521
AActor* GetOwner() const;
// :531
virtual UWorld* GetWorld() const override final { return (WorldPrivate ? WorldPrivate : GetWorld_Uncached()); }
```

That is the entire relationship surface of a `UActorComponent`, and it is exactly the two accessors RAD-95 gave our base class. **What we adopt:** the bounded pair, one-for-one, the absence of everything else, and now the name itself (Decision C2). A component that wants its owner's location writes `GetOwner()->GetActorLocation()` — one hop, which is our `GetOwner().GetLocation()`.

### How UE gets its ergonomics, and why that path is closed to us

This is the question the plan must answer honestly, because UE *looks* like it has a shorthand we are declining. It does not — it has three separate mechanisms, and only one of them produces the unqualified call.

**First: the behaviour unit has no location at all.** Not a forwarder, not a shorthand — nothing:

```text
$ grep -c "Location" Engine/Source/Runtime/Engine/Classes/Components/ActorComponent.h
0
```

**Second: gameplay classes get unqualified verbs by *inheriting the world-thing*.**

```cpp
// Classes/GameFramework/Character.h:241
class ACharacter : public APawn
// Classes/GameFramework/Pawn.h:42
class APawn : public AActor, public INavAgentInterface
```

Writing `AMyCharacter : ACharacter` makes you the actor, so `SetActorLocation()` unqualified is a legitimate IS-A. **That path is structurally closed to Radiant:** our world-thing is an `entt::entity` id in a registry private to `Level` — it is *data*, not a class, so nothing can inherit it. This is not an oversight but the direct consequence of RAD-98's locked position (*"UE couples its actor model to its storage; we deliberately do not, because UE's model is only affordable with reflection"*). UE buys the unqualified call with actor-IS-storage and pays for it with a reflection system. Having declined that trade, we are in `UActorComponent`'s seat **by construction** — and its answer is `GetOwner()->`.

**Third: `USceneComponent` has its own transform,** which is its own state rather than a forwarder:

```cpp
// Classes/Components/SceneComponent.h:1057
inline FVector GetComponentLocation() const
{
    return GetComponentTransform().GetLocation();
}
```

Radiant has no analog today — no attachment hierarchy, and RAD-94 category B forbids faking a graph we do not have.

**The detail that validates our shape:** `AActor::GetActorLocation()` delegates *down* to `RootComponent->GetComponentLocation()` (`Actor.h:2495`). Even in UE the named verb on the world-thing is sugar over a component lookup — precisely what `Entity::GetLocation()` will be over `TransformComponent`.

### The escape hatch is a search, deliberately one step less pleasant

```cpp
// Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:3823
/** Templatized version of FindComponentByClass that handles casting for you */
template<class T>
T* FindComponentByClass() const
{
    static_assert(TPointerIsConvertibleFromTo<T, const UActorComponent>::Value, "'T' template parameter to FindComponentByClass must be derived from UActorComponent");

    return (T*)FindComponentByClass(T::StaticClass());
}
```

Note that UE's escape hatch is an **O(n) linear search over the actor's component array** while the named verbs are a pointer dereference. Ours is the opposite way round on cost — `GetComponent<T>()` is a fast sparse-set lookup — so our escape hatch stays cheap and only the *vocabulary* pushes people toward the named verbs. Worth knowing so we do not copy UE's rationale where it does not apply.

### UE also narrows surfaces it cannot narrow by type

```cpp
// Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:5033
#define HIDE_ACTOR_TRANSFORM_FUNCTIONS() private: \
    FTransform GetTransform() const { return Super::GetTransform(); } \
    FTransform GetActorTransform() const { return Super::GetActorTransform(); } \
    FVector GetActorLocation() const { return Super::GetActorLocation(); } \
    ...
```

A macro whose whole job is to *hide* the transform verbs on actor subclasses where they make no sense. Two lessons. First, even UE treats "which verbs belong on this type" as a real design question with a real mechanism behind it — this card's inclusion rules are the same instinct, applied earlier. Second, the comment above the macro admits it "doesn't prevent access through function calls from parent classes (ie an `AActor*`)" — a narrowing that leaks. Our `GameplayLevel` narrows by **type** instead, which does not leak, and this card must not weaken that (playbook §10: never add an accessor returning the underlying pointer).

---

## 3. File Plan

```text
Radiant/Source/Radiant/Gameplay/                    (NEW MODULE)
├── EntityBehaviour.h        (moved+renamed) — behaviour base: lifecycle + 2 relationship accessors
├── EntityBehaviour.cpp      (moved+renamed) — GetLevel() out of line
├── GameplayLevel.h          (moved)         — the Level as gameplay may see it
├── GameplayLevel.cpp        (moved)
├── PhysicsBody.h            (moved)         — the entity's physics verbs
└── PhysicsBody.cpp          (moved)

Radiant/Source/Radiant/ECS/
    Entity.h                 (modify) — Get/SetLocation, Get/SetRotation + the inclusion rule
    Entity.cpp               (modify) — the four verbs, forwarding to Level
    Level.h                  (modify) — SetLocation/SetRotation; private shared move core
    Level.cpp                (modify) — implementations; Teleport refactored onto the core
    Components.h             (modify) — forward decl + NativeScriptComponent doc rename

Radiant/Source/Radiant/Physics/
    PhysicsWorld2D.h/.cpp    (modify) — quiet SetTransform; Teleport built on it

Radiant/Source/Radiant/
    Radiant.h                (modify) — three include paths change

Reaper/Source/
    CameraController.h       (modify) — base-class name; FIRST REAL CUSTOMER of the verbs
    CollisionLogger.h        (modify) — base-class name
    KinematicPlatform.h      (modify) — base-class name
    SpawnProbe.h             (modify) — base-class name (two classes)
    Layers/GameLayer.cpp     (modify) — one new cheat proving physics routing (retires with RAD-92)

Docs/
    Gameplay-Framework.md    (new)    — the Gameplay Object Model page RAD-98 requires
    ECS-And-Levels.md        (modify) — gameplay sections move out; pointer left behind
    Physics.md               (modify) — script-hook class name; SetLocation vs Teleport
    README.md                (modify) — index the new page

CLAUDE.md                    (modify) — Architecture Map gains a Gameplay/ row
.claude/references/radiant-playbook.md (modify) — §10 updated; verb-home rule added
```

| Action | Path | Description |
|--------|------|-------------|
| Move+rename | `ECS/ScriptableEntity.{h,cpp}` → `Gameplay/EntityBehaviour.{h,cpp}` | Class renamed; `GetEntity()` → `GetOwner()`; no members added or removed; class doc rewritten around the inclusion rule and the composition answer |
| Move | `ECS/PhysicsBody.{h,cpp}` → `Gameplay/` | Include paths adjust to `Radiant/ECS/…`; no logic change |
| Move | `ECS/GameplayLevel.{h,cpp}` → `Gameplay/` | Same |
| Modify | `ECS/Entity.h/.cpp` | Four named verbs + the written inclusion rule and its exclusion table |
| Modify | `ECS/Level.h/.cpp` | `SetLocation`/`SetRotation`; `Teleport` refactored onto a shared private core |
| Modify | `Physics/PhysicsWorld2D.h/.cpp` | Quiet `SetTransform`; `Teleport` = `SetTransform` + velocity policy + TRACE |
| Modify | `ECS/Components.h` | `class ScriptableEntity;` → `class EntityBehaviour;` and the three doc references |
| Modify | `Radiant.h` | Three include paths; comment updated to name the `Gameplay/` module |
| Modify | Reaper scripts ×4 | Base-class name; `GetEntity()` → `GetOwner()`; `CameraController` migrated onto the verbs |
| Modify | `Layers/GameLayer.cpp` | One cheat key exercising `SetLocation` on a body-backed entity |
| Create | `Docs/Gameplay-Framework.md` | The gameplay object model, with the sections lifted out of ECS-And-Levels |

---

## 4. Type Design

### EntityBehaviour (renamed from ScriptableEntity)

- **Kind:** class (abstract by intent; subclassed by game code)
- **Responsibility:** the root of the gameplay behaviour hierarchy — one unit of C++ behaviour attached to one entity, receiving lifecycle callbacks from the engine.
- **Ownership:** unchanged. Heap-allocated by `NativeScriptComponent::InstantiateScript`, owned by that component's raw pointer (the known plain-data violation, RAD-30), deleted by `Level::DestroyEntity` after `OnDestroy`.
- **Lifetime & threading:** unchanged. Instantiated lazily on the first `Level::OnFixedUpdate` after binding; `m_Entity` wired **after** construction, so constructors must not touch components. Main-thread only.
- **Public surface — one rename, no additions, deliberately:**
  - `GetOwner()` — relationship accessor (renamed from `GetEntity()`, Decision C2)
  - `GetLevel()` — relationship accessor
  - `operator==` / `operator!=`
  - protected lifecycle: `OnCreate`, `OnUpdate(Timestep)`, `OnDestroy`, `OnCollisionBegin(Entity)`, `OnCollisionEnd(Entity)`

```cpp
class EntityBehaviour        // two accessors and five hooks, and that is the whole class
{
public:
    Entity        GetOwner() const;    // relationship accessor
    GameplayLevel GetLevel() const;    // relationship accessor
protected:
    virtual void OnCreate() {}                        // lifecycle hooks
    virtual void OnUpdate(Timestep ts) {}
    virtual void OnDestroy() {}
    virtual void OnCollisionBegin(Entity other) {}
    virtual void OnCollisionEnd(Entity other) {}
};
```

Small enough that the inclusion rule below is visibly true rather than merely asserted.
- **New in the class doc — the inclusion rule:** only two kinds of member may ever be added here. A **lifecycle hook** (a method the engine calls on you at a defined point in the frame) or a **relationship accessor** (bounded: its entity, its level). Never an entity-scope verb — that lives on `Entity`, because one behaviour of several cannot claim to *be* the entity. Never a subsystem facade — unbounded, one per subsystem forever, reached through `GetOwner()` (playbook §10).
- **Playbook Patterns:** §10 (the gameplay seam), §2 (ownership stated at the declaration)

### Entity — the four added verbs

- **Kind:** existing value handle; no size or layout change (the verbs are member functions).
- **Responsibility added:** speak the everyday gameplay sentences about *where a thing is*, correctly on body-backed and body-less entities alike.
- **Key members added:**
  - `glm::vec3 GetLocation() const` — world position. One component lookup. Invalid handle → `{0,0,0}` + WARN.
  - `void SetLocation(const glm::vec3&)` — moves continuously: writes the ECS transform, pushes the pose to Box2D if the entity has a body, and **leaves the render snapshot alone** so motion stays interpolated.
  - `float GetRotation() const` — rotation about **Z, in radians**. X/Y Euler terms are authored data this surface does not speak about (the same choice `Teleport` already made).
  - `void SetRotation(float radians)` — as `SetLocation`, for rotation.
- **Ownership / lifetime:** unchanged — `Entity` owns nothing and stays transient (playbook §2).
- **Playbook Patterns:** §10 (the dividing rule — these name one entity, so they live here), §4 (routine/continuous verbs do not log)

### Level — the shared move core

- **Kind:** existing class; two public verbs plus one private core.
- **Public:** `void SetLocation(Entity, const glm::vec3&)`, `void SetRotation(Entity, float)` — the implementations `Entity` forwards into, because they write `TransformComponent`, which only the type owning the registry may do (playbook §4, RAD-90's split).
- **Private core:** one helper performing ECS write → optional snapshot stamp → optional physics push, with the snapshot policy expressed as an **enum, not a bool** (playbook §4: semantic flags are enums). `Teleport` and `SetLocation` become two-line callers of it, which is what keeps their guard and their ordering from drifting apart.
- **Playbook Patterns:** §4 (one resolver per layer; enums over bools), §8.8 (nothing here iterates a view)

### PhysicsWorld2D — the quiet setter

- **Added:** `void SetTransform(Entity, const glm::vec2& position, float rotation)` — places the body and wakes it, and says nothing.
- **Changed:** `Teleport` becomes `SetTransform` + the `TeleportType` velocity policy + its `RADIANT_TRACE`. The wake-before-zero-velocity ordering (`PhysicsWorld2D.cpp:272-277`) is preserved and stays commented — v3 only wakes from `SetLinearVelocity` when the velocity is non-zero.
- **Playbook Patterns:** §4 (rare and discontinuous verbs log; routine and continuous verbs do not)

---

## 5. Implementation Steps

### Phase 1 — Lock the decisions (no code) — ALL LOCKED 2026-08-03
- [x] **Answer the composition question** — many behaviours per entity, recorded on RAD-99 and carried to RAD-101. This is the input to everything below.
- [x] **Pick the name and its spelling** — `EntityBehaviour`, British `-our`, which becomes the project standard for gameplay-framework types (§12 B).
- [x] **Confirm the verb home** — `Entity`, not the base class (§1 C), with the accessor renamed to `GetOwner()` (§1 C2). This amends the story's AC; the AC write-back happens at plan approval.

### Phase 2 — The rename (commit 1, mechanical)
- [x] **Rename the class and its files** — `ScriptableEntity` → `EntityBehaviour`, `.h`/`.cpp` renamed with it. Sites: `Level.cpp` (include + 2 uses), `Components.h` (forward decl + `Bind<T>`'s `static_cast` + 3 doc mentions), `Radiant.h`, `Level.h` doc, `GameplayLevel.h` doc, and all four Reaper script headers.
- [x] **Rename the accessor** — `EntityBehaviour::GetEntity()` → `GetOwner()` (Decision C2). Sites: `Entity.h`'s doc cross-reference, `GameplayLevel.h`'s doc, `Docs/ECS-And-Levels.md`, playbook §10, and every Reaper script that calls it (`CameraController` ×2, `CollisionLogger` ×1, `KinematicPlatform` ×2, `SpawnProbe` — check each). Lands in the same commit: it is the same rename decision, and splitting it would touch the same lines twice.
- [x] **Post an Architecture Realignment note to RAD-95** — it is *In Review* and this card renames the accessor it just shipped. Same precedent as RAD-95 decision D, which realigned RAD-29's API while RAD-29 was in review: an API must not change silently under a card awaiting sign-off.
- [x] **Rewrite the class doc** around the role and the inclusion rule — what may be added here (lifecycle hooks, relationship accessors) and what never may (entity verbs, subsystem facades), each with its reason. Drop the "NAME IS PROVISIONAL" block: this card is the answer.
- [x] **Silence the lifecycle defaults** — `OnCreate`/`OnUpdate`/`OnDestroy` currently log INFO; `OnUpdate`'s default fires **every fixed step**. Make all three empty, matching `OnCollisionBegin`/`OnCollisionEnd`. A behaviour that overrides nothing is a legal no-op, not a mistake worth 60 log lines a second.
- [x] **Verify:** `grep -r ScriptableEntity` returns nothing outside `.claude/plans/` history; three configs build; Reaper runs.

### Phase 3 — The `Gameplay/` module (commit 2, mechanical)
- [x] **Create `Radiant/Source/Radiant/Gameplay/`** and move the three pairs into it. No premake edit — the glob is recursive (§1 F).
- [x] **Fix include paths** — moved headers now say `Radiant/ECS/Entity.h` / `Radiant/ECS/Level.h`; `Radiant.h`, `Level.cpp` and `Entity.cpp` point at `Radiant/Gameplay/…`.
- [x] **Verify the boundary holds:** no `Physics/` header includes an `ECS/` or `Gameplay/` header; no `ECS/` header includes a `Gameplay/` header (the dependency runs one way).
- [x] **Verify:** three configs; Reaper runs. *(Headless run covers launch → frame loop → clean teardown only; gameplay needs the Play button — Kareem's half.)*

### Phase 4 — The verb surface (commit 3, the design change)
- [x] **Add `PhysicsWorld2D::SetTransform`** — place + wake, no logging — and refactor `Teleport` onto it so the two share one placement path and differ only in policy and logging.
- [x] **Add `Level`'s shared move core** and rebuild `Teleport` on it; add `Level::SetLocation` / `Level::SetRotation`. Snapshot policy is an enum parameter of the private core, never a public bool.
- [x] **Add the four `Entity` verbs** — zero-logic forwarders, matching `Entity::Teleport`'s existing shape exactly: catch the level-less handle (the one case that cannot be delegated downward), then forward.
- [x] **Write the inclusion rule into `Entity.h`** — the two gates, plus the exclusion table (`SetScale`/`GetScale`: passes the semantic gate, no customer; `SetActive`, `SetColor`: fails the semantic gate). The next addition must meet a decision, not a precedent.
- [x] **Document the physics semantics on every setter** — units (world units; radians about Z), that the pose is pushed to Box2D, that motion stays interpolated, that there is no sweep (§2), and when to reach for `Teleport` or `PhysicsBody::MoveKinematic` instead.

### Phase 5 — Reaper: the real customer, and the proof (commits 4 and 5)
- [x] **Migrate `CameraController` onto the verbs** (commit 4, production code, not scaffolding) — `GetOwner().SetLocation(...)` / `GetOwner().SetRotation(...)` replacing the two direct transform writes. Behaviour-identical (no rigidbody → no physics push, no snapshot stamp), and it proves the interpolation half: the camera must stay as smooth as it is today.
- [x] **Add one cheat key exercising `SetLocation` on a body-backed entity** (commit 5, scaffolding) — the `Reaper` entity, whose body must actually move, unlike the raw transform write. Carries a `retires with RAD-92` comment, and RAD-92 gets a scope amendment **before** this lands (its own rule, applied to itself).

### Phase 6 — Records
- [ ] **Create `Docs/Gameplay-Framework.md`** — the object model (`Entity` = the world-thing, the behaviour base = the behaviour unit), the two inclusion rules, the verb table, the `SetLocation`-vs-`Teleport` distinction. The gameplay sections currently in `ECS-And-Levels.md` (native scripts, what a script can reach, the level narrowed) move here; a pointer stays behind.
- [ ] **Update `Docs/ECS-And-Levels.md`, `Docs/Physics.md`, `Docs/README.md`** in the same change as the code that invalidates them (CLAUDE.md's doc contract).
- [ ] **Update CLAUDE.md's Architecture Map** — a `Gameplay/` row; the `ECS/` row loses the facades.
- [ ] **Update the playbook** — §10 gains the verb-home rule and the `Entity`-is-`AActor` mapping; the composition answer is recorded.

---

## 6. Ownership & Lifetime Strategy

Nothing in this card creates, destroys, or transfers ownership of anything. That is worth stating explicitly, because a card that establishes a module and a base class *sounds* like it should.

| Thing | Owner | Created | Destroyed |
|-------|-------|---------|-----------|
| Behaviour instance | `NativeScriptComponent::Instance` (raw owning pointer — the known RAD-30 violation) | lazily, first `OnFixedUpdate` after `Bind<T>()` | `Level::DestroyEntity`, after `OnDestroy` |
| `Entity` | nothing — 16-byte value handle | copied freely | transient; dangles when its entity or Level dies |
| `PhysicsBody`, `GameplayLevel` | nothing — value handles over an `Entity` / a `Level*` | per call | transient; never stored |
| Box2D body | the Level's `PhysicsWorld2D` | `on_construct<RigidBody2DComponent>` | `on_destroy` |

The one lifetime hazard the new verbs introduce is the one every verb here already carries: a stale `Entity` reaching `Level::SetLocation`. It is handled the same way — `Entity::SetLocation` catches the level-less handle (the only failure it can see), `Level::SetLocation` warns and recovers on an invalid handle, and `ResolveBodyId` handles a dead body id. Three layers, one guard each, no verb carrying its own preamble (playbook §4).

The relocation changes no lifetimes at all — it changes six file paths.

---

## 7. Performance Notes

**Verbs.** `GetLocation` / `GetRotation` are one sparse-set lookup, identical to the `GetComponent<TransformComponent>()` they replace — no regression, no improvement. `SetLocation` / `SetRotation` on a body-less entity are one lookup plus two writes. On a body-backed entity they add one `b2Body_SetTransform` and one `b2Body_SetAwake` — the same cost `Teleport` already pays, minus the `emplace_or_replace<TransformSnapshotComponent>`, so `SetLocation` is strictly cheaper than `Teleport`.

No new per-frame allocation. No new per-frame work for entities nobody calls the verbs on. Scripts are single-digit in count today, and the verbs are call-site-driven, so there is no scan and nothing scales with entity count.

**One perf-shaped decision worth naming:** `SetLocation` is *not* free to call every step on a **dynamic** body — it fights the solver, which will then correct it, and the two disagree forever. That is a correctness note wearing a performance coat, and it goes in the header: continuous movement of a dynamic body belongs to `PhysicsBody`'s force/velocity verbs, and of a kinematic body to `MoveKinematic`.

**Logging.** Removing the `OnUpdate` default's INFO removes one formatted log call per unoverriding behaviour per fixed step. Small, but it is 60 lines/second/behaviour of pure noise today.

**Relocation and rename:** zero runtime cost. Compile-time is unchanged — the same headers with the same contents at different paths.

---

## 8. Logging & Diagnostics

Follows playbook §4's split without adding a new pattern:

- `Entity::SetLocation` / `SetRotation` on a **level-less handle** — `RADIANT_WARN` naming the verb, then a no-op. Identical to `Entity::Teleport`'s existing guard, because it is the identical failure.
- `Level::SetLocation` / `SetRotation` on an **invalid entity handle** — `RADIANT_WARN`, then a no-op. Content-level mistakes recover; they are not programmer errors.
- **No TRACE on the setters.** They are the routine, continuous verbs. `Teleport` keeps its TRACE because it is the rare, deliberate one, and that line is the audit trail of every explicit ECS→Box2D push.
- **No new asserts.** There is no programmer error to catch here that the existing guards do not already name — asking to move a dead entity is a content mistake, not a broken invariant.
- Rename and relocation add no diagnostics; the lifecycle defaults *remove* three INFO lines.

---

## 9. Scalability Review

**The verb surface — the pattern this card must not become.** A named verb per gameplay concept is O(N) edits per concept, which is exactly what RAD-94 exists to forbid. The mitigation is not a mechanism, it is the **inclusion rule with a customer gate** (§1 E): a verb only exists when the raw component write is silently wrong. That bounds the set by the number of *silent-failure traps in the engine*, which is a small and shrinking number, not by the number of components (which grows forever). If the set ever exceeds roughly a dozen, the rule has stopped working and the right response is a generic mechanism, not a thirteenth verb.

**The base class — already scalable, and this card is what keeps it so.** Its cost per new subsystem is zero, because subsystems are reached through `GetOwner()`. Writing the inclusion rule into the header is what preserves that property, and is why the card's headline deliverable on the base class is a *doc*, not a method.

**The `Gameplay/` module — scales by construction.** New framework types (`Pawn`, `Controller`, future ability/animation facades) land in one folder with one dependency direction and no premake edit.

**Two things flagged, both correctly deferred:**

1. **Typed behaviour retrieval over N behaviours (RAD-100 × RAD-101).** With one behaviour, `GetBehaviour<T>()` is a lookup and a `dynamic_cast`. With many it becomes a linear search over the entity's behaviour list, and duplicate types need a defined answer (RAD-101's AC names it; reject-at-attach is the cleaner one). Not this card's — but the composition answer recorded here is what makes it RAD-100's problem, so it must not be a surprise there.
2. **`Entity` will keep attracting verbs.** `GetVelocity` already lives on `PhysicsBody` and must stay there (subsystem facade, playbook §10). The pressure to hoist it up will be real once scripts write `GetOwner().GetPhysicsBody().GetLinearVelocity()`. The dividing rule answers it: physics state → the physics facade. Recorded in the exclusion table so the argument is not had twice.

**One thing that is already the most scalable option:** the `Level`-owns-ECS-writes / `PhysicsBody`-owns-physics-writes split (RAD-90). This card adds verbs on both sides of it without needing to touch the split, which is the evidence that it was drawn in the right place.

---

## 10. Risks & Edge Cases

- **The AC amendment is the biggest risk in the card.** §1 C moves the verbs off the base class, against the story's written AC. If that is wrong, Phase 4 lands in the wrong type and RAD-101 inherits N copies of an entity verb surface. *Mitigation:* locked explicitly before any code, and written back to the story (§12 C).
- **`SetLocation` routed naively through `Teleport` regresses two things** — interpolation (the snapshot stamp) and the log (a TRACE per step). Both are silent-ish: the judder is visible only against a high-refresh display, and the log spam only looks like verbosity. *Mitigation:* the split is designed in (§1 D), `CameraController` is migrated as the live proof, and the test plan checks the camera specifically.
- **Rename churn across two projects.** Eight files plus doc mentions. *Mitigation:* grep is exhaustive (the name is unique), and all three configs plus a Reaper run gate the commit. Per memory: never round-trip source through `Get-Content`/`Set-Content` — that double-encodes UTF-8 and adds a BOM.
- **The move can break Dist only.** Include-path mistakes surface at compile time in every config, but a header that only compiled because something else pulled it in first is exactly the `ScriptableEntity.h`/`Timestep` bug from RAD-95 (playbook §11). *Mitigation:* each moved header keeps its own `.cpp`, so it is still compiled alone.
- **`SetLocation`'s interpolation promise is conditional, and the condition is a heuristic.** Only entities the snapshot pass classifies as movers get a `TransformSnapshotComponent` — `Level::OnFixedUpdate` defines that as *"has physics, a camera, or a script"*. So a manager script moving a **plain decorative entity** (no body, no camera, no script of its own) via `SetLocation` gets no snapshot, and `OnRender` draws it un-interpolated at fixed-step granularity — exactly the judder the verb claims to avoid. Not a regression (the direct transform write has the identical gap today), but the verb's doc must not promise more than the heuristic delivers. *Mitigation:* state the condition in the header rather than paper over it. **Rejected:** having `SetLocation` stamp the snapshot with the entity's OLD pose when one is missing — it works, but the stale-snapshot sweep in the same pass removes snapshots from non-movers, so the component would be added and removed every single step. The real fix is the explicit mover marker `Level::OnFixedUpdate`'s own comment already anticipates (*"an explicit marker replaces this heuristic when other movers appear"*), which belongs to RAD-30. Flagged as a follow-up, not half-fixed here.
- **Edge: `SetLocation` on a *dynamic* body every step** fights the solver. Not an error, and not preventable — documented in the header with the pointer to `PhysicsBody`'s verbs. The verbs also inherit `PhysicsBody`'s documented *when-is-it-observed* asymmetry: scripts run **before** the step, so a `SetLocation` from `OnUpdate` is immediately re-simulated on a dynamic body, while the same call from a collision handler lands after the step.
- **The verb surface is being designed against four scripts of evidence.** Reaper has `CameraController`, `CollisionLogger`, `KinematicPlatform`, `SpawnProbe` — three of which are verification scaffolding due to retire with RAD-92. *Mitigation:* the customer gate in §1 E keeps the set at what is demonstrably needed, and Decision C's reversibility argument means widening later is cheap while narrowing is not.
- **Edge: `SetRotation` writes only Z.** X/Y Euler terms survive untouched. This matches `Teleport`'s existing contract, so it introduces no new asymmetry, but it must be stated or someone will assume a full reset.
- **Edge: a behaviour calling `SetLocation` on an entity destroyed earlier in the same script pass.** Already covered — the handle is invalid, `Level::SetLocation` warns and no-ops, exactly as `Teleport` does.
- **Scope watch: `Physics/` is touched.** `PhysicsWorld2D::SetTransform` is beyond the story's stated file list. It is small and it is what makes the `SetLocation`/`Teleport` split honest rather than a flag on a shared helper. Flagged rather than smuggled.

---

## 11. Verification (AC → proof)

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| The base class is renamed, and the name follows from a recorded decision about behaviour composition | §12 A records "many behaviours per entity" with its rejected alternative; §12 B derives the name from it. Both posted to RAD-99 and carried to RAD-101 |
| The base exposes named gameplay verbs with units and physics semantics documented; `SetLocation` routes through the physics push for body-backed entities | **AMENDED (§1 C):** the verbs live on `Entity`. Header review: units and radians-about-Z stated per verb. Proof of routing: the new cheat moves the `Reaper` entity's **body** — the sprite and the collider outline move together and it resumes falling from the new position |
| The class doc states the inclusion rule for future verbs | Header review of both types: the base class's two-kinds rule, and `Entity`'s two gates plus the exclusion table |
| Component access remains reachable via `GetOwner()` and is documented as the escape hatch | `GetOwner().GetComponent<T>()` unchanged and exercised by `SpawnProbe`; the base class doc names it as the escape hatch, one hop longer than the named verbs by design |
| **Added:** the accessor rename is complete | `grep -rn "GetEntity()" Radiant Reaper Docs .claude/references` — empty; `EntityBehaviour`'s public surface is `GetOwner()`, `GetLevel()`, the comparison operators, and nothing else |
| `Gameplay/` exists and holds the base class, `PhysicsBody` and `GameplayLevel`; no entity-scoped facade is left behind in `ECS/` | `ls Radiant/Source/Radiant/Gameplay` shows six files; `ECS/` holds only `Entity`, `Level`, `Components`, `EntityTemplates`, `LevelSerializer` |
| No `Physics/` header includes an `ECS/` header | `grep -n '#include' Radiant/Source/Radiant/Physics/*.h` — currently clean; must stay clean, and must not gain a `Gameplay/` include either |
| Rename is a separate commit from the verb-surface work; relocation is a separate commit from both | `git log --oneline` shows: rename → relocate → verbs → Reaper consumer → scaffolding, five commits |
| Docs, CLAUDE.md Architecture Map and playbook updated in the same change | `Docs/Gameplay-Framework.md` exists and is indexed in `Docs/README.md`; `ECS-And-Levels.md` points at it; CLAUDE.md has a `Gameplay/` row; playbook §10 carries the verb-home rule |
| No engine gameplay type names a Reaper concept | Header review of the three `Gameplay/` types — no "Reaper", no game-specific vocabulary |
| Compiles clean in Debug/Release/Dist, no new warnings | CLI MSBuild all three configs per CLAUDE.md's Build & Run section, on each of the five commits |
| Reaper runs and is visually verified | Launch from `Reaper/`; all three scripts behave identically; camera smooth; platform still carries its crate |
| Ownership documented for new types | No new owning type is introduced — §6 states this explicitly, and the moved headers keep their existing ownership blocks |
| **Test plan:** all Reaper scripts compile and behave identically on the renamed base | Visual run after commit 1: camera moves on WASD/QE, platform oscillates and carries the crate, `C` still arms the destroy-on-contact logger, `N`/auto-timer still cycles the spawn probe |
| **Test plan:** a script sets its location on a body-backed entity and the body actually moves | The new cheat: press it and the `Reaper` entity relocates *and* keeps simulating from there. Contrast check — a raw `transform.Translation` write at the same site does nothing visible, which is the trap being closed |
| **Test plan:** the camera stays smooth after migration | `CameraController` on the new verbs, run uncapped: motion must interpolate as before. Judder would prove `SetLocation` stamped the snapshot |
| **Test plan:** grep confirms no `ScriptableEntity` survives outside history | `grep -rn ScriptableEntity Radiant Reaper Docs CLAUDE.md .claude/references` — empty |
| **Test plan:** headless smoke run | Launch redirected, let the auto-timer cycle the spawn probe, `CloseMainWindow`, scan the log for `[W]`/`[E]` and for stale names |

---

## 12. Decisions

**A. Behaviour composition — LOCKED 2026-08-03: many behaviours per entity.** Recorded here per the card, implemented in RAD-101. Rejected: one-behaviour-per-entity-forever, which makes every reusable gameplay piece enter through the base class and rebuilds the fragile-base-class problem UE escaped *with* components. entt already composes data; nothing composes behaviour, and that is the gap. Consequence to carry to RAD-101: the side table holds a *list* per entity, update order must be defined (insertion order is the cheapest defensible answer), and duplicate types need a rule.

**B. The name — LOCKED 2026-08-03: `EntityBehaviour`, British spelling.** Derived from A, not chosen by taste. The full rejection table is in §1 B; the one worth repeating is **`EntityComponent`**, which is the most UE-faithful name and the one word we cannot borrow — "component" already names the opposite concept here (plain serializable data, playbook §3), and `*Component` is already the ECS naming convention, so it would read as a sibling of `SpriteComponent`. UE escapes this only by having no ECS. British spelling becomes the project standard for gameplay-framework types, recorded in the playbook so RAD-101/102 do not re-decide it.

**C. Where the named verbs live — LOCKED 2026-08-03: `Entity`, amending the story's AC.** Four independent reasons in §1 C, but the one that actually decided it is **reversibility**: adding forwarders to the base later is six one-liners that break nothing, while removing them later breaks every call site in the game (RAD-95 just did that removal and it broke Reaper's build). With a cost curve that asymmetric, start narrow and widen on evidence. Today's evidence is one script. Cost accepted: `GetOwner().` at every call site, forever. The escape valve is a game-side `ReaperBehaviour` shim, deliberately not written in this card — write scripts first and let the annoyance decide. Three in-engine "have both" approaches were considered and rejected (§1 C table); the closest, `EntityBehaviour : public Entity`, dies on the same argument that killed `Actor`.

**C2. `GetEntity()` → `GetOwner()`, verbs unprefixed — LOCKED 2026-08-03.** Full reasoning in §1 C2. `GetOwner()` is adopted because the entity *literally* owns the behaviour instance, so the name is true in both UE's sense and playbook §2's. Declined: returning `Entity*` for a `->` call (it would be the codebase's only by-pointer `Entity`, and a raw pointer cannot distinguish "no entity" from "dead entity" the way generation-checked `IsValid()` does) and `GetEntityLocation()` (UE's prefix exists because its call is *unqualified*; ours always has a receiver, so it would only stutter).

**J. Reflection (RAD-72) — DECIDED 2026-08-03: resequence out of the icebox to the Phase 4 gate.** Raised during this walkthrough; recorded here because it constrains nothing in RAD-99 and that fact is itself the finding.

Nothing in this card forecloses reflection: reflection is *additive* — you annotate existing types rather than restructure them — and the one thing that would **force** it immediately is coupling the actor model to storage, which RAD-98 already declined. Our components are plain data (playbook §3), which is the easiest possible thing to reflect; UE needs heavy reflection partly *because* its components are polymorphic `UObject`s with pointer graphs and GC. The architecture is reflection-**ready**, not reflection-blocked.

It is also far cheaper than UE's version. UnrealHeaderTool is a build-time C++ parser emitting `.generated.h` because UE reflects at *compile* time for its GC and Blueprint VM. Our two real customers — killing `LevelSerializer`'s hand-written block per component, and a Phase 5 editor inspector — are both satisfied by *runtime* reflection, and **`entt::meta` ships in the entt 3.13.2 amalgamation we already vendor** (verified: `meta_type`, `meta_factory` present in `Radiant/Vendor/entt/include/entt.hpp`). No new toolchain, no codegen step, no build integration.

Two consequences to carry: RAD-72 gets its two named customers attached and moves to the Phase 4 gate, with `entt::meta` evaluated before anything is hand-rolled; and **RAD-100's `dynamic_cast` default is the right pick precisely because it is what `entt::meta` trivially replaces later** — worth writing onto that card so the choice is not re-argued.

What reflection does **not** change: the answer to "should the gameplay framework be built like UE's". Reflection makes actor-as-storage *possible*, not *good* — UE's model also costs a hand-written reference graph, a GC, and the contiguous component layout entt exists to provide. RAD-98's position stands: a hierarchy of behaviour objects **over** ECS storage.

**D. `SetLocation` vs `Teleport` — LOCKED 2026-08-03: genuinely different verbs.** `SetLocation` preserves the render snapshot (motion stays interpolated) and is silent; `Teleport` stamps it (no smear), carries the `TeleportType` velocity policy, and TRACEs. Not two names for one thing — the line is statable in one sentence ("I moved" vs "I jumped") and visible on screen. Rejected: a `quiet`/`snapshot` bool on the shared verb, per playbook §4.

**E. Verb set — LOCKED 2026-08-03: four now.** `GetLocation`, `SetLocation`, `GetRotation`, `SetRotation`. `GetScale`/`SetScale` pass the semantic gate (a scale change desyncs the collider until `RefreshCollider`) but fail the customer gate — nothing in the engine or Reaper scales anything; the `G` cheat mutates `BoxCollider2DComponent::Size`, not transform `Scale`. Recorded in the exclusion table so the day a script scales something, the verb is admitted by rule rather than argued from scratch.

**F. `Docs/Gameplay-Framework.md` as a new page — LOCKED 2026-08-03.** RAD-98's exit criteria require a Gameplay Object Model page, and `ECS-And-Levels.md` already mixes storage with gameplay across 144 lines — the same mixing the `Gameplay/` module split fixes in code. The docs should mirror the module map.

**G. Lifecycle defaults go silent — LOCKED 2026-08-03.** `OnCreate`/`OnUpdate`/`OnDestroy` currently log INFO, and `OnUpdate`'s default fires every fixed step. A behaviour that overrides nothing is a legal no-op (a marker, a future `Pawn` base), not a mistake worth 60 lines a second. Matches `OnCollisionBegin`/`OnCollisionEnd`, which are already silent for exactly this reason.

**H. Commit order — following the card: rename → relocate → verbs → Reaper consumer → scaffolding.** The card fixes the rename first; the relocation is separated from both so the design diff in commit 3 is readable on its own. The Reaper scaffolding gets its own commit and its RAD-92 scope amendment **before** it lands, per RAD-92's own rule.

**I. `Level` → `World` — DECIDED 2026-08-03: it happens, on its own card, sequenced to a phase boundary.** Kept out of RAD-99 deliberately (this card is already three commits of change, and burying a design diff under a cross-cutting mechanical rename is what its own "rename lands separately" instinct guards against). `PhysicsWorld2D` → `PhysicsScene2D` is pre-cleared as part of it, resolving the collision below.

**What that card must decide, captured here so none of it is rediscovered:**

| Question | Note |
|----------|------|
| `.rdlvl` → `.rdwld`? | It is a committed format referenced by the `.rdar` registry — either a content migration, or a `World` class carrying a `.rdlvl` extension forever |
| `GameplayLevel` → `GameplayWorld`, `LevelSerializer` → `WorldSerializer` | Mechanical, but `GameplayLevel` is *relocated* by RAD-99 first, so it takes two touches |
| The renderer's naming | `SceneCamera`, `BeginScene`/`EndScene`, `SceneData` are Hazel residue for the very concept being renamed, so the card should sweep them too |
| **Keep `Level` as well as `World`** | Decided in principle 2026-08-03: adopt UE's taxonomy rather than swapping one name for another (below) |

**The taxonomy to adopt (verified in UE 5.7.4 source, 2026-08-03):**

| UE type | Declared at | Role |
|---------|-------------|------|
| `UWorld` | `Engine/Classes/Engine/World.h` | gameplay container + frame driver |
| `ULevel` | `Engine/Classes/Engine/Level.h:422` | serialized sub-container of actors *inside* a world |
| `FPhysScene_Chaos : FChaosScene` | `Engine/Public/Physics/Experimental/PhysScene_Chaos.h:115` | physics' representation of that world |
| `FScene : FSceneInterface` | `Renderer/Private/ScenePrivate.h:2874` | the **renderer's** representation of that world |
| `FSceneView` / `FSceneViewFamily` | `Engine/Public/SceneView.h:25-26` | one camera's view into `FScene` |

The pattern is worth adopting rather than merely copying, because there is an architecture in it: **`World` is the gameplay truth, and each subsystem keeps its own `<Subsystem>Scene` representation of it.** `PhysicsScene2D` then reads as "physics' view of the world", not as a competing world — which is what dissolves the collision that made RAD-95 reject `World` in the first place. It is also forward-looking for Phase 3: a renderer-side scene representation is exactly what a render graph consumes, where `Level::OnRender` walks the ECS registry directly today.

That leaves `Level` **unspent** for the streaming sub-container UE uses it for (`UWorld` holds a `PersistentLevel`, `World.h:939`, plus an *"Array of levels currently in this world"*, `:1420`). Radiant's `Level` today is a fusion of `UWorld` and `ULevel`; the card must decide whether to introduce `World` as the still-fused type and reserve `Level`, or split them — the former, almost certainly, since nothing streams yet and a split with one element is ceremony.

The reasons it is a separate card rather than a fourth commit here:

1. **"World" may be the wrong half of the concept to keep.** Radiant's `Level` is a *fusion* of UE's two types: it drives the frame and owns the physics world (`UWorld`), and it is a serialized container of entities loaded from a file (`ULevel` — UE's serializes to `.umap`, ours to `.rdlvl`). In UE these coexist: `UWorld` holds a `PersistentLevel` (`Engine/Classes/Engine/World.h:939`) plus an *"Array of levels currently in this world"* (`:1420`). If Radiant ever splits them — streaming, seamless zones — it needs **both** names back, and the half `.rdlvl` actually is, is the `ULevel` half. Renaming now spends the word we would need later on the concept that is not it.
2. **`PhysicsWorld2D`.** `World` owning a `PhysicsWorld2D` reads as "the world's physics world". Survivable (UE pairs `UWorld` with `FPhysScene`, so ours would become `PhysicsScene2D`) — but that is a second rename riding along, and RAD-95 decision A rejected `World` one day ago partly on this.
3. **Blast radius exceeds the base-class rename by a wide margin,** and part of it is not a rename at all: `.rdlvl` is a committed file format referenced by the `.rdar` asset registry, so either it becomes a content migration or a class called `World` carries a `.rdlvl` extension forever — the two-names-for-one-thing problem, permanently. Plus `LevelSerializer`, `Level::CollisionEvent`, `Level::CollisionCallbackHandle`, every Reaper call site, `Docs/ECS-And-Levels.md`, playbook §4, CLAUDE.md, and Jira titles (RAD-27 is "per-**Level** physics world").

The actual work on that card is answering World-vs-Level, not performing the rename. Until it lands, `Docs/Gameplay-Framework.md`'s mapping table carries the UE correspondence (`UWorld` → `Level`/`GameplayLevel`), which is where the legibility belongs anyway.