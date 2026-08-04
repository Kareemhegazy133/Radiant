# ECS & Levels

**Status:** Working — component-hygiene and physics-sync rework planned (Phase 2: RAD-28…RAD-30).

## The Problem This Solves

How do you represent "things in the world"? The instinctive answer — a class hierarchy (`GameObject` → `MovingObject` → `Character` → `Enemy`…) — rots fast: every new *combination* of abilities (a moving light? a camera attached to physics?) fights the tree, and features end up welded into base classes everything inherits.

ECS (Entity Component System) decomposes the idea into three parts. An **entity** is just an ID — a bare name with nothing attached. **Components** are plain data you attach to that ID: a transform, a sprite, a rigidbody. **Systems** are loops that say "for every entity that has components X and Y, do Z." Behavior emerges from *composition*: an entity is what it **has**, not what it inherits. Give an entity a `RigidBody2D` and it falls; add a `Sprite` and it's visible; there is no `FallingVisibleThing` class anywhere.

The picture to keep: a **spreadsheet**. Entities are rows, component types are columns, and systems walk "every row with values in these two columns." That layout is also why it's fast — same-typed components sit contiguously in memory, which CPUs read at full speed (arranging that storage is entt's whole job). A `Level` is one such spreadsheet: one world of entities plus the loops that update and render it.

## Architecture

### Level and Entity

A `Level` (`ECS/Level.h`) is the world: it privately owns an `entt::registry` (entt 3.13.2, vendored single header) and exposes entity management, the update/render loops, and serialization. The registry is **never** exposed publicly — all access goes through `Level`/`Entity` APIs.

`Entity` (`ECS/Entity.h`) is a 16-byte value handle `{entt::entity, Level*}` with templated component access (`AddComponent`, `GetComponent`, `HasComponent`, …) forwarding to the registry. Entities are identified persistently by `UUID` (random 64-bit) via a `Level`-owned map `UUID → Entity`; `entt` handles are transient and never serialized.

Composition in practice — this is Reaper spawning a physical object, and the whole point of ECS in five lines:

```cpp
auto square = m_Level->CreateEntity("Green Square");
square.AddComponent<SpriteComponent>(glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
square.AddComponent<RigidBody2DComponent>(RigidBody2DComponent::BodyType::Dynamic);
square.AddComponent<BoxCollider2DComponent>();
// no FallingGreenSquare class exists anywhere — the entity IS this list of components
```

Every entity carries `MetadataComponent` (UUID, tag string, `IsActive`) and `TransformComponent` (translation / Euler-radians rotation / scale, TRS matrix on demand), added at creation.

**Reaching a subsystem from an entity (RAD-90).** Beyond component access, `Entity` exposes *subsystem facades* — value handles that gather one subsystem's verbs for one entity:

```cpp
// ECS/PhysicsBody.h — 16 bytes, owns nothing, re-resolves per call
if (PhysicsBody body = entity.GetPhysicsBody())
    body.ApplyLinearImpulse({ 0.0f, 5.0f });
```

The shape is Unreal's `GetPhysicsActor()` idiom adapted to an ECS, and the distinction is worth stating because it is easy to get backwards. A facade is a namespace for **verbs**, so the handle knows which component stores the runtime state and `Entity` does not. Radiant deliberately does **not** add typed component wrappers (`entity.GetSprite()`): they would be `GetComponent<SpriteComponent>()` with a shorter name, and would force `Entity.h` to learn about every component type in the engine, so every new component would touch the header that everything includes. `PhysicsBody` is the first facade; the general convention — what qualifies, and what is rejected — is RAD-94.

### Components (`ECS/Components.h`)

| Component | Purpose |
|-----------|---------|
| `MetadataComponent` | identity: UUID, tag, active flag |
| `TransformComponent` | translation / rotation / scale |
| `SpriteComponent` | color, texture `AssetHandle` (0 = flat color), tiling |
| `CameraComponent` | `SceneCamera` + `Primary` + fixed-aspect flag |
| `RigidBody2DComponent` / `BoxCollider2DComponent` | physics binding (see [Physics](Physics.md)) |
| `NativeScriptComponent` | native script binding (below) |
| `TextComponent` | world text — parked until the MSDF revival (RAD-47) |
| `TransformSnapshotComponent` | previous-step transform for render interpolation — **runtime-only, never serialized** (see [Time-And-Simulation](Time-And-Simulation.md)) |

Adding a component is a three-site contract: declare in `Components.h`, add to `LevelSerializer`, and (Phase 5) add to the editor inspector. (Runtime-only components like `TransformSnapshotComponent` are the deliberate exception: the serializer never writes them.)

### Lifecycle & loops

Entity creation/destruction is **immediate** (no command buffer — RAD-97 revisits the destruction half). Physics bodies are managed reactively through entt signals: `on_construct<RigidBody2DComponent>` creates the Box2D body, `on_destroy` destroys it — component presence *is* the physics binding.

`Level::OnFixedUpdate(ts)` advances one FIXED simulation step: snapshots movable entities' transforms (for render interpolation), runs scripts (lazy-instantiating on first update), pushes transforms to physics, steps the physics world, and reads stepped body transforms back into the ECS as a dedicated post-step pass. `Level::OnRender(alpha)` finds the first `Primary` camera and draws sprite entities, blending movable entities between the last two simulation states by `alpha` — draw-only, it never mutates simulation state. Reaper's `GameLayer` drives the former from `Layer::OnFixedUpdate` and the latter from `Layer::OnUpdate` (see [Time-And-Simulation](Time-And-Simulation.md)).

"Systems" in Radiant are these loops inside `Level` — the spreadsheet walk ("every row with values in BOTH columns") as real code, from `OnRender`:

```cpp
auto view = m_Registry.view<TransformComponent, CameraComponent>();
for (auto entity : view)
{
    auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);
    if (camera.Primary) { /* found the view camera */ }
}
```

Game code never writes these — the registry is private to `Level`, so views exist only behind `Level`'s own update/render passes.

### Native scripts

`EntityBehaviour` (`ECS/EntityBehaviour.h`) is the C++ scripting seam: subclass it, override `OnCreate/OnUpdate/OnDestroy`, and bind with `NativeScriptComponent::Bind<T>()` — the component stores factory/destroy function pointers; the Level instantiates lazily on first update. One behaviour per entity today (RAD-101 makes it several); bindings are code-only (not serialized — they must be re-bound after level load, e.g. Reaper re-binds `CameraController` to its camera entity).

**The name says what the class is (RAD-99).** `Entity` is Radiant's world-thing and `EntityBehaviour` is Radiant's behaviour unit — the same split Unreal draws between `AActor` and `UActorComponent`. It is deliberately *not* called `Actor`: the entity exists whether or not a behaviour is attached, and most entities have none, so a type that is optional on the things it would claim to be cannot be those things. The predecessor name, `ScriptableEntity`, was inherited from Hazel and described a mechanism rather than a role.

#### What a script can reach (RAD-95)

**The problem this solves.** A script used to be handed its entity and then locked out of it: `m_Entity` was private with `friend class Level` as the only key. A script could not spawn a bullet, find another entity, or pass itself to anything — the first non-trivial gameplay line was unwritable. What existed instead was a handful of forwarders mirroring four of `Entity`'s methods, chosen by nobody and never revisited, and growing by one every time a subsystem appeared (RAD-90 added `GetPhysicsBody()` for exactly that reason).

Two accessors replace all of it, and everything chains off them:

```cpp
GetOwner()                                    // my entity
GetOwner().GetComponent<TransformComponent>() // my components
GetOwner().GetPhysicsBody().ApplyForce(...)   // my physics
GetLevel().CreateEntity("Bullet")             // the world
```

`GetOwner()` is named for a fact rather than for UE parity: the entity's `NativeScriptComponent` holds the behaviour instance by an owning pointer and deletes it, so the entity genuinely *owns* the behaviour in the sense playbook §2 uses the word. UE's `UActorComponent::GetOwner()` happens to agree.

**Why exactly two, and why that is not the forwarder pattern returning.** These are *relationship navigation* — a **bounded** set, sized by the relationships a script actually has, and it has two: the entity it drives, and the level that entity is in. A third requires a third *relationship* to exist first. What must never be added is a *subsystem* forwarder (`GetPhysicsBody`, a future `GetAbilitySystem`, `GetAnimation`): that set is **unbounded**, one per subsystem forever, and is the O(N)-edits-per-feature pattern RAD-94 exists to forbid. Unreal draws the identical line — `UActorComponent` carries both `GetOwner()` and `GetWorld()`, and no per-subsystem forwarder.

The verbosity is deliberate on the component path: `GetOwner().GetComponent<T>()` says *whose* component and looks like the sparse-set lookup it is, where a bare forwarder read like a member access. Game-side helpers stay Reaper's business (RAD-94); named gameplay verbs live on **`Entity`**, not on the behaviour base (RAD-99) — so `other.GetLocation()` in a collision handler and `GetOwner().GetLocation()` in a behaviour are the same API.

#### The level, narrowed

`GetLevel()` returns a **`GameplayLevel`** (`ECS/GameplayLevel.h`) — an 8-byte value handle over a `Level*` exposing the level-scope verbs gameplay may use: `CreateEntity`, `DestroyEntity`, `FindEntityByName`, `GetEntityByUUID`, `AddCollisionCallback`/`RemoveCollisionCallback`, and level identity. It owns nothing, caches nothing, and has one private `Resolve(verb)` guard, so no verb carries its own preamble (playbook §4).

It exists because `Level` is *also* the frame driver. `OnFixedUpdate`, `OnRender` and `OnViewportResize` belong to whoever drives the loop and to nothing else — a script calling `OnFixedUpdate` would step the physics world from inside the physics step. Handing gameplay a raw `Level*` would make that a one-keystroke mistake. Unreal narrows the same surface by tagging Blueprint-visible functions with `UFUNCTION`; with no reflection system (RAD-72) our version of "the tagged subset" has to be a type. Also withheld: `CreateEntityWithUUID` (a colliding caller-chosen UUID silently replaces a map entry), `GetAssetList` (tooling), `SetName` (authoring). **And never an accessor returning the underlying `Level*`** — that would hand back what the type exists to withhold.

**The dividing rule:** a verb that names one entity lives on `Entity` (`Teleport`, `Destroy`, `RefreshCollider`); a verb about the level as a whole lives on `GameplayLevel`.

**Lifetime.** `script → Entity → GameplayLevel → Level` is three non-owning hops. All are TRANSIENT: obtain, use, drop, never store across a level transition. An `Entity` can detect its own entity dying (the entt handle carries a generation); **nothing can detect the Level dying**, because a raw `Level*` has no generation. None of them holds a `Ref<Level>` — that would let gameplay keep a dead level alive and invert ownership.

#### Spawning and destroying from a script

Legal from any hook, and the script pass is built to survive it. `Level::OnFixedUpdate` snapshots the script set into a reusable buffer before running any of it, then walks plain entity ids, re-validating each and re-fetching the component after anything that hands control to gameplay. Without that, a spawner binding a script to its new entity would reallocate the pool being iterated, and a destroy would swap-and-pop it — undefined behaviour that survives Debug and corrupts under optimization (playbook §8.8).

This is Unreal's mechanism in miniature: its tick loop walks a separately-maintained tick-function list, never the authoritative actor storage, and diverts mid-tick registrations into a side buffer. Ours rebuilds the list per step; RAD-30's side table makes it maintained.

The resulting contract, which gameplay may rely on:

- An entity spawned during a step first runs its own `OnCreate`/`OnUpdate` on the **next** step.
- An entity destroyed during a step is skipped for the remainder of that step.
- A spawned entity has no render snapshot for the frame it was born in, so it draws un-interpolated once — correct, since there is no previous pose to blend from.

One caveat, and it is temporary: destroying **this** entity from a hook deletes the script instance whose method is executing, so it must be the last statement. RAD-97 removes that by deferring the reap.

A script registering a level-wide collision callback **must remove it in `OnDestroy`** — the Level owns the callback by value and therefore its captures, and cannot detect that its subscriber died.

### Serialization

`LevelSerializer` writes YAML (`.rdlvl`): a level name + entity list, each entity keyed by UUID with one block per present component, sorted by UUID for stable diffs. Deserialization recreates entities via `CreateEntityWithUUID` and adds components in dependency order (physics components last, so their entt signals fire after the transform exists). Asset references serialize as handles (`TextureHandle`), resolved through the AssetManager on load.

## Design Rationale

- **entt** is best-in-class sparse-set ECS (ships in Minecraft Bedrock); we deliberately do not hand-roll ECS storage — the learning budget goes to the systems built *on* it.
- **Registry private to Level** keeps a swappable seam and prevents game code from bypassing lifecycle rules (signals, UUID map maintenance).
- **UUID indirection** decouples persistent identity from entt's transient handles — required for serialization, and later for cross-level references and networking.
- **Signals for physics binding** make component presence authoritative — there is no separate "register with physics" step to forget.

## Known Issues & Evolution

- **Components must become plain data (RAD-30):** `NativeScriptComponent` holds an owning raw pointer, `RigidBody2DComponent` holds a `void*` body plus two `std::function` callbacks. Shallow copies duplicate raw pointers, which blocks the `Level::Copy()` that play-in-editor requires (RAD-52). Runtime state moves to Level-owned side tables; the rule is *if it can't be memcpy'd and serialized, it doesn't belong in a component* (playbook §3).
- **Serialization gaps:** `IsActive` and script bindings are lost on round-trip; several nested YAML reads are unguarded against malformed files. Hardened alongside the Phase 4 asset work.
- **No edit/play separation:** the engine has no `OnRuntimeStart/Stop` — physics and scripts run whenever the Level updates. Restored properly with play-in-editor (RAD-52).
- **Immediate destruction costs every caller a guard (RAD-97):** `DestroyEntity` tears down inside the call, so contact dispatch re-resolves both participants before every callback, and a script destroying its own entity must make it the last statement. Deferring the reap to a defined point removes all three symptoms — the same discipline the `EventQueue` already applies to input (playbook §1).
- **The script list is rebuilt each step, not maintained (RAD-30):** correct at today's scale (single-digit script counts) and one extra walk over a tiny set. The side table RAD-30 introduces *is* the maintained list, which is the shape Unreal's tick registry already has.
