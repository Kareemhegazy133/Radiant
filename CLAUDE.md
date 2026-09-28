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
4. ~~**Phase order:** 1 Triage → 2 Simulation Foundation → 3 RHI v2 + Vulkan → 4 Asset Pipeline v2 → 5 Editor.~~ **RE-OPENED AND REPLACED 2026-09-28 — see below.** Icebox (jobs, render graph, 3D, scripting, networking) no longer waits as a block; it is drawn from by the slice.

## Decision 4 replaced (2026-09-28) — slice ordering

**The roadmap is now ordered by the Reaper vertical slice, not by engine phase.** An engine card earns its place by being something the game cannot ship without.

**Why decision 4 was re-opened:** the board had no game in it. 104 issues across five horizontal phases, ~19 cards to a Vulkan triangle, ~50 to an editor, and **no card anywhere that produced something playable**. Eleven of the capabilities a playable slice needs were sitting in the Icebox behind Vulkan, the asset pipeline and the editor. The board identified *what* the engine needed correctly and *when* catastrophically wrong. Twelve weeks (Jul→Sep) produced eight consecutive ownership/lifetime plumbing cards, each validated only by a probe, with nothing visible at the end. That is also the antipattern the global CLAUDE.md rule 11 names — *"bias toward the smallest shippable vertical slice"* — and five horizontal layers is its opposite.

**The new order — five milestones, each ending in a playtest gate:**

| | Ends with | Epic |
|---|---|---|
| **M1 — "It moves"** | Run and jump in a test room, and it feels good | RAD-105 / RAD-108 |
| **Vulkan block** | First triangle → Renderer2D parity → **OpenGL deleted** | RAD-3 |
| **M2 — "It fights"** | Hitting a training dummy feels good | RAD-105 / RAD-108 |
| **M3 — "It fights back"** | Three enemies, real fights, you can die | RAD-105 / RAD-108 |
| **M4 — "It's a game"** | The full death → recover → rest → level loop | RAD-105 / RAD-108 |
| **M5 — "It's the slice"** | The fifteen minutes, end to end | RAD-105 / RAD-108 |

**The Vulkan block sits between M1 and M2 deliberately.** M1 touches no rendering at all (input, physics queries, possession, kinematic controller, camera-as-matrices), while M2–M4 are the renderer-heavy milestones (particles, game UI, tilemap batching). `RendererAPI.h` is clean but **immediate-mode shaped** — its own doc comment says *"calls execute immediately"* — so anything built against it before RHI v2 inherits that shape and gets reworked. Building the renderer-heavy milestones after RHI v2 costs nothing extra; building them before costs three reworks. Evidence recorded on RAD-3 and in the replan record.

**Amendment (2026-09-27) reversing this — RAD-104's pull-forward — is itself REVERSED (2026-09-28).** RAD-104 (reflection MVP / the component + behaviour collapse) defers until **after GATE M5**, which is named as an acceptance criterion on RAD-123 so the debt has a due date. Its original justification — that RAD-98's framework was under construction and every card would be rewritten — weakened when RAD-98 was paused by this replan; only RAD-102 survives into the slice. The accepted cost is a mechanical refactor of ~15 slice behaviour classes later (a `chore`), chosen over one more invisible plumbing card before anything moves on screen. **Full reflection (RAD-72) stays in Phase 4** as always.

**Game design is now a first-class input.** `Reaper/Design/GDD.md` is the source of truth for game scope — Reaper is a Hollow Knight-style metroidvania with Elden Ring's progression depth. If a Jira card contradicts it, one of the two is wrong and it gets resolved in the GDD first. Replan record: `.claude/plans/ROADMAP-REPLAN-2026-09-28.md`.

Two alternatives were considered and **declined** in the same session; do not relitigate them either. **Dropping entt for a `UObject`/GC substrate** — a greenfield rewrite of the core, which decision 1 forbids, and it inverts playbook §2's no-weak-references reasoning; note UE itself ships an archetype ECS (`Engine/Source/Runtime/MassEntity`, plus the `MassEntity`/`MassGameplay` plugins) precisely because `UObject`s do not scale for many simple things. **Waiting for RAD-72** — costs a rewrite of every RAD-98 card built meanwhile.

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
| `Radiant/Source/Radiant/ECS/` | Storage and the entity handle: `Level` (wraps private `entt::registry`), `Entity`, components, YAML LevelSerializer | Working. Entity lifetime reworked RAD-97: destruction **marks** (eager physics teardown, `PendingDestroyComponent`) and **reaps** at the end of the fixed step, so `DestroyEntity` is safe from anywhere; `GetLiveEntitiesWith` is the default view and `IsEngineComponent<T>` gates engine-private components. **Behaviour storage reworked RAD-101**: instances live in a Level-owned side table (`unordered_map<entt::entity, vector<Scope<EntityBehaviour>>>` + a maintained walk order), leaving the empty engine-private `BehaviourComponent` tag — so `Components.h` has no plain-data violations left, and RAD-30's script half is closed. The gameplay facades moved out to `Gameplay/` with RAD-99 |
| `Radiant/Source/Radiant/Gameplay/` | The framework games subclass, and the entity-scoped facades: `EntityBehaviour` (behaviour base — Radiant's `UActorComponent` to `Entity`'s `AActor`; **several per entity of distinct types since RAD-101**, attach-order update, LIFO teardown), `GameplayLevel` (the Level's gameplay-safe verb surface, frame-driving methods structurally unreachable — RAD-95), `PhysicsBody` (entity-scoped physics facade — RAD-90) | Established RAD-99 (2026-08-03). Depends on `ECS/` and `Physics/`, never the reverse. Grows `Pawn`/`Controller` with RAD-102 |
| `Radiant/Source/Radiant/Physics/` | `PhysicsWorld2D` — per-Level Box2D v3 world; `ContactEvent` — vendor-free contact record | Phase 2 rework complete: per-Level worlds (RAD-27), sync semantics (RAD-28: explicit verbs, move-event drain), collision events (RAD-29: post-step contact drain, validity-checked dispatch to Level-wide callbacks + script hooks), verb surface (RAD-91: one id-resolution helper per id kind, `Entity` by value), dynamics verbs (RAD-90: forces/impulses/velocity/kinematic mover on the `PhysicsBody` handle in `Gameplay/`, plus `TeleportType`). Follow-up: RAD-76 query API |
| `Radiant/Source/Radiant/Asset/` | Handle-based asset manager + YAML registry (`.rdar`) | Working; lifecycle redesign in Phase 4 |
| `Radiant/Source/Radiant/Serialization/` | Stream I/O + binary AssetPack | AssetPack is dead code, parked for Phase 4 |
| `Reaper/Source/` | The game: state machine (MainMenu/Gameplay/Paused), GameLayer (Level driver), UILayer (ImGui). ~1,990 lines, roughly a third of it engine-verification probes retiring with RAD-92. Grows into the vertical slice under RAD-108 | Working shell → the slice |
| `Reaper/Design/` | **`GDD.md` — the game's design and the source of truth for game scope.** Lives with the game, never in `Docs/`: engine docs describe how Radiant works, this describes what Reaper is, and the engine never knows about the game | Established 2026-09-28 |

Custom asset formats: `.rdlvl` (YAML level), `.rdar` (YAML asset registry), `.rdap` (binary asset pack — future), `.rdfa` (cached font atlas).

---

# Engineering Rules

**Ownership — one story per type, stated where the type is declared.**
- `Ref<T>` (intrusive refcount) for shared engine resources and assets. `Scope<T>` for unique ownership. Never both on one type.
- No raw owning pointers in new code. Types owning raw API handles (GL/Vulkan objects) must delete copy or implement rule-of-5.

**Components are plain data** (rule enforced from Phase 2): trivially copyable, serializable, no owning raw pointers, no `std::function`. Runtime state (physics bodies, behaviour instances) lives in Level-owned side tables keyed by entity — behaviours are the first one (RAD-101), and its shape is the precedent. *If it can't be memcpy'd and serialized, it doesn't belong in a component.* A side table that must also be findable from an `entt` view keeps an **empty, engine-private tag** alongside it; that tag is maintained only by the table's own attach/detach path, in both directions.

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
- **`ROADMAP.md` (repo root) is the readable view of where the project is** — current milestone, next card, every slice card with its status, what's parked, what's done. **Read it first in a fresh session.** It is a view of the board, not a second source of truth: if it and Jira disagree, Jira is right and `ROADMAP.md` is stale. Update it when a card finishes, a gate passes, a merge lands, or scope changes.
- **Tracking:** everything lives in Jira project RAD. Every non-epic issue is parented to exactly one epic.
  - **Active:** **RAD-105** (Slice Engine Capabilities — engine work the slice needs), **RAD-108** (Reaper — the game), **RAD-3** (RHI v2 + Vulkan, running between M1 and M2).
  - **Parked:** **RAD-106** (Reaper World Layer — dialogue, inventory, shops, map, classes), **RAD-107** (Engine Debt — non-blocking cleanup, pulled from as capacity allows).
  - **Historical / deferred:** RAD-1 (Triage, Done), RAD-2 (Simulation, closing), RAD-4 (Assets), RAD-5 (Editor), RAD-6 (Icebox), RAD-98 (Gameplay Framework, reduced).
  - **Engine vs game placement is the invariant that matters most:** engine capability parents to RAD-105 and **must never name a Reaper type, asset or gameplay concept**; everything under `Reaper/` parents to RAD-108. An epic says *what kind of work*, a milestone label (`m1`…`m5`) says *when*.
  - Labels: `mentorship` (user implements, Claude guides) / `chore` (Claude implements) / `milestone` (a playtest gate) / `m1`–`m5` (slice sequencing) / `world-layer` / `audit-finding` / `design` / `icebox`.
- **Creating/updating issues:** use the `/jira` skill — never ad-hoc `createJiraIssue` calls.
- **Planning a story:** `/plan-feature RAD-XX` — plan file in `.claude/plans/`, posted to the issue on approval, then guided piece-by-piece implementation.
- **Story cadence (every story, in order):** `/plan-feature RAD-XX` → guided implementation (Kareem writes core, Claude does chores/guards/logging/docs) → Kareem runs `/review` → Claude writes the **Story Implementation Report** (format in the plan-feature skill: associate-level explanation of what/why with code snippets, posted to the Jira story) → transition per Definition of Done.
- **Merge cadence (revised 2026-09-28):** stories commit to `dev` as they finish; `dev` merges to `master` **at milestone boundaries** (M1…M5, and the Vulkan block's gates). A card reaches **Done** when its milestone's gate passes and the merge lands — roughly every 3–5 weeks.
  - **The old rule (phase boundaries, decided 2026-08-01) was replaced because it lied about status.** Eight finished cards sat In Review from July to late September — work that was done read as unfinished for three months, by construction. `master` still only ever holds coherent runnable states; the states are just milestones now, which is what the original rule was actually protecting.
- **Reviewing:** `/review` before committing engine changes.
- **Playbook:** `.claude/references/radiant-playbook.md` holds established patterns and hard-won rules; cite it by section, keep it current via `/audit-standards`.
- **System docs:** `Docs/` holds per-system architecture documentation (index: `Docs/README.md`). **The update contract: any change that alters a system's behavior or architecture updates that system's doc in the same change** — `/review` flags stale docs as an ERROR. `/sync-docs` pushes `Docs/` to Confluence.
