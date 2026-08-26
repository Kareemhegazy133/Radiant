# Implementation Plan — RAD-101: Behaviour composition: more than one behaviour per entity

| Field | Value |
|-------|-------|
| **Jira** | [RAD-101](https://hndredgames.atlassian.net/browse/RAD-101) |
| **Epic** | RAD-98 — Gameplay Framework: engine base classes for game code |
| **Story status** | To Do |
| **Dependencies** | RAD-95, RAD-99, RAD-100 (In Review, on `dev`). **BLOCKED BY RAD-97** — agreed 2026-08-26, see §1 D7 |
| **Absorbs** | RAD-30's `NativeScriptComponent` scope — agreed 2026-08-26, see §1 D1 |
| **Planned** | 2026-08-25 (drafted); walkthrough paused at §1 pending RAD-97 |
| **Status of this plan** | §1 D1 and D7 locked. §§2–11 drafted but **not yet walked through or posted** — revisit after RAD-97 lands, since it reshapes teardown (§5 Phase 3) |

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
- **No fix for the self-destruct wart on its own** — in fact composition makes it worse, which is why §1 D7 proposes sequencing RAD-97 first.

### What we have / what we're building

**What we have:** one C++ behaviour per entity, so every reusable piece of gameplay has to enter through a base class, and composition is only spellable as inheritance.

**What we're building:** a small owned list of behaviours per entity, with a defined update order, a defined teardown order, and an attach API that never makes game code name the component the list lives behind.

---

## 1. Architecture Decision

Seven decisions carry this card. D1 and D7 are sequencing calls that need agreement before implementation starts; the rest are design calls with their rejected alternatives recorded.

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

**Why keep a component at all**, when the side table's key set already answers "who has behaviours"? Because a real customer needs it in a `view`: `Level::OnFixedUpdate`'s snapshot pass classifies movers with `view<NativeScriptComponent, TransformComponent>` (`Level.cpp:492`) and `any_of<RigidBody2DComponent, CameraComponent, NativeScriptComponent>` (`Level.cpp:503`). Delete the component and that classification loses a third of its inputs. The tag is maintained only by the attach/detach path — **one writer** — so it cannot desync from the table.

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

### D6 — Teardown order: reverse of insertion (LIFO)

A behaviour attached later may have found an earlier one in `OnCreate`, so the dependent should die before its dependency. That is the same discipline as C++ destructors and stack unwinding, which every reader already has intuitions for.

Note we are being **stricter than UE** here, cheaply: `AActor::UninitializeComponents` (`Actor.cpp:6350-6362`) just walks `GetComponents()` and calls `UninitializeComponent()` in whatever order the set yields — UE does not define it. We can, because our list is ordered anyway.

### D7 — **Sequencing: RAD-97 lands first. AGREED 2026-08-26.**

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

*Excluded with reason:* `RemoveBehaviour<T>()` — fails RAD-94's customer gate; nothing detaches a behaviour today. `HasBehaviour<T>()` — it is `GetBehaviour<T>() != nullptr`.

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
| Modify | `ECS/Components.h` | `NativeScriptComponent` (owning ptr + 2 `std::function`s) → empty `BehaviourComponent` tag. Closes RAD-30's plain-data violation. |
| Modify | `ECS/Level.h` / `.cpp` | Level-owned side table of `Scope<EntityBehaviour>` lists; attach/detach; the maintained walk replacing the per-step rebuild; LIFO teardown in `DestroyEntity`. |
| Modify | `ECS/Entity.h` / `EntityTemplates.h` | `AddBehaviour<T>()`; `GetBehaviour<T>()` becomes a scan of the entity's list; drop the "not built yet" null case. |
| Modify | `Gameplay/EntityBehaviour.h` | Contract: several behaviours per entity, insertion-order update, LIFO teardown, duplicates rejected. |
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

### Phase 0 — Prerequisite (see D7)
- [ ] **Land RAD-97 (deferred destruction) first**, or accept the scoped stopgap and file its retirement card. Do not start Phase 2 below until this is decided.

### Phase 1 — Storage
- [ ] **Add the side table to `Level`** — the map of `Scope<EntityBehaviour>` lists, plus its accessors. Nothing uses it yet; the old path still runs.
- [ ] **Replace `NativeScriptComponent` with the `BehaviourComponent` tag** in `Components.h`, and update the `LevelSerializer` entry. This is the RAD-30 half; the two `std::function`s and the owning pointer disappear here.

### Phase 2 — API
- [ ] **`Entity::AddBehaviour<T>()`** — construct into the table, add the tag, enforce the duplicate policy (assert + warn + return existing).
- [ ] **Reshape `Entity::GetBehaviour<T>()`** — scan the entity's list with `dynamic_cast`. Signature unchanged (RAD-100 designed for this). **Remove the "instance not built yet" null case from the doc block** — eager construction retires it.

### Phase 3 — The loop
- [ ] **Rewrite the script pass in `Level::OnFixedUpdate`** — walk the maintained list rather than rebuilding `m_ScriptUpdateList` from a view each step. Keep RAD-95's snapshot discipline: gameplay may attach and destroy mid-walk, so the walk must survive both. Re-validate the entity per behaviour.
- [ ] **`OnCreate` dispatch for newly attached behaviours** — at the start of the next script pass, before `OnUpdate`, in insertion order.
- [ ] **LIFO teardown in `DestroyEntity`** — every behaviour's `OnDestroy` runs exactly once, in reverse insertion order, before the entity's components are removed.

### Phase 4 — Migration and docs
- [ ] **Migrate Reaper** — 5 attach sites in `GameLayer.cpp` plus `SpawnProbe.h`'s in-script attach, to `AddBehaviour<T>()`. This deletes the last mention of the behaviour component from game code.
- [ ] **Docs and playbook** — `Gameplay-Framework.md` (composition, both orders, the duplicate decision and why it differs from UE), `ECS-And-Levels.md` (the side table is storage), playbook §3/§10.

### Phase 5 — Verification
- [ ] **Three behaviours on one entity** in Reaper, proving order and lifecycle (see §11).
- [ ] **Build all three configs; run Reaper in Debug and Dist.**

---

## 6. Ownership & Lifetime Strategy

- **Who creates:** `Entity::AddBehaviour<T>()`, eagerly, at the call site.
- **Who owns:** the `Level`, via `Scope<EntityBehaviour>` in the side table. One ownership story, stated at the declaration (playbook §2). This is the change that closes RAD-30's known violation: the raw owning pointer inside a component becomes unique ownership outside it.
- **Who deletes:** `Level::DestroyEntity` (LIFO, after `OnDestroy`), and `~Level` for anything still attached.
- **What the caller gets:** a **non-owning observer** `T*`, transient exactly as RAD-100 documented — and now more fragile, because a *sibling* behaviour's actions can invalidate it, not only the entity dying. The existing contract already says this; it becomes load-bearing rather than theoretical.

**The hazard this card must not get wrong:** the walk hands control to gameplay, and gameplay can attach behaviours (growing the `vector`, invalidating references into it) and destroy entities (erasing map entries). Playbook §8.8 names both halves — iterator invalidation *and* reference invalidation are different bugs needing different fixes, and RAD-95 learned that the hard way in this exact loop. Index-based iteration plus re-resolution after any gameplay call, not cached references.

---

## 7. Performance Notes

**Net: neutral-to-better per frame, with one new allocation on a cold path.**

- **Removed:** the per-step rebuild of `m_ScriptUpdateList` from an `entt` view (`Level.cpp:529-536`). Today that runs every fixed step whether or not anything changed. A maintained list replaces it — the shape UE's tick registry already has.
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
- **Self-destruct with siblings** — the reason D7 proposes RAD-97 first. Without it, this card ships a worse version of a landmine it cannot fully defuse.
- **`OnCreate` ordering across a spawn burst.** A behaviour attached during the walk gets `OnCreate` on the *next* step, matching RAD-95's existing spawn contract. Must be documented, not left to be discovered — `SpawnProbe` already demonstrates this ordering for entities and would demonstrate it for behaviours too.
- **`Level::Copy` (play-in-editor, Phase 5)** — behaviours are code-only and not copyable in general. Copy must produce a level with the tag component but an empty side table, and the copier re-attaches. State it now; the alternative is discovering it in Phase 5.
- **The tag component and the side table can desync** if anything writes one without the other. Mitigation: `AddBehaviour`/teardown are the only writers, and both live in `Level`.
- **Reaper's `B`/`C`/`N` cheats must keep working.** They go through `GetBehaviour<T>()`, whose signature does not change — a good sign the RAD-100 seam was drawn correctly, and a free regression test for this card.

---

## 11. Verification (AC → proof)

| Acceptance Criterion | How to verify |
|---|---|
| An entity carries more than one behaviour, each receiving lifecycle and update callbacks | Reaper scaffolding: three trivial behaviours on one entity, each logging `OnCreate`/`OnUpdate` once; all three appear |
| Update order between behaviours is defined and documented | The three log in attach order, every step, across many steps. Contract stated in `EntityBehaviour.h` and `Gameplay-Framework.md` |
| Destroying an entity runs every `OnDestroy` exactly once | Destroy the three-behaviour entity: three `OnDestroy` lines, **reverse** order, no repeats. Run in Dist too — a double-delete is where Debug's allocator hides things |
| A behaviour destroying its own entity leaves no sibling on freed storage | Middle behaviour of three destroys its own entity mid-`OnUpdate`; the method completes, siblings do not run afterwards, nothing leaks. Dist + (once available) ASan |
| Typed retrieval has defined duplicate semantics | Attach a duplicate type: asserts in Debug, warns in Dist, one instance exists, `GetBehaviour<T>()` returns it |
| Components remain plain data; instances live in the Level-owned side table | `Components.h` has no owning pointer and no `std::function`; grep confirms. `BehaviourComponent` is trivially copyable (`static_assert`) |
| The merge-with-RAD-30 question is answered before implementation | §1 D1 of this plan, posted to the story, and RAD-30 amended |
| Ownership documented: who owns the list, who deletes each instance | `Level.h` declaration + §6; `Scope<EntityBehaviour>`, Level is sole deleter |
| Compiles clean in Debug/Release/Dist, no new warnings | CLI MSBuild all three per CLAUDE.md |
| Reaper runs and is visually verified | Debug and Dist. `B`/`C`/`N` still behave — the RAD-100 seam survives the storage change unchanged |
