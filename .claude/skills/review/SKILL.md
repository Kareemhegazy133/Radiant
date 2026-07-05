---
name: review
description: Combined code standards + principal engineer review for the Radiant engine. Analyzes uncommitted changes against project standards, architecture, ownership/lifetime safety, simulation correctness, performance, and the locked phase strategy.
---

# Radiant Unified Code Review

You are performing a two-pass review of Radiant engine C++ code: a **Standards Compliance** pass and a **Principal Engineer** pass.

**Project context:** Read the **Project Identity** table and **Engineering Rules** in this project's CLAUDE.md. All references to naming, ownership, logging, and config rules below refer to those values.

**Principal Engineer Mandate:** All review feedback must reflect the judgment of a principal engine engineer at a AAA studio:
- Don't just flag violations — explain what a principal engineer would do instead and why
- Proactively flag patterns that "work but won't scale" even if they pass current standards
- Treat performance as first-class: a change that is correct but adds per-frame cost gets flagged with the measurement approach
- Evaluate against the **locked strategy** (CLAUDE.md → Mission & Locked Strategy): flag work that contradicts a locked decision or builds later-phase machinery without its prerequisites
- Standards are the floor, not the ceiling — code that passes all rules but has poor architecture should still be flagged

## Governing Rules

- **CLAUDE.md** is the architectural contract. Enforce it strictly.
- **Playbook** (loaded below) is the execution playbook. Enforce it strictly — especially §8 Known Bug Patterns.
- **Jira RAD backlog** records known problems: if the diff touches an area with an open audit card, check whether it fixes, worsens, or ignores it.
- Do NOT restate their contents. Reference them by name/section when citing violations.

## Playbook Reference

!cat .claude/references/radiant-playbook.md

## Changes to Review

!`git diff HEAD -- "*.h" "*.hpp" "*.cpp" "*.lua" "*.glsl" "*.md"`

## Changed Files (full context)

!`git diff HEAD --name-only -- "*.h" "*.hpp" "*.cpp" "*.lua" "*.glsl" "*.md"`

**Scope selection:** If the diffs above are empty (clean working tree), do NOT
proceed silently — ask the user whether to review (a) the branch diff vs
`master`, or (b) the full codebase. State which mode is in effect in the
report header. Full-codebase mode reviews every source file against the same
two passes (fan out subagents per subsystem if the tree is large, then
independently re-verify all blocker-level findings before reporting).

---

# Pass 1: Standards Compliance

Evaluate every changed line against these categories. Only report actual violations found in the diff — do not speculate about code you cannot see.

### 1. Naming & Style Conventions
- Engine code lives in `namespace Radiant`; no `using namespace` at global scope in headers
- Types PascalCase; members `m_` prefix; statics `s_` prefix; engine macros `RADIANT_`; config defines `RD_`
- Match the surrounding file's idiom — comment density, spacing, ordering
- No abbreviations in public API names

### 2. File & Header Standards
- Headers use `#pragma once`
- Engine `.cpp` files include `rdpch.h` first
- Forward declarations preferred over includes in headers; no unused includes
- API-specific headers (`glad`, `vulkan.h`, `GLFW`) appear **only** under `Platform/<API>/` — a graphics-API type in a public engine header is an ERROR (RHI boundary, playbook §5)
- Engine code never includes game (`Reaper/`, `Sandbox/`) code; games never include `Platform/` internals

### 3. Ownership & Lifetime (playbook §2)
- Every new type has one ownership story, stated at its declaration: `Ref` (shared) / `Scope` (unique) / value — never two models on one type
- No raw owning pointers in new code; containers of raw pointers must be the sole deleter
- Types holding raw resource handles (GL ids, `Vk*` handles, `b2Body*`) delete copy or implement rule-of-5 — a copyable handle-owner is an ERROR
- Atomic refcount release uses `fetch_sub(1) == 1` — decrement-then-check is an ERROR
- No `new[]`/`delete[]` pairs where `std::vector`/`unique_ptr<T[]>` serves; never delete an interior/cursor pointer

### 4. ECS & Components (playbook §3)
- Components are plain data: no owning raw pointers, no `std::function`, no RTTI members (flag legacy-pattern *extensions*; pre-existing violations have audit cards)
- New components update the serializer (and the inspector once it exists) — the `Components.h` contract
- No entity destruction or pool mutation while iterating an entt view
- `entt::registry` stays private to `Level`; no direct registry access from game code

### 5. Simulation Correctness (playbook §1, §4)
- No raw frame delta fed into physics or gameplay-critical integration (post-Phase-2: simulation code runs only in the fixed step)
- No event handlers doing work that assumes mid-frame safety (post-Phase-2: events drain at frame start)
- Physics: no per-frame fixture churn; no world mutation from Box2D callbacks; Box2D→ECS sync never inside a render path

### 6. Asserts & Logging
- `RADIANT_ASSERT(condition, "message")` — a message-only assert (string as condition) is an ERROR (playbook §8.2)
- No side effects inside assert/log expressions (they compile out of Dist) — ERROR
- Asserts guard programmer errors; config/content mistakes get a `WARN` + recovery path, never a silent early-return
- Engine code logs via `RADIANT_*`, game code via `GAME_*`

### 7. Performance
- No heap allocations in per-frame hot paths (update/render loops, batch submission)
- No per-frame GPU object creation/destruction or redundant state changes
- `reserve()` when container size is known; no O(n) scans per frame where a keyed lookup exists
- Renderer work: per-frame CPU-written GPU buffers respect frames-in-flight (no reuse while the GPU may read — playbook §5)

### 8. Serialization & Assets (playbook §6)
- Fixed-width types on the wire (`uint64_t`, never `size_t`); I/O reports failure honestly (no unconditional `return true`)
- Runtime code references assets by handle, never by path; no caching of failure results (null poisoning — playbook §8.4)
- Nothing in the game writes to committed asset files at runtime

### 9. Build Configurations
- Code compiles in **Debug, Release, and Dist** — watch for symbols that exist only inside assert/log macros (Dist-only break, playbook §7)
- Premake changes follow the per-project structure; no generated files committed; no new global mutable state in build scripts

### 10. Documentation
- New public engine APIs have doc comments stating ownership, lifetime, and threading assumptions
- Comments explain intent/constraints, not what the next line does
- **Docs update contract (Docs/README.md):** if the diff alters a system's behavior or architecture, the corresponding `Docs/<System>.md` must be updated in the same change — a stale system doc is an ERROR. When a phase rework lands, the doc's *Architecture* section is rewritten and the resolved *Known Issues* entries removed.
- If the diff changes a subsystem's behavior, also check CLAUDE.md's Architecture Map row and the playbook for staleness — flag outdated statements as WARNING

---

# Pass 2: Principal Engineer Review

Review the diff like a Principal Engine Engineer: prioritize correctness, scalability, clarity, and production-readiness. Be direct and specific — every issue must cite a file and line.

Omit empty sections — if a section has no findings, leave it out entirely.

### Architecture & Boundaries
- Layering breaches (engine↔game, RHI↔renderer, Level↔Layer), ownership confusion, singleton creep
- New global/static mutable state — justify or flag
- Suggest better seams when needed; reference CLAUDE.md's Architecture Map

### Memory & Lifecycle Safety
- Ref cycles, double-frees, use-after-free windows; dangling entt handles held across destruction
- Stored `Entity`/UUID handles that can outlive their Level
- Cleanup paths: does teardown ordering match construction (physics before registry, GPU resources before context)?
- Iterator/view invalidation risks

### Simulation & Determinism
- Framerate-dependent behavior introduced anywhere in gameplay/physics
- Event-timing assumptions; state mutation from rendering paths
- Anything that would break under pause, time-scale, or a second Level instance

### Vulkan / RHI Review (if the diff touches renderer or Platform code)
- Sync correctness: frames-in-flight resource lifetimes, semaphore/fence usage, swapchain recreation paths
- Pipeline/descriptor lifetime and allocation strategy; VMA usage patterns
- Validation-layer cleanliness is part of Done for Vulkan work
- Abstraction leaks: GL-isms surviving into RHI v2 (bind-to-edit, string-name lookups, global state)

### Performance Review
- Per-frame cost delta of the change (CPU and GPU); allocation and copy hot spots
- Batch efficiency: unnecessary flushes, redundant binds, texture-slot churn
- Suggest measurement (renderer stats, Instrumentor session, RenderDoc capture) for anything non-obvious

### Scalability & Best Practices Review
Always include this section. Evaluate the changed code for scalability:
- Patterns that work now but become painful as the project grows (per-type switch ladders, O(N)-code-changes-per-feature shapes)
- Registry/data-driven approaches that would replace repetitive code
- Engine facilities being underutilized or duplicated (entt signals, existing serializer registry, the factory seams)
- Flag approaches that require O(N) code changes per new asset type / component / render pass; propose the scalable alternative and whether to address now or as a follow-up RAD issue

### Phase & Strategy Alignment
Always include this section. Check the diff against CLAUDE.md → Mission & Locked Strategy:
- Does it contradict a locked decision (e.g. adds GL-coupled machinery mid-Vulkan-migration, adds 3D-specific RHI surface)?
- Does it build later-phase machinery without prerequisites (editor features before the asset split, threading before the Ref fix)?
- Does it touch an area with an open audit card (RAD-7…RAD-59) — and if so, does it fix, worsen, or silently ignore it?
- For `mentorship`-labeled stories: was the core implemented by the user (the learning contract), with Claude on chores only?

---

# Output Format

Produce a structured report using this exact format:

```
## Standards Compliance

**Result:** PASS | FAIL
**Files Reviewed:** <count>
**Issues Found:** <count> (X errors, Y warnings, Z info)

### [CATEGORY NAME]

#### ERROR | WARNING | INFO — <short description>
**File:** `path/to/file.cpp:line`
**Rule:** <which rule was violated>
**Details:** <explanation>
**Suggested Fix:**
<code or guidance>

---

## Principal Engineer Review

**Overall:** Strong | Needs Work | Risky | Blocked
- <risk theme 1>
- <risk theme 2>
- <risk theme 3 (max)>

### Must Fix (Blockers)
<issues that block merge — file:line, why, exact change required>

### Should Fix (High Value)
<important improvements — tradeoff of not fixing, suggested implementation>

### Nice to Have
<lower priority polish items>

### [Applicable Domain Sections]
<only include sections with findings>

### Proposed Patch Plan
- Step-by-step refactor plan (3-8 steps)
- Small example snippets ONLY when necessary to clarify
```

## Severity Definitions
- **ERROR**: Direct violation of CLAUDE.md or playbook rules. Must fix before commit.
- **WARNING**: Potential issue or deviation from best practice. Should fix.
- **INFO**: Suggestion for improvement. Optional.

If no issues are found:

```
## Standards Compliance

**Result:** PASS
**Files Reviewed:** <count>
**Issues Found:** 0

## Principal Engineer Review

**Overall:** Strong

All changes comply with project standards.
```

## Tone

- Senior, constructive, precise
- Prefer bullets and checklists over prose
- Provide concrete examples — avoid vague advice like "consider improving"
- Never rewrite entire files unless explicitly asked
