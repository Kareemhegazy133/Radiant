# ECS & Levels

**Status:** Working — component-hygiene and physics-sync rework planned (Phase 2: RAD-28…RAD-30).

## The Problem This Solves

How do you represent "things in the world"? The instinctive answer — a class hierarchy (`GameObject` → `MovingObject` → `Character` → `Enemy`…) — rots fast: every new *combination* of abilities (a moving light? a camera attached to physics?) fights the tree, and features end up welded into base classes everything inherits.

ECS (Entity Component System) decomposes the idea into three parts. An **entity** is just an ID — a bare name with nothing attached. **Components** are plain data you attach to that ID: a transform, a sprite, a rigidbody. **Systems** are loops that say "for every entity that has components X and Y, do Z." Behavior emerges from *composition*: an entity is what it **has**, not what it inherits. Give an entity a `RigidBody2D` and it falls; add a `Sprite` and it's visible; there is no `FallingVisibleThing` class anywhere.

The picture to keep: a **spreadsheet**. Entities are rows, component types are columns, and systems walk "every row with values in these two columns." That layout is also why it's fast — same-typed components sit contiguously in memory, which CPUs read at full speed (arranging that storage is entt's whole job). A `Level` is one such spreadsheet: one world of entities plus the loops that update and render it.

## Architecture

### Level and Entity

A `Level` (`ECS/Level.h`) is the world: it privately owns an `entt::registry` (entt 3.13.2, vendored single header) and exposes entity management, the update/render loops, and serialization. The registry is **never** exposed publicly — all access goes through `Level`/`Entity` APIs.

`Entity` (`ECS/Entity.h`) is a 16-byte value handle `{entt::entity, Level*}` with templated component access (`AddComponent`, `GetComponent`, `HasComponent`, …) forwarding to the registry, plus the behaviour family — `AddBehaviour<T>` / `GetBehaviour<T>` / `HasBehaviour<T>` / `RemoveBehaviour<T>` / `GetBehaviours()` — where the typed query returns `nullptr` rather than asserting (see [Gameplay-Framework](Gameplay-Framework.md)). Every one is templated or untyped, so the header costs one declaration however many behaviour types a game defines. Entities are identified persistently by `UUID` (random 64-bit) via a `Level`-owned map `UUID → Entity`; `entt` handles are transient and never serialized.

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
// Gameplay/PhysicsBody.h — 16 bytes, owns nothing, re-resolves per call
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
| `BehaviourComponent` | empty tag: this entity has behaviours in the Level's side table — **runtime-only, never serialized, engine-private** (below) |
| `TextComponent` | world text — parked until the MSDF revival (RAD-47) |
| `TransformSnapshotComponent` | previous-step transform for render interpolation — **runtime-only, never serialized** (see [Time-And-Simulation](Time-And-Simulation.md)) |
| `PendingDestroyComponent` | empty tag: destroyed, awaiting the reap — **runtime-only, never serialized, engine-private** (above) |

Adding a component is a **four-site contract**: declare in `Components.h`, add to `LevelSerializer`, (Phase 5) add to the editor inspector, and **decide whether it is engine-private**. (Runtime-only components like `TransformSnapshotComponent` and `PendingDestroyComponent` are the deliberate exception to the second site: the serializer never writes them.)

The fourth site exists because `Radiant.h` exports `Components.h` wholesale — it must, since games need `SpriteComponent` and friends — while `Entity::AddComponent<T>` is public and unconstrained, so by default *every* component type is writable by game code. Engine-owned runtime state specialises `IsEngineComponent<T>` (bottom of `Components.h`), which makes `Entity`'s three mutating accessors reject it at compile time; reads stay open. Unreal reaches the same guarantee one level down — `AActor::bActorIsBeingDestroyed` is private, readable via `IsPendingKillPending()`, writable only through a struct whose constructor is friended to `UWorld` alone. Forgetting this step is silent, which is why it is on the checklist; RAD-30's Level-owned side tables are the eventual fix, since game code cannot name a side table at all.

### Lifecycle & loops

Entity creation is **immediate**. Destruction is **deferred**, and the reason is worth understanding rather than memorising.

The problem it solves: if destroying an entity freed its storage inside the call, then anything already walking the world — a loop over contact events, a pass over scripts, the very method that asked for the destroy — could be left standing on freed memory. That is not hypothetical; it is the crash Box2D v2's contact listener was famous for, and it is why Unity's `Object.Destroy()` has never destroyed anything at the moment you call it either.

So Radiant nails a **condemned notice** to the door instead (RAD-97). Destruction happens in two halves:

- **The mark** (`Level::DestroyEntity`) runs the behaviour's `OnDestroy`, tears the Box2D body down **eagerly**, erases the UUID map entry, and tags the entity `PendingDestroyComponent`. From that instant it is dead to everything: `IsValid()` is false, lookups miss it, no pass iterates it, nothing draws it.
- **The reap** (`Level::ReapDestroyedEntities`) frees the behaviour instance and the entt row, at exactly one moment — the end of the fixed step, after contact dispatch.

Physics dies at the mark and not at the reap deliberately: a body surviving to the reap would keep generating contacts for the rest of the step, so a destroyed bullet would hit twice. End of *step* rather than end of *frame* matters too — one frame can run several fixed steps, so reaping per frame would make the pending window depend on frame rate, which is the exact framerate-dependence the fixed timestep exists to remove.

The payoff is that `DestroyEntity` is safe from anywhere — a collision handler, a script's `OnUpdate`, an entity destroying itself — with no positional discipline, and that the defensive code this used to force on callers collapses. Contact dispatch now resolves each participant **once per event** instead of before every callback, because an `Entity` is a value handle that re-asks at each use: a callback destroying a participant changes what the handle *answers*, rather than turning it into a dangling ticket.

What it is **not**: a garbage collector. There is no reference graph and no tracing pass — entt owns the storage, `Ref`/`Scope` state ownership per type, and validity stays a generation-handle question. This is Unreal's `MarkAsGarbage` half without the collector.

Physics bodies are managed reactively through entt signals: `on_construct<RigidBody2DComponent>` creates the Box2D body, `on_destroy` destroys it — component presence *is* the physics binding.

`Level::OnFixedUpdate(ts)` advances one FIXED simulation step: snapshots movable entities' transforms (for render interpolation), runs scripts (lazy-instantiating on first update), steps the physics world, drains the move events back into ECS transforms as a dedicated post-step pass, dispatches the step's contacts to gameplay, and finally **reaps** the entities destroyed during it. Nothing pushes ECS transforms into Box2D here — physics owns the transform of dynamic bodies, and pushes happen only at spawn and through the explicit verbs (RAD-28). `Level::OnRender(alpha)` finds the first `Primary` camera and draws sprite entities, blending movable entities between the last two simulation states by `alpha` — draw-only, it never mutates simulation state. Reaper's `GameLayer` drives the former from `Layer::OnFixedUpdate` and the latter from `Layer::OnUpdate` (see [Time-And-Simulation](Time-And-Simulation.md)).

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

### Native scripts and the gameplay framework

`EntityBehaviour` (`Gameplay/EntityBehaviour.h`) is the C++ gameplay seam: subclass it, override `OnCreate/OnUpdate/OnDestroy`, and attach with `Entity::AddBehaviour<T>()`. An entity carries any number of behaviours of **distinct** types, updated in attach order and torn down in reverse (see [Gameplay-Framework](Gameplay-Framework.md) for the full rules). Bindings are code-only and not serialized — they must be re-attached after level load, e.g. Reaper re-attaches `CameraController` to its camera entity.

**Where the instances live (RAD-101), because this is the storage doc.** Not in a component: an `EntityBehaviour` is neither trivially copyable nor serializable, so it fails the plain-data rule. The `Level` owns them in a **side table** — `unordered_map<entt::entity, vector<Scope<EntityBehaviour>>>` — plus a maintained `vector<entt::entity>` giving the cross-entity walk order. What stays in the ECS is the empty `BehaviourComponent` tag, and it exists for exactly one reason: an `entt` view cannot consult a `std::unordered_map`, and the render-interpolation snapshot pass classifies movers by component.

Three consequences worth knowing:

- **The Level is the sole deleter, and only at the reap.** `DestroyEntity` and `RemoveBehaviour<T>` both only *mark*; nothing is freed until the end of the fixed step. That is what makes a behaviour destroying its own entity, or detaching itself mid-`OnUpdate`, safe.
- **Instance addresses are stable.** An attach during the walk may reallocate the `vector`, which moves the `Scope`s but not the objects they point at — so a raw observer held across a gameplay callback survives. This is the concrete reason instances are pointed-to rather than stored by value.
- **The tag is engine-private.** `IsEngineComponent<BehaviourComponent>` makes `AddComponent<BehaviourComponent>()` a compile error, so the "only the attach/detach path writes it" invariant is enforced rather than merely documented. Attach adds it; the reap strips it when an entity's last behaviour leaves.

A behaviour reaches everything through two accessors, and nothing else:

```cpp
GetOwner()                        // my entity - Entity is the world-thing
GetOwner().GetLocation()          // named gameplay verbs
GetLevel().CreateEntity("Bullet") // the world, narrowed to what gameplay may do
```

**The object model, the named verb surface (`GetLocation`/`SetLocation`/`SetTransform`/…), how `SetLocation` differs from `Teleport`, the rules governing what may be added to either type, and the `Gameplay/` module's layering all live in [Gameplay-Framework](Gameplay-Framework.md).** They moved there with RAD-99: this document is about how the world is *stored*, and that one is about how gameplay *talks* about it.

Two consequences belong here, because they are storage concerns:

**Spawning and destroying from a behaviour** is legal from any hook, and the script pass is built to survive it. `Level::OnFixedUpdate` snapshots the behaviour set into a reusable buffer before running any of it, then walks plain entity ids, re-validating each and re-fetching the component after anything that hands control to gameplay. Without that, a spawner binding a behaviour to its new entity would reallocate the pool being iterated, and a destroy would swap-and-pop it — undefined behaviour that survives Debug and corrupts under optimization (playbook §8.8). This is Unreal's mechanism in miniature: its tick loop walks a separately-maintained tick-function list, never the authoritative actor storage. Ours rebuilds the list per step; RAD-30's side table makes it maintained.

The resulting contract, which gameplay may rely on:

- An entity spawned during a step first runs its own `OnCreate`/`OnUpdate` on the **next** step.
- An entity destroyed during a step is skipped for the remainder of that step.
- A spawned entity has no render snapshot for the frame it was born in, so it draws un-interpolated once — correct, since there is no previous pose to blend from.

Destroying **this** entity from a hook is safe and needs no positional discipline — `GetOwner().Destroy();` may be followed by more statements and the method runs to completion, because only the reap deletes the instance (RAD-97). The entity does report `IsValid() == false` from that line onward, so anything after it reads a dead handle; that is a reason to put the destroy last out of taste, not out of safety.

A behaviour registering a level-wide collision callback **must still remove it in `OnDestroy`** — the Level owns the callback by value and therefore its captures, and cannot detect that its subscriber died. `OnDestroy` fires at the *mark*, not at the reap, precisely so that unregistration takes effect immediately rather than leaving a logically-dead subscriber receiving events for the rest of the batch.

### Serialization

`LevelSerializer` writes YAML (`.rdlvl`): a level name + entity list, each entity keyed by UUID with one block per present component, sorted by UUID for stable diffs. Deserialization recreates entities via `CreateEntityWithUUID` and adds components in dependency order (physics components last, so their entt signals fire after the transform exists). Asset references serialize as handles (`TextureHandle`), resolved through the AssetManager on load.

## Design Rationale

- **entt** is best-in-class sparse-set ECS (ships in Minecraft Bedrock); we deliberately do not hand-roll ECS storage — the learning budget goes to the systems built *on* it.
- **Registry private to Level** keeps a swappable seam and prevents game code from bypassing lifecycle rules (signals, UUID map maintenance).
- **UUID indirection** decouples persistent identity from entt's transient handles — required for serialization, and later for cross-level references and networking.
- **Signals for physics binding** make component presence authoritative — there is no separate "register with physics" step to forget.

## Known Issues & Evolution

- **Components must become plain data (RAD-30) — half done.** `NativeScriptComponent`'s owning raw pointer is **gone** (RAD-101: instances moved to the Level-owned side table described above, leaving an empty tag). Still outstanding: `RigidBody2DComponent` holds a `void*` body plus two `std::function` callbacks. Shallow copies duplicate raw pointers, which blocks the `Level::Copy()` that play-in-editor requires (RAD-52). The rule is *if it can't be memcpy'd and serialized, it doesn't belong in a component* (playbook §3).
- **Serialization gaps:** `IsActive` and script bindings are lost on round-trip; several nested YAML reads are unguarded against malformed files. Hardened alongside the Phase 4 asset work.
- **No edit/play separation:** the engine has no `OnRuntimeStart/Stop` — physics and scripts run whenever the Level updates. Restored properly with play-in-editor (RAD-52).
- **Deferred destruction landed (RAD-97, 2026-08-26)** — the three symptoms it was filed for are gone: contact dispatch resolves each participant once per event, the "last statement" caveat is deleted, and the move-drain assert now holds *because* physics teardown stayed eager. Two residual notes. An entity marked **outside** a fixed step (a cheat key, a UI action, a destroy while paused) is not reaped until the next step and never if the level is never stepped again — invalid and invisible throughout, so the cost is memory, and `~Level` collects it. And `Level::Copy()` (RAD-52, play-in-editor) must skip entities carrying the tag rather than copy a corpse into the new level.
- **The script list is rebuilt each step, not maintained (RAD-30):** correct at today's scale (single-digit script counts) and one extra walk over a tiny set. The side table RAD-30 introduces *is* the maintained list, which is the shape Unreal's tick registry already has.
