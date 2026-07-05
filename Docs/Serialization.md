# Serialization

**Status:** Working for YAML paths; binary stream layer needs hardening before the Phase 4 pack work (RAD-45).

## Architecture

Radiant has two serialization families with different jobs:

### 1. YAML (human-readable authoring formats)

Levels (`.rdlvl`) and asset registries (`.rdar`) serialize via **yaml-cpp**, with `Utilities/YAMLSerializationHelpers.h` providing `convert<>` specializations for glm vectors and `UUID`. YAML is the right choice for authored content: diffable in git, hand-editable, order-stable (levels sort entities by UUID before writing). See [ECS-And-Levels](ECS-And-Levels.md) for the level format and [Asset-System](Asset-System.md) for the registry format.

### 2. Binary streams (`Serialization/`)

The binary layer is a small stream abstraction intended for the asset-pack pipeline:

- **`StreamWriter` / `StreamReader`** — abstract base classes with position control and typed helpers layered on two primitives (`WriteData`/`ReadData` on raw bytes): `WriteRaw<T>` (memcpy of trivial types), `WriteString`, `WriteBuffer`, `WriteObject<T>` (calls static `T::Serialize(writer, obj)` / `T::Deserialize(reader, obj)`), and `WriteMap`/`WriteArray` container helpers that dispatch on triviality.
- **`FileStreamWriter` / `FileStreamReader`** — `std::ofstream`/`ifstream` implementations.
- **`AssetPackFile.h`** — the `.rdap` binary pack layout: `FileHeader` (`"RDAP"` magic, format version, build version) → index table (level infos → per-asset offset/size/type entries) → payload blobs. The serializer writes a zero-filled index placeholder, streams payloads while recording offsets, then seeks back and overwrites the index. Currently dead code, revived as the Phase 4 cook pipeline (RAD-46).

## Design Rationale

- **Two families on purpose:** authored content optimizes for humans (YAML); shipped content optimizes for load speed and integrity (binary pack). The Phase 4 cook step is the bridge — it consumes the YAML world and emits the binary one. Cooked levels will serialize binary too (today the pack scaffolding embeds levels as YAML text, which defeats the format's purpose).
- **`WriteObject`'s static-method contract** (rather than a virtual `ISerializable`) keeps serializable types free of vtables and lets plain structs participate — appropriate for a format layer that mostly moves POD.
- **Wire-format rules (the Phase 4 contract, playbook §6):** fixed-width integer types only (`uint64_t`, never `size_t` — the current `WriteString` writes a `size_t` length, making the format ABI-dependent); the format is declared little-endian (x86/x64 only — no byte-swapping tax for platforms we don't target); headers are zero-initialized POD (no uninitialized padding bytes on disk → deterministic output, which enables binary-identical rebuild verification).

## Known Issues & Evolution

- **Error handling is dishonest (RAD-45):** `FileStreamReader::ReadData` / `FileStreamWriter::WriteData` ignore stream state and unconditionally return `true` — a truncated or corrupt file reads garbage and reports success, and the assert layered on top can never fire. The fix threads real stream status through the primitives so corruption fails loudly at the read site.
- **`size_t` on the wire (RAD-45)** — see rules above.
- **Uninitialized padding in pack headers (RAD-45)** — `AssetInfo` writes 4 uninitialized bytes; zero-init fixes determinism.
- **`Serialization.h` shells** — empty `SBuffer`/`SArray`/`SMap` scaffolding with no implementation; removed in the Phase 1 dead-code sweep (RAD-23).
