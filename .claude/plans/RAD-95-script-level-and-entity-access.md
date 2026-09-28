# Implementation Plan — RAD-95: Native scripts cannot reach their Level or their own Entity

| Field | Value |
|-------|-------|
| **Jira** | [RAD-95](https://hndredgames.atlassian.net/browse/RAD-95) |
| **Epic** | RAD-2 — Phase 2, Simulation Foundation |
| **Story status** | To Do |
| **Dependencies** | None. **Blocks RAD-98** (Gameplay Framework epic). Relates: RAD-94, RAD-30, RAD-97, RAD-58, RAD-90, RAD-92 |
| **Planned** | 2026-08-01 (revised 2026-08-02) |

---

## 0. The Problem, Ground Up

### What the code does today

A **native script** is a C++ class the engine calls once per simulation step so gameplay can happen — a camera that follows the player, a platform that slides, a turret that fires. You write one by subclassing `ScriptableEntity` and overriding `OnUpdate`. The engine attaches it to an **entity** (one thing in the world: the camera, the platform, the turret) and drives it.

Stripped to its load-bearing lines, that is:

```text
// Engine, once per fixed simulation step (Level::OnFixedUpdate)
for each entity that has a script:
    script->OnUpdate(fixedDelta)

// The script base class, stripped
class ScriptableEntity
{
    virtual void OnUpdate(Timestep ts);   // you override this
private:
    Entity m_Entity;                      // WHICH entity this script drives
    friend class Level;                   // ...and only Level may read it
};
```

Read those last two lines together. The engine hands the script its entity, and then locks the door behind it. `m_Entity` is `private`, and the only `friend` is `Level`. Your subclass inherits the field but cannot name it.

*(Terms, once: a **handle** is a small value that identifies something without owning it — a coat-check ticket, not the coat. `Entity` is a handle: 16 bytes, `{entt handle, Level*}`, freely copyable, and it dangles if the thing it names is destroyed. `Level` is the world — the container of all entities plus the loops that update and render them. A **facade** is a type whose only job is to present a deliberately chosen subset of something bigger.)*

### The failure, concretely

Write the most ordinary gameplay script there is — a turret that fires a bullet every two seconds:

```cpp
void Turret::OnUpdate(Timestep ts)
{
    m_Cooldown -= ts;
    if (m_Cooldown > 0.0f) return;
    m_Cooldown = 2.0f;

    Entity bullet = /* ??? */ CreateEntity("Bullet");   // needs the Level
    bullet.Teleport(/* ??? */ MuzzlePosition());        // needs its own Entity
}
```

Neither line can be written. Not "is awkward to write" — there is **no legal C++ expression** inside that function that reaches either one. `CreateEntity` lives on `Level`, and the script holds no `Level`. The muzzle position needs this entity's transform, and the script cannot name its own `Entity`.

The only workaround is to open an engine header and add `friend class Turret;` to `ScriptableEntity` — which means **every game recompiles the engine to add a gameplay script**. That is not a workaround; it is a description of a broken seam.

This stopped being hypothetical the day the card was filed. `Reaper/Source/KinematicPlatform.h` — a platform that slides back and forth — needed RAD-90's `PhysicsBody` verbs. It could not reach them, because reaching them means `m_Entity.GetPhysicsBody()`. Rather than block RAD-90, a stopgap shipped: a fourth method on `ScriptableEntity` that does nothing but call through to the entity.

That fourth method is the real tell. Count what `ScriptableEntity` exposes today:

| Method | What it is |
|--------|-----------|
| `AddComponent<T>()` | forwards to `m_Entity.AddComponent<T>()` |
| `GetComponent<T>()` | forwards to `m_Entity.GetComponent<T>()` |
| `RemoveComponent<T>()` | forwards to `m_Entity.RemoveComponent<T>()` |
| `GetPhysicsBody()` | forwards to `m_Entity.GetPhysicsBody()` — the RAD-90 stopgap |

Four methods, mirroring four of `Entity`'s roughly fifteen. Nobody ever decided on *those four*; they accumulated. And the trend line is the problem: add an ability system and someone writes `GetAbilitySystem()`; add animation and someone writes `GetAnimation()`; audio, navigation, and so on. **One engine-header edit per subsystem, forever** — which is precisely the O(N)-edits-per-feature pattern RAD-94 exists to forbid, reintroduced one layer down from where RAD-94 was looking.

### The fix, as one everyday thing

**A hotel keycard.**

Today's script has been walked into a room and the door locked behind it; when it needs something, a member of staff slides one more tool under the door. That is the forwarder pattern — and it grows one tool-shaped slot per tool, forever.

A keycard is the other shape. One card, in the guest's hand, and it opens the room, the pool and the gym. Nobody has to anticipate which of those the guest will want. New amenity? The same card opens it.

The keycard deliberately does **not** open the boiler room. That is not distrust of guests — it is that the boiler room is where the building is *run*, and a guest turning the boiler off mid-service breaks the building for everyone in it.

Our boiler room is `Level::OnFixedUpdate` and `Level::OnRender` — the methods that *drive the frame*. A script calling `OnFixedUpdate` would step the physics world from inside the physics step, which is a crash waiting for the right timing. So: hand gameplay a card, not a room key — and leave the boiler room off the card.

### How Unreal solves it

Unreal hands out both halves as plain accessors and does not think twice about it:

- `UActorComponent::GetOwner()` — "which Actor do I belong to?" That is our missing `GetEntity()`.
- `AActor::GetWorld()` — "which world am I in?" That is our missing level access.

And the `UWorld*` you get back is the *whole* world, `Tick()` included — `GetWorld()->Tick(LEVELTICK_All, 0.016f)` compiles in Unreal today. Unreal gets away with it because its actual *scripting* layer is Blueprint, and Blueprint can only see functions tagged `UFUNCTION`. `UWorld::Tick` carries no tag, so no script can reach it. **Unreal narrows by annotation.**

Radiant has no reflection system (RAD-72, iceboxed), so we cannot tag a subset of `Level` and have a tool enforce it. Our version of "the tagged subset" has to be a **type**: a small value handle that exposes the safe verbs and structurally cannot express the unsafe ones. Same intent as Unreal's; different mechanism, because we have a different toolbox.

### What we gain, and what we deliberately do not

**Gain:** scripts become real gameplay code — they can spawn, find, destroy, and reach every subsystem. `ScriptableEntity` stops growing one method per subsystem, permanently. The frame-driving methods become unreachable from a script *at compile time*, not by convention.

**Not gained — three honest ones:**

1. **No new engine capability.** Everything the facade exposes was already callable from `GameLayer`. This card changes *who may call*, not *what exists*.
2. **Not a security boundary.** A determined script can `#include "Level.h"` and write `Level` in its own file. What it cannot do is *obtain a `Level*`* from the handle it was given. That closes the accident, not the intent — and closing the accident is the whole job. (Nothing short of a real scripting sandbox closes intent, and that is RAD-58's problem, not this card's.)
3. **Not safe on its own.** Handing scripts the power to spawn and destroy entities, on top of today's script loop, is handing out a loaded gun: that loop iterates a live entt view, and mutating the world from inside it is undefined behaviour (playbook §8.8). **The loop has to be made safe first** — see §1. The card did not ask for that; it is a prerequisite the card implies.

**What we have:** a script that can read its own components and nothing else, plus a class quietly growing one forwarder per engine subsystem.
**What we're building:** a script that holds its own entity handle, and reaches everything else — physics, other entities, the level itself — by chaining off that one handle.

---

## 1. Architecture Decision

### The decisive constraint

The story lists three candidates. One of the Acceptance Criteria settles it outright:

> The frame-driving methods are **not** reachable from gameplay — a script cannot call `OnFixedUpdate`/`OnRender`
> …and the Test Plan: *"Attempting to reach a frame-driving method from a script fails to compile"*

`Entity::GetLevel() → Level*` cannot satisfy that. A `Level*` exposes every public method on `Level`, and `OnFixedUpdate` must stay public because Reaper's `GameLayer` — the legitimate driver — calls it. Making it private and friending game code is worse: the engine would have to name its consumers.

`ScriptableEntity::GetEntity()` alone closes the own-handle half and leaves the level half open, so it is necessary but not sufficient.

That leaves the narrowed facade, and it is the right answer for the stated reason rather than by elimination: **the safe set has to be written down somewhere, and a type is the only place a C++ compiler can read it.**

### The chosen design

Three additions, and one deletion:

1. **`GameplayLevel`** — a value handle over a `Level*` exposing the level-scope verbs gameplay may use. Owns nothing, caches nothing, re-resolves per call. Obtained from `Entity::GetLevel()`, never constructed by gameplay. Exactly the shape `PhysicsBody` established in RAD-90 (playbook §4), applied one level up.
2. **`Entity::GetLevel()`** → `GameplayLevel` by value. RAD-94 category B (relationship navigation): the relationship genuinely exists — `Entity` already stores the `Level*` — so this fakes no graph we do not have.
3. **`ScriptableEntity::GetEntity()` and `ScriptableEntity::GetLevel()`** — the script's two relationship accessors, both returning by value. See "the bounded set" below for why there are exactly two and why that is not the forwarder pattern returning.
4. **Delete every subsystem and component forwarder on `ScriptableEntity`** — `GetPhysicsBody()` (the RAD-90 stopgap, per the card) and the three component forwarders (decision B below).

The reach chain becomes uniform and unbounded:

```cpp
GetEntity()                                      // my handle
GetEntity().GetComponent<TransformComponent>()   // my components
GetEntity().GetPhysicsBody().ApplyForce(...)     // my physics   (RAD-90)
GetEntity().GetAbilitySystem().TryActivate(...)  // my abilities (future — free)
GetLevel().CreateEntity("Bullet")                // the world
```

### The bounded set (why two accessors, not one, and not four)

The rule that matters is **not** "expose exactly one thing" — it is RAD-94's taxonomy:

- **Category C, subsystem facades** (`GetPhysicsBody`, `GetAbilitySystem`, `GetAnimation`) — an **unbounded** set. One per subsystem, forever. This is what must never be forwarded, and what the RAD-90 stopgap proved.
- **Category B, relationship navigation** (`GetEntity`, `GetLevel`) — a **bounded** set, sized by how many relationships actually exist. A script has two: the entity it drives, and the level that entity lives in. It grows only if a genuinely new relationship appears, and RAD-94 already forbids fabricating one ("adopt when the underlying relationship exists, never before" — which is why there is no `GetParent()`).

Unreal draws the identical line: `UActorComponent` carries **both** `GetOwner()` and `GetWorld()` (`ActorComponent.h:519-531`) and does not make callers write `GetOwner()->GetWorld()`. It does not, however, carry a forwarder per engine subsystem.

The concrete payoff is at the call site. Spawning is a *level-scope* operation, so routing it through the script's own entity reads as a non-sequitur — `GetEntity().GetLevel().CreateEntity("Enemy")` says "fetch my entity, fetch its level, make an unrelated entity", where two thirds of that sentence is plumbing. `GetLevel().CreateEntity("Enemy")` says what is happening.

Every future subsystem facade is reachable the day it is written, with **zero** edits to `ScriptableEntity`. That is the entire point.

### The dividing rule (new, and worth writing into the playbook)

Once both `Entity` and `GameplayLevel` carry verbs, "which one does this verb go on?" needs an answer that is not taste:

> **A verb that names one entity lives on `Entity`. A verb about the level as a whole lives on `GameplayLevel`.**

`Teleport` and `Destroy` already follow it (`Entity::Teleport`, `Entity::Destroy` forward to `Level`). `CreateEntity`, `FindEntityByName`, `AddCollisionCallback` are level-scope. The rule is mechanical, so it does not drift.

### The prerequisite the card did not ask for

`Level::OnFixedUpdate` runs scripts like this (`Level.cpp:429`):

```cpp
m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
    {
        ...
        nsc.Instance->OnUpdate(ts);     // ← gameplay runs HERE, inside a live view
    });
```

`Level.h` already states the rule this breaks: *"never destroy entities or add/remove the iterated component types while iterating."* Today no script can break it, because no script can reach `CreateEntity` or `DestroyEntity`. **This card is what makes it reachable.** A spawner binding a script to its new entity, or a script destroying another scripted entity, mutates the very pool being walked — undefined behaviour, and the kind that works in Debug and corrupts in Dist.

There is a second, subtler hazard in the same loop: `nsc` is a **reference into the component pool**. If a script's `OnCreate` or `OnUpdate` adds a `NativeScriptComponent` to any entity, that pool may reallocate and `nsc` dangles — a *pointer* invalidation, distinct from the *iterator* invalidation above, and not fixed by the same thing.

**Decision: fix the loop first, in this card, as step one.** Collect the script entities into a reusable member vector, then walk the vector, re-validating each element and re-fetching the component pointer after any call that can mutate. Snapshot-then-walk, ~20 lines, no steady-state allocation. It converts undefined behaviour into a stated contract:

> Structural changes made from a script take effect **immediately**. The script pass is snapshotted at its start, so an entity spawned during a step first runs its own `OnUpdate` on the **next** step, and an entity destroyed during a step is skipped for the rest of this one.

**This is Unreal's mechanism, in miniature — not a cheap substitute for it.** Unreal's tick loop never iterates the authoritative actor storage; it walks a separately-maintained list of tick functions, and anything registering mid-tick lands in a side buffer (`NewlySpawnedTickFunctions`) drained at defined points. See §2 for the source. Snapshot-then-walk is that idea with the list rebuilt per step instead of maintained.

Two adjacent mechanisms were considered and are **not** substitutes:

- **A deferred command buffer** (`CreateEntity`/`DestroyEntity` enqueue, flush after the pass) would require deferring component adds too — at which point `AddComponent<T>()` cannot return `T&`, which breaks essentially every call site in the engine. Unreal does not do this either.
- **Deferred destruction** (Unreal's `MarkAsGarbage`) is genuinely worth having and is now filed as **RAD-97**, but it does **not** fix this card's hazard: the spawner case is *add*-during-iteration, which deferring destruction does not touch. The two are complementary, not alternatives.

The remaining choice is therefore *rebuilt* list (here) versus *maintained* list (Unreal's shape). Maintained is the scalable end state and is RAD-30's territory — its Level-owned side table keyed by entity **is** the maintained list. Rebuilding costs one extra walk over a set with fewer than five members today, and the loop is written so RAD-30 absorbs it rather than replacing it. See §9.

### Playbook sections applied

§2 (ownership: the facade owns nothing, holds no `Ref`), §3 (registry stays private — the facade never exposes it), §4 (one resolution helper per layer; facades are values that cache nothing), §8.8 (entt view invalidation — the prerequisite above), and the CLAUDE.md rule that no per-frame heap allocation enters a hot path.

---

## 2. UE Reference

Unreal answers both halves of this card with plain relationship accessors, and answers the *narrowing* question somewhere completely different.

**The own-handle half** — `Engine/Source/Runtime/Engine/Classes/Components/ActorComponent.h:519`:

```cpp
	/** Follow the Outer chain to get the  AActor  that 'Owns' this component */
	UFUNCTION(BlueprintCallable, Category="Components", meta=(Keywords = "Actor Owning Parent"))
	AActor* GetOwner() const;
```

**The world half** — `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:3764`:

```cpp
	/** Getter for the cached world pointer, will return null if the actor is not actually spawned in a level */
	ENGINE_API virtual UWorld* GetWorld() const override final;
```

**And the world it hands back is the whole world** — `Engine/Source/Runtime/Engine/Classes/Engine/World.h:3302`, sitting under a plain `public:`:

```cpp
	/**
	 * Update the level after a variable amount of time, DeltaSeconds, has passed.
	 * All child actors are ticked after their owners have been ticked.
	 */
	UE_API void Tick( ELevelTick TickType, float DeltaSeconds );
```

So in Unreal C++, `GetWorld()->Tick(...)` compiles. The boiler room has no lock on it. What stops gameplay walking in is that Unreal's *scripting* layer is Blueprint, and Blueprint sees only what carries a `UFUNCTION` tag — which `Tick` does not. **Unreal narrows by annotation, enforced by a reflection system and a code generator (UHT).**

Where Unreal wants a curated world-scope surface for scripts, it builds `UGameplayStatics`: a static library whose every entry takes the world as an explicit context parameter — `Engine/Source/Runtime/Engine/Classes/Kismet/GameplayStatics.h:97`:

```cpp
	UFUNCTION(BlueprintCallable, Category="Actor",  meta=(WorldContext="WorldContextObject", DeterminesOutputType="ActorClass", DynamicOutputParam="OutActors"))
	static ENGINE_API void GetAllActorsOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors);
```

### How Unreal keeps its own update loop safe (relevant to §1's prerequisite)

Unreal's spawn is **immediate** — `UWorld::SpawnActor` constructs and registers on the spot. What makes that safe is that the tick loop never iterates the authoritative actor storage: it walks a separately-maintained list of tick functions, and a tick function registered *while the tick is running* is diverted into a side buffer — `Engine/Source/Runtime/Engine/Private/TickTaskManager.cpp:1680`:

```cpp
			if (bTickNewlySpawned)
			{
				NewlySpawnedTickFunctions.Add(TickFunction);
			}
```

…drained by `QueueNewlySpawned` at defined points, with runaway protection (`LogAndDiscardRunawayNewlySpawned`) for the spawner-that-spawns-a-spawner case.

Destruction is the separate half — `Engine/Source/Runtime/Engine/Private/LevelActor.cpp:1033,1054`:

```cpp
	RemoveActor( ThisActor, bShouldModifyLevel );
	...
	ThisActor->MarkAsGarbage();
```

Out of the level immediately; memory reclaimed later by GC. That is what makes an `AActor*` survivable, and it is why `IsValid()` exists over there.

**What we adopt from this:** the iteration mechanism, in miniature — §1's snapshot-then-walk. **What we defer:** the maintained-list upgrade (RAD-30) and deferred destruction (RAD-97). **What we reject outright:** the tracing collector. Unreal's GC is affordable because reflection hands it the reference graph for free; without reflection (RAD-72, iceboxed) it degrades into one hand-written `AddReferencedObjects` per type — `Engine/Source/Runtime/CoreUObject/Public/UObject/Object.h:786` — which is the O(N)-edits-per-type pattern RAD-94 forbids. entt already owns entity storage and `Ref`/`Scope` already state ownership, so there is nothing here for a collector to collect; handle validity stays a generation-handle question (playbook §2, decided 2026-07-09).

**What we adopt:** both relationship accessors, one-for-one. `GetOwner()` → `ScriptableEntity::GetEntity()`; `GetWorld()` → `Entity::GetLevel()`.

**What we deliberately do differently:** we express the safe subset as a **type**, not an annotation. We have no reflection and no header tool (RAD-72 is iceboxed), so `UFUNCTION`-style tagging is unavailable — and a comment saying "scripts must not call this" is not enforcement. A facade type gets the same guarantee out of the compiler we already have. It also happens to be *stronger* than Unreal's: in Radiant the frame-driving methods are unreachable from a script's handle in C++ too, where Unreal's are merely untagged.

**What we deliberately simplify:** no `WorldContextObject` free-function library. Unreal needs it because a Blueprint graph node has no implicit `this` to hang a world off. In C++ a per-entity accessor is more discoverable (`entity.GetLevel().` completes; a static library does not) and one indirection shorter. If a *global* query surface is ever wanted, that is RAD-76's decision, not this card's.

---

## 3. File Plan

```text
Radiant/Source/Radiant/ECS/
├── GameplayLevel.h        (new)    — the level's gameplay-safe verb surface, as a value handle
├── GameplayLevel.cpp      (new)    — the forwarders + this layer's one resolution helper
    Entity.h               (modify) — GetLevel() declaration; forward-declare GameplayLevel
    Entity.cpp             (modify) — GetLevel() definition
    ScriptableEntity.h     (modify) — add GetEntity() + GetLevel(); delete all four forwarders
    Level.h                (modify) — m_ScriptUpdateList member + the script-pass contract
    Level.cpp              (modify) — snapshot-then-walk script pass (the prerequisite)
Radiant/Source/Radiant/
    Radiant.h              (modify) — export GameplayLevel.h to games

Reaper/Source/
    CameraController.h     (modify) — GetComponent<T>() -> GetEntity().GetComponent<T>()
    CollisionLogger.h      (modify) — same
    KinematicPlatform.h    (modify) — GetPhysicsBody() -> GetEntity().GetPhysicsBody()
    Layers/GameLayer.cpp   (modify) — RAD-95 verification script + cheat key (own commit)

Docs/ECS-And-Levels.md     (modify) — the scripting seam, the reach chain, lifetime contract
.claude/references/radiant-playbook.md (modify) — the gameplay-seam rule
```

| Action | Path | Description |
|--------|------|-------------|
| Create | `Radiant/Source/Radiant/ECS/GameplayLevel.h` | The facade: entity spawn/find/destroy, the level-wide collision-callback channel, level identity. Documents its own inclusion rule so the next verb's placement is not a guess. |
| Create | `Radiant/Source/Radiant/ECS/GameplayLevel.cpp` | Thin forwarders over one `Resolve(verb)` guard, mirroring `PhysicsBody.cpp`. |
| Modify | `Radiant/Source/Radiant/ECS/Entity.h` | Declare `GameplayLevel GetLevel() const;`, forward-declare `GameplayLevel` (same incomplete-type trick already used for `PhysicsBody`, lines 12–16). |
| Modify | `Radiant/Source/Radiant/ECS/Entity.cpp` | Define `GetLevel()` — returns `GameplayLevel(m_Level)`, false for a level-less handle. |
| Modify | `Radiant/Source/Radiant/ECS/ScriptableEntity.h` | Add `Entity GetEntity() const` and `GameplayLevel GetLevel() const` (the bounded relationship set, §1). Delete `GetPhysicsBody()` and the three component forwarders. Drop the now-unneeded `PhysicsBody.h` include. Rewrite the class doc around the new seam. |
| Modify | `Radiant/Source/Radiant/ECS/Level.h` | Add `std::vector<entt::entity> m_ScriptUpdateList` (reused across steps); document the snapshot contract on `OnFixedUpdate`; rename the collision-callback API (below). |
| Modify | `Radiant/Source/Radiant/ECS/Level.cpp` | Replace the `view().each()` script pass with snapshot-then-walk, with per-element re-validation and component re-fetch; apply the collision-callback rename. |
| Modify | `Radiant/Source/Radiant/ECS/Entity.h` / `.cpp` | Also add `RefreshCollider()` — the forwarder that closes the dividing rule's one hole. |
| Modify | `Radiant/Source/Radiant/Radiant.h` | Include `GameplayLevel.h` — Reaper includes only `<Radiant/Radiant.h>`, and needs the complete type to call through the handle. |
| Modify | `Reaper/Source/*.h` (3 scripts) | Route every entity access through `GetEntity()`. Mechanical. |
| Modify | `Reaper/Source/Layers/GameLayer.cpp` | RAD-95 verification scaffolding — its own commit, enrolled in RAD-92 first (see §5 Phase 4). |
| Modify | `Docs/ECS-And-Levels.md` | The scripting seam section: the reach chain, what is deliberately absent, the lifetime contract, the script-pass ordering contract. |
| Modify | `.claude/references/radiant-playbook.md` | The gameplay-seam rule + the `Entity`-vs-`GameplayLevel` dividing rule. |

---

## 4. Type Design

### GameplayLevel

- **Kind:** class (value handle)
- **Responsibility:** present the subset of `Level` that gameplay may use, and structurally exclude the rest.
- **Ownership:** owns nothing. Holds a raw, non-owning `Level*`. Deliberately **not** a `Ref<Level>` — a gameplay handle that keeps a level alive inverts ownership (the `GameLayer` owns the level) and would cost a refcount touch per acquisition.
- **Lifetime & threading:** TRANSIENT, exactly like `Entity` and `PhysicsBody`. Obtain it, use it, drop it. Storing one across a level transition dangles precisely as an `Entity` does. Main-thread only.
- **Size:** 8 bytes. Copy freely.
- **Key Members:**
  - `Level* m_Level` — the only state.
  - `Level* Resolve(const char* verb) const` — **private**, this layer's single guard (playbook §4, one resolver per layer). Null level → `RADIANT_WARN` naming the verb → null. No verb carries its own preamble.
  - `IsValid()` / `operator bool()` — "does this handle name a live level?"
  - `CreateEntity(name)` → `Entity`
  - `DestroyEntity(Entity)` / `DestroyEntity(UUID)`
  - `FindEntityByName(std::string_view)` → `Entity` (O(n), documented)
  - `GetEntityByUUID(UUID)` → `Entity`
  - `AddCollisionCallback(std::function<void(const Level::CollisionEvent&)>)` → `Level::CollisionCallbackHandle`
  - `RemoveCollisionCallback(Level::CollisionCallbackHandle&)`
  - `GetUUID()` / `GetName()` — identity, for logging and cross-level references
- **Deliberately absent, and why each:**

| Not exposed | Reason |
|-------------|--------|
| `OnFixedUpdate` / `OnRender` / `OnViewportResize` | The boiler room. Frame driving belongs to the owner (`GameLayer`), and a script re-entering the fixed update would step the physics world from inside the step. |
| `CreateEntityWithUUID` | Deserialization path. A caller-chosen UUID that collides *silently replaces* the previous entry in the UUID map — a documented footgun that gameplay has no reason to hold. |
| `GetAssetList` | Tooling/serialization concern, not gameplay. |
| `SetName` | Authoring operation. Running the game never mutates authored content (playbook §6). |
| `Teleport` / `RefreshCollider` | Entity-scope verbs — they live on `Entity` by the dividing rule in §1. `Entity::Teleport` already exists; **`Entity::RefreshCollider` is added by this card** (locked 2026-08-02) so the rule is true on day one rather than aspirational. |
| Any escape hatch (`GetLevel()`, `operator Level*`) | Would hand back the room key and undo the card. **Never add one.** |

- **Playbook Patterns:** §2 (owns nothing, no `Ref`), §3 (registry never exposed), §4 (one resolver per layer; value facade, caches nothing).

### Entity — additions

- `GameplayLevel GetLevel() const` — defined out of line in `Entity.cpp` (returns an incomplete type by value from the declaration; the same reason `GetPhysicsBody()` is defined there). Always returns a handle; false when the entity has no level. Idiom: `if (GameplayLevel level = entity.GetLevel())`.
- **Const-correctness note, stated so review does not re-litigate it:** `GetLevel() const` hands back a handle through which the level can be mutated — the same latitude `GetPhysicsBody() const` already takes. `const` on a handle constrains the handle, not the thing it names, exactly as with a pointer. Documented at the declaration.

### ScriptableEntity — after the change

- **Kind:** abstract base class (a *behaviour* attached to an entity — no longer a partial impersonation of one).
- **Public surface, in full:** `virtual ~ScriptableEntity()`, `Entity GetEntity() const`, `GameplayLevel GetLevel() const`, and the `==`/`!=` operators.
- **Protected surface:** the lifecycle hooks, unchanged — `OnCreate`, `OnUpdate`, `OnDestroy`, `OnCollisionBegin`, `OnCollisionEnd`.
- **`GetLevel()` is a one-line forward to `m_Entity.GetLevel()`** and is bounded, not accreted — see "the bounded set" in §1. Its doc comment must say so, naming the category-B/category-C distinction, or the next person reads it as licence to add a third accessor.
- **Why both are public, not protected:** protected would serve the script itself but push every *other* caller back to `friend class Level`, which is the disease. It is also the shape a non-C++ host has to bind (RAD-58). A script's entity is not a secret — it is the one fact about the script that everything else needs.
- **Kept:** `friend class Level` — the Level still *wires* `m_Entity` after construction. The friendship stays for writing; the getter opens reading. That asymmetry is correct: only the Level may say which entity a script drives.

---

## 5. Implementation Steps

### Phase 1 — Make the script pass survive structural change (prerequisite)

- [x] **Add the reusable snapshot buffer** — `std::vector<entt::entity> m_ScriptUpdateList;` on `Level`, cleared (not reallocated) each step. A local vector would heap-allocate every fixed step, which the CLAUDE.md performance rule forbids in a hot path; a member vector reaches steady state after a few steps and never allocates again.
- [x] **Rewrite the script pass as snapshot-then-walk** — collect handles from the view, then iterate the vector. Per element, in this order: `m_Registry.valid(handle)` → `try_get<NativeScriptComponent>` → lazy-instantiate if needed → **re-validate and re-fetch** after `OnCreate` → active check → `OnUpdate`. Keep the existing "assign `Instance` *before* calling `OnCreate`" order: a script that destroys its own entity in `OnCreate` must be findable by `DestroyEntity`, or the instance leaks and `OnDestroy` never runs.
- [x] **Document the ordering contract on `Level::OnFixedUpdate`** — spawned-this-step entities first update next step; destroyed-this-step entities are skipped for the remainder. This is now a promise gameplay may rely on, not an implementation detail. *(Also documents the one un-interpolated frame for a mid-step spawn.)*
- [x] **Verify nothing downstream broke** — the move-event drain asserts every moved entity is still in `m_EntityMap`. Scripts still run strictly *before* `Step`, so the invariant holds; its comment now states that scripts **can** destroy entities and why the ordering keeps this an assert rather than a guard.

*Phase 1 verified 2026-08-02: all three configs clean with zero warnings; 6 s headless Debug run reached gameplay and drove the rewritten loop ~270 fixed steps (540 default-`OnUpdate` lines from the two `CollisionLogger` instances), 4 script instances lazily created and all 4 torn down through `OnDestroy`, collisions dispatching normally, clean exit, no `[W]`/`[E]`/assert lines. **Not yet exercised:** spawn/destroy from a script and the re-fetch-after-`OnCreate` path — nothing spawns from gameplay until Phase 4's verification script exists.*

### Phase 2 — The seam

- [x] **Write `GameplayLevel`** — header with the full contract doc (transient lifetime, owns nothing, the inclusion rule, the exclusion table from §4), `.cpp` with `Resolve(verb)` plus one two-line forwarder per verb. Model it directly on `PhysicsBody.h`/`.cpp`; the shapes should be recognisably siblings.
- [x] **Add `Entity::GetLevel()`** — forward-declare `GameplayLevel` in `Entity.h` with the same explanatory comment `PhysicsBody` carries (lines 12–16); define in `Entity.cpp`.
- [x] **Add `Entity::RefreshCollider()`** — a zero-logic forwarder to `Level::RefreshCollider`, identical in shape to `Entity::Destroy` (`Entity.cpp:45`): warn on a level-less handle, otherwise forward. Closes the one hole in §1's dividing rule — a script that mutates its own collider can now re-apply it without reaching for the Level.
- [x] **Add `ScriptableEntity::GetEntity()` and `GetLevel()`** and rewrite the class doc comment around the new seam — what they are, what chains off them, why the relationship set is bounded at two, and why there are no subsystem forwarders. *(Provisional-name note pending, with the doc rewrite in Phase 3.)*
- [x] **Export from `Radiant.h`** so Reaper sees the complete type.

*Phase 2 notes (2026-08-02):*
- *The collision-callback rename (Phase 3, decision D) was pulled forward and done **first**, so `GameplayLevel` was written against the final names rather than being touched twice.*
- *`GetLevel()` is defined in a new `ScriptableEntity.cpp` rather than inline: returning `GameplayLevel` by value needs the complete type, and including `GameplayLevel.h` (→ `Level.h` → `GameApplication.h`) in `ScriptableEntity.h` would make every script header expensive.*
- *Giving `ScriptableEntity` its own translation unit exposed a latent bug: the header names `Timestep` without including it, and had only ever compiled inside `Level.cpp`, which pulled it in first. Include added — a header that is never compiled alone is never proven self-contained.*
- *New `.cpp` files require `Scripts/Setup-Windows.bat` before building; premake globs the tree but the `.vcxproj` is generated and stale until regenerated.*

### Phase 3 — Retire the accidental surface

- [x] **Delete `ScriptableEntity::GetPhysicsBody()`** — the RAD-90 stopgap, with the removal trigger its own doc comment named. Drop the `PhysicsBody.h` include with it. *(Dropping the include broke Reaper: it had been getting `PhysicsBody`'s complete type transitively through this header. Correct home is the umbrella — `Radiant.h` now exports both facades explicitly, with a comment saying why.)*
- [x] **Delete the three component forwarders** (decision B, locked) and fix the call sites: `CameraController.h`, `CollisionLogger.h`, `KinematicPlatform.h`. *Five `GetEntity()` call sites across all three scripts. `CameraController`'s rewrite replaced three component fetches with one — the forwarder had made a sparse-set lookup read like a member access, hiding a double fetch on adjacent lines. `CollisionLogger::Name()` now forwards to `Entity::Name()`.*
- [x] **Update `Docs/ECS-And-Levels.md`** — *Native scripts* rewritten around the reach chain, with a problem-first "what a script can reach" subsection, the bounded/unbounded rule, the `GameplayLevel` narrowing and its exclusions, the lifetime chain, and the spawn/destroy contract. Two new *Known Issues* entries (RAD-97 immediate-destruction cost, RAD-30 rebuilt-vs-maintained list).
- [x] **Add the seam rule to the playbook** — new **§10 The Gameplay Seam**: bounded-vs-unbounded accessors, the dividing rule, narrow-by-type, document-your-exclusions, umbrella-exports-facades, and headers-need-their-own-TU. §8.8 extended with the reference-vs-iterator invalidation distinction.
- [x] **Rename the collision-callback API** (chore, own commit) — `AddCollisionObserver` → `AddCollisionCallback`, `RemoveCollisionObserver` → `RemoveCollisionCallback`, `CollisionObserverHandle` → `CollisionCallbackHandle`, and the private `CollisionObserver` slot struct → `CollisionCallback` (its `Callback` member becomes `Function` to avoid `CollisionCallback::Callback`). Also `m_CollisionObservers` → `m_CollisionCallbacks`, `m_FreeObserverSlots` → `m_FreeCallbackSlots`, `m_PendingObserverReleases` → `m_PendingCallbackReleases`, and `ReleaseObserverSlot` → `ReleaseCallbackSlot`. Touches `Level.h`, `Level.cpp`, `GameLayer.cpp/.h`, `Docs/Physics.md`, `Docs/ECS-And-Levels.md`, playbook §4.
- [ ] **Record the rename on RAD-29** as an Architecture Realignment note (the `/jira` convention) — RAD-29 is In Review, and its API must not change silently underneath it.
*Phase 3 verified 2026-08-02: Debug/Release/Dist clean, no new warnings. 16 s headless run (~2 full platform sweeps): 1646 script ticks, 4 instances created and destroyed, clean exit. The load-bearing evidence is an absence — `KinematicPlatform <-> PlatformRider` logged a **BEGIN and no END** across both sweeps. The platform's half-width is 2.0 and its centre travels ±3.0, so an uncarried rider would leave its span at each extreme and END the contact. It never did, so friction is still dragging the rider — i.e. `MoveKinematic` still works through `GetEntity().GetPhysicsBody()` with the forwarder gone.*

### Phase 4 — Prove it (separate commit)

- [x] **Enroll the scaffolding in RAD-92 *before* writing it** — done first. RAD-92's description now lists `SpawnProbe.h` and the `N` cheat in its inventory, carries a dated 2026-08-02 Scope Amendment explaining why both details are load-bearing, adds `SpawnProbe.h` to its deletion AC, and notes this is the *easiest* of the three scaffolds to replace with a test (entirely headless, no visual component). Linked Relates ↔ RAD-95.
- [x] **Write the verification script** — `Reaper/Source/SpawnProbe.h`: `SpawnProbe` spawns an entity through `GetLevel().CreateEntity(...)` **and binds `ProbePassenger` to it**, then destroys it by UUID on the next toggle. Both from inside `OnUpdate`, i.e. inside the script pass.
- [x] **Prove the negative** — verified by compiler, then reverted:
      `error C2039: 'OnFixedUpdate': is not a member of 'Radiant::GameplayLevel'`
      `error C2039: 'OnRender': is not a member of 'Radiant::GameplayLevel'`
- [x] **Build all three configs via CLI MSBuild, then run Reaper** — see below.

*Phase 4 notes (2026-08-02):*
- *The probe **auto-toggles on a 2 s simulated-time timer** as well as on the `N` key. Added deliberately: a key-only probe cannot be exercised headlessly, and a headless **Dist** run is exactly where the undefined behaviour would surface. The cheat key remains for interactive hammering.*
- *`GAME_TRACE` compiles out in Dist (`Log.h:45`, `#ifndef RD_DIST`), so the Dist run proves **absence of crash/corruption**, not pairing. Pairing is proven in Debug. Stated rather than blurred — these are different claims.*

*Phase 4 verified 2026-08-02: Debug/Release/Dist clean, no new warnings.*
- *Debug, 20 s: **4 spawns, 4 destroys, 4 `passenger OnCreate`, 4 `passenger OnDestroy`**, in strict `spawn → OnCreate → destroy → OnDestroy` order, zero "already gone" warnings, zero `[E]` lines, clean exit. No orphans, no leaks.*
- *Dist, 20 s: clean exit, empty stderr, zero `[E]`. Gameplay demonstrably ran (both RAD-29 collision cheats fired), so the script pass executed spawn/destroy many times under optimized allocation without corrupting the iterated pool.*
- *Caveat on the ordering contract: the log confirms `OnCreate` follows the spawn and never precedes it. That it lands on the **next** step specifically follows by construction — the snapshot is taken before the spawn, so the new entity cannot be in this step's list — rather than being independently proven by second-resolution timestamps.*

---

## 6. Ownership & Lifetime Strategy

Nothing in this card owns anything new. That is the design, not an omission.

| Thing | Owner | Lifetime |
|-------|-------|----------|
| `Level` | `Ref<Level>` held by Reaper's `GameLayer` | Released in `GameLayer::OnDetach` |
| `GameplayLevel` | Nobody — a 8-byte value | Obtain, use, drop. Dangles if the Level dies. |
| `Entity` | Nobody — a 16-byte value | Unchanged (transient, per `Entity.h`) |
| `ScriptableEntity` instance | `NativeScriptComponent` (an owning raw pointer — the known RAD-30 violation) | Deleted by `Level::DestroyEntity` after `OnDestroy` |
| `m_ScriptUpdateList` | `Level` | Lives as long as the Level; cleared per step, never shrunk |

**The hazard worth naming:** the chain `script → Entity → GameplayLevel → Level` is three non-owning hops. Every one of them dangles if the Level dies, and none of them can detect it. This is not new — `Entity` has always had it — but the card widens the blast radius, so the contract is stated identically on all three types: *transient, obtain-use-drop, never store across a level transition.*

**The one real trap, and it already exists:** `AddCollisionCallback` takes a `std::function` the Level owns by value, so it owns whatever the callback captured. A script registering a callback that captures `this` and never removing it in `OnDestroy` leaves the Level holding a callable into a deleted script instance — a use-after-free on the next collision. `Level`'s doc already states this; exposing it to scripts makes the trap much easier to fall into, so it is restated on `GameplayLevel::AddCollisionCallback` in the imperative: *a script that registers a collision callback must remove it in `OnDestroy`.*

**RAD-30 interaction:** moving script instances into a Level-owned side table changes *where* `m_Entity` is wired and who owns the instance. It does not touch the seam — `GetEntity()` returns the same handle either way. Phase 1's snapshot loop is where RAD-30 lands, and snapshot-then-walk is friendlier to a side table than a view walk was.

---

## 7. Performance Notes

**Hot path — the script pass (once per fixed step, currently ~2 scripts):**

- **Added:** one extra walk over the `NativeScriptComponent` pool to collect handles, plus a `registry.valid()` + `try_get()` per element in the second walk (the old code got its component from the view for free). Two pointer-chases per script per step against a set that is currently smaller than five and would be surprising above a few hundred.
- **Allocation:** zero at steady state. `m_ScriptUpdateList` is a member, `clear()`d not destroyed, so capacity is reached within the first few steps and never reallocates. This is the whole reason it is a member — a local `std::vector` here would be a per-step heap allocation, which CLAUDE.md forbids in a hot path.
- **Honest cost statement:** this is strictly slower than the current loop. It buys defined behaviour, which the current loop does not have once scripts can spawn. If the script count ever reaches the thousands, the answer is the *maintained* list (RAD-30, §9.2), not reverting this.

**The seam itself — no hot-path impact:**

- `GetEntity()` returns a 16-byte copy; `GetLevel()` copies one pointer into an 8-byte handle. Both trivially inlined.
- Every `GameplayLevel` verb is one null check plus one call into the same `Level` method gameplay would have called directly. No allocation, no virtual dispatch, no indirection beyond the forward.
- **Removed cost:** none. The forwarders being deleted were already zero-cost; the win there is API shape, not speed.

**GPU:** untouched.

---

## 8. Logging & Diagnostics

Follows the RAD-90/RAD-91 policy exactly (playbook §4): *guards on caller mistakes warn and recover; asserts are for programmer errors; rare and discontinuous verbs log, routine ones do not.*

| Situation | Response |
|-----------|----------|
| Verb called on a level-less `GameplayLevel` | `RADIANT_WARN` naming the verb, from `Resolve` — the one place, never copied per verb |
| `DestroyEntity` with a stale handle | Already handled by `Level::DestroyEntity` (warns and returns). The facade does not double-warn. |
| `AddCollisionCallback` with an empty callable | Already asserted by `Level::AddCollisionCallback`. Not duplicated. |
| `NativeScriptComponent` with no `Bind<T>()` | Existing assert, preserved verbatim through the loop rewrite |
| Script entity destroyed mid-pass | **Silent skip.** This is normal and expected under the new contract — warning would fire on correct gameplay. |
| Spawn/destroy from a script | **Nothing.** Routine gameplay; a spawner at 60 Hz would drown the log. |

The verification script in Phase 4 logs its own spawn/destroy pairs with `GAME_TRACE` — that is *game* diagnostics proving the AC, and it retires with RAD-92.

---

## 9. Scalability Review

**1. The facade is a forwarder per verb — is that the O(N) pattern RAD-94 forbids?**

Nearly, and the distinction matters, so state it rather than assume it. RAD-94's category-A rejection is about growth keyed to *content*: one accessor per component type means every new component edits a core header. `GameplayLevel` grows by one forwarder per new **gameplay-safe level verb** — a set that has grown by roughly three entries across all of Phase 2, and where each addition is a deliberate "is this safe for gameplay to call?" decision. **That decision is the feature.** The forwarder is where the decision is recorded.

The honest failure mode is the opposite one: someone adds a `Level` verb and forgets to expose it, so gameplay silently cannot reach it. Mitigation: `GameplayLevel`'s doc comment states the inclusion rule and the exclusion table, so the next person meets the decision instead of guessing. **Verdict: accept, no follow-up.**

**2. A rebuilt script list versus a maintained one.**

Rebuilding costs one extra O(N) walk per step over a set with fewer than five members. Unreal's shape is a *maintained* list — tick functions registered once, plus a side buffer for mid-pass registrations — which is O(1) amortized and is what scales. That upgrade is **RAD-30's**: its Level-owned side table keyed by entity *is* the maintained list, so building it here would be doing part of RAD-30 with none of its context. The loop is written to be absorbed by it (it asks the registry per element rather than riding a component reference), not replaced.

Two things this does **not** address, both now tracked: entity-lifetime deferral is **RAD-97**, and the ordering non-determinism that appears once many scripts spawn and destroy each other stays open — it is invisible below a handful of scripts and is properly a consequence of whatever RAD-30 and RAD-97 settle on. **Verdict: correct for today's scale, and shaped so the scalable version is an upgrade rather than a rewrite.**

**3. Does the seam survive a non-C++ scripting host (RAD-58)?**

Better than the alternatives, with one caveat worth writing down now. A binding generator's instruction becomes *"bind `Entity`, `PhysicsBody`, `GameplayLevel`; do not bind `Level`"* — three whole types instead of a hand-curated method list that drifts from the header. The caveat: all three are **transient raw-pointer handles**, and a garbage-collected host will happily keep one alive past its level. RAD-58 must therefore bind them as by-value structs passed per call, never as stored managed objects. **Verdict: sound, and the constraint belongs in RAD-58's notes now, while the reason is fresh.**

**4. Does `ScriptableEntity` still grow?**

No. That is the measurable outcome of the card: its public surface becomes fixed at the two relationship accessors plus comparison operators, and every future subsystem facade costs zero edits to it. The bounded/unbounded distinction in §1 is what keeps that true — a third accessor requires a third *relationship*, not a third subsystem. **Verdict: this is the scalability win the card exists for.**

---

## 10. Risks & Edge Cases

- **The prerequisite is the risk.** Shipping Phase 2 without Phase 1 produces a seam that looks correct and corrupts memory under a spawner. Order is not negotiable, and Phase 4's verification script must genuinely mutate the iterated pool (spawn *with* a bound script) or it proves nothing.
- **Pointer invalidation is a separate bug from iterator invalidation.** Re-fetching `nsc` after `OnCreate` is not paranoia — a script that spawns a scripted entity in `OnCreate` reallocates the pool the caller is holding a reference into. Easy to "simplify" away in review; the comment must say why it is there.
- **Self-destruction inside `OnUpdate`.** A script destroying its own entity deletes the instance whose method is running. `OnCollisionBegin`'s doc already states the rule (*"it must be the last statement"*); `OnUpdate` needs the same sentence now that destruction is reachable from it.
- **Collision-callback leak from a script.** Registering a level-wide callback capturing `this` without removing it in `OnDestroy` is a use-after-free on the next collision — the Level owns the callable and cannot detect that its subscriber died. Documented imperatively on the facade; a candidate for a future debug-build leak check (not this card).
- **Render interpolation for a mid-step spawn.** The transform-snapshot pass runs *before* scripts, so an entity spawned during a step has no `TransformSnapshotComponent` for that frame and renders un-interpolated once. Correct (there is no previous pose to blend from) and self-correcting next step — but it will be noticed, so document rather than "fix".
- **`GameplayLevel::FindEntityByName` is O(n).** Handing scripts an easy linear scan invites a per-frame lookup in every script. The doc comment must carry the warning `Level`'s does, in the imperative: resolve once in `OnCreate`, store the UUID, not the handle.
- **Reaper compile churn.** Removing the component forwarders touches three script files. Small, mechanical, and it is the proof that the seam is complete — if something cannot be re-expressed through `GetEntity()`, the facade is missing a verb.
- **Pre-existing gap surfaced by the dividing rule, now closed in this card:** `Level::RefreshCollider` had no `Entity` forwarder, so a script mutating its own collider could not re-apply it. Six mechanical lines identical to `Entity::Destroy`. Included (locked 2026-08-02) rather than deferred, because a dividing rule with a hole in it on the day it ships is a rule nobody will trust.

---

## 11. Verification (AC → proof)

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| A script can obtain its own `Entity` handle | `KinematicPlatform` and `CollisionLogger` compile and run using `GetEntity()` with no forwarders present; platform still oscillates, crate still rides it |
| A script can spawn, find, and destroy entities and register a collision callback, with no back-door `friend` | Phase 4 verification script does all four through `GetLevel()`; `git diff` shows no `friend` added to `ScriptableEntity`, `Entity` or `Level` |
| `ScriptableEntity::GetPhysicsBody()` is removed | Absent from the header; `KinematicPlatform` drives its platform through `GetEntity().GetPhysicsBody()` |
| No per-subsystem forwarder survives on `ScriptableEntity` | Header review: public surface is `GetEntity()`, `GetLevel()` and the comparison operators, nothing else. The two accessors are category-B relationship navigation (a bounded set), not category-C subsystem facades (the unbounded set the criterion names) — see §1 |
| Frame-driving methods are not reachable from gameplay | Add `GetLevel().OnFixedUpdate(ts)` to a script → **compile error**, capture it, then delete. `GameplayLevel` exposes no `Level*` and no conversion. |
| The entt registry remains private; no new `friend` on `Level` | `GameplayLevel` forwards only to public `Level` methods, so it needs no friendship at all — confirm by grep |
| The seam is documented with its lifetime contract and survives a non-C++ host | `Docs/ECS-And-Levels.md` carries the reach chain, the transient contract, and the exclusion table; §9.3's binding constraint recorded on RAD-58 |
| Compiles clean in Debug/Release/Dist, no new warnings | CLI MSBuild all three configs per CLAUDE.md; compare warning counts against a pre-change build |
| Reaper runs and is visually verified | Interactive run: platform oscillates, rider is carried, collision logs pair correctly, spawn/destroy cheat leaves no orphan and no crash |
| **(Test Plan)** Spawn on one key press, destroy on the next — no crash, no leak, correct log pairing | Phase 4 script, run in **Debug and Dist** (the entt hazard is exactly the kind that hides in Debug); press the key ~20 times and confirm paired traces and a stable entity count |

---

## 12. Decisions

**A. The facade's name — LOCKED 2026-08-02: `GameplayLevel`.** "The level, as gameplay sees it", and it reads correctly at the only place the type name appears (`GameplayLevel level = entity.GetLevel();`). Alternatives considered: `LevelView` (rejected — "view" implies read-only, and this spawns and destroys), `LevelHandle` (rejected — `TimerHandle`/`AssetHandle`/`CollisionObserverHandle` all mean *identity token*, so reusing "Handle" for a verb facade overloads established vocabulary), `World` (rejected — collides with `PhysicsWorld2D`, and two names for one concept is worse than a slightly long one).

**B. The three component forwarders — LOCKED 2026-08-02: delete them.** The O(N) argument does not apply to them (they are templates and do not grow per subsystem), so the case rests elsewhere: they are three of `Entity`'s fifteen methods mirrored for no stated reason, which makes the surface unpredictable — a reader cannot tell which of `Entity`'s methods exist here without checking. `GetEntity().GetComponent<T>()` also says *whose* component, where the bare forwarder silently means "mine". Cost: three call sites in Reaper and a few extra characters per access forever.

The Gameplay Framework decision (RAD-98, 2026-08-02) settles the case rather than reopening it. A framework base class *should* have an ergonomic surface — but a designed one: `AActor` fronts `GetActorLocation()`/`SetActorRotation()`, **named gameplay verbs**, and keeps `FindComponentByClass` as the escape hatch rather than the idiom. Deleting the accidental generic forwarders now is what makes room for that designed surface later; keeping them would seed the framework's API with component plumbing.

**C. Renaming `ScriptableEntity` — AGREED, sequenced to RAD-99 (first commit of that card).** The name describes a Hazel implementation detail rather than the role the class is about to take as the root of the gameplay hierarchy. It does **not** happen in this card, for a specific reason rather than tidiness:

**The name is a consequence of the composition decision (RAD-101), not an independent choice.**

- *One* behaviour per entity, permanently → the behaviour **is** the world-thing → `Actor` is right, and `Entity` gets re-explained as its handle (which is honest: UE's `AActor` is the object, `TWeakObjectPtr` is the handle).
- *Many* behaviours per entity → it is one of several attached to a world-thing → `Actor` is actively wrong, and the right shape is `Behaviour`/`EntityBehaviour` (Unity's `MonoBehaviour` is the exact structural analog: holds its `gameObject`, has `Start`/`Update`, you subclass it).

Naming before that is settled risks renaming twice across `Level.cpp`, `Components.h`, `Radiant.h`, `Docs/`, the playbook and every Reaper script. RAD-99's AC requires the composition question to be answered before the name is picked, and lands the rename as its own commit ahead of the verb-surface work so the design diff stays reviewable.

**What RAD-95 does about it:** one sentence in the rewritten class doc noting the name is provisional pending RAD-99. Free, and it stops the name reading as endorsed by a card that just redesigned the class.

**D. Collision-callback API rename — LOCKED 2026-08-02: `AddCollisionCallback` / `RemoveCollisionCallback` / `CollisionCallbackHandle`, in this card.** "Observer" names the GoF pattern, which is implementation trivia — it tells a reader nothing about what they get or what they owe. CLAUDE.md's naming rule ("a name states exactly what the thing is and nothing more") points at "callback", which is what it literally is, and keeps the `*Handle` vocabulary consistent with `TimerHandle`/`AssetHandle`.

Rejected: `Listener` (Box2D v2's `b2ContactListener` precedent, but it is also a pattern name — the same objection). Deferred: a UE-style multicast delegate (`Level::OnCollision().Add(...)`), which is the right long-term shape and generalises to every future engine event, but needs a `MulticastDelegate<T>` utility and belongs to RAD-98.

**Why in this card rather than its own:** RAD-29 is In Review with exactly one caller — a lambda in `GameLayer::OnAttach`. After RAD-95 every script in the game can register one. This is the cheapest the rename will ever be. It lands in its own commit, and RAD-29 gets an Architecture Realignment note so the API does not change silently under a card in review.

**E. `ScriptableEntity::GetLevel()` — LOCKED 2026-08-02: include it.** An earlier draft of this plan exposed only `GetEntity()`, on a "one keycard, everything chains off it" rule, and rejected `GetLevel()` as a second forwarder. That rule was wrong, and the call site is what exposed it: `GetEntity().GetLevel().CreateEntity("Enemy")` routes a *level-scope* operation through the script's own entity, so two thirds of the expression is plumbing.

The correct rule is RAD-94's taxonomy, not a count: category-C subsystem facades are an **unbounded** set (one per subsystem, forever — the RAD-90 stopgap's failure mode) and must never be forwarded; category-B relationship navigation is a **bounded** set, sized by the relationships that actually exist. A script has two. Unreal agrees explicitly — `UActorComponent` carries both `GetOwner()` and `GetWorld()` (`ActorComponent.h:519-531`) and no per-subsystem forwarder.

The guard against drift: a third accessor requires a third *relationship* to exist first (RAD-94: "adopt when the underlying relationship exists, never before"). The class doc states this so the next addition meets a decision rather than a precedent.

**F. `Gameplay/` module placement — deferred to RAD-99 (agreed 2026-08-02).** `ECS/` is becoming a grab bag: storage (`Level`), handles (`Entity`), a facade named for another subsystem (`PhysicsBody`), scripts, components, the serializer — and this card adds `GameplayLevel`. The right home for the entity-scoped facades and the framework classes is a `Gameplay/` module, which RAD-98 needs anyway for `Pawn`/`Controller`.

`GameplayLevel` is nonetheless created in `ECS/` here, deliberately: creating it in a new `Gameplay/` folder would leave `PhysicsBody` behind in `ECS/` — two facades of the same category in two places, which is a worse intermediate state than both being in the old place. RAD-99 moves them together. Note for whoever does it: `PhysicsBody` cannot move *down* into `Physics/` — it stores an `Entity` by value, so its header must include `ECS/Entity.h`, which transitively pulls `Renderer/SceneCamera.h` and `Renderer/Texture.h` into the physics module. `PhysicsWorld2D` escapes this only because it takes `Entity` as a *parameter* and can forward-declare it (`PhysicsWorld2D.h:17`).