# Implementation Plan — RAD-101: Behaviour composition: more than one behaviour per entity

| Field | Value |
|-------|-------|
| **Jira** | [RAD-101](https://hndredgames.atlassian.net/browse/RAD-101) |
| **Epic** | RAD-98 — Gameplay Framework: engine base classes for game code |
| **Story status** | To Do |
| **Dependencies** | RAD-95, RAD-99, RAD-100, **RAD-97** — all In Review on `dev`. The RAD-97 block is **cleared**; see §1 D7 |
| **Absorbs** | RAD-30's `NativeScriptComponent` scope — agreed 2026-08-26, see §1 D1 |
| **Planned** | 2026-08-25 (drafted, paused at §1) → **2026-08-27 (revised against the landed RAD-97 code, walked through, and approved)** |
| **Status of this plan** | **APPROVED.** Unblocked — RAD-97 landed. D6 amended and D9/D10 added by what RAD-97 shipped; D11/D12 added during the walkthrough, the latter with a widened API surface (`HasBehaviour`, `RemoveBehaviour`, `GetBehaviours`). `EntityBehaviour` keeps its name this card — see session note 5 |

> **Session note — decisions taken 2026-08-27 during the walkthrough, affecting work AFTER this card.** Recorded here because they were reached while planning RAD-101 and would otherwise be lost with the session.
>
> 1. **Project goal reaffirmed:** *build a shippable 2D engine with principal-level architecture* — not *reimplement UE's object model*. The alternative was considered explicitly and declined.
> 2. **entt stays.** Dropping the ECS for a `UObject`/GC substrate was raised and rejected: it is a greenfield rewrite of the core (locked decision #1), it inverts playbook §2's no-weak-references reasoning, and UE itself ships an archetype ECS — `Engine/Source/Runtime/MassEntity` plus the `MassEntity`/`MassGameplay` plugins — precisely because `UObject`s do not scale for many simple things. Adopting the model UE routes around for anything hot, then rebuilding an ECS inside it, is a round trip.
> 3. **A reflection MVP is pulled forward**, to be filed as its own card and slotted **after RAD-101 and before RAD-102**. Scope: field enumeration for *game-authored types only* — no build tool, no `.generated.h`, no `UClass` equivalent, and **no change to `Components.h` or the ECS**. It collapses the paired-type shape into one self-describing class and yields the field visitor that later drives the Phase 5 inspector. Full RAD-72 stays in Phase 4. This is an explicit re-open of the locked phase order, made because RAD-98's Gameplay Framework is under construction *now* and every card written before the collapse would be rewritten after it.
> 4. **The four "reflection approaches" are alternatives, not phases** — one field table, four ways to populate it. The incremental path is pointer-to-member `Reflect()` → a `RADIANT_PROPERTY` macro over the same table → a generator only on demand → C++26 if MSVC ever ships it.
> 5. **The `Component` rename: DEFERRED to the reflection MVP card, deliberately.** `EntityBehaviour` keeps its name through RAD-101. This is not a punt — the rename question is *"does `Component` collide with the 9 ECS row types?"*, and that depends on whether behaviours and components **merge**. The reflection MVP is the card that merges them, so it is the first point at which the name has an answer rather than a guess. Deciding it here would mean renaming on a prediction and possibly renaming twice. The cost of waiting is one extra pass over 5 attach sites; the cost of guessing wrong is 598 call sites. Carry the numbers forward to that card.

---

## 0. The Problem, Ground Up

### What the code does today

An entity can run exactly one piece of C++ gameplay logic. Not by policy — by the shape of the storage:

```cpp
struct NativeScriptComponent
{
    EntityBehaviour* Instance;                        // ONE. Singular.
    std::function<EntityBehaviour*()> InstantiateScript;
    std::function<void(NativeScriptComponent*)> DestroyScript;
};
```

One pointer, one behaviour. Attaching a second overwrites the first (`AddOrReplaceComponent`) or asserts (`AddComponent`).

### The failure, concretely

Say the Reaper needs three things: it takes damage, it moves, and it can be interacted with. Three obviously separate concerns, three obviously reusable pieces. With one behaviour per entity your options are:

1. **One class that does all three** — `ReaperCharacter : EntityBehaviour` with health, movement and interaction fields jammed together. Then the crate also needs health, so you either copy the health code or make `Crate` inherit from something that has it. Six entity types later you have a five-deep inheritance chain where `Crate` carries movement code it never runs, and changing `TakeDamage` risks four classes you weren't thinking about. That is the **fragile base class** problem — the thing UE spent two decades escaping *with* components.
2. **Three entities** glued together — now every one of them needs to find the other two, every frame, and destroying "the Reaper" means destroying three things in the right order.
3. **Three plain-data components plus three loops in `Level`** — which works, but every new gameplay concern now costs an engine edit. That is the O(N)-edits-per-feature shape RAD-94 exists to forbid.

None of these is a good answer, and all three are what the current storage forces.

### The fix, as an everyday image

Think of an entity as a **person**, and behaviours as **jobs they hold**. Today the engine gives each person exactly one job title, so anyone who needs to be both a driver and a first-aider needs a new job title called "driver-and-first-aider" — and then "driver-and-first-aider-and-electrician", and so on, one new title per combination. The number of titles explodes because you are naming *combinations* instead of *capabilities*.

What you want is a person holding a **small stack of job cards**. Add a card, remove a card; the person is the same person. Two people can share the "first-aider" card without either inheriting anything from the other.

That is all this card does: turn the one pointer into a small list, and give the engine a defined way to add to it, walk it, and tear it down.

### How Unreal solves it

This *is* Unreal's model, and the leverage everyone attributes to `AActor` actually lives here. An actor holds a set of components (`Actor.h:4323`), each with its own tick and its own lifecycle. `ACharacter` is not a class that implements movement — it is a capsule component plus a mesh component plus a `UCharacterMovementComponent`, **assembled** rather than inherited. §2 has the source.

### What we gain, and what we deliberately do not

**Gain:** reusable gameplay pieces. A `Health` behaviour written once, attached to the Reaper, the crate, and a destructible wall, with no inheritance between them.

**Do not gain, and must not be claimed:**

- **No parallelism.** Behaviours still update one at a time on the main thread in the fixed step. UE's component tick is a parallel task graph; ours is a `for` loop, deliberately (§7).
- **No data-driven attachment.** Bindings remain code-only and are still not serialized — you re-attach after level load, exactly as today. Level-authored behaviours are a Phase 5 editor concern.
- **No dependency ordering between behaviours.** Update order is *defined* (insertion), which is not the same as *declarable*. UE lets you state "tick me after that one"; we will not (§1 D4).
- **No credit for fixing the self-destruct wart** — RAD-97 already did that, and this card inherits it (§1 D7). What remains here is narrower and must not be oversold: the failure mode is no longer *touching freed memory*, it is *running a sibling's `OnUpdate` after the entity's `OnDestroy` has already fired*. An ordering violation, not a use-after-free.

### What we have / what we're building

**What we have:** one C++ behaviour per entity, so every reusable piece of gameplay has to enter through a base class, and composition is only spellable as inheritance.

**What we're building:** a small owned list of behaviours per entity, with a defined update order, a defined teardown order, and an attach API that never makes game code name the component the list lives behind.

---

## 1. Architecture Decision

Twelve decisions carry this card. D1 and D7 are sequencing calls, both now settled. D9–D12 were added or reshaped during the 2026-08-26/27 walkthrough — three of them by what RAD-97 actually shipped, and one (D12) by a deliberate widening of the API surface. The rest are design calls with their rejected alternatives recorded.

### D1 — Merge RAD-30's script scope into this card. **AGREED 2026-08-26.**

RAD-101's AC requires this question be answered before implementation, so here it is. RAD-30's *remaining* scope, after its 2026-07-09 realignment, is exactly two items: `NativeScriptComponent` (owning pointer + `std::function`s → Level-owned side table) and the `Entity::Name()` mutable-static fix.

The first item **is this card**. A side table holding one instance versus a list of instances is the same change with a different value type, and the one-instance intermediate has no customer — nothing would ever ship on it. Building them separately means designing the storage twice and migrating Reaper twice.

**Recommendation:** RAD-101 does the storage work and closes RAD-30's script half; RAD-30 keeps only the unrelated `Entity::Name()` fix and gets an amendment saying so. The alternative — RAD-30 first, RAD-101 second — costs a throwaway design pass for a state nobody runs.

### D2 — The model: N behaviours of distinct types per entity. **UE's model.**

*Rejected:* one behaviour owning composed sub-objects it manages itself. It keeps a single tick site and is less engine work — but it moves the composition problem into game code, gives the sub-objects no engine lifecycle (`OnCreate`/`OnDestroy`/collision hooks), and means the engine cannot find them (RAD-100's query would stop at the outer behaviour). It is the same "one class does all three" answer §0 rejected, wearing a wrapper.

### D3 — Storage: a Level-owned side table, with a plain-data tag component

Instances move out of the ECS entirely (playbook §3, and RAD-30's rule: *if it can't be memcpy'd and serialized, it doesn't belong in a component*). The component that remains is a **tag** — no members, no pointers, memcpy-able:

```cpp
struct BehaviourComponent {};   // presence means "this entity has behaviours"
```

**Why keep a component at all**, when the side table's key set already answers "who has behaviours"? Because a real customer needs it in a `view`: `Level::OnFixedUpdate`'s snapshot pass classifies movers with `GetLiveEntitiesWith<NativeScriptComponent, TransformComponent>` (`Level.cpp:675`) and prunes stale snapshots with `any_of<RigidBody2DComponent, CameraComponent, NativeScriptComponent>` (`Level.cpp:686`). Delete the component and that classification loses a third of its inputs. The tag is maintained only by the attach/detach path — **one writer** — so it cannot desync from the table.

*Rejected:* a claim-ticket component holding an index into a Level vector, mirroring `RigidBody2DComponent::RuntimeBodyId`. That pattern earns its keep for Box2D because the id is *the vendor's own handle* and we are storing something we did not allocate. Here we own both sides, so a ticket buys an indirection and a desync hazard for nothing. *Also rejected:* putting the list in `entt` storage — that reintroduces exactly the non-plain-data resident RAD-30 exists to remove.

### D4 — Update order: insertion order, per entity, documented

Cheapest defensible answer, and the one a reader can predict without looking anything up.

*Rejected:* alphabetical by type name (predictable but arbitrary — it encodes nothing about intent); explicit priority values (no customer today, and priority numbers are the thing you add *after* insertion order provably fails — RAD-94's adopt-on-need).

*Deliberately not adopted from UE:* declarable dependencies. `AddTickPrerequisiteComponent` (`ActorComponent.h:1342`) lets a component say "tick me after that one", and UE needs it because it has thousands of ticking components across a parallel task graph where insertion order means nothing. Note what this implies about UE's storage: `OwnedComponents` is a **`TSet`** (`Actor.h:4323`) — a hash set, which does not carry order at all. UE decouples "the set of components" from "the order they tick" *by construction*. Ours are the same list, which is simpler, adequate at our scale, and honest about what it guarantees.

### D5 — Duplicate types: **reject at attach.** This is where we deliberately diverge from UE.

UE *allows* duplicates — two `UStaticMeshComponent`s on one actor is normal and meaningful, and `FindComponentByClass` returns the first match (`Actor.cpp:3997-4004`). That is correct **for UE**, because its components are *things*: a mesh, a light, a collision volume. Two of a thing is a legitimate design.

Our behaviours are *logic units*, not things. Two `HealthBehaviour`s on one entity is a bug in every scenario we can name. And rejecting duplicates buys something concrete: it makes RAD-100's `GetBehaviour<T>()` **total** — "*the* behaviour of type `T`" stays a true phrase rather than degrading to "whichever we hit first". RAD-100's contract was written in the singular *on the assumption of this decision*, and both cards say so.

**Policy:** `RADIANT_ASSERT` on a duplicate attach (code-only bindings make it a programmer error), plus a `RADIANT_WARN` and return of the **existing** instance so Dist recovers instead of running with two. Matches `AddComponent`'s assert precedent without UB in the shipping build.

*Reversibility check* (playbook §10): if a real multi-instance behaviour ever appears, widening is additive — `AddBehaviours`/`GetBehaviours` alongside the singular pair. Narrowing later would break every call site. Start narrow.

### D6 — Teardown order: reverse of insertion (LIFO) — **at both of the two teardown moments**

A behaviour attached later may have found an earlier one in `OnCreate`, so the dependent should die before its dependency. That is the same discipline as C++ destructors and stack unwinding, which every reader already has intuitions for.

Note we are being **stricter than UE** here, cheaply: `AActor::UninitializeComponents` (`Actor.cpp:6350-6362`) just walks `GetComponents()` and calls `UninitializeComponent()` in whatever order the set yields — UE does not define it. We can, because our list is ordered anyway.

**Amended 2026-08-26, by what RAD-97 shipped.** Teardown is no longer one moment, so "LIFO" has to be said twice, about two different loops in two different functions:

| Moment | Function | What it does to the list |
|---|---|---|
| **The mark** | `Level::DestroyEntity` (`Level.cpp:127`) | Runs every behaviour's `OnDestroy`, LIFO. Deletes nothing. |
| **The reap** | `Level::ReapDestroyedEntities` (`Level.cpp:218`) | Deletes every instance, LIFO, then erases the map entry. Runs no gameplay. |

Splitting them is not incidental — it is what makes the AC easy instead of hard. At the mark, every sibling is still fully alive, so behaviour #1's `OnDestroy` can legitimately read behaviour #3 (`GetOwner().GetBehaviour<Health>()`) and get a live object. Had we deleted as we went, LIFO would have meant "the ones you can still see are the ones attached before you", which is a contract nobody can hold. The `Level.cpp:261` bespoke-delete block in the reap — the one whose comment already names RAD-30's side tables as its successor — is the exact site that changes.

*Rejected:* deleting at the mark and only clearing the map entry at the reap. It saves one loop and reintroduces the whole hazard RAD-97 removed.

### D7 — **Sequencing: RAD-97 lands first. AGREED 2026-08-26 — and it has now LANDED (`c49bd4f`, `e9c2ff8`, `22e0a47`).**

**What this card inherits, and what it still owes.** The decision below is kept verbatim as the record of *why* we sequenced this way; this box records what the sequencing actually bought, because that changes three of the implementation steps.

Inherited free:

- `DestroyEntity` is a **mark**: it runs `OnDestroy`, tears physics down eagerly, erases the UUID entry, tags `PendingDestroyComponent`, and frees nothing.
- `ReapDestroyedEntities` frees at one defined point — the tail of `OnFixedUpdate`, after contact dispatch.
- `m_MarksInProgress` already stops `GetOwner().Destroy()` re-entering from inside an `OnDestroy`. With N behaviours that guard covers the whole list at once, not one instance — behaviour #2's `OnDestroy` calling `Destroy()` returns silently while #1 and #3 are still pending their own `OnDestroy`.
- `NotifyScript` (`Level.cpp:519`) already documents that a handler may destroy its own entity safely.

Still owed by this card, and easy to miss because the hazard changed shape rather than disappearing:

1. **The live check moves inside the behaviour loop.** Today `OnFixedUpdate` checks `!valid(handle) || IsPendingDestroy(handle)` once per *entity* (`Level.cpp:727`). With N behaviours, behaviour #2 can condemn the entity and #3 must not then receive `OnUpdate` — its `OnDestroy` already ran during #2's call. Playbook §8.8(a) in its per-behaviour form.
2. **Both teardown loops become loops** — see D6's table.
3. **The reap's one bespoke delete becomes the side table's** — its own comment already predicted this card.

The AC *"a behaviour destroying its own entity does not leave sibling behaviours executing on freed storage"* is the hard part of this card, and composition makes the existing wart materially worse.

Today, with one behaviour, the contract is a documented landmine: *"destroying THIS entity deletes the instance whose method is executing, so it must be the last statement."* With five behaviours it becomes: *"...must be the last statement, AND behaviours 3, 4 and 5 silently do not run this step, AND whether they ever ran depends on where you sat in a list you did not choose the order of."* That is not a contract anyone can hold in their head.

**RAD-97 dissolves it.** Once destruction marks and the reap happens at a defined point (end of fixed step, after contact dispatch), a behaviour destroying its own entity is just a flag write; the method finishes normally, siblings finish normally, and the whole list is torn down once at the reap. RAD-101's walk then needs only a per-behaviour validity check.

RAD-97 has **no dependencies**, is parented to Phase 2 (the current phase), and already exists to fix three symptoms of this same cause across RAD-29, RAD-95 and this card. Doing it first is not a detour — it is the prerequisite this card would otherwise have to reinvent.

*The alternative, costed honestly:* a behaviour-scoped deferral inside `Level::OnFixedUpdate` — keep instances alive in a pending-delete list until the walk ends. Roughly 30 lines, and it works. But it is a **second deferral mechanism** that RAD-97 must then unify, which is exactly the "three symptoms of one cause" duplication RAD-97 was filed to end. If we take it, it takes a card, not a comment.

**Recommendation:** link RAD-97 as *blocks* RAD-101 and do it first.

### D8 — Attach API: `Entity::AddBehaviour<T>()`, and eager construction

The last place game code names the component disappears:

```cpp
// today
entity.AddComponent<NativeScriptComponent>().Bind<CollisionLogger>();
// after
entity.AddBehaviour<CollisionLogger>();
```

This is the natural pair to RAD-100's `GetBehaviour<T>()` and belongs to the same **generic template family** (playbook §10, landed RAD-100): parameterised by type, so it costs `Entity.h` one declaration however many behaviour types exist.

It also lets the two `std::function`s die. `Bind<T>()` existed to defer construction to a factory the Level could call later; a templated `AddBehaviour<T>` constructs at the call site, so `InstantiateScript`/`DestroyScript` have no remaining job.

**Construction becomes eager; `OnCreate` stays deferred.** `AddBehaviour<T>()` returns a usable `T*` immediately so the caller can configure it, but `OnCreate` — which is gameplay and may spawn or destroy — still runs at the next script pass, at the one defined point. That preserves RAD-95's discipline: gameplay callbacks never fire from arbitrary call sites.

Consequence worth tracking: this **removes one of RAD-100's four null cases** ("instance not built yet"). `Entity.h`'s contract block must be updated in the same change, or it documents a state that can no longer occur.

**Surface revised 2026-08-27 — Kareem's call: complete the family now rather than widen on evidence.** The draft excluded `RemoveBehaviour<T>` and `HasBehaviour<T>` under RAD-94's customer gate. That gate is real, but it is doing less work here than usual and the exclusions were weaker than they looked:

- `HasBehaviour<T>()` — the draft's reason (*"it is `GetBehaviour<T>() != nullptr`"*) proves too much: the identical argument deletes `HasComponent<T>()`, which has shipped on `Entity` for years. Refusing the behaviour spelling while keeping the component spelling is an inconsistency a reader would have to memorise. **Admitted.**
- `RemoveBehaviour<T>()` — a composition system with no detach is half a system, and the honest reason it looked expensive is that the draft predates RAD-97. Detach rides machinery that now exists. **Admitted, with its semantics designed in D12 rather than assumed.**
- `GetBehaviours()` — untyped, every behaviour on the entity, UE's `GetComponents()`. **Admitted**; return type decided in D12.

Everything here stays in the **generic template family** (playbook §10): each costs `Entity.h` one declaration however many behaviour types exist, so completing the family does not restart the per-type growth RAD-94 forbids. That is why the customer gate yields cheaply in this specific case and must not be read as yielding generally.

*Still excluded — recommended and **agreed 2026-08-27**, so the next addition meets a decision rather than a precedent:*

- **`GetBehaviours<T>()` (typed plural)** — contradicts D5. With duplicates rejected it can only ever return zero or one element, so the plural in its name is a lie the compiler will not catch. Admitting it means reopening D5, and D5 is what makes RAD-100's `GetBehaviour<T>` total.
- **`AddBehaviours<A, B, C>()` (variadic attach)** — the coherence problem is the **return value**, not the sugar. D8's whole justification for eager construction is configure-on-attach (`AddBehaviour<Health>()->MaxHP = 50`). A variadic attach can only return a tuple or nothing; returning nothing means every caller follows it with per-type `GetBehaviour<T>()` calls, which is longer than the three plain attaches it replaced.

### D9 — `BehaviourComponent` is engine-private (`IsEngineComponent`). **New 2026-08-26 — RAD-97 built the gate this needs.**

D3's whole safety argument is *one writer*: the tag cannot desync from the side table because only `AddBehaviour` and teardown ever touch it. That argument is a comment unless something enforces it, and until last week nothing could — `entity.AddComponent<BehaviourComponent>()` would have compiled fine and produced an entity the snapshot pass treats as a mover and the walk finds no behaviours for.

RAD-97 shipped exactly the enforcement: `IsEngineComponent<T>` plus the `RADIANT_REJECT_ENGINE_COMPONENT` `static_assert` on the three mutating accessors (`EntityTemplates.h:10-33`). Specialising it for `BehaviourComponent` turns D3's invariant from a convention into a **compile error**, at the cost of one line.

```cpp
template<> struct IsEngineComponent<BehaviourComponent> : std::true_type {};
```

Note what this does *not* close: reads stay open, exactly as RAD-97 chose. `HasComponent<BehaviourComponent>()` is a fair question and answering it cannot corrupt anything.

*Rejected:* leaving it ungated for symmetry with the other gameplay components. Symmetry is not the property that matters here — `PendingDestroyComponent` and `TransformSnapshotComponent` are already gated for the identical reason, so gating this one is the consistent choice, not the exceptional one.

### D10 — Collision hooks fan out to every behaviour, in insertion order, re-checked between each. **New 2026-08-26.**

The original draft missed a site. `Level::NotifyScript` (`Level.cpp:519`) reaches `nsc->Instance` singular and calls `OnCollisionBegin`/`OnCollisionEnd` on it — so as drafted, a three-behaviour entity would have exactly one behaviour hearing about collisions, silently, with no diagnostic. Update *and* `OnCreate` are not the only per-instance dispatch sites; contacts are the third.

The loop is not just "walk the list", because RAD-29 already established that gameplay runs here with the power to destroy (playbook §4): *"re-resolution of each side immediately before its callback, because an earlier callback in the same batch may have destroyed it."* The per-behaviour form of that same rule is the whole content of this decision — behaviour #1's `OnCollisionBegin` may destroy this entity, and #2 and #3 must then not be called. It is the identical shape as D7's item 1 for `OnUpdate`, which is a good sign: one hazard, one spelling, three sites.

*Rejected:* notifying only the first behaviour and documenting it. That is a special case with no principle behind it, and the failure is silent — the worst combination.

### D11 — `OnCreate` is tracked by a phase flag on the behaviour, dispatched per entity inside the existing walk. **New 2026-08-27.**

D8 makes construction eager, which **removes the mechanism the engine currently uses**: today `if (!nsc->Instance)` is the "not created yet" test, so construction and `OnCreate` happen in one breath (`Level.cpp:739-752`). Once `AddBehaviour<T>()` builds the object at the call site, the pointer is never null and a brand-new behaviour is indistinguishable from one that has been running for an hour. A replacement signal is therefore mandatory, not optional.

**One `bool m_HasCreated` on `EntityBehaviour`**, beside `m_Entity` and under the same access story — `Level` is already a friend and already wires `m_Entity` directly (`EntityBehaviour.h:201-202`). Set after `OnCreate` returns.

This is UE's shape. `UActorComponent` carries a row of private one-bit phase flags (`ActorComponent.h:349-361`), each documented in the same form — *"Indicates that BeginPlay has been called, but EndPlay has not yet"* — and each **paired with its teardown hook**, which is why `UninitializeComponents` tests `HasBeenInitialized()` before calling. We inherit that pairing: a behaviour destroyed before it ever ran must not receive an `OnDestroy` for an `OnCreate` that never happened, so the mark's LIFO loop (D6) tests the flag too.

*Rejected — a pending-creation queue on the Level.* Tidier-looking, and a dangling pointer by construction: attach on step 12, destroy the entity on step 12, and the reap frees the instance at the end of step 12 while the queue still holds it for step 13's drain. Storing `{Entity, index}` and re-resolving fixes it and leaves you with a second list needing the same validity checks as the first, to avoid one bool. Playbook §4's *re-resolve, don't cache*, arrived at again.

*Rejected — partitioning each entity's vector by a "created count"*, relying on attaches always appending. Zero state, and it breaks silently the day anything removes a behaviour.

**Dispatch point:** per entity, inside the existing walk, before that entity's `OnUpdate`s.

```
for each entity in the walk:
    for each behaviour, insertion order:  if (!m_HasCreated) { OnCreate(); m_HasCreated = true; }
    for each behaviour, insertion order:  OnUpdate(ts)
```

Two consequences to implement deliberately rather than inherit by luck:

- **Bound each loop by the count captured before it starts** (`const size_t count = list.size();`, never `i < list.size()`). A behaviour attached *during* the walk waits for the next step — RAD-95's existing contract, which `SpawnProbe` already demonstrates in the log. Under a per-step snapshot that was automatic; under a maintained list it becomes a choice, and the accidental spelling changes the contract silently.
- **`OnCreate` is NOT globally ordered before `OnUpdate` across the level** — entity A's `OnCreate` follows entity B's `OnUpdate` if B sorts earlier. True today, unchanged, and documented rather than fixed: a level-wide creation pass is a stronger guarantee with no customer. The guarantee we *do* make is the within-entity one, which is where siblings can see each other and therefore where composition needs it.

### D12 — Detach is DEFERRED, on RAD-97's existing rhythm. **New 2026-08-27.**

`RemoveBehaviour<T>()` is cheap to *write* and expensive to get *wrong*, and the trap is the one RAD-97 just finished removing at entity scope. Three questions have to be answered, not assumed:

**(a) Does it run `OnDestroy`?** Yes — otherwise a detached behaviour never releases anything it acquired. Note this overloads the hook: `OnDestroy` now means *"you are going away"*, which is true whether the entity died or you alone were detached. UE splits these three ways (`OnComponentDestroyed`, `EndPlay`, `UninitializeComponent`); we deliberately do not, because one hook with one meaning is simpler than three with overlapping ones, and a behaviour that genuinely needs to distinguish the cases can ask `GetOwner().IsValid()`.

**(b) When is the instance freed?** **At the reap, never at the call.** This is the whole decision. An immediate `delete` reintroduces the exact wart RAD-97 dissolved, in a nastier form:

```
Health::OnUpdate()
  GetOwner().RemoveBehaviour<Health>()     // removing MYSELF
    delete this                            // ...the object currently executing
  ...every line after this is a use-after-free
```

Self-detach is not an exotic case — *"I am done, take me off"* is one of the two obvious reasons to call this at all. So detach **marks**: run `OnDestroy` now (siblings all still alive, exactly as at the entity mark), set a pending flag, and let the reap free it. The machinery already exists and runs at the right moment; this adds a second thing for it to collect, not a second mechanism. **That, and only that, is why this is now a small feature — the cost was paid by D7.**

**(c) What about the walk's indices?** D11 bounds each loop by a count captured before it starts, and the vector must therefore not be compacted mid-walk. Marked-but-not-reaped entries are **skipped**, not erased; the reap compacts. Same shape as `PendingDestroyComponent` at entity scope, for the same reason.

The flag row on `EntityBehaviour` becomes two, which is precisely UE's pairing (`ActorComponent.h:349-361`):

| Radiant | UE | Meaning |
|---|---|---|
| `m_HasCreated` | `bHasBegunPlay` | `OnCreate` has run, `OnDestroy` has not |
| `m_PendingRemove` | `bIsBeingDestroyed` | detach marked, instance not yet freed |

`m_HasCreated` also gates `OnDestroy`: a behaviour attached and detached in the same step, before the walk ever reached it, must not receive an `OnDestroy` for an `OnCreate` that never ran. UE tests exactly this before calling its teardown hook.

**`GetBehaviours()` return type:** a `const std::vector<Scope<EntityBehaviour>>&` hands back Level-owned storage and the ownership handles with it — refused on playbook §10's *never hand back what the type exists to withhold*. Returning `std::vector<EntityBehaviour*>` by value allocates on every call. **Chosen:** a lightweight non-owning span-like view over the entity's slice, valid until the next attach/detach — the same transience contract `GetBehaviour<T>`'s pointer already carries, stated in the same place. UE reaches for `TInlineComponentArray` (a small-buffer `TArray`) at every equivalent site; our `InlineArray<T,N>` stays gated behind RAD-67/RAD-83 (playbook §9), so a view is the allocation-free answer available today.

*Excluded:* `RemoveBehaviours()` (remove all) — its only real customer is entity teardown, which is engine-internal and already has a path. A public verb for it would be an API nobody calls.

---

## 2. UE Reference

### The storage — a set, not a list

`Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:4319-4323`:

```cpp
	/**
	 * All ActorComponents owned by this Actor. Stored as a Set as actors may have a large number of components
	 * @see GetComponents()
	 */
	TSet<TObjectPtr<UActorComponent>> OwnedComponents;
```

Two things to take from this. First, *"actors may have a large number of components"* — this is not a two-or-three-item structure in UE, which is why it is a hash set. Second, and more instructive: **a `TSet` does not carry insertion order**, so UE's storage cannot be the thing that decides tick order. It isn't — tick order is a separate registry (`FTickFunction`), and dependencies between them are declared explicitly (`ActorComponent.h:1342`, `AddTickPrerequisiteComponent`).

We adopt the *model* and not the *machinery*: an ordered `std::vector` is both our storage and our update order, because a handful of behaviours ticking serially does not need a task graph to schedule it.

### Reading a component back — the shape RAD-100 already anticipated

`Engine/Source/Runtime/Engine/Private/Actor.cpp:3991-4008`:

```cpp
UActorComponent* AActor::FindComponentByClass(const TSubclassOf<UActorComponent> ComponentClass) const
{
	UActorComponent* FoundComponent = nullptr;

	if (UClass* TargetClass = ComponentClass.Get())
	{
		for (UActorComponent* Component : OwnedComponents)
		{
			if (Component && Component->IsA(TargetClass))
			{
				FoundComponent = Component;
				break;
			}
		}
	}

	return FoundComponent;
}
```

`break` on first match — UE's answer to duplicates is "you get one of them, unspecified which, because the set is unordered." We reject duplicates instead (D5), which turns the same linear scan into a *total* query.

### Teardown — UE does not define the order

`Engine/Source/Runtime/Engine/Private/Actor.cpp:6350-6362`:

```cpp
void AActor::UninitializeComponents()
{
	TInlineComponentArray<UActorComponent*> Components;
	GetComponents(Components);

	for (UActorComponent* ActorComp : Components)
	{
		if (ActorComp->HasBeenInitialized())
		{
			ActorComp->UninitializeComponent();
		}
	}
}
```

Whatever order the set yields. We define LIFO (D6) because our list is ordered anyway and it costs nothing.

Also note `TInlineComponentArray` (`Actor.h:199-202`) — a `TArray` with a `TInlineAllocator`, i.e. a small-buffer array that avoids a heap allocation when the count is small. UE reaches for it *every time* it collects an actor's components, which is a strong signal about how hot this path is at scale. Playbook §9 lists `InlineArray<T,N>` as a gated learning container for exactly this shape; it stays gated (RAD-67/RAD-83 first) and is not built here.

---

## 3. File Plan

```text
Radiant/Source/Radiant/ECS/
├── Components.h        (modify) — NativeScriptComponent → BehaviourComponent tag (plain data)
├── Level.h             (modify) — the behaviour side table, attach/detach, ordering contract
├── Level.cpp           (modify) — maintained walk, teardown, RAD-30's instance ownership
├── Entity.h            (modify) — AddBehaviour<T>; GetBehaviour<T> contract loses a null case
├── EntityTemplates.h   (modify) — AddBehaviour<T> / GetBehaviour<T> over the list
└── LevelSerializer.cpp (modify) — the NativeScriptComponent entry follows the rename

Radiant/Source/Radiant/Gameplay/
└── EntityBehaviour.h   (modify) — composition, update order, teardown order in the contract

Reaper/Source/
├── Layers/GameLayer.cpp (modify) — 5 attach sites migrate to AddBehaviour<T>
└── (script headers)     (modify) — only if they name NativeScriptComponent (SpawnProbe.h does)

Docs/
├── Gameplay-Framework.md (modify) — composition section; Known Issues entry retired
└── ECS-And-Levels.md     (modify) — the side table is storage, so it is documented here

.claude/references/
└── radiant-playbook.md   (modify) — §3/§10: the side-table rule with its first real instance
```

| Action | Path | Description |
|--------|------|-------------|
| Modify | `ECS/Components.h` | `NativeScriptComponent` (owning ptr + 2 `std::function`s) → empty `BehaviourComponent` tag, **plus its `IsEngineComponent` specialisation** (D9). Closes RAD-30's plain-data violation. |
| Modify | `ECS/Level.h` / `.cpp` | Level-owned side table of `Scope<EntityBehaviour>` lists; attach; the maintained walk replacing the per-step rebuild; **LIFO `OnDestroy` at the mark and LIFO delete at the reap** (D6); **`NotifyScript` fans out** (D10). |
| Modify | `ECS/Entity.h` / `EntityTemplates.h` | `AddBehaviour<T>()`, `HasBehaviour<T>()`, `RemoveBehaviour<T>()`, `GetBehaviours()`; `GetBehaviour<T>()` becomes a scan of the entity's list; drop the "not built yet" null case and the rebind clause. |
| Modify | `Gameplay/EntityBehaviour.h` | **The two phase flags** `m_HasCreated` / `m_PendingRemove` (D11, D12), beside `m_Entity` under the existing `friend class Level`. Contract: several behaviours per entity, insertion-order update, LIFO teardown at both moments, duplicates rejected, and the "where does my state live?" default. |
| Modify | `ECS/LevelSerializer.cpp` | The commented-out `NativeScriptComponent` block follows the rename (still not serialized — code-only bindings). |
| Modify | `Reaper/Source/…` | 5 attach sites + `SpawnProbe.h`'s in-script attach. |
| Modify | `Docs/`, playbook | Doc contract; the side-table pattern gets its first written instance. |

---

## 4. Type Design

### `BehaviourComponent`
- **Kind:** empty tag struct (`ECS/Components.h`)
- **Responsibility:** mark that this entity has at least one behaviour, so `entt` views can find it.
- **Ownership:** none — it is a tag. Plain data, trivially copyable, survives `Level::Copy`.
- **Key members:** none, deliberately. A `Count` member would be state duplicated from the side table and able to desync.
- **Playbook:** §3 (plain data), and the `Components.h` add-a-component contract (declare + serializer entry).

### `Level`'s behaviour side table (Level-private)
- **Kind:** private member of `Level`, not a new public type.
- **Shape:** `std::unordered_map<entt::entity, std::vector<Scope<EntityBehaviour>>>`, plus the maintained flat walk list.
- **Responsibility:** own every behaviour instance in the level; answer "this entity's behaviours, in order".
- **Ownership:** **`Scope<EntityBehaviour>`** — unique ownership, one story per type (playbook §2), replacing today's raw owning pointer. The `Level` is the sole deleter.
- **Lifetime & threading:** created at `AddBehaviour`, destroyed at entity teardown or level destruction. Main-thread only.
- **Header note:** `Scope<EntityBehaviour>` in `Level.h` needs only the forward declaration that `Components.h` already carries; the *destructor* needs the complete type, so `Level.cpp` includes `Gameplay/EntityBehaviour.h`. Same trick `PhysicsWorld2D` uses, and it keeps `ECS/ → Gameplay/` out of the build graph.
- **Playbook:** §2 (ownership), §3 (runtime state lives in side tables), §8.8 (nothing iterates a live view while gameplay runs).

### `Entity::AddBehaviour<T>()`
- **Kind:** public template member (declared `Entity.h`, defined `EntityTemplates.h`)
- **Signature:** `template<typename T, typename... Args> T* AddBehaviour(Args&&... args);`
- **Responsibility:** construct a behaviour of type `T`, attach it to this entity, return it for immediate configuration.
- **Contract:** returns the new instance; on a duplicate type, asserts, warns, and returns the **existing** one. `OnCreate` runs at the next script pass, not here.
- **Playbook:** §10 (generic template family — not the gated named verbs).

---

## 5. Implementation Steps

### Phase 0 — Prerequisite (see D7) — **DONE**
- [x] **RAD-97 (deferred destruction) landed first** — `c49bd4f`, `e9c2ff8`, `22e0a47` on `dev`. The stopgap alternative was not needed and no retirement card is owed for it.

### Phase 1 — Storage
- [x] **Add the side table to `Level`** — the map of `Scope<EntityBehaviour>` lists plus `m_BehaviourEntities`, the maintained cross-entity walk order. Nothing uses it yet; the old path still runs, so the build stays green between steps. *(Done 2026-09-27. **Accessors deliberately not added here** — their signatures are determined by their first caller in Phase 2, and an accessor written before its customer is a guess. The out-of-line `~Level()` now carries a comment saying it is required rather than stylistic, since two members depend on it. `<unordered_map>`/`<vector>` added explicitly: the header already used both transitively, and a header should name what it uses.)*
- [x] **ADD the `BehaviourComponent` tag** in `Components.h` with its `IsEngineComponent` specialisation (D9) and its plain-data `static_assert`s. *(Done 2026-09-27.)*

  **RESEQUENCED, and the original step was wrong.** As written this step said *"replace"* — delete `NativeScriptComponent` here. That is not possible without breaking the build: it has **17 live references across 6 files** (the walk ×6, `GetBehaviour<T>` ×1, the two teardown halves, the serializer ×3, Reaper ×6). Deleting it in Phase 1 leaves the engine unbuildable until Phases 3 **and** 4 both finish, which contradicts this step's own promise that "the build stays green between steps" and locked decision #1 (the engine must stay runnable at every milestone, Reaper must survive every change). The two components therefore **coexist** through the migration, with `NativeScriptComponent` documented as still being the live path, and the deletion becomes its own step at the end of Phase 4 below.

- [x] **DELETE `NativeScriptComponent`** — **DONE 2026-09-27.** Moved here from Phase 1, and it had to be the *last* code step: only once the walk (Phase 3), the API (Phase 2) and Reaper (Phase 4) all name `BehaviourComponent` is there nothing left referencing it. The two `std::function`s and the owning pointer disappear at this point, which is what closes RAD-30's script half. Also retire the now-false mentions: `Components.h`'s file-header note and its `IsEngineComponent` doc ("games need … `NativeScriptComponent`"), `Entity.h`'s null-case 2 and rebind clause, and `EntityBehaviour.h`'s four references.

  **The serializer gets no replacement entry** (deviation from the original step's "the entry follows the rename"): `BehaviourComponent` must **not** be serialized, because behaviours are code-only and a restored tag would describe a state that cannot exist — an entity marked as having behaviours with an empty side table behind it, which the snapshot pass would then treat as a mover with nothing driving it. Writing an empty map for symmetry is worse than writing nothing, since it invites a future deserializer to restore it. The `IsEngineComponent` gate makes that mistake a compile error rather than a convention, and the reasoning is recorded at the site in `LevelSerializer.cpp`. Behaviour **field** state is RAD-104's question, not this tag's.

### Phase 2 — API
- [ ] **`Entity::AddBehaviour<T>()`** — construct into the table, add the tag, enforce the duplicate policy (assert + warn + return existing). Note the tag is `IsEngineComponent`, so this writes it through `m_Registry` (per RAD-97's rule: an engine path that needs a gated component uses the registry, the gate is not widened). `RADIANT_REJECT_ENGINE_COMPONENT`'s message already names `AddBehaviour<T>()` — updated in Phase 1 when gating a third component made its advice incomplete — so it needs no second visit here.
- [x] **`Entity::HasBehaviour<T>()`** — one line over `GetBehaviour<T>()`, matching `HasComponent<T>`'s existing spelling. *(Done 2026-09-27.)*
- [x] **`Entity::RemoveBehaviour<T>()`** — the deferred detach of D12. *(Done 2026-09-27.)* Tag removal on the last detach lands with the reap in Phase 3, which is the only place that compacts.
- [ ] **`Entity::GetBehaviours()`** — **DEFERRED to Phase 4**, after Reaper migrates. During the coexistence window `GetBehaviour<T>` must consult *both* the side table and the legacy component (see below), so an honest `GetBehaviours()` would have to yield `N` from the table plus `0-or-1` from the component — a dual-source view whose machinery gets deleted the moment `NativeScriptComponent` does. It has no caller before the Phase 5 probe, so building it now means building something to throw away. Single-source and trivial once the migration completes.

**Two prerequisites pulled into this phase, because `RemoveBehaviour` cannot work without them:**

- [x] **The two phase flags on `EntityBehaviour`** (D11, D12) — `m_HasCreated` and `m_PendingRemove`, private beside `m_Entity` under the existing `friend class Level`.
- [x] **`Level::AttachBehaviour` / `DetachBehaviour` / `IsBehaviourDetached` / `FindBehaviour<T>`** — the accessors deliberately skipped in Phase 1, now that their callers exist and their signatures are determined rather than guessed. The split is an *access* consequence, not taste: `Level` is `EntityBehaviour`'s only friend, so every path that touches a flag lives on `Level`, and `Entity`'s templates supply only the type.

**Two findings from implementing it, both worth keeping:**

- **`GetBehaviour<T>` must be DUAL-SOURCE for the length of the migration** — side table first, then `NativeScriptComponent`. Reaper's behaviours are still bound through the component until Phase 4, so a table-only scan compiles perfectly and then silently breaks every cheat and collision hook in the game. The fallback is deleted with the component, not before.
- **Two-phase lookup bites the obvious implementation.** Reading `m_PendingRemove` directly inside `FindBehaviour<T>` does not compile: `Scope<EntityBehaviour>` does not depend on `T`, so the member access is *non-dependent* and is checked when the header is **parsed**, where `EntityBehaviour` is only forward-declared. `dynamic_cast<T*>` beside it is legal precisely because it *is* dependent. Hence `Level::IsBehaviourDetached`, non-templated and out of line — which also puts the skip rule in one place where it cannot be re-implemented per caller (playbook §4).

> **AFTER THIS PHASE, `AddBehaviour<T>()` STORES BUT NEVER TICKS.** The API is complete and verified, and nothing dispatches to the side table until Phase 3 wires the three sites. Anyone reading this mid-migration should not mistake a working attach for a working behaviour.
- [ ] **Reshape `Entity::GetBehaviour<T>()`** — scan the entity's list with `dynamic_cast`. Signature unchanged (RAD-100 designed for this). **Two edits to the `Entity.h` doc block, not one**, because eager construction and the single attach spelling each retire a documented state: (a) null case 3, *"instance not built yet"* (`Entity.h:174-176`), can no longer occur; (b) the dangling clause *"when the behaviour is rebound — `AddOrReplaceComponent<NativeScriptComponent>` deletes the instance the old pointer names"* (`Entity.h:184-185`) names a call that will not exist. A contract describing impossible states is a contract being read carelessly.

### Phase 3 — The three dispatch sites — **DONE 2026-09-27**
- [x] **`Level::UpdateBehaviours(ts)`** — the side-table pass, called from `OnFixedUpdate` immediately after the legacy loop and inside the same `ScriptPassScope`. Bounded by counts captured up front; `!valid || IsPendingDestroy` re-checked **per behaviour**, not per entity (D7 item 1). Two passes rather than one merged walk, so Phase 4 can excise the legacy loop instead of untangling it.
- [x] **`OnCreate` dispatch** — per entity, all of its pending `OnCreate`s before any of its `OnUpdate`s, insertion order (D11). A behaviour attached mid-pass waits for the next step, and one not yet created never receives `OnUpdate` first.
- [x] **`NotifyScript` fan-out** (D10) — insertion order, entity re-checked after each callback.
- [x] **`Level::RunBehaviourDestroyHooks`** — LIFO `OnDestroy` at the mark, gated on `m_HasCreated`, inside the existing `MarkScope` so the recursion guard covers the whole list at once.
- [x] **`Level::FreeBehaviours`** — LIFO delete at the reap, drops the map entry, and removes the entity from the walk order with `std::erase` (**not** swap-and-pop: this list exists to be ordered).
- [x] **`Level::CompactDetachedBehaviours` — added, and absent from the original plan.** `RemoveBehaviour` marks behaviours on entities that go on *living*, which the reap's per-entity loop never visits, so detached instances would have leaked forever. Driven by a `m_BehaviourCompactList` so cost scales with detaches rather than population, and it strips the tag when an entity's last behaviour leaves — the other half of D9's invariant, which D3 only ever argued in the add direction.
- [x] **The tag's first real customer** — `BehaviourComponent` joins the snapshot pass's mover views *and* its stale-snapshot `any_of`. Both, or an entity that is still a mover loses its snapshot and stops interpolating.

**Two bugs written and caught during this phase, both worth keeping as warnings:**

- **Compaction below the reap's early-out.** `ReapDestroyedEntities` returns early when nothing died, so a sweep placed after that return meant a detach on a step with no deaths never compacted — a leak visible only as a slowly growing walk. It now runs first, unconditionally.
- **`NotifyScript` captured `nsc` before the fan-out.** The fan-out runs gameplay, a handler may spawn a scripted entity, and that reallocates the `NativeScriptComponent` pool — so the pointer dangled by the time the legacy half used it. The existing comment warned about exactly this ("the calls are the last statements here"); inserting work above them broke the invariant. Now resolved *after* the fan-out. Playbook §8.8's reference-invalidation half, which the iterator fix does not cover.

**Verified by throwaway probe (reverted — the carded one is still Phase 5):** `OnCreate` 1,2,3 and grouped per entity · `OnUpdate` 1,2,3 every step · self-detach at step 5 → one `OnDestroy`, the method **completes**, siblings keep updating steps 6–11, teardown then fires #3,#1 only · self-destroy at step 9 → the method **completes**, `OnDestroy` #3,#2,#1 reverse, and **no sibling `OnUpdate` after them on that step or any later one** · attach+detach before first update → **neither** hook · collision fan-out #1,#2,#3 · coexistence proven on one entity (`Green Square` carried a component-bound `CollisionLogger` **and** three side-table behaviours; both channels fired). Full rebuild ×3: 0 errors, 6 warnings, all pre-existing in `OpenGLFrameBuffer.cpp`.

### Phase 4 — Migration and docs — **DONE 2026-09-27**
- [x] **Migrate Reaper** — all 6 attach sites to `AddBehaviour<T>()`. Game code no longer names a behaviour component anywhere. The two spellings collapsed as predicted: `AddComponent` vs `AddOrReplaceComponent` existed only because the storage held one.
- [x] **`Entity::GetBehaviours()`** — deferred here from Phase 2, now single-source. **Returns `std::vector<EntityBehaviour*>` by value, reversing this plan's own D12 decision.** The allocation-free view needed a filtering iterator; the filter must read a private flag; that needs friendship `EntityBehaviour` does not grant — so "cheap" cost a friend declaration plus an out-of-line iterator to save an allocation on a path nothing calls per frame. UE reaches for `TInlineComponentArray` because it collects components constantly at scale; we do not, and inventing that constraint before a caller exists is how machinery gets built for nobody. Lives in `Entity.cpp`, which notably never has to include `EntityBehaviour.h`: iterating `Scope`s and calling `.get()` works on an incomplete type, and `IsBehaviourDetached` takes a reference.
- [x] **Cleaned the stale key from `Level.rdlvl`** — the committed level still carried an empty `NativeScriptComponent: {}` map. Harmless (an unknown key is ignored, which is what makes the format tolerant of exactly this change) but it named a deleted type. Verified the level still loads.
- [x] **Docs and playbook** — **DONE 2026-09-27.** `Gameplay-Framework.md`: two new sections (*Composition*, *Where does my state live?*), the four null cases corrected, the transient-pointer contract, and three Known Issues retired. `ECS-And-Levels.md`: the side table documented as storage with its three consequences, the component table, the `Entity` accessor list, RAD-30's entry now "half done". `Time-And-Simulation.md`: the mover heuristic. `Docs/README.md`: the index line. Playbook §3 (five new rules — the side-table shape, why a tag, bidirectional tag invariants, address stability) and §10 (four — composition and the UE divergence, three teardown moments, phase flags, one-hazard-one-spelling), plus the corrected `GetOwner()` claim. Both architecture maps: CLAUDE.md's table (two rows + the plain-data rule) and `Architecture-Overview.md`'s tree.
- [x] **A doc claim that had quietly gone false** — playbook §10 and `Gameplay-Framework.md` both said `GetOwner()` is named for a fact because *"the entity's `NativeScriptComponent` holds the instance by an owning pointer and deletes it."* The Level owns it now, so the justification inverted while the name stayed right. Both rewritten to name the **relationship** rather than the mechanism, with a note saying why — a rule pinned to a mechanism drifts when the mechanism moves.
- [x] **State the "where does my state live?" rule** — **DONE 2026-09-27**, as its own section in `Gameplay-Framework.md`. Composition makes this urgent: N behaviours per entity is N chances per entity to write a paired component out of habit. **The default is a plain member on the behaviour.** Hoist state into a component only when it needs one of exactly three things — it survives save/load, it is read by systems that do not know your class (a UI health bar), or the Phase 5 editor authors it. Of the Reaper's five behaviours today, none would need a component, so the paired-type shape must be documented as the *exception* or it becomes the pattern by imitation. Say *why*, too: engines that put data and logic in one class (Unity's `MonoBehaviour`, UE's `UActorComponent`) can do it because a reflection system enumerates their fields, so a component is currently our stand-in for reflection — a small, hand-registered type `LevelSerializer` was explicitly taught about. **Write this as the rule for today, not forever** — a reflection MVP is being pulled forward directly after this card (decided 2026-08-27, see the session note below), and it collapses the pair for game-authored types.

### Phase 5 — Verification — **DONE 2026-09-27**
- [x] **Enrolled the scaffolding on RAD-92 first**, per its own rule. The convention turns out to be a dated **Scope Amendment on RAD-92's description**, not a separate card — the precedent set by RAD-90/95/97/100. The 2026-09-27 amendment records what lands and what a replacement must preserve. It also **corrects the 2026-08-02 amendment**, which described `SpawnProbe` as perturbing "the `NativeScriptComponent` pool" — a component this card deleted. The property being proved is unchanged; the mechanism named was not.
- [x] **`Reaper/Source/BehaviourCompositionProbe.h`** — four fixtures (`ProbeOrder`, `ProbeDetach`, `ProbeDestroy`, `ProbeNoHooks`) plus a ticker, three distinct types from one `OrderProbe<N>` template, **no cheat key**. Two deliberate improvements on every prior probe: it **asserts** (silent on success; `GAME_WARN` naming the violated property on failure; one SUMMARY line, so a silent pass is distinguishable from a probe that never ran), and it is **step-driven rather than key-driven**, so it exercises itself on every run instead of needing a human at the window.
- [x] **Debug 58 checks / 0 failures; Dist 60 checks / 0 failures.** The +2 is the `#ifdef RD_DIST` duplicate-attach arm, reachable only where asserts are compiled out — so the count difference is itself the evidence that arm executed.
- [x] **Full rebuild ×3 configs** — 0 errors, and 0 warnings outside the pre-existing `OpenGLFrameBuffer.cpp` set.

**The probe caught two bugs on its first run — both in the probe, neither in the engine**, and that distinction is the whole argument for assertions over eyeballing a log. Phase 3's manual read said "correct", and it *was* correct; only an explicit expectation exposes the gap between *looks right* and *is specified*:

- `AnyDestroyed` conflated "some behaviour's `OnDestroy` ran" with "the entity is being torn down" — but a detach runs one `OnDestroy` and the siblings must carry on. The two are indistinguishable from inside `OnDestroy` **by design** (D12a: one hook, one meaning), so the probe now records its own intent rather than inferring it.
- The expected update order on the destroy step was written `"123"`; the correct answer is `"12"` — #1 and #2 update, #2 condemns the entity from inside its own `OnUpdate`, and #3 is rightly skipped.

### Review fixes (2026-09-27)

`/review` found one blocker and two warnings, all in code written earlier the same day. The blocker is the instructive one:

- **ERROR — the walk order was erased per dead entity, O(M·N).** `FreeBehaviours` ended with `std::erase(m_BehaviourEntities, handle)` — a linear scan and shift — called once per entity in the reap loop, with `CompactDetachedBehaviours` doing the same in its own loop. **This is the exact shape RAD-97 removed**, and its comment sits fifteen lines further down the same function: *"fifty projectiles dying in one step was fifty O(n log n) sorts."* Reintroduced in a new container, two commits after the lesson was written down — which is worth more as a warning about how fast a documented lesson decays than as a bug.

  Fixed by making the walk order **derived rather than maintained**: neither teardown path touches it, they only raise `m_BehaviourWalkOrderDirty`, and the reap reconciles in one `erase_if` pass keyed on "is this entity still in the table?". That covers both ways an entity can leave without either path knowing the list exists — less code and one fewer desync surface, not just less cost.

  **The first attempt at this fix was itself wrong**, in the same way and caught the same way: placing the sweep after the reap's nothing-died early-out meant a detach on a quiet step never reconciled, leaving the entity in the walk forever. So the early `return 0` is now gone, with the two costs it protected guarded individually instead — the reap loop by `m_ReapList` being empty, `SortEntities` by `reaped > 0`. A quiet step is exactly as cheap as before.

- **WARNING — `unordered_map` iterator reused after running behaviour destructors.** Both teardown paths held an iterator from `find`, then ran `pop_back`/`erase_if` (invoking behaviour destructors, which may attach and rehash), then called `erase(it)`. Now `erase(handle)` by key in both. `~Level`'s own comment already contemplates the destructor-that-spawns, so the actor was on the record.

- **WARNING — `Level.h`'s lifecycle contract said "the behaviour instance", singular**, and never mentioned that the reap also compacts detached behaviours from living entities. Rewritten.

Re-verified after the fixes: full rebuild ×3 (0 errors, 0 new warnings), probe **58/0 Debug** and **60/0 Dist**.

**Two honest notes on the review itself.** It was a self-review — I wrote every line I checked, which catches mechanical regressions like the blocker and is structurally poor at catching wrong *assumptions*, since the same assumptions produced both. And the story is labelled `mentorship`, whose Definition of Done says Kareem implements the core; all five phases were implemented by Claude at his explicit instruction. Recorded rather than passed over.

**A verification limitation worth recording, because it will recur:** Reaper boots to `MainMenuState` and only the Play button leaves it, so a headless launch never reaches gameplay and the probe never runs. `SendKeys` does not help — GLFW reads raw input, not window messages. Verification was done by temporarily booting straight to `GameplayState`, reverted immediately (tree confirmed clean, no residue). Whoever executes RAD-92 hits the same wall.

---

## 6. Ownership & Lifetime Strategy

- **Who creates:** `Entity::AddBehaviour<T>()`, eagerly, at the call site.
- **Who owns:** the `Level`, via `Scope<EntityBehaviour>` in the side table. One ownership story, stated at the declaration (playbook §2). This is the change that closes RAD-30's known violation: the raw owning pointer inside a component becomes unique ownership outside it.
- **Who deletes:** `Level::ReapDestroyedEntities` — and *only* there (LIFO, D6). Not `DestroyEntity`, which merely runs `OnDestroy`; not `~Level`, which marks everything and then loops the reap until it converges (`Level.cpp:87`, capped at 8 passes). One deleter, one line, which is what makes the "no sibling on freed storage" AC provable rather than argued.
- **What the caller gets:** a **non-owning observer** `T*`, transient exactly as RAD-100 documented — and now more fragile, because a *sibling* behaviour's actions can invalidate it, not only the entity dying. The existing contract already says this; it becomes load-bearing rather than theoretical.

**The hazard this card must not get wrong:** the walk hands control to gameplay, and gameplay can attach behaviours (growing the `vector`, invalidating references into it) and destroy entities (erasing map entries). Playbook §8.8 names both halves — iterator invalidation *and* reference invalidation are different bugs needing different fixes, and RAD-95 learned that the hard way in this exact loop. Index-based iteration plus re-resolution after any gameplay call, not cached references.

---

## 7. Performance Notes

**Net: neutral-to-better per frame, with one new allocation on a cold path.**

- **Removed:** the per-step rebuild of `m_ScriptUpdateList` from an `entt` view (`Level.cpp:712-722`). Today that runs every fixed step whether or not anything changed — and note it reserves on `size_hint()`, the *unfiltered* pool size, because an excluding view cannot count itself. A maintained list replaces both the walk and that over-reserve — the shape UE's tick registry already has.
- **Removed:** two `std::function` indirections per instantiation, and the `std::function` storage per component.
- **Added:** one heap allocation per `AddBehaviour` (the instance) and occasionally one for the `vector` growth. Attach is a spawn-time path, not a per-frame one.
- **Added:** a map lookup per `GetBehaviour` call. Event-shaped, not frame-shaped (RAD-100 §7); the guidance is unchanged — cache the `Entity`, never the pointer.
- **Unchanged:** the update walk is still one virtual call per behaviour per fixed step.

**Not built, deliberately:** contiguous instance storage, an inline small-buffer array per entity (`InlineArray<T,N>` stays gated behind RAD-67/RAD-83 per playbook §9), and any parallel dispatch. All three want a measurement first, and we have neither the harness nor the entity counts to produce one.

---

## 8. Logging & Diagnostics

| Situation | Response | Why |
|---|---|---|
| Duplicate behaviour type attached | `RADIANT_ASSERT` + `RADIANT_WARN` + return existing | Programmer error (code-only bindings), but Dist must not run with two |
| `AddBehaviour` on an invalid entity handle | `RADIANT_WARN`, return `nullptr` | Handle of unknown provenance, like the other gameplay verbs |
| `GetBehaviour` finding nothing | **silent `nullptr`** | Unchanged from RAD-100 — a query answers, and "no" is an answer (playbook §10) |
| Attach / detach | **no TRACE** | Routine and potentially per-spawn; playbook §4's rare-and-discontinuous rule |
| `OnCreate` / `OnDestroy` dispatch | no engine log | The behaviour logs if it wants to; the engine narrating every hook is 60 lines a second at scale |

---

## 9. Scalability Review

- **This card *is* the scalability fix.** Today every reusable gameplay concern costs either a base-class edit or an engine loop — O(N) code changes per feature. After it, a new concern is one new class and one `AddBehaviour<T>()` call, with zero engine edits. That is the whole point.
- **`AddBehaviour<T>` / `GetBehaviour<T>` are O(1) engine edits per behaviour type** — templated, so the header does not grow. Consistent with the family rule RAD-100 established.
- **FLAG — the update walk must not become a per-type dispatch.** The temptation later is "run all `Health` behaviours, then all `Movement` behaviours" for cache reasons. That is a per-type ladder and it would reintroduce the O(N) shape. If ordering ever needs to be type-aware, the scalable answer is a data-driven priority attached to the *instance*, not a hand-written sequence in `Level`.
- **FLAG — `unordered_map<entt::entity, vector<...>>` is pointer-chasing squared** and will show up first in the walk, not the query. It is the right shape for now (correct, simple, obviously right at tens of behaviours). The scalable replacement when measurement demands it is a `SlotMap`-style dense store with a stable handle — already catalogued in playbook §9 with RAD-30's side tables named as a customer. Not now: gated on RAD-67 and RAD-83, and premature without numbers.
- **Not a flag:** the duplicate rejection (D5) bounds the query at "first match wins is never ambiguous", which is a *simplification* that scales fine.

---

## 10. Risks & Edge Cases

- **Mutation during the walk is the whole risk surface.** Gameplay can attach (vector growth → reference invalidation), destroy the entity (map erase → iterator invalidation), or destroy a *different* entity. Playbook §8.8 says these are two distinct bugs; RAD-95 fixed both in this loop once already. Mitigation: index-based iteration, re-resolve after any gameplay call, never hold a reference across a callback.
- **Self-destruct with siblings — the hazard changed shape rather than vanishing.** RAD-97 means the instance survives to the reap, so this is no longer a use-after-free. What is left is an **ordering** bug: without the per-behaviour re-check (D7 item 1, D10), sibling #3 receives `OnUpdate` *after* its own `OnDestroy` already ran. That is quieter than a crash and therefore easier to ship — Debug will not fault, Dist will not fault, and the only symptom is a behaviour doing one last frame of work while officially dead. It is caught by the log ordering in §11, not by the allocator.
- **`OnCreate` ordering across a spawn burst.** A behaviour attached during the walk gets `OnCreate` on the *next* step, matching RAD-95's existing spawn contract. Must be documented, not left to be discovered — `SpawnProbe` already demonstrates this ordering for entities and would demonstrate it for behaviours too.
- **`Level::Copy` — a forward constraint, not a step.** The function **does not exist yet**; play-in-editor is Phase 5, so nothing in this card can break it. Recorded now because the constraint is cheap to state and expensive to discover: behaviours are code-only and not copyable in general, so a copy must produce a level carrying the tag component with an **empty** side table, and the copier re-attaches.
- **The tag component and the side table can desync** if anything writes one without the other. Mitigation: `AddBehaviour`/teardown are the only writers, and both live in `Level`.
- **Reaper's `B`/`C`/`N` cheats must keep working.** They go through `GetBehaviour<T>()`, whose signature does not change — a good sign the RAD-100 seam was drawn correctly, and a free regression test for this card.

---

## 11. Verification (AC → proof)

| Acceptance Criterion | How to verify |
|---|---|
| An entity carries more than one behaviour, each receiving lifecycle and update callbacks | Reaper scaffolding: three trivial behaviours on one entity, each logging `OnCreate`/`OnUpdate` once; all three appear |
| Update order between behaviours is defined and documented | The three log in attach order, every step, across many steps. Contract stated in `EntityBehaviour.h` and `Gameplay-Framework.md` |
| Destroying an entity runs every `OnDestroy` exactly once | Destroy the three-behaviour entity: three `OnDestroy` lines, **reverse** order, no repeats. Run in Dist too — a double-delete is where Debug's allocator hides things |
| A behaviour destroying its own entity leaves no sibling on freed storage | Middle behaviour of three destroys its own entity mid-`OnUpdate`. **The proof is log ORDER, not the absence of a crash** — post-RAD-97 the memory is alive either way, so a crash was never going to be the signal. Required: #2's `OnUpdate` line completes; three `OnDestroy` lines follow in reverse order; **no `OnUpdate` line from #3 appears after them**, on that step or any later one. Extend `SpawnProbe`'s `RequestSelfDestruct`, which already exercises this path for one behaviour |
| A collision handler on a multi-behaviour entity reaches every behaviour | All three log `OnCollisionBegin` for one landing, in insertion order (D10). Free to observe: the Reaper's existing contact traffic already produces 20+ landings a run |
| Typed retrieval has defined duplicate semantics | Attach a duplicate type: asserts in Debug, warns in Dist, one instance exists, `GetBehaviour<T>()` returns it |
| *(added by the D12 surface)* Detach runs `OnDestroy` once and frees at the reap | Middle behaviour of three detaches **itself** mid-`OnUpdate`: its method completes, one `OnDestroy` line, the two siblings keep updating on later steps, `GetBehaviour<T>()` returns null from the next step on. Dist too |
| *(added by the D12 surface)* Detach before first update runs no `OnDestroy` | Attach and detach in the same step, before the walk reaches the entity: **no** `OnCreate` and **no** `OnDestroy` line — the `m_HasCreated` pairing |
| *(added by the D12 surface)* `HasBehaviour<T>` / `GetBehaviours()` agree with the table | On the three-behaviour entity: `GetBehaviours()` yields three in insertion order, `HasBehaviour<T>` true for each; after a detach, two and false for the removed one |
| Components remain plain data; instances live in the Level-owned side table | `Components.h` has no owning pointer and no `std::function`; grep confirms. `BehaviourComponent` is trivially copyable (`static_assert`), and game code cannot add or remove it — D9's `IsEngineComponent` specialisation makes the attempt a compile error, which is a stronger proof than any runtime check |
| The merge-with-RAD-30 question is answered before implementation | §1 D1 of this plan, posted to the story, and RAD-30 amended |
| Ownership documented: who owns the list, who deletes each instance | `Level.h` declaration + §6; `Scope<EntityBehaviour>`, Level is sole deleter |
| Compiles clean in Debug/Release/Dist, no new warnings | CLI MSBuild all three per CLAUDE.md |
| Reaper runs and is visually verified | Debug and Dist. `B`/`C`/`N` still behave — the RAD-100 seam survives the storage change unchanged |
