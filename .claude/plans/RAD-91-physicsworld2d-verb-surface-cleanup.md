# Implementation Plan — RAD-91: PhysicsWorld2D verb-surface cleanup (ticket-resolution helper + Entity by value)

| Field | Value |
|-------|-------|
| **Jira** | [RAD-91](https://hndredgames.atlassian.net/browse/RAD-91) |
| **Epic** | RAD-2 — Phase 2, Simulation Foundation |
| **Story status** | To Do → In Progress |
| **Dependencies** | RAD-28 (Done), RAD-29 (In Review — committed to `dev`; same file, so this lands on top) |
| **Blocks** | RAD-90 (gameplay dynamics verbs) |
| **Planned** | 2026-08-01 |

---

## 0. The Problem, Ground Up

### What the code literally does today

`PhysicsWorld2D` is the one place in the engine allowed to speak Box2D. Everything gameplay wants
done to a physical body goes through one of its methods — destroy this body, teleport it, rebuild
its collider shape. Each of those methods has to answer the same question before it can do anything:
**"is the thing I was asked to act on still there?"**

A body isn't reached by pointer. The component stores a *ticket* — a 64-bit packed `b2BodyId`, which
is a slot number plus a generation counter (playbook §2). "Generation" means: when Box2D reuses slot
#7 for a different body, it bumps a counter, so an old ticket for slot #7 is detectably out of date
rather than silently pointing at a stranger. Handing back a ticket for a body that has already died
is *checkable*, and checking it is exactly what these methods must do.

So every method opens with the same preamble:

```text
read the ticket off the component
if the ticket is 0                    -> the body was never created; quietly do nothing
unpack the ticket into a live id
if the id doesn't check out           -> WARN naming the verb, zero the ticket, do nothing
...now, finally, do the actual work (usually one Box2D call)
```

That is roughly eight lines of policy in front of one line of work, and it is currently written out
**five separate times** — in `DestroyBody`, `Teleport`, `CreateBoxShape`, `DestroyBoxShape` and
`UpdateBoxShape` (`Radiant/Source/Radiant/Physics/PhysicsWorld2D.cpp:139-314`).

### Why five copies is a problem, with real numbers

RAD-90 is the next story on this file, and it adds roughly **eight more verbs**: `ApplyForce`,
`ApplyImpulse` (two overloads), `SetLinearVelocity`, `SetAngularVelocity`, their two getters, and
`MoveKinematic`. Every one of them needs the identical preamble. Written the current way that is
**13 hand-written copies of one policy**, ~104 lines of guard in front of ~13 lines of actual work.

Now imagine the policy changes — and it will. Three plausible near-term changes:

- RAD-90's verbs may fire from a script *every fixed step*. If a stale ticket starts warning 60×/second
  the log becomes unreadable, and someone downgrades that WARN to a rate-limited TRACE. That edit has
  to land in 13 places.
- `Level::Copy` (play-in-editor, Phase 5) must zero copied tickets, because a v3 id embeds its world
  index — a shallow-copied id silently addresses the *original* world's body. When that lands, the
  stale-recovery path may need to say something different.
- The day a second collider shape per entity exists, "zero the ticket" stops being one field.

Miss one of 13 and you get the worst possible failure shape: a guard that is *almost* everywhere.
The code reviews fine, runs fine in Debug, and the one un-updated path is the one that reaches Box2D
with a dead id in Dist — where `RADIANT_ASSERT` is compiled out and Box2D's own `B2_ASSERT` is
compiled out too, so there is nothing left between a stale ticket and undefined behaviour.

This failure class is not hypothetical. Apple's 2014 "goto fail" TLS bug was a *duplicated line in a
validation preamble*: the certificate-check code repeated its guard pattern, one copy drifted, and
signature verification silently succeeded for everyone. The bug was not in the crypto; it was in
hand-copied control flow around the crypto. Duplicated guards are where this class of defect lives.

### The fix, as an everyday thing

Today, every department in the building runs its own sign-in desk, and each desk has its own copy of
the visitor rules taped to the wall. When the rules change, thirteen pieces of tape need replacing,
and the one nobody replaces is the door a stranger walks through.

We are building **one front desk**. You state which department you're visiting (that's the `verb`
string, so the log still says *who* was called), the desk checks your badge against the one copy of
the rules, and it either hands you a validated badge or turns you away — logging the turn-away
itself. Departments stop owning rules; they just check whether they got a badge.

That is the whole first half of this story: `ResolveBodyTicket` and `ResolveShapeTicket`, one per id
kind, replacing five copies and pre-paying for eight more.

### The second half: a form that lies

Every public method on `PhysicsWorld2D` currently takes `Entity&` — a **mutable reference** to a
16-byte value handle whose own header says it is *"copied freely"*
(`Radiant/Source/Radiant/ECS/Entity.h:12`).

In C++, `Type&` in a parameter list is a promise with two halves. It says *"I may write to your copy
of this"* — which is a lie here; no method ever does. And it says *"you must hand me a variable, not
an expression"* — so `world.Teleport(level.GetEntityByUUID(id), pos, 0.f)` does not compile, for no
reason anyone can defend. Meanwhile `Level::Teleport(Entity entity, ...)` — the layer directly above,
in the same subsystem — takes it **by value**. Two files, one type, two contradictory conventions,
introduced a story apart across RAD-27 and RAD-28. Fixing it is a one-character-per-signature edit;
the value is that RAD-90's eight new signatures get written once, correctly.

### What we gain, and what we deliberately do not

**Gained:** one home for the ticket-resolution policy; a verb surface that costs ~3 lines per new
verb instead of ~10; two files that agree about how `Entity` is passed; a stated convention RAD-90
can follow instead of inventing a third.

**Not gained — and this matters:** no performance win (the same checks run, in the same order), no
new capability, no bug fixed. Nothing in Reaper behaves differently after this story. This is
pure structural interest paid *before* taking out the RAD-90 loan, which is the only time paying it
is cheap: 5 call sites to touch now, ~18 later.

---

## 1. Architecture Decision

**The shape.** Two file-local functions in `PhysicsWorld2D.cpp`'s existing anonymous namespace —
alongside `RigidBody2DTypeToBox2D` and `ResolveShapeEntity`, which already establish that pattern in
this exact file:

```cpp
b2BodyId  ResolveBodyId (Entity entity, uint64_t& packedId, const char* verb);
b2ShapeId ResolveShapeId(Entity entity, uint64_t& packedId, const char* verb);
```

Each returns a **live id, or the null id**. On a stale id it warns (naming `verb`, preserving the
existing per-caller wording), zeroes `packedId`, and returns null. On a zero id it returns null
silently. Callers become:

```cpp
b2BodyId body = ResolveBodyId(entity, rb2d.RuntimeBodyId, "Teleport");
if (B2_IS_NULL(body))
    return;
```

**Naming (settled during implementation, 2026-08-01).** The plan first called these
`Resolve*Ticket`, importing "ticket" from `Docs/Physics.md`'s *claim check* metaphor. Rejected on
review: the field is `RuntimeBodyId`, the vendor type is `b2BodyId`, the loader is `b2LoadBodyId` —
the surrounding code says **id**, and CLAUDE.md's naming rule puts consistency with it above taste.
`Handle` was rejected too: playbook §2 says "generation handles", but adopting it here would make a
third word for one concept. Same pass corrected D4 — `EntityFromShape` was not verb-first, so the
pure shape→entity query is `ResolveEntityFromShape`, unambiguous against `ResolveShapeId` without
leaving the convention.

Three design points, each with its rejected alternative:

**Free functions, not private members.** The story's technical note suggests a private method. These
touch **zero** member state — no `m_WorldId`, no buffers — so making them members would put two
declarations in a public header, force every dependent TU to recompile when their signature moves,
and imply access to state they don't use. File-local is the honest scope, and it matches the two
helpers already sitting there. (Flagged as a deliberate deviation from the story text — D1.)

**`Entity`, not `UUID`, as the first parameter — and this is not cosmetic.** The obvious "cleaner"
signature takes `UUID entityId` so the helper never touches the ECS. But `RADIANT_WARN` **survives
Dist** (`Core/Log.h:57`), so its arguments are evaluated in every config; passing the UUID in means
every call site evaluates `entity.GetUUID()` — a `GetComponent<MetadataComponent>()` sparse-set
lookup — on the **hot** path, to build a string that is thrown away 99.999% of the time. Passing the
`Entity` keeps that lookup inside the cold `if (!IsValid)` branch. With RAD-90's `ApplyForce` running
per-entity-per-fixed-step, that is the difference between a free guard and a per-call ECS lookup.

**Returning a value, not taking a callback.** UE's equivalent is `FPhysicsCommand::ExecuteWrite(handle,
lambda)` — resolve, validate, invoke. Rejected here (see §2): UE passes a lambda because it needs a
*scope* — it holds a scene write-lock across the body — and only a callback can bracket that. We need
a *value*. Returning the id keeps every verb body flat and readable and costs no indirection.

**The line this preserves.** `PhysicsWorld2D` translates and forwards; it never holds **policy**
(what a collision means, which entity wins). Policy lives in `Level`. Ticket validity is not gameplay
policy — it is translation hygiene — so it belongs here. That boundary is what keeps the class's
method count from being the real problem (playbook §4).

**Playbook sections in force:** §4 (physics integration — guard semantics, never rebuild shapes),
§2 (generation handles), §3 (components stay plain data — no verbs migrate onto components), §8.5
(no side effects in log/assert arguments — the direct source of the `Entity`-not-`UUID` decision).

---

## 2. UE Reference

### UE repeats the guard too — but it costs one line, not eight

Every `FBodyInstance` dynamics verb opens with the same predicate
(`Engine/Source/Runtime/Engine/Private/PhysicsEngine/BodyInstance.cpp:3801`):

```cpp
void FBodyInstance::AddImpulse(const FVector& Impulse, bool bVelChange, ...)
{
	const bool bIsInternal = TimeStamp.IsValid();
	ApplyAsyncPhysicsCommand(TimeStamp, bIsInternal, PlayerController, [&, Impulse, bVelChange, bIsInternal]()
	{
		if (FPhysicsInterface::IsInScene(GetPhysicsActor()) && IsBodyDynamic(GetPhysicsActor(), bIsInternal))
		{
			...
			FPhysicsInterface::AddImpulse_AssumesLocked(GetPhysicsActor(), Impulse, bIsInternal);
		}
	});
}
```

`AddForce`, `AddForceAtPosition`, `AddImpulseAtPosition`, `AddVelocityChangeImpulseAtLocation` — all
identical (lines 3695, 3707, 3820, 3832). **UE copy-pastes its guard across every verb.** The lesson
is not "don't repeat"; it is *what* got factored: resolution is an accessor (`GetPhysicsActor()`) and
validity is a named predicate (`IsInScene` / `IsBodyDynamic`), so the repeated part is **one legible
line**. That is exactly our target — we are not eliminating the per-verb guard, we are shrinking it
from eight lines to two.

### The callback-shaped alternative, and why we decline it

`Engine/Source/Runtime/Engine/Private/PhysicsEngine/Experimental/PhysInterface_Chaos.cpp:589`:

```cpp
bool FPhysInterface_Chaos::ExecuteWrite(const FPhysicsActorHandle& InActorReference, TFunctionRef<void(const FPhysicsActorHandle& Actor)> InCallable)
{
	if (InActorReference)
	{
		FScopedSceneLock_Chaos SceneLock(InActorReference, EPhysicsInterfaceScopedLockType::Write);
		InCallable(InActorReference);
		return true;
	}

	return false;
}
```

Read it as "validate, then run the caller's work inside a scope". The `FScopedSceneLock_Chaos` is the
reason for the lambda: UE's physics state lives on another thread, so a write must be bracketed by a
lock that outlives the work. Radiant is main-thread-only with no lock to hold, so a callback would
buy an indirection and a nesting level and nothing else. **Adopted:** validate-once-centrally.
**Deliberately simplified:** value return instead of a callback, because we have no scope to bracket.

### UE is deprecating our exact `Entity&` mistake

`Engine/Source/Runtime/Engine/Public/Physics/Experimental/PhysInterface_Chaos.h:80`:

```cpp
static ENGINE_API bool ExecuteWrite(const FPhysicsActorHandle& InActorReference, TFunctionRef<void(const FPhysicsActorHandle& Actor)> InCallable);
// TODO_CHAOSAPI: Deprecate non-const ref API
static ENGINE_API bool ExecuteWrite(FPhysicsActorHandle& InActorReference, TFunctionRef<void(FPhysicsActorHandle& Actor)> InCallable);
```

A non-const reference to a handle, kept for compatibility, with a standing TODO to remove it. Same
handle type, same mistake, same fix — they just can't make it without breaking licensees. We can,
today, for free.

---

## 3. File Plan

```text
Radiant/Source/Radiant/Physics/
├── PhysicsWorld2D.h    (modify) — Entity& → Entity on 6 methods; verb-surface convention in the class doc
└── PhysicsWorld2D.cpp  (modify) — add 2 anon-namespace resolvers; collapse 5 preambles; signatures

Radiant/Source/Radiant/ECS/
└── Level.cpp           (modify) — 4 signal handlers + 2 verb call sites (recompile-only if lvalues already)

Docs/
└── Physics.md          (modify) — ticket-resolution policy + verb-surface convention; Status line

.claude/references/
└── radiant-playbook.md (modify) — §4 bullet: one resolver per id kind; Entity by value
```

| Action | Path | Description |
|--------|------|-------------|
| Modify | `Radiant/Source/Radiant/Physics/PhysicsWorld2D.h` | `Entity&` → `Entity` on all six public methods; class doc gains the stated verb-surface convention. No include change — a by-value parameter of an incomplete type is legal in a *declaration*. |
| Modify | `Radiant/Source/Radiant/Physics/PhysicsWorld2D.cpp` | Two resolvers added to the existing anonymous namespace; five preambles collapse to two lines each; signatures follow the header. Net ~-25 lines. |
| Modify | `Radiant/Source/Radiant/ECS/Level.cpp` | Four entt signal handlers and the `Teleport`/`RefreshCollider` call sites already pass lvalues, so bodies are unchanged — this is a recompile, verified not assumed. |
| Modify | `Docs/Physics.md` | Doc contract: the resolution policy and the parameter convention are now architecture, not incident. |
| Modify | `.claude/references/radiant-playbook.md` | §4 gains the rule so RAD-90 and RAD-76 inherit it. |

---

## 4. Type Design

No new types. Two file-local functions and one stated convention.

### ResolveBodyId / ResolveShapeId
- **Kind:** free functions in `PhysicsWorld2D.cpp`'s anonymous namespace (internal linkage)
- **Responsibility:** turn a packed id into a live Box2D id, applying the one stale-recovery policy
- **Ownership:** own nothing; mutate only the caller's id field, and only to zero it
- **Lifetime & threading:** main-thread only, called only from `PhysicsWorld2D` verbs
- **Key Members:**
  - `Entity entity` — used **only** inside the warn branch, for `GetUUID()`; keeps the ECS lookup off the hot path
  - `uint64_t& packedId` — read, and zeroed on the stale path; component-agnostic by design (any future component storing a body id works unchanged)
  - `const char* verb` — a string literal, never allocated; preserves RAD-28's per-verb audit wording
  - **returns** a live id, or `b2_nullBodyId`/`b2_nullShapeId` — callers test `B2_IS_NULL` (a bare `index1 == 0` compare, `box2d/id.h:84`), never a second `b2*_IsValid` world lookup
- **Playbook Patterns:** §2 (generation handles), §4 (guard semantics), §8.5 (no side effects in log arguments)

### The verb-surface convention (documented, not coded)

`PhysicsWorld2D`'s methods currently split two ways with no stated rule, which is precisely how a
third convention gets invented in RAD-90. The rule to write down:

> **A verb takes a component reference only when its caller necessarily already holds one.** entt
> signal handlers are handed the component by entt; `Level::RefreshCollider` fetches the collider for
> its own precondition assert. Every other verb takes `Entity` plus its arguments and resolves the
> component itself — a gameplay call site must not need to know which component stores the ticket.

Under that rule the current surface is already correct, and RAD-90's eight verbs all take
`(Entity, args…)`.

---

## 5. Implementation Steps

### Phase 1 — The front desk
- [x] **Add the two resolvers** to the anonymous namespace in `PhysicsWorld2D.cpp`, next to
      `ResolveShapeEntity`. Doc comment states the contract in one sentence: *returns a live id, or
      null after warning and clearing a stale ticket; a zero ticket is silent.* Keep the `entity`
      parameter unused outside the warn branch — that placement is the perf decision, so it earns a
      one-line `//` comment.
- [x] **Collapse `DestroyBody` and `Teleport`.** `DestroyBody` keeps its early `ticket == 0` return
      (a body that never existed orphans no shape) and its trailing collider-ticket clear, which now
      runs unconditionally after the destroy attempt — correct on both paths, one line shorter.
- [x] **Collapse `DestroyBoxShape` and `UpdateBoxShape`.** `UpdateBoxShape` **keeps its own
      `ticket == 0` WARN** before calling the resolver: a *refresh* of a non-existent shape is a
      caller state mistake, unlike a *destroy* of nothing. That difference is real and stays visible
      at the site that owns it — it does not become a policy flag on the shared helper.
- [x] **`CreateBoxShape` — route through the resolver (decision D2).** Today it silently returns on
      an invalid body id and leaves the stale ticket in place. Routing it through the resolver makes
      a stale body ticket warn and clear, like everywhere else. This is a **deliberate, tiny
      behaviour change**; the story's "no log-output change" AC is amended to name it.
- [x] **Rename `ResolveShapeEntity` → `ResolveEntityFromShape` (decision D4).** File-local,
      mechanical. Two `ResolveShape*` functions meaning opposite directions (packed id → shape vs.
      shape → entity) is a readability trap worth closing while we are in the file. The first
      attempt, `EntityFromShape`, was dropped for breaking CLAUDE.md's verb-first rule.

### Phase 2 — The signature
- [x] **`Entity&` → `Entity`** on all six public methods (header + definitions), and align the doc
      comments. Purely mechanical.
- [x] **Verify `Level.cpp` needs no edit**, don't assume it — the four signal handlers and both verb
      call sites pass lvalues today, so the change is source-compatible. Confirm by compiling, and
      say so in the report.

### Phase 3 — The record
- [x] **State the verb-surface convention** in the `PhysicsWorld2D` class doc comment (§4 above), so
      RAD-90 inherits a rule instead of a precedent it has to reverse-engineer.
- [x] **`Docs/Physics.md`:** a short paragraph under *Body & shape lifecycle* on the single
      resolution policy and the parameter convention; update the Status line to note RAD-91.
- [x] **Playbook §4:** one bullet — *ticket resolution lives in one helper per id kind; verbs take
      `Entity` by value; a component reference is a parameter only when the caller already holds one.*
- [x] **Verify:** CLI MSBuild Debug/Release/Dist clean, then the Reaper smoke run in §11.

---

## 6. Ownership & Lifetime Strategy

Nothing changes about ownership; this story exists partly to stop *implying* an ownership change that
never happens.

- `Entity` remains a 16-byte value handle owning nothing (`Entity.h:12`). By value, callers may now
  legitimately pass a temporary — `world.Teleport(level.GetEntityByUUID(id), …)` — which is the whole
  point of a value handle.
- The resolvers own nothing and hold nothing across calls. Their only write is zeroing a stale ticket,
  which is the existing recovery, relocated.
- Ticket semantics are unchanged: a ticket is a claim check, not ownership; the world owns bodies and
  shapes and zeroes component fields on destroy.
- One hazard worth naming for RAD-90: **`Level::Copy` (Phase 5) must zero copied tickets**, because a
  v3 id embeds its world index — a shallow-copied ticket addresses the *original* world's body and
  passes `b2Body_IsValid`. Centralizing resolution does not fix that and must not be mistaken for
  fixing it; it only means there is now one obvious place to add a world-identity check if we ever
  want one.

---

## 7. Performance Notes

**No hot-path impact by construction, and one hot-path regression deliberately avoided.**

- Same checks, same order, same count. `B2_IS_NULL` at the call site is an `index1 == 0` integer
  compare (`box2d/id.h:84`) — strictly cheaper than the `b2Body_IsValid` world lookup it replaces at
  that position, since the resolver already did the real check.
- The resolvers are static, single-caller-shaped, and trivially inlinable within the TU; expect the
  optimizer to erase the call entirely in Release/Dist.
- **The avoided regression:** passing `UUID` instead of `Entity` would hoist a
  `GetComponent<MetadataComponent>()` sparse-set lookup out of the cold branch onto every verb call,
  in every config, because `RADIANT_WARN` survives Dist (`Core/Log.h:57`). Harmless at 5 verbs called
  rarely; a real per-fixed-step cost at 13 verbs with RAD-90's forces. Playbook §8.5 in its
  performance guise.
- `Entity` by value copies 16 bytes (two registers) versus passing an 8-byte pointer — arithmetically
  worse, practically identical after inlining, and it removes a pointer indirection on every member
  access inside the callee. Not a reason to choose either way; noted so nobody re-litigates it.
- No allocations. `const char* verb` is always a string literal.

---

## 8. Logging & Diagnostics

- **Preserved exactly:** `RADIANT_WARN("PhysicsWorld2D: {0} called with a stale body id for entity
  {1}", verb, entity.GetUUID())` reproduces today's four messages verbatim once `verb` is the
  caller's name. The per-verb wording is RAD-28's audit trail and does not regress to a generic
  message.
- **Preserved exactly:** `UpdateBoxShape`'s distinct zero-ticket WARN ("no live shape - refresh
  skipped"), and every `RADIANT_TRACE` (teleport, collider refresh).
- **The one intended addition (D2):** `CreateBoxShape` gains the stale-body-ticket WARN it currently
  lacks. Not spam — a stale body ticket at shape-creation time means a lifecycle path outside the
  entt signals touched the body, the same programmer error the other four verbs already report.
- **Unchanged:** guard-vs-assert split. Zero/stale tickets are *survivable content and lifecycle
  states* → warn and recover. Programmer errors (a collider on a body-less entity, a null world in a
  signal handler) stay `RADIANT_ASSERT` + Dist recovery.

---

## 9. Scalability Review

**This story is itself the scalability fix**, so the review is about what it does *not* solve.

| Pattern | Verdict |
|---------|---------|
| One method per verb on `PhysicsWorld2D` | **Keep.** Verbs are a genuinely open set with distinct signatures; a data-driven "verb table" would buy nothing and cost type safety. After this story a new verb is ~3 lines. The class is 327 lines with one responsibility — growth along a single legitimate axis is not sprawl. |
| Two near-identical resolvers (body, shape) | **Keep two.** A template over `{load, isValid}` unifies ~6 lines and costs readability at the rule-of-**two**. Revisit only if a third id kind appears (`b2JointId`, RAD-76's query handles) — that is the rule-of-three moment, not this one. |
| `const char* verb` passed by hand | **Accept.** The alternative — `RADIANT_FUNCTION`/`__func__` — silently rewrites the log text on every rename and yields decorated names in MSVC. An explicit literal is a deliberate, greppable audit string. |
| Getters in RAD-90 (`GetLinearVelocity`) need a resolver on a read path | **Flagged for RAD-90.** They must stay non-`const` on the world, or resolution cannot clear a stale ticket. A `const` overload that warns without clearing would leave the stale ticket to warn again on every subsequent call — log spam plus a permanently broken field. Decide it there, deliberately. |
| `Level::Copy` and cross-world tickets | **Out of scope, tracked.** Documented in §6 and already noted in `Docs/Physics.md`; belongs to the Phase 5 Copy story. |

---

## 10. Risks & Edge Cases

- **Silent semantic drift during the collapse.** The five preambles are *not* identical today — three
  distinct zero-ticket policies exist (silent in `DestroyBody`/`DestroyBoxShape`, silent-and-unclearing
  in `CreateBoxShape`, warning in `UpdateBoxShape`). A careless "unify" flattens them and changes
  behaviour. *Mitigation:* the step list treats each verb individually and names the intended
  deviation (D2) explicitly; §11's line-for-line log comparison is the proof.
- **`DestroyBody`'s collider-ticket clear.** It must still run on the stale path. Moving it after the
  resolver so it runs unconditionally is safe *because* the zero-ticket early return still precedes
  it — a never-created body cannot have orphaned a shape. Worth a comment at the site.
- **Incomplete-type by value.** `PhysicsWorld2D.h` only forward-declares `Entity`. By-value parameters
  are legal in a declaration and complete at every definition and call site (all of which include
  `Level.h`). If a future consumer includes `PhysicsWorld2D.h` alone and calls a verb, it will need
  `Entity.h` — correct and unavoidable, not a regression.
- **Merge order.** This lands on top of committed RAD-29 (`4f15710`), on `dev`. RAD-90 starts after.
  Doing it in the other order means refactoring ~18 call sites instead of ~6.
- **Scope discipline.** The card is two mechanical fixes. The verb-surface convention (§4) is
  *documentation*, deliberately kept as documentation — no verb signatures are normalized "while
  we're in here". Anything else surfaced during implementation gets a card, not a diff.

---

## 11. Verification (AC → proof)

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| The ticket-resolution preamble exists once per id kind (body, shape), not once per verb | Read the diff: exactly two resolver definitions; no verb contains `b2LoadBodyId`/`b2LoadShapeId` followed by a validity check. `grep -c "b2Load" PhysicsWorld2D.cpp` returns 2. |
| Per-verb WARN wording is preserved — a stale-ticket log still names which verb was called | Diff the two format strings against the four originals; run the stale-ticket repro below and read the log line. |
| Guard semantics unchanged: zero ticket = silent no-op; stale ticket = warn, clear, recover | Per-verb walk in review, plus the Reaper log comparison. `UpdateBoxShape`'s zero-ticket WARN survives as its own line. |
| Public methods take `Entity` by value; `PhysicsWorld2D` and `Level` agree on the convention | Header inspection: zero `Entity&` parameters remain in `Physics/`. Compiles with no `Level.cpp` body edits. |
| No behaviour or log-output change | **Reaper smoke run, before and after, diffed line for line:** launch, Play, spawn, let bodies settle (fall-asleep TRACEs), collide, `T` teleport, `G` collider refresh, exit cleanly. Logs must match — **except** the one intended `CreateBoxShape` stale-body WARN (D2), which cannot fire in a healthy run and so should not appear in either log. |
| Stale-ticket path still recovers rather than crashing | Remove a `BoxCollider2DComponent` at runtime, then call `RefreshCollider` on that entity: one WARN, no crash, simulation continues. |
| Compiles clean in Debug/Release/Dist, no new warnings | CLI MSBuild all three configs (CLAUDE.md *Build & Run*). Dist is the one that matters here: `RADIANT_TRACE` compiles out, so any resolver argument that only existed to feed a TRACE becomes an unused-variable warning. |
| Reaper runs and is visually verified | Interactive run by Kareem: bodies fall, stack, sleep, collide, teleport, refresh — identical to the pre-refactor build. |
| Docs and playbook updated | `Docs/Physics.md` Status line names RAD-91 and documents the resolution policy + parameter convention; playbook §4 carries the rule. `/review` treats a stale doc as an ERROR. |

---

## Locked Decisions (2026-08-01)

All four locked as recommended during the plan walkthrough.

| # | Decision | Locked |
|---|----------|--------|
| **D1** | Resolvers as anon-namespace free functions vs. the story's "private resolver" | **Free functions.** They touch no member state; keeps the header and its dependents untouched, and matches the two helpers already in that namespace. |
| **D2** | Route `CreateBoxShape` through the resolver, gaining a stale-body WARN it lacks today | **Yes.** A stale ticket there is the same programmer error; leaving one verb silently un-policed is how the next drift starts. AC amended to name the deviation. |
| **D3** | State the verb-surface convention (`Component&` only when the caller already holds one) in the class doc | **Yes, documentation only.** Zero code change; RAD-90 inherits a rule instead of guessing. |
| **D4** | Rename `ResolveShapeEntity` to avoid collision with the new body/shape resolver | **Yes → `ResolveEntityFromShape`.** Two `ResolveShape*` functions meaning opposite directions is a readability trap; the rename is file-local and free. `EntityFromShape` was the first choice and was dropped during implementation for breaking CLAUDE.md's verb-first rule. |