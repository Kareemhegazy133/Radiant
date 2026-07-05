---
name: jira
description: Create or update Jira items (epics, stories, tech enablers, tasks, bugs) for the Radiant engine project to a AAA studio standard. Enforces issue-type taxonomy, engine-vs-game placement, phase-epic parenting, Definition of Ready/Done, dependency links, and the architecture-realignment convention. Usage - /jira <type> <summary> or /jira to be prompted.
---

# Jira Item Manager — AAA Standard (Radiant)

You manage Jira items for the Radiant engine project to the standard a AAA studio's backlog is held to. You create new items OR update existing ones — **never duplicate**, never let the backlog drift from the architecture.

**Project context:** Read the **Project Identity** table at the top of this project's `CLAUDE.md`. Jira Project Key is `RAD`, Cloud ID is `hndredgames.atlassian.net`.

**Principal Engineer Mandate:** Every epic, story, technical note, and acceptance criterion must read like a principal engine engineer at a AAA studio wrote it:
- Technical Notes specify the architecturally correct approach — name the pattern, the playbook section, the subsystem seam. Never just "implement X."
- AC reject approaches a principal would reject (variable timestep in simulation code; GL concepts leaking into the RHI; raw owning pointers; components holding behavior; paths instead of handles).
- Flag architectural prerequisites as explicit dependencies and propose the better design when the described approach won't scale.
- Capture "the right way to build it," not the minimum to ship.

---

## The Backlog Contract (what "AAA" means here)

Enforce these invariants on every create/update, and flag violations you notice elsewhere:

1. **Every non-epic item has exactly one epic parent** (the `parent` field). No orphans. The standing parents are the phase epics: RAD-1 (Triage), RAD-2 (Simulation), RAD-3 (RHI/Vulkan), RAD-4 (Assets), RAD-5 (Editor), RAD-6 (Icebox). New epics require a real system scope (e.g. a dedicated Reaper-gameplay epic once the game grows beyond engine-proving) — no `Misc`/catch-all epics.
2. **Epics are phase- or system-scoped with a written scope statement and exit criteria.** A phase epic **closes when its exit criteria are met**; a system epic lives as long as the system does.
3. **Status reflects reality.** Work that shipped is `Done`; work underway is `In Progress`; nothing delivered sits in `To Do`. An epic whose exit criteria are met gets transitioned.
4. **Dependencies are modeled as issue links**, not only prose (see Dependency Modeling). Stale links are removed.
5. **No item instructs a forbidden pattern.** If the architecture moved (e.g. the event queue replaced blocking dispatch), stories still prescribing the old way are realigned (see Architecture Realignment), not left to mislead.
6. **Labels are current and meaningful:** `mentorship` (Kareem implements, Claude guides), `chore` (Claude implements), `audit-finding` (traced to the 2026-07-04 audit), `milestone` (integration proof points), `design` (produces a decision/doc), `icebox` (deferred).
7. **Phase discipline:** an item that belongs to a later phase's territory is parented there — not smuggled into the current phase. Work contradicting a locked decision (CLAUDE.md → Mission & Locked Strategy) is flagged, not filed.

---

## Issue Type Taxonomy (pick the right type)

| Type | Use for | Shape |
|------|---------|-------|
| **Epic** | A phase or long-lived system delivered over many issues. Scope statement + exit criteria. | n/a |
| **Story** | A vertical slice of engine or game capability with an observable result (renders, simulates, loads, edits). | Problem-statement format (below) or `As an engineer…` where it genuinely fits. |
| **Tech Enabler** | A seam, refactor, or contract skeleton that unblocks future work but has no observable result on its own. | `What it unblocks` stated explicitly. |
| **Task** | Concrete chore: build config, git hygiene, docs, tooling, asset wrangling. Research/decision work is a Task labeled `design` with a time-box and a deliverable (decision/doc, not code). | Imperative summary. |
| **Bug** | A defect: observed vs expected, with `file:line` where known. | Bug format (below). |
| **Feature** | Icebox-level future system (parented to RAD-6). Promoted to Stories when its phase activates. | Scope sketch + activation prerequisites. |

When unsure between Story and Tech Enabler: if you can *see or measure* the result by running Reaper (or the editor), it's a Story; if only later work benefits, it's a Tech Enabler.

---

## Engine vs Game Placement (critical — this is where backlogs rot)

Radiant (the engine) is game-agnostic and consumed by Reaper today and other games later. **Where an item lives is determined by reusability — not by who is working on it:**

- **Engine work** (anything under `Radiant/Source/`) **must not name a Reaper type, asset, or gameplay concept.** Game behavior flows in via the engine's public API — layers, components, callbacks, asset handles.
- **Game work** (anything under `Reaper/Source/` or `Reaper/Assets/`) consumes engine APIs and may not reach into `Platform/` internals.
- When a feature spans both (e.g. a new engine capability + Reaper's use of it), **split it**: an engine story and a game story, each correctly placed, linked `Relates`.

An engine story whose AC mentions `ReaperContext` or a Reaper asset is misplaced — fix the placement or the design.

---

## Definition of Ready (before an item is pickup-ready)

- [ ] What + why written; the problem is stated, not just the task.
- [ ] Technical Notes name the architecture/pattern/playbook section, not just the goal.
- [ ] AC are specific, testable, and reject the wrong approach.
- [ ] `file:line` references included where the work touches existing code.
- [ ] Dependencies identified and linked; blockers are `Done` or sequenced.
- [ ] Correct issue type, phase-epic parent, labels (`mentorship`/`chore` at minimum), and priority set.
- [ ] Performance implications stated for anything on a hot path (per-frame, per-entity, per-draw).

## Definition of Done (mirrors CLAUDE.md)

- [ ] Compiles clean in **Debug, Release, and Dist**, no new warnings.
- [ ] **Reaper runs** — the proving-ground contract; visually verified for renderer work.
- [ ] Ownership/lifetime documented for new types (Ref/Scope/value, who deletes).
- [ ] Asserts use `(condition, "message")` form; misconfiguration paths log warnings.
- [ ] Follows the playbook — no known bug patterns (§8) introduced.
- [ ] For `mentorship` items: Kareem implemented the core with guidance (the learning contract).
- [ ] Reviewed (`/review`), merged, issue transitioned to `Done`.

---

## Priority Rubric

- **Highest** — blocks other work right now, or a crash/corruption-class bug.
- **High** — on the critical path of the current phase.
- **Medium** — wanted this phase, not blocking.
- **Low** — polish, nice-to-have, or trigger-gated until a precondition exists.

## Status Workflow & Transition Discipline

`To Do → In Progress → In Review → Done`. `Won't Do` for superseded/cancelled.
- Move to **In Progress** when work actually starts — never leave started work in `To Do`.
- Move to **Done** only when the Definition of Done is met **and the change is merged**.
- When closing as superseded, use **Won't Do**, name the superseding issue in a closing comment, and add a `Duplicate`/`Relates` link.

## Dependency Modeling

Model dependencies as **issue links**, and summarize them inline in Context:
- **Blocks / is blocked by** — hard sequencing (B must merge before A starts).
- **Relates** — same area / informative, no hard gate.
- **Duplicate** — one supersedes the other (survivor stays open; the other goes `Won't Do`).

Rules: remove links whose target is `Done` when they no longer inform sequencing; convert relative dates to absolute (`2026-07-04`); never invent a dependency you haven't verified.

---

## Architecture Realignment Convention (updating stale items)

When an existing item's design has been overtaken by an architecture decision, **do not silently rewrite history**:
1. Append a dated note section:
   ```
   ## Architecture Realignment (YYYY-MM-DD)
   <what changed in the architecture, the superseding issue/decision, and why>
   - <which AC/Tech-Note lines are corrected and how>
   ```
2. Correct the specific conflicting AC/Technical-Note lines **in place**.
3. Preserve the still-valid scope; only the overtaken parts change.
4. Add/refresh the relevant `Relates`/`Duplicate` links.

---

## Workflow (every invocation)

1. **Gather context.** Use the user's description; if vague, ask. Suggest considerations they may have missed (perf implications, ownership, phase fit, whether the editor/Vulkan work will invalidate it).
2. **Search the backlog FIRST** (`searchJiraIssuesUsingJql`, project `RAD`) — broadly, by keyword + subsystem names + variations. The 2026-07-04 audit filed 50+ issues; most known problems already have a card. **Never create without searching** (hard rule).
3. **If a match exists:** show it (key, summary, status, preview); ask whether to UPDATE it (incorporate the new requirement, or realign per the convention above) rather than create a duplicate.
4. **If no match:** determine the issue type, the phase-epic parent (apply Engine-vs-Game placement), the labels, the priority, and the dependencies.
5. **Draft** the item in the format below and **show it to the user for approval** before writing.
6. **Create/Update** via `createJiraIssue` / `editJiraIssue` (cloudId `hndredgames.atlassian.net`, projectKey `RAD`). Set `parent` to the epic. Set labels/priority via `additional_fields`.
7. **Link** dependencies via `createIssueLink`.
8. **Return** the issue key + URL.

---

## Description Format

**Formatting rules:** start directly with the problem statement (no `## Problem` heading); AC and Test Plan use `- [ ]` task lists; list dependencies inline in Context **and** as issue links; include `file:line` refs wherever the work touches existing code.

```
**What:** [the problem or capability — concrete, with file:line refs where applicable]

**Why:** [why it matters — what breaks, what it unblocks, what it teaches]

## Context
[2-3 sentences: how it fits the phase and the wider architecture.]
Dependencies: [RAD-XX (status), RAD-YY (status)] or "None"

## Technical Notes
- [Architecture/pattern guidance — name the playbook section, the seam, the design]
- [Ownership/lifetime approach for new types]
- [Performance notes for hot-path work]

## Acceptance Criteria
- [ ] [Specific, testable criterion]
- [ ] [Rejects the wrong approach where relevant]
- [ ] Compiles clean in Debug/Release/Dist, no new warnings
- [ ] Reaper runs and is visually verified (where applicable)
- [ ] Ownership documented for new types

## Test Plan
- [ ] [Happy path — concrete run steps in Reaper/Sandbox]
- [ ] [Failure/edge path]
- [ ] [Config check: build all three configurations]
```

### Bug Format
```
## Description
[What is happening vs what should happen — file:line]

## Steps to Reproduce
1. … 2. … 3. …

## Expected Behavior
…

## Actual Behavior
…

## Environment
- Branch: [name]   - Config: [Debug / Release / Dist]   - App: [Reaper / Sandbox]
```

---

## Before Creating or Updating

Always show the drafted description and ask for confirmation before writing. **Always use this skill (not ad-hoc `createJiraIssue` calls) when creating Jira items**, so every item lands at this standard.
