# Asset System

**Status:** Working for loose-file loading — lifecycle redesign is Phase 4 (RAD-43…RAD-48).

## The Problem This Solves

Content lives on disk as files (PNGs, level files, fonts); gameplay wants live objects in memory. The naive bridge — "load whatever's at this file path" — breaks the moment real content exists: move or rename a file and every reference to it dies; two systems loading the same path get two copies; nothing knows when memory can be freed; and a shipped game shouldn't be reading loose files at all.

The asset system is a **library**. At import, every asset is issued a permanent card number — a 64-bit handle — and game data stores *only numbers, never shelf locations*. The catalog (the registry) maps numbers to current locations; the librarian (the `AssetManager`) fetches a book the first time it's requested and hands everyone the same copy thereafter. Move a file? Update one catalog entry — every reference in every level keeps working, because nothing but the catalog ever knew where the file lived.

## Architecture

### Identity: handles, not paths

Every asset is identified by an `AssetHandle` — a random 64-bit `UUID`. Handle `0` is the invalid sentinel. Runtime code (components, levels) stores and serializes **handles only**; paths exist solely in registry and import code. This means files can move without breaking references — the same philosophy as UE's asset registry, scaled down (64-bit random vs UE's GUIDs; collision odds are acceptable at this project's asset counts).

The idiom in code — the component holds the card number, the librarian resolves it on demand (from `SpriteComponent` and `Level::OnRender`, condensed):

```cpp
struct SpriteComponent { AssetHandle TextureHandle = 0; /* color, tiling */ };   // a number, never a path

if (Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(sprite.TextureHandle))
    Renderer2D::DrawSprite(worldTransform, texture, sprite.TilingFactor, sprite.Color);
```

### Registry & metadata

`AssetMetadata` = `{Handle, AssetType, FilePath}`. The registry (`AssetRegistry`) is an in-memory `unordered_map<AssetHandle, AssetMetadata>`, persisted as YAML with the `.rdar` extension (handle / path / type triples, forward-slash paths). Asset types today: `Level` (`.rdlvl`) and `Texture2D` (`.png/.jpg/.jpeg`). There is no `Font` value in the `AssetType` enum — only an unregistered, definition-less `FontSerializerAPI` and `Font`'s commented-out asset hooks exist; the type gets wired in the MSDF revival (RAD-47).

### AssetManager (`Asset/AssetManager.{h,cpp}`)

A static subsystem with two load paths:

- **`GetAsset(handle)`** — the runtime path: looks up metadata, lazily deserializes on first request, caches in a loaded-assets map. Assets are `Ref<Asset>`; the base `Asset` carries its handle and type.
- **`LoadAsset(path)`** — the import-ish path: if the path isn't registered, it *mints a fresh handle* and registers it on the fly, then loads.

Serialization is delegated through a **per-type serializer registry**: `AssetSerializer::Init` maps each `AssetType` to an `AssetSerializerAPI` implementation (`TextureSerializerAPI` → stb_image decode, `LevelAssetSerializerAPI` → `LevelSerializer` YAML). Adding an asset type = one enum value + one serializer class + an extension-map entry.

### Usage pattern (Reaper)

Reaper scopes assets per game state with **one registry file per state** (`MainMenu.rdar`, `Gameplay.rdar`): each state's `OnEnter` loads its registry, `OnExit` clears all loaded assets. This is a hand-rolled scoped-lifetime scheme — it works, and it is exactly the shape the Phase 4 refcount-driven lifecycle replaces.

### AssetPack (parked)

`Serialization/AssetPack*` defines a binary pack format (`.rdap`): header (`"RDAP"` magic + version) → index table (levels → their assets, offsets/sizes) → payload blobs. The write/read scaffolding exists but is **dead code**: nothing calls it, only levels can serialize into it (as embedded YAML text), and texture/font pack paths are stubs. It is the seed of the Phase 4 cook pipeline (RAD-46), not a working system.

## Design Rationale

- **Handle indirection** is the load-bearing decision and it is correct: content can be reorganized, renamed, and later cooked into packs without touching a single component. Everything else in the system is replaceable plumbing around it.
- **Per-type serializer registry** keeps asset-type addition O(1) in code sites — the pattern survives Phase 4 intact.
- **What's missing by design** (Phase 4 scope): an editor/runtime split — `LoadAsset`'s mint-on-load behavior is an *import* operation fused into a runtime API, which is how Reaper's level acquired a dangling font handle re-minted every run (the save-on-exit vector was removed in RAD-17; the fusion itself remains until Phase 4); refcount-driven unload (assets are already `Ref`s — the count is unused); and placeholder assets on failed loads instead of nulls.

## Known Issues & Evolution

- **Failure paths return null (interim):** failed loads now log and return `nullptr` without poisoning the cache (RAD-13); the real fix — placeholder assets — is Phase 4 (RAD-44). Callers must still null-check until then.
- **Editor/runtime split (RAD-43):** `AssetManager` becomes an interface — editor implementation (loose files, YAML registry, explicit import) and runtime implementation (immutable registry, binary pack).
- **Binary pack + cook step (RAD-46):** all asset types serialize binary; Dist Reaper ships as exe + `.rdap`, no loose files.
- **Fonts unmanaged (RAD-47):** `.ttf` registry entries are silently dropped (`Font` type unwired); fonts revive with the MSDF pipeline.
- **Engine-owned resources (RAD-48):** engine shaders currently live inside Reaper's asset folder — the engine's own runtime resources are game-owned; they move engine-side with build staging.
