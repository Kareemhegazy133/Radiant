# ECS & Levels

**Status:** Working — component-hygiene and physics-sync rework planned (Phase 2: RAD-28…RAD-30).

## Architecture

### Level and Entity

A `Level` (`ECS/Level.h`) is the world: it privately owns an `entt::registry` (entt 3.13.2, vendored single header) and exposes entity management, the update/render loops, and serialization. The registry is **never** exposed publicly — all access goes through `Level`/`Entity` APIs.

`Entity` (`ECS/Entity.h`) is a 16-byte value handle `{entt::entity, Level*}` with templated component access (`AddComponent`, `GetComponent`, `HasComponent`, …) forwarding to the registry. Entities are identified persistently by `UUID` (random 64-bit) via a `Level`-owned map `UUID → Entity`; `entt` handles are transient and never serialized.

Every entity carries `MetadataComponent` (UUID, tag string, `IsActive`) and `TransformComponent` (translation / Euler-radians rotation / scale, TRS matrix on demand), added at creation.

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

Adding a component is a three-site contract: declare in `Components.h`, add to `LevelSerializer`, and (Phase 5) add to the editor inspector.

### Lifecycle & loops

Entity creation/destruction is **immediate** (no command buffer). Physics bodies are managed reactively through entt signals: `on_construct<RigidBody2DComponent>` creates the Box2D body, `on_destroy` destroys it — component presence *is* the physics binding.

`Level::OnUpdate(ts)` runs scripts (lazy-instantiating on first update), pushes transforms to physics, and steps the physics world. `Level::OnRender()` finds the first `Primary` camera, begins the 2D scene, and draws sprite entities. Games drive both per frame (`GameLayer` in Reaper).

### Native scripts

`ScriptableEntity` (`ECS/ScriptableEntity.h`) is the C++ scripting seam: subclass it, override `OnCreate/OnUpdate/OnDestroy`, and bind with `NativeScriptComponent::Bind<T>()` — the component stores factory/destroy function pointers; the Level instantiates lazily on first update. Scripts access their entity's components through the embedded `Entity`. One script per entity; bindings are code-only (not serialized — they must be re-bound after level load, e.g. Reaper re-binds `CameraController` to its camera entity).

### Serialization

`LevelSerializer` writes YAML (`.rdlvl`): a level name + entity list, each entity keyed by UUID with one block per present component, sorted by UUID for stable diffs. Deserialization recreates entities via `CreateEntityWithUUID` and adds components in dependency order (physics components last, so their entt signals fire after the transform exists). Asset references serialize as handles (`TextureHandle`), resolved through the AssetManager on load.

## Design Rationale

- **entt** is best-in-class sparse-set ECS (ships in Minecraft Bedrock); we deliberately do not hand-roll ECS storage — the learning budget goes to the systems built *on* it.
- **Registry private to Level** keeps a swappable seam and prevents game code from bypassing lifecycle rules (signals, UUID map maintenance).
- **UUID indirection** decouples persistent identity from entt's transient handles — required for serialization, and later for cross-level references and networking.
- **Signals for physics binding** make component presence authoritative — there is no separate "register with physics" step to forget.

## Known Issues & Evolution

- **Components must become plain data (RAD-30):** `NativeScriptComponent` holds an owning raw pointer, `RigidBody2DComponent` holds a `void*` body plus two `std::function` callbacks. Shallow copies duplicate raw pointers, which blocks the `Level::Copy()` that play-in-editor requires (RAD-52). Runtime state moves to Level-owned side tables; the rule is *if it can't be memcpy'd and serialized, it doesn't belong in a component* (playbook §3).
- **Script instance leak + early-destroy crash (RAD-11):** `DestroyScript` is never invoked, and destroying a never-updated scripted entity null-derefs.
- **Physics readback is coupled to rendering (RAD-28):** Box2D→ECS transform sync currently happens inside the sprite render loop — see [Physics](Physics.md).
- **Serialization gaps:** `IsActive` and script bindings are lost on round-trip; several nested YAML reads are unguarded against malformed files. Hardened alongside the Phase 4 asset work.
- **No edit/play separation:** the engine has no `OnRuntimeStart/Stop` — physics and scripts run whenever the Level updates. Restored properly with play-in-editor (RAD-52).
