# Build System

**Status:** Working — restructure planned (Phase 1: RAD-20, RAD-21, RAD-22, RAD-24).

## The Problem This Solves

Radiant is ~150 engine source files, three executables, seven third-party libraries, and three build configurations. *Something* has to answer "compile what, in what order, with which flags, linking against what" — and answer it identically on any machine, today and after a year away.

Hand-maintained Visual Studio project files rot instantly (every new file, flag, or dependency is a manual click-through), so Radiant treats them as **build artifacts, not source**: `Build.lua` is the recipe (checked in, human-edited), premake is the cook (vendored in the repo, zero install), and the `.sln`/`.vcxproj` files are the dish — regenerated on demand, gitignored, never edited by hand. Change the recipe and re-run the cook; editing the dish directly is always a mistake, because the next regeneration throws your edit away.

## Architecture

### Generation

The build is **Premake 5**. Premake binaries live under `Tools/Binaries/Premake/` (build tooling — distinct from `Radiant/Vendor/`, which holds library dependencies) so a fresh clone needs no installs:

```text
Scripts/Setup-Windows.bat   → deletes bin/bin-int, runs premake5 --file=Build.lua vs2022
```

The script layout mirrors per-module build files (UE's `.Build.cs` analogue):

```text
Build.lua            workspace root — configuration + aggregation only
Dependencies.lua     shared IncludeDir table (anchored to %{wks.location})
Radiant/premake5.lua the engine project
Reaper/premake5.lua  the game project (explicit debugdir for asset loading)
```

Generated solution/project files are build artifacts: gitignored, never committed, regenerated on demand.

### Projects & configurations

| Project | Kind | Notes |
|---------|------|-------|
| `Radiant` | StaticLib | the engine; PCH `Radiant/rdpch.h`; C++20 |
| `Reaper` | ConsoleApp | the game; links `Radiant`; **startproject** |
| `Dependencies` group | — | glfw, glad, box2d, yaml-cpp, imgui (each with its own vendor premake script) |

| Config | Defines | Meaning |
|--------|---------|---------|
| Debug | `RD_DEBUG` | symbols, asserts, debug runtime, live-reference tracking |
| Release | `RD_RELEASE` | optimized, asserts **on** — the day-to-day iteration config |
| Dist | `RD_DIST` | shipping: `WindowedApp` (WinMain forwards to `main`), LTO, no asserts |

Workspace: x64 only, `MultiProcessorCompile`, output pattern `bin/<Config>-<system>-<arch>/<Project>`.

### Dependencies

| Library | Form | Notes |
|---------|------|-------|
| box2d | git submodule | **v3.1.1** — Kareem's fork of erincatto/box2d, branch `radiant/v3.1.1` (upstream tag + premake commit); upgraded from 2.4 per RAD-60, swapped in RAD-27. C17 static lib — the one C-language vendor project |
| spdlog, glm, yaml-cpp, glfw, imgui | git submodules | glfw/imgui are Kareem's forks; yaml-cpp/msdf-atlas-gen still TheCherno forks — migrations to owned forks filed as RAD-88/RAD-89 |
| msdf-atlas-gen | submodule, **orphaned from the build** | deliberately parked; revives with the Phase 4 MSDF font pipeline (RAD-47) |
| glad, entt (3.13.2), stb_image | vendored in-tree | generated code / single headers — correct to vendor |

**Pin policy:** submodules move only deliberately — updating one is a reviewed change with a reason, never a side effect of `submodule update --remote`. **Fork policy (locked 2026-07-09, RAD-27):** every submodule points at Kareem's *own* fork, never a third-party fork — third-party forks are someone else's maintenance schedule and delete button. Forks add exactly one thing (the premake script) on a branch named `radiant/<base-tag>`, so the upstream tag a pin is based on stays readable even though the submodule pin itself is a fork commit hash. The ImGui vendor project is re-opened from `Build.lua` to align its C++ standard with the workspace (premake project re-entry), so the submodule itself is never modified. All submodules carry `ignore = untracked` in `.gitmodules` because premake generates project files inside their working trees.

## Design Rationale

- **Premake over CMake — a deliberate, revisitable call:** CMake is the industry lingua franca, but migrating buys nothing for this project's goals — the Vulkan SDK integrates via the `VULKAN_SDK` environment variable (headers, `vulkan-1.lib`, prebuilt shaderc) in a few premake lines, VMA is a single header, and build-system migration is weeks of effort with near-zero learning return on engine architecture. Revisit only if cross-platform CI or heavy third-party integration becomes real.
- **Submodule-with-premake-fork** is the pragmatic dependency pattern for premake ecosystems (no package manager); the discipline that makes it sound is pinning to deliberate tags/commits — pinning to fork *master heads* (current state, RAD-22) makes builds non-reproducible.
- **Three configs with distinct jobs:** Debug = stepping/diagnosing, Release = fast iteration *with checks*, Dist = what a player receives. The audit found Dist was Release-with-a-define — a shipping config that can't actually ship (console window, no asset staging); RAD-21 makes it real.

## Known Issues & Evolution

- **CI awaiting first green run (RAD-24):** `.github/workflows/build.yml` builds all three configurations per push; unverified until pushed to GitHub.
- **Asset staging gap:** nothing copies assets next to built executables; runs depend on the project-relative working directory (`debugdir` covers IDE runs). Resolved with engine-resource staging in Phase 4 (RAD-48).
