# Build System

**Status:** Working — restructure planned (Phase 1: RAD-20, RAD-21, RAD-22, RAD-24).

## Architecture

### Generation

The build is **Premake 5**, driven by `Build.lua` at the repo root. Premake binaries are vendored (`Vendor/Binaries/Premake/`) so a fresh clone needs no installs:

```text
Scripts/Setup-Windows.bat   → deletes bin/bin-int, runs premake5 --file=Build.lua vs2022
```

Generated solution/project files are build artifacts and are gitignored (a few legacy ones are still tracked — RAD-22 untracks them).

### Projects & configurations

| Project | Kind | Notes |
|---------|------|-------|
| `Radiant` | StaticLib | the engine; PCH `rdpch.h`; C++20 |
| `Reaper` | ConsoleApp | the game; links `Radiant`; **startproject** |
| `Sandbox` | ConsoleApp | legacy scratch app |
| `Dependencies` group | — | glfw, glad, box2d, yaml-cpp, imgui (each with its own vendor premake script) |

| Config | Defines | Meaning |
|--------|---------|---------|
| Debug | `RD_DEBUG` | symbols, asserts, debug runtime |
| Release | `RD_RELEASE` | optimized; asserts **on** once RAD-9 lands — the day-to-day iteration config |
| Dist | `RD_DIST` | shipping: optimized, no asserts; becomes `WindowedApp` + LTO with RAD-21 |

Workspace: x64 only, `MultiProcessorCompile`, output pattern `bin/<Config>-<system>-<arch>/<Project>`.

### Dependencies

| Library | Form | Notes |
|---------|------|-------|
| spdlog, glm, yaml-cpp, box2d, glfw, imgui | git submodules | glfw/imgui are Kareem's forks, box2d/yaml-cpp/msdf-atlas-gen are TheCherno forks — forks exist to add premake scripts |
| msdf-atlas-gen | submodule, **orphaned from the build** | deliberately parked; revives with the Phase 4 MSDF font pipeline (RAD-47) |
| glad, entt (3.13.2), stb_image | vendored in-tree | generated code / single headers — correct to vendor |

## Design Rationale

- **Premake over CMake — a deliberate, revisitable call:** CMake is the industry lingua franca, but migrating buys nothing for this project's goals — the Vulkan SDK integrates via the `VULKAN_SDK` environment variable (headers, `vulkan-1.lib`, prebuilt shaderc) in a few premake lines, VMA is a single header, and build-system migration is weeks of effort with near-zero learning return on engine architecture. Revisit only if cross-platform CI or heavy third-party integration becomes real.
- **Submodule-with-premake-fork** is the pragmatic dependency pattern for premake ecosystems (no package manager); the discipline that makes it sound is pinning to deliberate tags/commits — pinning to fork *master heads* (current state, RAD-22) makes builds non-reproducible.
- **Three configs with distinct jobs:** Debug = stepping/diagnosing, Release = fast iteration *with checks*, Dist = what a player receives. The audit found Dist was Release-with-a-define — a shipping config that can't actually ship (console window, no asset staging); RAD-21 makes it real.

## Known Issues & Evolution

- **Monolithic `Build.lua` (RAD-20):** all three projects inline, with Sandbox/Reaper copy-pasted identically. Restructure: thin workspace root including per-project `premake5.lua` files + a shared `Dependencies.lua` — the premake analogue of UE's per-module `.Build.cs`. Prerequisite for cleanly adding the Vulkan wiring (RAD-33) and the editor project (RAD-49).
- **Dist isn't shippable (RAD-21):** `WindowedApp` + LTO + explicit `debugdir`; remove the dead generic `WINDOWS` define; align ImGui's vendor project to C++20.
- **Git hygiene (RAD-22):** untrack committed `.sln`/`.vcxproj`; `ignore = untracked` per submodule (the git-status `?` noise is premake artifacts inside submodule trees); pin submodules.
- **No CI (RAD-24):** GitHub Actions building Debug + Release per push — the cheapest regression net a solo project can buy.
- **Asset staging gap:** nothing copies assets next to built executables; runs depend on VS's project-relative working directory. Resolved with engine-resource staging in Phase 4 (RAD-48).
