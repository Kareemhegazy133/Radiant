# Radiant — Game Engine Development Guidelines

---

# Project Identity

| Field | Value |
|-------|-------|
| Project Name | Radiant (engine) / Reaper (proving-ground game) |
| Language | C++20, MSVC (VS2022), Windows-only for now |
| Namespace | `Radiant` |
| Macro Prefix | `RADIANT_` (engine macros), `RD_` (config defines: `RD_DEBUG`/`RD_RELEASE`/`RD_DIST`) |
| PCH | `Radiant/rdpch.h` (engine project only) |
| Build System | Premake 5 — root `Build.lua`, generated via `Scripts/Setup-Windows.bat` |
| Renderer | OpenGL 4.5 (legacy) → Vulkan (Phase 3 target) |
| Jira | Project **RAD** @ `hndredgames.atlassian.net` |
| Branches | `master` (PR base), `dev` (working) |
| UE Reference Source | `C:\dev\HNDREDGAMES\UE_5_7_4` (study reference for system design) |
| Key Dependencies | entt 3.13.2, Box2D 3.1.1 (owned fork, branch `radiant/v3.1.1`), GLFW, ImGui (docking), glm, spdlog, yaml-cpp, stb_image; msdf-atlas-gen (parked submodule, revives in Phase 4) |

---

# Mission & Locked Strategy

Radiant is a **learning-first engine**: the goal is principal-level architecture experience, not shipping speed. The user's global CLAUDE.md Mentorship Mode applies strongly here — plan and teach first; the user implements core systems (`mentorship`-labeled work); Claude implements chores (`chore`-labeled work).

Decisions locked 2026-07-04 (do not relitigate; flag if work contradicts them):

1. **Incremental re-architecture in this repo** — no greenfield rewrite. The engine must stay runnable at every milestone; **Reaper is the proving ground and must survive every change**.
2. **2D-first, dimension-agnostic RHI** — the RHI layer never knows about "2D"; 3D later is a renderer-module addition, not a rewrite.
3. **Vulkan approach:** raw Vulkan for instance/device/swapchain/sync/pipelines/descriptors + **VMA** for GPU memory + **shaderc** for GLSL→SPIR-V. No vk-bootstrap. The OpenGL backend is deleted once the Vulkan Renderer2D reaches parity.
4. **Phase order:** 1 Triage → 2 Simulation Foundation → 3 RHI v2 + Vulkan → 4 Asset Pipeline v2 → 5 Editor. Icebox (jobs, render graph, 3D, scripting, networking) waits.

Full audit (2026-07-04) is filed as Jira issues RAD-7…RAD-59 with `file:line` references — check the board before re-diagnosing a known problem.

---

# Architecture Map

| Path | Responsibility | State |
|------|----------------|-------|
| `Radiant/Source/Radiant/Core/` | App lifecycle (`GameApplication`), fixed-timestep loop + time services (`FrameClock`/`TimerManager`/`Time`), window, layers, events entry, `Ref`/`Scope`, logging, asserts, input | Working; Hazel-derived |
| `Radiant/Source/Radiant/Events/` | Event types + dispatcher + `EventQueue` (callbacks enqueue; processed at frame start — RAD-26) | Working |
| `Radiant/Source/Radiant/Renderer/` | API-agnostic renderer + batched Renderer2D | Working on GL; RHI v2 replaces the abstraction in Phase 3 |
| `Radiant/Source/Radiant/Platform/OpenGL/` | GL backend (deleted at end of Phase 3) | Legacy |
| `Radiant/Source/Radiant/Platform/Windows/` | Window/input/filesystem impl | Working |
| `Radiant/Source/Radiant/ECS/` | `Level` (wraps private `entt::registry`), `Entity` handle, components, YAML LevelSerializer, native scripts | Working; component hygiene rework in Phase 2 |
| `Radiant/Source/Radiant/Physics/` | `PhysicsWorld2D` — per-Level Box2D v3 world; `ContactEvent` — vendor-free contact record | Phase 2 rework complete: per-Level worlds (RAD-27), sync semantics (RAD-28: explicit verbs, move-event drain), collision events (RAD-29: post-step contact drain, validity-checked dispatch to Level observers + script hooks). Follow-ups: RAD-90 dynamics verbs, RAD-91 verb-surface cleanup |
| `Radiant/Source/Radiant/Asset/` | Handle-based asset manager + YAML registry (`.rdar`) | Working; lifecycle redesign in Phase 4 |
| `Radiant/Source/Radiant/Serialization/` | Stream I/O + binary AssetPack | AssetPack is dead code, parked for Phase 4 |
| `Reaper/` | The game: state machine (MainMenu/Gameplay/Paused), GameLayer (Level driver), UILayer (ImGui) | Working shell |

Custom asset formats: `.rdlvl` (YAML level), `.rdar` (YAML asset registry), `.rdap` (binary asset pack — future), `.rdfa` (cached font atlas).

---

# Engineering Rules

**Ownership — one story per type, stated where the type is declared.**
- `Ref<T>` (intrusive refcount) for shared engine resources and assets. `Scope<T>` for unique ownership. Never both on one type.
- No raw owning pointers in new code. Types owning raw API handles (GL/Vulkan objects) must delete copy or implement rule-of-5.

**Components are plain data** (rule enforced from Phase 2): trivially copyable, serializable, no owning raw pointers, no `std::function`. Runtime state (physics bodies, script instances) lives in Level-owned side tables keyed by entity. *If it can't be memcpy'd and serialized, it doesn't belong in a component.*

**Asserts:** always `RADIANT_ASSERT(condition, "message")` — never message-only (a string literal is always truthy). Never put side effects in an assert expression (compiled out in Dist). Asserts are active in Debug and Release, out in Dist. Guards on config/content mistakes log a `WARN` and recover; asserts are for programmer errors.

**Logging:** `RADIANT_*` macros = engine, `GAME_*` macros = game code. Warn loudly on misconfiguration instead of failing silently.

**Naming (principal standard):** concise and precise — a name states exactly what the thing is and nothing more (`ConsumeStep`, not `ProcessAccumulatedTimeStep`). Follow the codebase's conventions: `PascalCase` types/functions, `m_`/`s_` member/static prefixes, `Get`/`Set` accessors, verb-first functions, no abbreviations that force a lookup. Consistency with the surrounding code wins over personal taste — including Claude's.

**Comments & API docs (three altitudes, no overlap):** `Docs/` explains systems; header doc comments state contracts; inline comments state constraints. Public engine API gets a Doxygen-compatible `/** */` block — full sentences, tag-light (`@param`/`@return` only when they add information beyond the signature) — always stating ownership, lifetime, threading, units, and failure semantics where applicable. Inline `//` comments exist only for why/constraints/invariants — never to restate what code does. No file-header boilerplate. A stale comment is worse than none: comments update with the code they describe (review-enforced).

**Performance-first:** no per-frame heap allocations or GPU resource creation/destruction in hot paths; `reserve()` known sizes; no O(n) scans per frame where a map exists. Flag perf implications alongside correctness in every review.

**Simulation (from Phase 2):** simulation must be framerate-independent (fixed timestep + accumulator); events are queued and drained at frame start; rendering never mutates simulation state.

**Boundaries:**
- Graphics-API types and headers appear only under `Platform/<API>/`. No GL/Vulkan types in public engine headers.
- Engine code never includes game code. Games never reach into `Platform/`.
- No `using namespace` at global scope in **engine** headers — the engine never forces namespace pollution on consumers. Game code may opt in inside its own files.

**Serialization:** fixed-width types on the wire (`uint64_t`, never `size_t`); honest error propagation (no unconditional `return true`); format is declared little-endian.

**Assets:** reference by handle, never by path, in runtime code. Running the game must never modify committed assets — saving is an explicit editor/tool action.

**entt usage:** never destroy entities or mutate the iterated pool while iterating a view; the registry stays private to `Level`.

---

# Build & Run

- Generate solution: `Scripts/Setup-Windows.bat` (runs vendored premake with `--file=Build.lua vs2022`).
- Configs: **Debug** (symbols, asserts) / **Release** (optimized, asserts) / **Dist** (shipping, no asserts).
- Verify any engine change by **building all configs and running Reaper** (startproject). Assets load relative to the project working directory — run from the project dir (VS default).

**CLI build (how Claude self-verifies; verified 2026-07-05):**

```powershell
# Locate MSBuild (stable across VS updates); on this machine it resolves to
# C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe
& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe

& $msbuild Radiant.sln -p:Configuration=Debug -p:Platform=x64 -m -v:m -nologo   # repeat for Release, Dist
```

- Output: `bin/<Config>-windows-x86_64/Reaper/Reaper.exe`; run it with working directory `Reaper/` (assets are working-dir-relative).
- **Chore Definition of Done:** Claude builds **all three configs** via this CLI clean before handing off any chore — "please build and check" is not a handoff.

---

# Workflow

- **Sessions are disposable; artifacts are not.** Start a fresh conversation per story/task (`/plan-feature RAD-XX` is the usual opener). All durable context lives outside the chat: this file + `Docs/` + the playbook + Jira + `.claude/plans/` + Claude's persistent memory. If something decided in a session isn't recorded in one of those homes before the session ends, record it — a conversation whose loss would hurt means the record-keeping failed.
- **Tracking:** everything lives in Jira project RAD. Phase epics: RAD-1 (Triage), RAD-2 (Simulation), RAD-3 (RHI/Vulkan), RAD-4 (Assets), RAD-5 (Editor), RAD-6 (Icebox). Every non-epic issue is parented to a phase epic. Labels: `mentorship` (user implements, Claude guides) / `chore` (Claude implements) / `audit-finding` / `milestone` / `design` / `icebox`.
- **Creating/updating issues:** use the `/jira` skill — never ad-hoc `createJiraIssue` calls.
- **Planning a story:** `/plan-feature RAD-XX` — plan file in `.claude/plans/`, posted to the issue on approval, then guided piece-by-piece implementation.
- **Story cadence (every story, in order):** `/plan-feature RAD-XX` → guided implementation (Kareem writes core, Claude does chores/guards/logging/docs) → Kareem runs `/review` → Claude writes the **Story Implementation Report** (format in the plan-feature skill: associate-level explanation of what/why with code snippets, posted to the Jira story) → transition per Definition of Done.
- **Reviewing:** `/review` before committing engine changes.
- **Playbook:** `.claude/references/radiant-playbook.md` holds established patterns and hard-won rules; cite it by section, keep it current via `/audit-standards`.
- **System docs:** `Docs/` holds per-system architecture documentation (index: `Docs/README.md`). **The update contract: any change that alters a system's behavior or architecture updates that system's doc in the same change** — `/review` flags stale docs as an ERROR. `/sync-docs` pushes `Docs/` to Confluence.
