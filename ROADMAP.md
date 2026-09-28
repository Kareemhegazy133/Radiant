# Radiant / Reaper — Roadmap

**Living document.** Where the project is right now and what comes next. Updated as cards land and gates pass.

- **Why the roadmap looks like this:** `.claude/plans/ROADMAP-REPLAN-2026-09-28.md` (the dated decision record — read once, not maintained)
- **What the game is:** `Reaper/Design/GDD.md`
- **Board:** Jira project [RAD](https://hndredgames.atlassian.net/jira/software/projects/RAD)

---

## Where we are

> **M1 — "It moves."** Building the player controller. Nothing is playable yet; M1's gate is the first time it will be.

**Next card: [RAD-69](https://hndredgames.atlassian.net/browse/RAD-69) — Input action mapping.**

| | |
|---|---|
| Last merged to `master` | 2026-09-28 — Phase 2 simulation foundation + the gameplay framework (8 cards) |
| Cards to the next playable moment | **7** (GATE M1) |
| Cards to a complete slice | ~34 |

---

## The organizing principle

**The game orders the roadmap.** An engine card earns its place by being something Reaper cannot ship without. Every milestone ends in a **gate** — you stop and play it, and the gate can send cards back.

This replaced phase ordering on 2026-09-28. The old board had 104 issues across five horizontal phases, ~19 cards to a Vulkan triangle, and **no card anywhere that produced something playable**.

---

## The slice

### M1 — "It moves"
**Gate: run and jump in a test room, and it feels good.**

| | Card | |
|---|---|---|
| 1 | [RAD-69](https://hndredgames.atlassian.net/browse/RAD-69) — Input action mapping | ☐ |
| 2 | [RAD-110](https://hndredgames.atlassian.net/browse/RAD-110) — Input buffering + frame windows | ☐ |
| 3 | [RAD-76](https://hndredgames.atlassian.net/browse/RAD-76) — Physics query API (raycasts, shape casts, overlaps) | ☐ |
| 4 | [RAD-102](https://hndredgames.atlassian.net/browse/RAD-102) — Possession: the Pawn / Controller split | ☐ |
| 5 | **[RAD-109](https://hndredgames.atlassian.net/browse/RAD-109) — Kinematic character controller** ← the centrepiece | ☐ |
| 6 | [RAD-75](https://hndredgames.atlassian.net/browse/RAD-75) — Camera framework + screenshake | ☐ |
| 7 | [RAD-111](https://hndredgames.atlassian.net/browse/RAD-111) — *Reaper:* player pawn + movement tuning pass | ☐ |
| — | **[RAD-112](https://hndredgames.atlassian.net/browse/RAD-112) — GATE M1** | ☐ |

> RAD-76 comes **before** the controller: collide-and-slide and ground checks are shape casts.
> M1 touches no rendering at all — which is why the Vulkan block can follow it cleanly.

### Vulkan block — [RAD-3](https://hndredgames.atlassian.net/browse/RAD-3)
**Runs between M1 and M2.** M1 needs no renderer; M2–M4 are renderer-heavy, and `RendererAPI` is immediate-mode shaped — so RHI v2 lands before them rather than after, or particles, UI and tilemap all get built twice.

RAD-32 → 33 → 34 → 35 → 36 → 37 → **RAD-38 (gate: first triangle)** → 39 → **RAD-40 (gate: Reaper runs on Vulkan)** → 41 → **RAD-42 (gate: OpenGL deleted)**

### M2 — "It fights"
**Gate: hitting a training dummy feels good.**

[RAD-63](https://hndredgames.atlassian.net/browse/RAD-63) attributes (health/Focus/poise) · [RAD-64](https://hndredgames.atlassian.net/browse/RAD-64) sprite animation + frame events · [RAD-31](https://hndredgames.atlassian.net/browse/RAD-31) time scale + hit-stop · [RAD-70](https://hndredgames.atlassian.net/browse/RAD-70) particles · [RAD-118](https://hndredgames.atlassian.net/browse/RAD-118) *Reaper:* attack set, hitboxes, dash i-frames, pogo · **[RAD-115](https://hndredgames.atlassian.net/browse/RAD-115) GATE M2**

### M3 — "It fights back"
**Gate: three enemies, real fights, you can die.**

[RAD-74](https://hndredgames.atlassian.net/browse/RAD-74) spawn templates · [RAD-78](https://hndredgames.atlassian.net/browse/RAD-78) AI framework · [RAD-71](https://hndredgames.atlassian.net/browse/RAD-71) game UI · [RAD-119](https://hndredgames.atlassian.net/browse/RAD-119) *Reaper:* Husk, Bell-Keeper, Warden · **[RAD-116](https://hndredgames.atlassian.net/browse/RAD-116) GATE M3**

### M4 — "It's a game"
**Gate: the full death → recover → rest → level loop.**

[RAD-113](https://hndredgames.atlassian.net/browse/RAD-113) trigger volumes + interaction verb · [RAD-114](https://hndredgames.atlassian.net/browse/RAD-114) save/load + world flags · [RAD-73](https://hndredgames.atlassian.net/browse/RAD-73) tilemap · [RAD-120](https://hndredgames.atlassian.net/browse/RAD-120) *Reaper:* bench, souls, bloodstain, level-up · **[RAD-117](https://hndredgames.atlassian.net/browse/RAD-117) GATE M4**

### M5 — "It's the slice"
**Gate: the fifteen minutes, end to end.**

[RAD-65](https://hndredgames.atlassian.net/browse/RAD-65) audio · [RAD-121](https://hndredgames.atlassian.net/browse/RAD-121) *Reaper:* The First Warden (two-phase boss) · [RAD-122](https://hndredgames.atlassian.net/browse/RAD-122) *Reaper:* slice level authoring + feel pass · **[RAD-123](https://hndredgames.atlassian.net/browse/RAD-123) GATE M5**

> GATE M5 also repays [RAD-104](https://hndredgames.atlassian.net/browse/RAD-104) (the component + behaviour collapse), deferred on 2026-09-28 with this as its named due date.

---

## Scheduled bug work

Not parked — these are real risk in code the slice hammers constantly.

| Card | Do it before | Why |
|---|---|---|
| [RAD-79](https://hndredgames.atlassian.net/browse/RAD-79) — `Ref::Reset` refcount bug | **M2** | Textures, animation clips and tilemaps are about to be allocated heavily; latent memory corruption gets exponentially harder to find later |
| [RAD-81](https://hndredgames.atlassian.net/browse/RAD-81) — `~Level` sorts pool mid-iteration | **M3** | A crash path in code the slice runs constantly |
| [RAD-80](https://hndredgames.atlassian.net/browse/RAD-80) — Level deserialization swallows failure | **M4** | Levels stop being one test file and become authored content |

---

## After the slice

**[RAD-106](https://hndredgames.atlassian.net/browse/RAD-106) — Reaper World Layer.** Lands **with region 1**, not later: a region with no map, no merchant and nobody to talk to does not prove the game.

Dialogue ([RAD-124](https://hndredgames.atlassian.net/browse/RAD-124)) · inventory ([RAD-125](https://hndredgames.atlassian.net/browse/RAD-125)) · shops ([RAD-126](https://hndredgames.atlassian.net/browse/RAD-126)) · map ([RAD-127](https://hndredgames.atlassian.net/browse/RAD-127)) · activation graph ([RAD-128](https://hndredgames.atlassian.net/browse/RAD-128)) · starting classes ([RAD-129](https://hndredgames.atlassian.net/browse/RAD-129))

**Then v1.0 Reaper** — 3 regions, 4 main bosses, 3 mini-bosses, 6 minor dungeons, 6 weapons, 3 spells, 5 classes, 2 traversal upgrades. Built in this order, content breadth last:

> slice → region 1 (incl. world layer) → region 2 → region 3 → classes → minor dungeons → polish

**Designated cuts if the schedule slips, in order:** minor dungeons → classes → the third region. All additive; nothing depends on them.

---

## Parked

**[RAD-107](https://hndredgames.atlassian.net/browse/RAD-107) — Engine Debt.** Real improvements with no deadline, pulled from opportunistically: doctest harness (RAD-67), ASan config (RAD-83), yaml-cpp fork (RAD-88), doc sweep (RAD-82), scaffolding retirement (RAD-92), Tracy (RAD-68), log categories (RAD-84), source layout (RAD-96), `Level`→`World` rename (RAD-103).

> RAD-30 and RAD-94 are likely **already delivered** by RAD-101 and RAD-99 — verify and close rather than carry.

**Deferred phases:** [RAD-4](https://hndredgames.atlassian.net/browse/RAD-4) Assets · [RAD-5](https://hndredgames.atlassian.net/browse/RAD-5) Editor · [RAD-6](https://hndredgames.atlassian.net/browse/RAD-6) Icebox (jobs, render graph, 3D, scripting, networking)

---

## Done

**Phase 1 — Triage** ([RAD-1](https://hndredgames.atlassian.net/browse/RAD-1)): 25 cards from the 2026-07-04 audit.

**Phase 2 — Simulation foundation + gameplay framework** (merged to `master` 2026-09-28): fixed-timestep loop with accumulator (RAD-25) · event queue (RAD-26) · per-Level Box2D v3 world (RAD-27) · physics transform ownership (RAD-28) · collision event queue (RAD-29) · gameplay dynamics verbs (RAD-90) · physics verb surface (RAD-91) · script Level/Entity access (RAD-95) · deferred entity destruction (RAD-97) · Gameplay module + facades (RAD-99) · typed behaviour retrieval (RAD-100) · behaviour composition (RAD-101)

---

## Cadence

- **Stories commit to `dev` as they finish.** `dev` merges to `master` at **milestone boundaries** — a card reaches Done when its gate passes and the merge lands, roughly every 3–5 weeks.
- **Every story:** `/plan-feature RAD-XX` → guided implementation → `/review` → Story Implementation Report on the Jira card → transition.
- **Gates are playtests, not build checks.** They can send cards back, and passing one is the moment to capture demo-reel footage — first-time-working is unrepeatable.

## Keeping this current

Update on: a card finishing, a gate passing, a merge to `master`, or a scope decision. **If this file and Jira disagree, Jira is right and this file is stale** — it is a readable view of the board, not a second source of truth.
