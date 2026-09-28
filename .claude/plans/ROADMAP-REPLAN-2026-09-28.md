# Roadmap Replan — 2026-09-28

**Status:** DRAFT — awaiting approval before any Jira changes
**Trigger:** the roadmap had no game in it. 104 issues across five horizontal phases, ~19 cards to a Vulkan triangle, ~50 to an editor, and no card anywhere that produced something playable.
**Design source of truth:** `Reaper/Design/GDD.md` (v0.6)

---

## 1. What Changes

**The organizing principle changes from phase to slice.** Phases 1–5 (Triage → Simulation → RHI/Vulkan → Assets → Editor) stop being the roadmap. They become a backlog that the game draws from.

**An engine card now earns its place by "the slice can't ship without it."** Everything else parks.

**Five playable gates replace one distant milestone.** Every 4–6 cards, you stop and play it. That is the fix for the actual complaint — not that the work was wrong, but that nothing validated it except a probe.

### The headline number

| | Old roadmap | New roadmap |
|---|---|---|
| Cards to the first visible, playable win | **19** (Vulkan first triangle) | **6** (M1 — "it moves") |
| Cards to a complete playable experience | not on the board at all | **~23** (M5 — the slice) |
| Playable checkpoints along the way | 0 | 5 |

---

## 2. New Board Shape

| Epic | Status | Contents |
|---|---|---|
| **RAD-105 — Slice Engine Capabilities** (new) | **Active** | 16 engine cards + 5 gates, sequenced across 5 milestones |
| **RAD-108 — Reaper (the game)** (new) | **Active** | All game-side cards: player pawn, enemies, boss, level authoring, feel passes |
| RAD-3 Phase 3 — RHI v2 + Vulkan | **Active, scheduled between M1 and M2** | 11 cards, 2 internal gates. See §3.5. |
| **RAD-106 — Reaper World Layer** (new) | Parked — starts after M5 | 6 cards: dialogue, inventory, shops, map, activation graph, classes |
| **RAD-107 — Engine Debt** (new) | Parked — pulled from as needed | The non-blocking Phase 2 tail |
| RAD-1 Phase 1 Triage | Done | unchanged |
| RAD-2 Phase 2 Simulation | **Closing** | Blocking bugs retained; rest moves to RAD-107 |
| RAD-98 Gameplay Framework | **Reduced** | RAD-102 moves to the slice; RAD-104 defers |
| RAD-4 Phase 4 Assets | Deferred | unchanged |
| RAD-5 Phase 5 Editor | Deferred | unchanged |
| RAD-6 Icebox | **Drained of 11 cards** | Remainder unchanged |

**Engine / game split:** engine capability cards parent to **RAD-105**; everything under `Reaper/` parents to **RAD-108**. Milestones sequence both — an epic says *what kind of work*, a milestone says *when*. This matches the engine-vs-game placement rule the `/jira` skill enforces.

---

## 3. The Slice — RAD-105

Cards in dependency order. `[P]` = promoted from Icebox. `[N]` = new card. `[K]` = kept from an existing epic.

### M1 — "It moves" · gate: run and jump in a test room, and it feels good

| # | Card | Type | Label | Source |
|---|---|---|---|---|
| 1 | Input action mapping: actions, bindings, rebinding surface | Tech Enabler | mentorship | `[P]` RAD-69 |
| 2 | Input buffering and frame windows: coyote time, jump buffer, action queue | Tech Enabler | mentorship | `[N]` |
| 3 | Physics query API: raycasts, shape casts, overlaps | Story | mentorship | `[P]` RAD-76 |
| 4 | Possession: the Pawn / Controller split | Story | mentorship | `[K]` RAD-102 |
| 5 | **Kinematic character controller: gravity, collide-and-slide, slopes, one-way platforms** | Story | mentorship | `[N]` |
| 6 | Camera framework: follow, lookahead, deadzone, bounds, shake | Story | mentorship | `[P]` RAD-75 |
| 7 | Reaper: the player pawn and the movement tuning pass | Story | mentorship | `[N]` game |
| — | **GATE M1** — movement feel sign-off | Task | milestone | `[N]` |

**Card 5 is the centrepiece of the entire plan.** It is where 2D game feel lives — variable jump height, apex float, fast fall, corner correction, collide-and-slide. It is the single most educational artifact in gameplay programming and it is explicitly yours to write.

**Why RAD-76 precedes the controller:** collide-and-slide and ground checks are shape casts against the Box2D world. The query facade has to exist first.

**Why M1 is safe to build on OpenGL:** not one of these seven cards touches rendering. Input, physics queries, possession, controller physics, and a camera that produces view/projection matrices. The renderer is a consumer of M1's output, never a dependency of it.

---

## 3.5 — The Vulkan Block · between M1 and M2

**Decision: RHI v2 + Vulkan runs after M1 and before M2, not after the slice.**

This reverses the "slice ships on OpenGL" position in the first draft. The reversal is driven by evidence from the codebase, recorded here.

### The evidence

| Check | Result |
|---|---|
| OpenGL types leaking outside `Platform/OpenGL/` | **None.** Every `glad`/`GL*` match was GLFW — *windowing*, which survives the Vulkan move untouched. |
| Public headers exposing the backend | **None.** Every `Platform/OpenGL` include sits in a factory `.cpp` (`Buffer.cpp`, `Texture.cpp`, `Shader.cpp`, …) behind `Create()`. Correct pattern, correctly implemented. |
| `Renderer2D` public surface | Pure `glm` + `Ref<Texture2D>` + `Ref<SubTexture2D>`. `DrawQuad`, `DrawSprite`, `DrawLine`, `DrawRect`. Fully API-agnostic. |
| Backend size | `Platform/OpenGL` = 1,436 lines. Abstraction = 1,793 lines, of which `Renderer2D.cpp` is 509. |

**So the good news first: the boundary is genuinely clean, and the port is contained.**

### The finding that decides it

`RendererAPI.h` is clean — but it is **immediate-mode shaped**. Its own doc comment says so:

> *"Main-thread only; calls execute immediately."*

`SetClearColor` / `Clear` / `DrawIndexed` is a GL-shaped vocabulary. Vulkan does not work that way — you record into command buffers and submit with frames-in-flight. **The abstraction has to change, not just gain a second implementation.** That is precisely what RAD-32 (RHI v2 architecture) exists for.

**The consequence:** anything built against the current RHI before v2 lands inherits the immediate-mode shape and gets reworked. And M2–M4 are exactly the renderer-heavy milestones:

- **M2** — particles / VFX batching
- **M3** — game UI rendering
- **M4** — tilemap chunked batch rendering

Building all three against a GL-shaped immediate-mode RHI, then reworking all three, is real double work. Building them *after* RHI v2 costs nothing extra.

**M1 sits entirely outside that blast radius**, which is why it goes first and why this sequencing works rather than being a compromise.

### The block — 11 cards, 2 internal gates

| # | Card | Board |
|---|---|---|
| V1 | RHI v2 architecture design: command lists, pipelines, descriptors | RAD-32 |
| V2 | Premake: Vulkan SDK + shaderc wiring | RAD-33 |
| V3 | Vulkan bring-up: instance, validation layers, device + queues | RAD-34 |
| V4 | Swapchain + frames-in-flight architecture | RAD-35 |
| V5 | VMA integration + RHI buffer/texture resources | RAD-36 |
| V6 | Shader pipeline: shaderc GLSL→SPIR-V + reflection | RAD-37 |
| — | **GATE — first triangle through RHI v2** | RAD-38 `milestone` |
| V7 | Pipeline state objects + descriptor set abstraction | RAD-39 |
| — | **GATE — Renderer2D on Vulkan, batch parity, Reaper runs** | RAD-40 `milestone` |
| V8 | ImGui Vulkan backend | RAD-41 |
| V9 | **Delete the OpenGL backend** | RAD-42 `milestone` |

**Stays out of the block:** RAD-68 (Tracy), RAD-84 (log categories), RAD-96 (source layout), RAD-103 (Level→World rename). All are Phase 3-parented but none gates the slice.

### The cost, stated plainly

**+11 cards, roughly +3 months.** Slice completion moves from ~23 cards to ~34.

That is the price of not building three renderer features twice, and of deleting OpenGL while the game is still small — which is the cheapest moment it will ever be.

### M2 — "It fights" · gate: hitting a training dummy feels good

| # | Card | Type | Label | Source |
|---|---|---|---|---|
| 8 | Attribute system (GAS-lite): health, Focus, poise, damage, scaling | Story | mentorship | `[P]` RAD-63 *(rescoped smaller)* |
| 9 | Sprite animation: flipbook clips + frame events | Story | mentorship | `[P]` RAD-64 |
| 10 | Time scale and real pause (hit-stop) | Story | mentorship | `[P]` RAD-31 |
| 11 | Particles / VFX: hit sparks, dust, impact | Feature | chore | `[P]` RAD-70 |
| 12 | Reaper: attack set, hitboxes, dash i-frames, pogo | Story | mentorship | `[N]` game |
| — | **GATE M2** — combat feel sign-off | Task | milestone | `[N]` |

**Particles land here, not in polish.** Impact feel is the whole question M2 answers, and it is ~30% hit-stop and sparks.

### M3 — "It fights back" · gate: three enemies, real fights, you can die

| # | Card | Type | Label | Source |
|---|---|---|---|---|
| 13 | Spawn templates / prefabs | Feature | mentorship | `[P]` RAD-74 |
| 14 | AI framework: state machines, perception, leashing, ledge probes | Feature | mentorship | `[P]` RAD-78 |
| 15 | Game UI framework: health masks, Focus meter, souls counter | Feature | mentorship | `[P]` RAD-71 |
| 16 | Reaper: the three slice enemies (Husk, Bell-Keeper, Warden) | Story | mentorship | `[N]` game |
| — | **GATE M3** — encounter sign-off | Task | milestone | `[N]` |

### M4 — "It's a game" · gate: the full death → recover → rest → level loop

| # | Card | Type | Label | Source |
|---|---|---|---|---|
| 17 | Trigger volumes and the shared interaction verb | Tech Enabler | mentorship | `[N]` |
| 18 | Save / load: world flags, player state, bench checkpoints | Story | mentorship | `[N]` |
| 19 | Tilemap: chunked storage, batched rendering, collision generation, one-way platforms | Feature | mentorship | `[P]` RAD-73 |
| 20 | Reaper: bench, souls, bloodstain, level-up | Story | mentorship | `[N]` game |
| — | **GATE M4** — the loop sign-off | Task | milestone | `[N]` |

**Card 17 absorbs W1 from the GDD.** Benches, NPCs, shops, pickups and levers are one interaction primitive — built once here rather than five times later.

### M5 — "It's the slice" · gate: the fifteen minutes, end to end

| # | Card | Type | Label | Source |
|---|---|---|---|---|
| 21 | Audio: impact, telegraph cues, movement | Feature | chore | `[P]` RAD-65 |
| 22 | Reaper: The First Warden — two-phase boss | Story | mentorship | `[N]` game |
| 23 | Reaper: slice level authoring and the feel pass | Story | mentorship | `[N]` game |
| — | **GATE M5** — the slice is done | Task | milestone | `[N]` |

---

## 4. Phase 2 Triage

### Retained — real risk, done inside the slice window

| Card | Why it stays |
|---|---|
| **RAD-79** — `Ref::Reset` adopts without IncRef; BufferSafe lacks copy control | A live refcount bug. Textures, animation clips and tilemaps are about to be allocated heavily. Latent memory corruption gets exponentially harder to find later. **Do before M2.** |
| **RAD-81** — same-frame push/pop layer leak; `~Level` sorts pool mid-iteration | A crash path in code the slice exercises constantly. **Do before M3.** |
| **RAD-80** — Level deserialization swallows failure | Levels stop being one test file and become authored content in M4. Silent load failure becomes very expensive then. **Do before M4.** |

### Moved to RAD-107 (Engine Debt), parked

| Card | Note |
|---|---|
| RAD-30 — Data-only components | Largely closed by RAD-101. **Verify, then close or shrink** rather than carry. |
| RAD-94 — Entity accessor conventions | Largely delivered by RAD-99. Same treatment. |
| RAD-67 — doctest harness | Genuinely valuable, not slice-blocking. |
| RAD-92 — Retire Reaper verification scaffolding | Blocked on RAD-67; the probes are harmless meanwhile. |
| RAD-82 — Doc-pass minor findings | Cosmetic. |
| RAD-83 — Debug-ASan configuration | Valuable when the refcount bugs are chased. Pair with RAD-79 if it fights back. |
| RAD-88 — yaml-cpp re-home to owned fork | Policy tidiness, no functional impact. |

---

## 5. The RAD-104 Call — a deliberate reversal, flagged

**RAD-104 (reflection MVP / component+behaviour collapse) defers out of the slice.** This reverses the amendment recorded in CLAUDE.md and commit `d87c374` — *yesterday*. It gets stated plainly rather than quietly dropped.

**Why it was pulled forward:** RAD-98's framework was under construction, and every card written before the collapse would be rewritten after it.

**Why that reason has weakened:** RAD-98 is now largely paused. Only RAD-102 survives into the slice. The body of framework cards that justified the pull-forward is no longer being written.

**The honest counter-argument, stated:** the slice adds ~15 new behaviour classes (player, three enemies, boss, bench, bloodstain, camera). If the model collapses later, those get refactored.

**Verdict — defer, and here is the trade being made:**

| | Defer (chosen) | Do it first |
|---|---|---|
| Cost | A mechanical refactor of ~15 behaviour classes later — a **chore**, bounded, Claude's to do | One more invisible plumbing card before anything moves on screen |
| Risk | Refactor is larger than estimated | Another week of the exact pattern that caused this replan |

**Repayment point: after GATE M5, before region 1 work begins.** Recorded as debt, with a named date, not forgotten.

---

## 6. Merge Cadence Fix

**Problem:** "dev merges to master at phase boundaries" has left 8 finished cards sitting In Review — some since July. Work that is done reads as unfinished for months, by construction.

**Fix, two parts:**

1. **Immediately:** merge `dev` → `master` and transition RAD-29, 90, 91, 95, 97, 99, 100, 101 to **Done**. Phase 2 is winding down; that work is finished and should say so.
2. **Going forward:** merge at **milestone boundaries** (M1…M5), not phase boundaries. Cards reach Done when their milestone's gate passes — roughly every 3–5 weeks instead of every 3 months.

`master` still only ever holds coherent, runnable states. The states are just milestones now, which is what the original rule was actually protecting.

---

## 7. Execution Order

On approval, in this order:

1. Create epics **RAD-105** (Slice Engine Capabilities), **RAD-108** (Reaper), **RAD-106** (World Layer, parked), **RAD-107** (Engine Debt, parked)
2. Reparent the 11 promoted Icebox cards → RAD-105, rescoping each description to its slice role
3. Reparent RAD-102 → RAD-105; RAD-31 → RAD-105
4. Create the 6 new engine cards + 5 gate cards in RAD-105
5. Create the 6 Reaper game cards in RAD-108
6. Move the 7 deferred Phase 2 cards → RAD-107
7. Create the 6 World Layer cards in RAD-106
8. **Leave RAD-3 and its 11 Vulkan cards in place**, retagged with milestone position (between M1 and M2); move RAD-68/84/96/103 out to RAD-107
9. Retag RAD-104 with the deferral note and its repayment point (after GATE M5)
10. Merge `dev` → `master`; transition the 8 In Review cards to Done
11. Update `CLAUDE.md`: locked decision 4 (phase order) re-opened and replaced by **slice ordering with the Vulkan block between M1 and M2**; record the merge cadence change; add `Reaper/Design/GDD.md` as the game's source of truth

---

## 8. Honest Estimate

**~34 substantive cards, 8 gates.** At the observed ~1 card/week:

| Stage | Cards | Cumulative | Ends with |
|---|---|---|---|
| **M1** | 7 | ~7 wks | **Playable: run and jump, and it feels good** |
| Vulkan bring-up | 6 | ~13 wks | **Gate: first triangle through RHI v2** |
| Vulkan parity | 2 | ~15 wks | **Gate: Reaper runs on Vulkan** |
| Vulkan cleanup | 2 | ~17 wks | **Gate: OpenGL deleted** |
| **M2** | 5 | ~22 wks | **Playable: combat feels good** |
| **M3** | 4 | ~26 wks | **Playable: real fights, you can die** |
| **M4** | 4 | ~30 wks | **Playable: the full death/rest/level loop** |
| **M5** | 3 | ~34 wks | **The slice** |

**Roughly eight months to a complete slice.** That is honest, and it is three months longer than the OpenGL-only route.

What actually changed is the shape, not the order:

- **Never more than ~7 cards from a visible win**, versus 19 to a triangle and nothing playable ever
- **8 gates**, versus 0
- Several cards (animation, particles, audio, premake wiring) are materially faster than a week
- Three renderer features get built once instead of twice, and OpenGL dies while the game is small

**The honest risk:** the Vulkan block is 10 weeks with only two small visible wins in it. That is the same shape as the pattern that caused this replan, and it is being accepted deliberately because the alternative is reworking particles, UI and tilemap. If momentum stalls during it, M2 can be pulled forward ahead of RAD-41/42 and the GL backend deleted later.

If M1 lands and movement feels good, the rest of this plan is worth trusting. If it does not, we find out in seven weeks instead of six months.
