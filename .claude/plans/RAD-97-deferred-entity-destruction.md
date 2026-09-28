# Implementation Plan — RAD-97: Deferred entity destruction: mark now, reap at a defined point

| Field | Value |
|-------|-------|
| **Jira** | [RAD-97](https://hndredgames.atlassian.net/browse/RAD-97) |
| **Epic** | Phase 2 — Simulation Foundation (RAD-2) |
| **Story status** | To Do |
| **Dependencies** | None. Blocks RAD-101. Relates RAD-95 (In Review), RAD-29 (In Review), RAD-30, RAD-81 |
| **Planned** | 2026-08-26 |

---

## 0. The Problem, Ground Up

### What the code does today

`Level::DestroyEntity` does everything, immediately, inside the call:

```text
DestroyEntity(entity):
    run the script's OnDestroy()
    delete the script instance          <-- frees memory
    remove the collider component       --> Box2D shape destroyed
    remove the rigidbody component      --> Box2D body destroyed
    registry.destroy(handle)            <-- frees the entity + every component
    erase it from the UUID map
    re-sort the metadata pool
```

Two of those lines free memory that the *calling code might still be standing on*. That is the whole
story.

A word first, because everything below leans on it. An **entity handle** in Radiant is not a pointer
to an object — it is a small value, `{entt id, Level*}`, 16 bytes, that *names* a row in the level's
storage. `IsValid()` asks the storage "is this row still occupied?". The row is the real thing; the
handle is a ticket to it. So "destroying an entity" means freeing the row, and every ticket anyone is
holding goes stale in that same instant.

### The failure, concretely

Picture one fixed simulation step in Reaper. Physics runs, and the step produces a batch of, say,
**12 contact events** — twelve pairs of things that started or stopped touching. The engine then
walks that batch and tells gameplay about each one.

Contact event #3 is "Green Square touched Platform". The platform's script handles it and destroys
the square. That call frees the square's row *right now*.

Events #4 through #12 are still queued, and four of them name the square. Each of those events
already holds the square's identity. If the code walking the batch had resolved that identity to a
handle even one line too early, it is now holding a ticket to a freed row — and reading it is
undefined behaviour: it works in Debug, where the freed slot still happens to hold plausible bytes,
and corrupts under optimisation where the slot has already been reused by the next spawn.

So the engine defends itself. `Level::DispatchContactEvents` re-resolves **both** participants from
their UUID immediately before **every single callback** (`Level.cpp:449-458`). With 5 registered
level-wide callbacks and 12 events, that is `2 × 5 × 12 = 120` hash-map lookups per step whose only
job is to survive a free that has already happened.

That is symptom one. There are two more, and all three have the same cause:

- `EntityBehaviour`'s hooks carry a landmine in their documentation: *"destroying THIS entity deletes
  the instance whose method is executing, so it must be the last statement"* (`EntityBehaviour.h:161-165`,
  `:181-184`). A gameplay programmer who writes `GetOwner().Destroy();` and then one more line has
  written a use-after-free — the object whose method is running was freed by the call.
- The move-event drain asserts entities cannot vanish between the physics step and the drain
  (`Level.cpp:610`), a fact that has to be re-argued in a comment every time the ordering changes.

This is not an exotic bug class. Box2D v2's `b2ContactListener` documentation warns that you must not
destroy bodies inside a contact callback — the crash it caused is one of the most frequently reported
issues in the library's history, and dodging it is why Radiant's contact events are queued at all
(RAD-29). Unity solved the same problem for the same reason: `Object.Destroy()` does not destroy
anything when you call it — it flags the object, and the real destruction happens after the current
update loop finishes. Unity added `DestroyImmediate` for editor tooling and its own manual tells you
not to use it in gameplay.

### The fix, as one everyday thing

Today, the moment anyone says *"tear that building down"*, the wrecking ball swings — mid-street, at
noon, while the mail carrier is still inside the lobby handing out letters. Every mail carrier
therefore has to stop before *each envelope* and check whether the building is still standing. That is
the 120 lookups.

The fix is what cities actually do: you nail a **condemned notice** to the door. From that instant the
building is off the books — the gas is shut off, no new deliveries, it is struck from the street
directory, and anyone walking past can read the notice and know not to go in. But the wrecking ball
does not swing until **6pm, when the street is empty**.

Three parts, and each maps to a piece of this story:

| The part | What it is here |
|----------|-----------------|
| The notice | A marker on the entity. `IsValid()` reads it and answers "no" from the instant you destroy. |
| The gas shut-off | Physics teardown stays **eager** — the body dies at the mark, not at 6pm. |
| 6pm | The **reap**: one defined point at the end of the fixed step where rows are actually freed. |

The gas shut-off is the part that is easy to get wrong by being too clever. If the building kept its
utilities until 6pm, a bullet you destroyed would keep colliding for the rest of the step — *"I killed
it and it damaged me twice"*. So physics dies immediately; only the storage row waits.

### How Unreal does it, in plain words

Unreal's `UWorld::DestroyActor` does exactly this, in this order: it tells the actor it is dying
(`Destroyed()`), **removes it from the level's actor list immediately**, **unregisters its components
immediately** — which is what tears down its physics — and then calls `MarkAsGarbage()`, which flips a
flag that makes every `IsValid(Ptr)` check in the engine answer false. The actor's *memory* is not
freed there. It is freed later, by the garbage collector.

So Unreal's answer is: strike it from the directory now, cut its utilities now, flag it now — free it
later, at a defined moment. That is the same three parts.

The one thing we do **not** take is the collector. Unreal needs a tracing GC because a `UObject` can
be referenced by any other `UObject`, and only a graph walk can tell when the last reference is gone.
Radiant has no such question: `entt` owns entity and component storage outright, `Ref`/`Scope` already
state ownership per type (playbook §2), and weak references are generation handles rather than
pointers (playbook §2, decided 2026-07-09). There is nothing for a tracer to collect. We are taking
`MarkAsGarbage` and the deferred free; we are not taking `AddReferencedObjects`, a reference graph, or
a collection pass.

### What we gain, and what we deliberately do not

**Gain:** `DestroyEntity` becomes safe from anywhere — a collision handler, a script's `OnUpdate`, an
entity destroying itself, a script destroying the entity whose callback is one item later in the same
batch. The three defensive sites above collapse to one validity check each. The `SortEntities()` call
moves from per-destroy to per-reap, so destroying 50 bullets in one step goes from 50 sorts to 1.

**Not a gain — and worth saying plainly:**

- **No memory win.** The row is freed at the end of the step instead of mid-step. In steady state that
  is identical; in the worst case it is one extra step of memory held.
- **No latency win, no frame-time win.** The work moved in *space*, not in *time*. We do the same
  teardown, at a different moment.
- **It does not fix add-during-iteration.** Deferring destruction says nothing about a script that
  *spawns* an entity mid-pass and reallocates the pool being walked. That is RAD-95's snapshot, and it
  is why both stories exist rather than one.
- **It does not make handles safe to store.** A handle to a reaped entity still names nothing; what
  changes is that it now answers `IsValid() == false` for a defined window instead of racing the free.
  Entity handles remain transient — the UUID is still the durable identity (playbook §2).
- **It is not a garbage collector.** No reference graph, no tracing, no collection pass. Validity stays
  a generation-handle question.

**What we have / what we're building.** Today, destroying an entity frees it under the feet of whatever
is running, so three separate places in the engine carry defensive code and gameplay carries a
documented landmine. We're adding a condemned notice: destruction marks the entity dead immediately —
invisible to physics, scripts, rendering and lookups from that instant — and the actual free happens
once, at the end of the fixed step, when nothing is standing on it.

---

## 1. Architecture Decision

**The shape:** one empty tag component (`PendingDestroyComponent`) is the notice; `Level::DestroyEntity`
becomes the *mark*; a new private `Level::ReapDestroyedEntities()` is the *reap*, called at exactly one
place — the end of `Level::OnFixedUpdate`, after contact dispatch. `Entity::IsValid()` composes the
registry's own validity with the absence of that tag, so **every existing validity check in the engine
and in game code becomes pending-aware for free**, with no call sites edited.

That last sentence is the load-bearing design claim. It works because `Entity` is a *value handle* that
re-asks the question at every use rather than caching an answer (`Entity.h:100`). Validity is a
property of the world, not a property of the moment you looked it up. Once that is true,
`DispatchContactEvents` can resolve each participant **once per event** and hand the same
`CollisionEvent` to every callback: a callback that destroys the square does not invalidate the
`Entity` value the next callback holds — it changes what that value *answers*, which is the whole
point. The 120 lookups become 24.

**Reap point: end of the fixed step, after `DispatchContactEvents`.** Not end of frame. Rendering must
never observe a half-destroyed entity, and the fixed step is the only place the engine already
guarantees the physics world is idle and no gameplay is on the stack. This is the same discipline
`EventQueue` applies to input — record now, drain at one defined point (playbook §1) — applied to
entity lifetime. Placed *outside* the `if (m_PhysicsWorld)` guard, because a scratch level with no
world still runs scripts and still needs its corpses collected.

**Physics stays eager; only ECS storage defers.** Stated in the story and confirmed against Unreal's
own source, where `UnregisterAllComponents()` reaches `UActorComponent::ExecuteUnregisterEvents()`
whose *first* line is `DestroyPhysicsState()` (`ActorComponent.cpp:2476-2478`) — all of it before
`MarkAsGarbage()`. The alternative (defer the body too) produces the "I killed it and it damaged me
twice" bug and is rejected.

**The marker is a tag component. Both alternatives were considered and rejected** (asked during the
2026-08-26 walkthrough, recorded here so the next reader meets the decision rather than the precedent).

***Rejected: a `bool` member on the `Entity` class.*** This cannot work, and the reason is the story's
central idea. `Entity` is a 16-byte **value handle**, copied freely: the level's `m_EntityMap` holds a
copy, `CollisionEvent` holds two freshly-built copies, a script holds a copy in a member, `GetOwner()`
returns a copy. `DestroyEntity` receives *one* of them, by value — flagging it flips a bool on a
temporary, while every other copy in the engine still answers "alive". There is no way to enumerate the
copies to update them; some are on stacks and some are in game code. State belongs to the **row**;
handles only ask questions about it. Same reason `PhysicsBody` caches nothing and re-resolves per call
(playbook §4), and same reason there is no `WeakRef` type (§2). Note the size argument is *not* the
reason: `entt::entity` is 4 bytes followed by 4 bytes of padding before the `Level*`, so a `bool` would
slot in free.

***Rejected: a bit in `MetadataComponent`.*** This one lives on the row, so it is defensible — the
aliasing problem above does not apply. Four reasons the tag wins, strongest first:

1. **A bool does not compose with `entt::exclude`.** `exclude` filters on *pool membership*, so it
   cannot see a field inside a component. Every gameplay-visible pass would instead need a manual
   `if (metadata.PendingDestroy) continue;` in its loop body — the O(N)-edits-per-feature problem from
   §9, except now **unfixable by the `GetLiveEntitiesWith` seam**, because there is nothing to bake into
   the view. This is the decisive reason.
2. **The tag pool *is* the worklist.** With the tag, the reap on a step where nothing died is a `size()`
   check on an empty pool. With a bool the reap must either scan every entity every step to discover
   that nothing died, or maintain a parallel `std::vector<entt::entity>` — a second source of truth that
   can disagree with the bool. The tag yields the flag and an ordered list in one structure (entt packs
   storage in insertion order, so the list is in mark order).
3. **`MetadataComponent` is serialized.** `LevelSerializer::SerializeEntity` writes it
   (`LevelSerializer.cpp:104-113`). Pending-destroy is transient runtime state, and putting it in the one
   component guaranteed present on every entity *and* written to disk is how it eventually reaches a
   `.rdlvl` and a level loads full of corpses (playbook §3). `TransformSnapshotComponent` is the existing
   precedent: runtime-only state gets its own component rather than riding in an authored one.
4. **`IsValid()` becomes hot, so it should touch the smallest structure.** `MetadataComponent` is a UUID
   plus a `std::string` plus a bool — roughly 48 bytes per entity, pulled into cache to read one bit. An
   empty type allocates **no value array at all**; verified in the vendored copy at `entt.hpp:11750`,
   `page_size = !std::is_empty_v<T> * ENTT_PACKED_PAGE`, which is 0 for an empty type.

*Where the bool would genuinely win:* one fewer type in `Components.h`, and marking never allocates,
whereas the first `emplace<PendingDestroyComponent>` in a level allocates the pool's sparse pages. That
cost is one-time (entt sparse sets do not shrink) and is noise beside the hash-map erase and two
component removals already in the same call.

**Nothing iterates a corpse — we do not accept a visible zombie.** The story asks this explicitly.
Unreal does not accept one either: `UWorld::RemoveActor` nulls the level's actor slot *inside*
`DestroyActor`, so the actor is out of the level's enumeration before the frame continues. Our
equivalents: the UUID map entry is erased **at the mark** (so `GetEntityByUUID` and
`DestroyEntity(UUID)` naturally stop finding it), and every gameplay-visible view excludes the tag. The
zombie exists only in memory, reachable only through a raw handle someone kept — and that handle
answers `false`. Playbook §4's rule about one guard per layer applies: the exclusion belongs in *one*
place (a `GetLiveEntitiesWith` helper, §9), not sprinkled per view.

**Applicable playbook sections:** §1 (record now, drain at one defined point), §2 (ownership,
generation handles), §3 (data-only components — why the tag rather than a metadata bit), §4 (one
resolver per layer; eager-vs-deferred verb semantics; logging discipline), §8.8 (never destroy while
iterating — this story is the systemic answer to it), §10 (the gameplay seam and its contracts).

---

## 2. UE Reference

Three snippets carry the design. All paths are relative to `C:\dev\HNDREDGAMES\UE_5_7_4`.

**(a) The mark, and why a double-destroy is not an error.**
`Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:3301`

```cpp
inline bool IsPendingKillPending() const
{
    return bActorIsBeingDestroyed || !IsValidChecked(this);
}
```

and its first use, `Engine/Source/Runtime/Engine/Private/LevelActor.cpp:856`:

```cpp
// If already on list to be deleted, pretend the call was successful.
// We don't want recursive calls to trigger destruction notifications multiple times.
if (ThisActor->IsPendingKillPending())
{
    return true;
}
```

*We adopt this exactly.* Destroying an already-pending entity is a silent no-op, not a warning — the
second call is asking for a state that already holds. It is also what makes the mark safe to call
re-entrantly, which matters the moment a script's `OnDestroy` destroys something that destroys it back.

Note *which* of Unreal's two pending states this is, because they are easy to conflate. The
**object-level** one (UE4's `IsPendingKill`) is gone in UE5, replaced by `MarkAsGarbage()` +
`IsValid(Object)`; it exists because a tracing collector frees memory on its own schedule, so an
unbounded number of raw `UObject*` may outlive the object — UE even used to run a pass nulling every
such pointer, now the optional `gc.GarbageEliminationEnabled` (`ObjectBaseUtility.cpp:183-187` still
warns on the old `gc.PendingKillEnabled` key). The **actor-level** one above kept its name because it
means something narrower: `bActorIsBeingDestroyed` is "`Destroy()` was called and teardown is in
progress". That narrower one is what we are copying. Radiant has no object graph and no collector, so
our pending window is *bounded* — at most one fixed step, ended by exactly one function.

**(b) The three-part teardown, in order.** `LevelActor.cpp:1033-1058` — the tail of `DestroyActor`:

```cpp
    // Remove the actor from the actor list.
    RemoveActor( ThisActor, bShouldModifyLevel );
    ...
    // Clean up the actor's components.
    ThisActor->UnregisterAllComponents();

    // Mark the actor and its direct components as pending kill.
    ThisActor->MarkAsGarbage();
```

*We adopt all three and the order.* `RemoveActor` → our `m_EntityMap.erase`. `UnregisterAllComponents`
→ our eager removal of the collider and rigidbody components (whose entt `on_destroy` signals tear down
Box2D). `MarkAsGarbage` → our tag emplace, last. The eager-physics claim is not inferred from the name:
`UnregisterAllComponents` reaches `UActorComponent::ExecuteUnregisterEvents()`
(`Engine/Source/Runtime/Engine/Private/Components/ActorComponent.cpp:2476`) whose body opens with
`DestroyPhysicsState();`.

**(c) Removal from enumeration is immediate, and is a null rather than a compaction.**
`Engine/Source/Runtime/Engine/Private/World.cpp:2802`

```cpp
CheckLevel->Actors[ActorListIndex] = nullptr;
```

Worth reading twice, because it is the one place our storage is *better* and the difference explains
why this story is needed at all. Unreal can afford to remove an actor from a live array mid-iteration
because it **nulls the slot instead of compacting** — every other index stays put, so an iterator
walking that array survives. `entt` compacts: destroying an entity **swap-and-pops** its component out
of the dense array, moving an unrelated entity into the hole under any live iterator (playbook §8.8).
Unreal pays for its safety with a permanently sparse array it has to periodically compact and with null
checks on every walk; we pay for our density with the rule that you must not destroy while iterating.
Deferring the reap is how we keep the density and lose the rule.

**What we deliberately simplify:** no reference graph, no `AddReferencedObjects`, no collection pass, no
`bNetTemporary` / role checks / replication destruction records, no editor undo (`Modify()`), no
`FSetActorWantsDestroyDuringBeginPlay` deferral for destroys issued during `BeginPlay`. Radiant has one
reap point where Unreal has a GC that runs on its own schedule, and our scale (single-digit to
low-hundreds entities, one thread, no replication) does not buy anything back from that machinery.

---

## 3. File Plan

```text
Radiant/Source/Radiant/ECS/
    Components.h          (modify) — add PendingDestroyComponent (empty tag, runtime-only);
                                     add the IsEngineComponent trait + its two specialisations
    Level.h               (modify) — lifecycle contract rewritten; IsPendingDestroy,
                                     ReapDestroyedEntities, GetLiveEntitiesWith declared
    Level.cpp             (modify) — DestroyEntity becomes the mark; the reap; views excluded;
                                     DispatchContactEvents resolves once per event
    Entity.h              (modify) — IsValid()/Destroy() contracts restated
    Entity.cpp            (modify) — IsValid() consults the pending marker
    EntityTemplates.h     (modify) — static_assert(!IsEngineComponent<T>) in the three
                                     mutating accessors (the engine-private gate)
Radiant/Source/Radiant/Gameplay/
    EntityBehaviour.h     (modify) — DELETE the two "must be the last statement" caveats
    GameplayLevel.h       (modify) — DELETE the same caveat on DestroyEntity
Radiant/Source/Radiant/Serialization/
    LevelSerializer.cpp   (verify) — confirm the tag is never written (it has no serializer entry)
Docs/
    ECS-And-Levels.md     (modify) — lifecycle section, component table, the caveat, Known Issues
    Gameplay-Framework.md (modify) — the hook-contract caveat
.claude/references/
    radiant-playbook.md   (modify) — §8.8 gains the systemic answer; §1 gains the reap point

Reaper/Source/                       (SEPARATE COMMIT — see Phase 5)
    CollisionLogger.h     (modify) — destroy is no longer the last statement
    SpawnProbe.h          (modify) — ProbePassenger gains self-destruct-mid-OnUpdate
    Layers/GameLayer.cpp  (modify) — K cheat key
```

| Action | Path | Description |
|--------|------|-------------|
| Create | `ECS/Components.h` (addition) | `PendingDestroyComponent` — the condemned notice |
| Modify | `ECS/Level.h` / `Level.cpp` | mark / reap split, live views, dispatch collapse |
| Modify | `ECS/Entity.h` / `Entity.cpp` | `IsValid()` becomes pending-aware |
| Modify | `Gameplay/EntityBehaviour.h`, `Gameplay/GameplayLevel.h` | remove the three caveats |
| Modify | `Docs/`, playbook | contract of record follows the code (CLAUDE.md doc contract) |
| Modify | `Reaper/Source/*` | verification scaffolding, own commit, enrolled in RAD-92 |

---

## 4. Type Design

### PendingDestroyComponent

- **Kind:** empty `struct` (entt tag component)
- **Responsibility:** marks one entity as destroyed-but-not-yet-freed. Nothing else. It carries no data
  on purpose — a reason string or a timestamp would make it a value type and cost the pool a dense
  array.
- **Ownership:** the `Level`'s registry owns it, like every component. No allocation, no destructor.
- **Lifetime & threading:** exists from the mark until the reap — at most one fixed step. Main-thread
  only, like the rest of the ECS.
- **Key members:** none. `struct PendingDestroyComponent {};`
- **Playbook patterns:** §3 (runtime-only, never serialized — the `TransformSnapshotComponent`
  precedent); §2 (owns nothing).
- **Naming note:** *not* `PendingKillComponent` and *not* `GarbageComponent`, and the reason is worth
  keeping. `UObject::IsPendingKill()` **no longer exists in UE5** — it was replaced by `MarkAsGarbage()`
  (`UObjectBaseUtility.h:182`) plus the free function `IsValid(const UObject*)` (`Object.h:1875`), and
  the old name survives only as a literal alias, `RF_InternalPendingKill = RF_MirroredGarbage`
  (`ObjectMacros.h:603`), beside a `UE_DEPRECATED(5.4, "... Use IsValid(Object) instead.")` on its
  sibling. So `PendingKill` is retired vocabulary. `Garbage` is the live term, but it implies a
  collector, and this story's first acceptance criterion is that there is no collector.
  `PendingDestroy` names the exact fact: `Destroy()` was called and has not completed — which is also
  precisely what UE's *actor-level* `bActorIsBeingDestroyed` means, and that is the concept we are
  actually copying (see §2a).

### `IsEngineComponent<T>` — the engine-private gate

- **Kind:** type trait (`std::false_type` primary template, explicit `true_type` specialisations)
- **Responsibility:** names the components game code must never add, replace or remove by hand.
- **Why it exists:** `Radiant.h:31` exports `ECS/Components.h` — it must, since games need
  `SpriteComponent`/`TransformComponent`/`NativeScriptComponent` — and `Entity::AddComponent<T>` is
  public and unconstrained. So today there is **no membrane at all** around component types.
  `TransformSnapshotComponent` has carried that exposure since RAD-25 and is largely harmless
  (self-healing: the snapshot pass overwrites it on movers and strips it from non-movers every step).
  `PendingDestroyComponent` is not harmless — a hand-added tag is a **destroy without the mark's
  bookkeeping**: `OnDestroy()` never fires, so a script's level-wide collision callback is never
  unregistered and the next contact calls into a freed instance (the use-after-free `GameplayLevel.h`
  already warns about); the ordered collider-then-rigidbody teardown that lets the `on_destroy`
  handlers still read `MetadataComponent`/`TransformComponent` is skipped; and the UUID map entry is
  never erased.
- **Specialisations:** `PendingDestroyComponent`, `TransformSnapshotComponent`.
- **Cost to the engine: zero.** `Level`'s own paths never go through `Entity`'s accessors — the mark
  calls `m_Registry.emplace<PendingDestroyComponent>` and the snapshot pass calls
  `m_Registry.emplace_or_replace<TransformSnapshotComponent>` directly. The gate closes the one door
  game code has, and does it at compile time with a message pointing at `Entity::Destroy()`.
- **UE reference:** the same shape, one level down. `bActorIsBeingDestroyed` is `private`
  (`Actor.h:631`), public to *read* via `IsPendingKillPending()` (`:3301`), and writable only through
  `FMarkActorIsBeingDestroyed` (`:4677`) — a struct whose **constructor is private** with
  `friend UWorld`, so nobody but `UWorld` can even construct it. Readable by everyone, writable by one
  class. We reach the same guarantee with a trait because our leak is one universal setter rather than
  one field.

### Level — new private members

| Member | Purpose |
|--------|---------|
| `bool IsPendingDestroy(entt::entity) const` | The one predicate. `registry.valid()` is a separate question and stays separate — composing them is `Entity::IsValid()`'s job, not this one's. |
| `void ReapDestroyedEntities()` | The 6pm demolition. Called from exactly one place. |
| `std::vector<entt::entity> m_ReapList` | Reusable snapshot buffer for the reap, same reasoning as `m_ScriptUpdateList`: a member so the pass allocates nothing once capacity settles, and it must be walked as plain ids rather than as a live view (playbook §8.8). |
| `template<typename... C> auto GetLiveEntitiesWith()` | `view<C...>(entt::exclude<PendingDestroyComponent>)`. The default for every gameplay-visible pass; `GetAllEntitiesWith` survives as the deliberate exception (see §9). |

### Entity — changed contract, no new members

`IsValid()` becomes `registry.valid(handle) && !m_Level->IsPendingDestroy(handle)`. Nothing else on
`Entity` changes shape — which is the point. Every `if (entity)`, every `operator bool`, every
`TryGetComponent`/`HasComponent` assert and `GetBehaviour<T>`'s leading validity check inherits the new
answer without an edit.

---

## 5. Implementation Steps

### Phase 1 — The notice

- [x] **Add `PendingDestroyComponent`** to `Components.h` with a doc comment stating: empty tag,
      runtime-only, never serialized, present only between a `Destroy()` and the reap, and that
      `Level::Copy()` (RAD-52) must skip entities carrying it. Add it to `Docs/ECS-And-Levels.md`'s
      component table with the runtime-only marker, beside `TransformSnapshotComponent`.
- [x] **Add the `IsEngineComponent<T>` trait** to `Components.h` (primary `false_type`, specialised
      `true` for `PendingDestroyComponent` and `TransformSnapshotComponent`), and a
      `static_assert(!IsEngineComponent<T>::value, ...)` in `Entity::AddComponent`,
      `AddOrReplaceComponent` and `RemoveComponent` (`EntityTemplates.h`). Message points the caller at
      `Entity::Destroy()`. `RemoveComponentIfExists` inherits it through `RemoveComponent`. Verify the
      engine still builds untouched — `Level`'s mark and snapshot passes go through `m_Registry`, not
      through `Entity`, so nothing legitimate trips it.
- [x] **Add `Level::IsPendingDestroy(entt::entity)`** — one line, private.
- [x] **Make `Entity::IsValid()` consult it** (`Entity.cpp:25`). One `&&`. Everything downstream is free.
- [x] **Split `DestroyEntity` into the mark.** New body, in this order, and the order is load-bearing:
      1. registry-invalid handle → `WARN` and return (today's behaviour, unchanged);
      2. already pending → **silent** return (UE's `IsPendingKillPending` early-out — a double destroy
         is idempotent, not a mistake);
      3. **already being marked → silent return** (the recursion guard — see the correction below);
      4. push the handle onto `m_MarksInProgress` under an RAII `MarkScope`, then `OnDestroy()` on the
         behaviour instance if one exists — but **do not delete it**;
      5. `RemoveComponentIfExists<BoxCollider2DComponent>()` then `<RigidBody2DComponent>()` — collider
         first, unchanged reasoning: a destroyed body destroys its own shapes inside Box2D, so
         body-first hands the collider handler a stale ticket;
      6. `m_EntityMap.erase(id)`;
      7. `emplace<PendingDestroyComponent>` **last** — steps 4–6 use `Entity`'s checked accessors (and
         so do the physics `on_destroy` handlers), which assert on `IsValid()`, and the tag is what
         makes `IsValid()` false;
      8. **no** `SortEntities()` — it moves to the reap.

      **CORRECTION, found while implementing (2026-08-26).** The order originally planned here —
      `OnDestroy` at step 3, tag last — allows **infinite recursion**: a behaviour whose `OnDestroy`
      calls `GetOwner().Destroy()` re-enters `DestroyEntity`, is not yet tagged, and runs `OnDestroy`
      again forever. UE hit this and guards it explicitly, `LevelActor.cpp:908`:
      `// Prevent recursion` / `FMarkActorIsBeingDestroyed MarkActorIsBeingDestroyed(ThisActor);` —
      set **before** `Destroyed()`.
      But the tag cannot simply move to the top, because it is also what `IsValid()` reads, and
      `OnDestroy` must still be able to read its own entity ("spawn an effect where I died" is the
      ordinary case, and the physics `on_destroy` signal handlers reach the entity through checked
      accessors too). UE's resolution is that these are **two separate flags** —
      `bActorIsBeingDestroyed` set early for recursion only, and the garbage flag set at the very end,
      which is why `IsValid(Actor)` is still true throughout `Destroyed()`. We adopt the same split:
      `m_MarksInProgress` (a small LIFO vector, scanned linearly, non-empty only while a mark is on the
      stack) is the recursion guard; `PendingDestroyComponent` remains the death marker and stays last.
      Locked as **decision D4** in §10.

### Phase 2 — 6pm

- [x] **Write `Level::ReapDestroyedEntities()`**: snapshot the tag view into `m_ReapList` (clear +
      reserve + push, mirroring the script pass), then for each id: delete the behaviour instance via
      `NativeScriptComponent::DestroyScript` (or `delete` if unbound), then `m_Registry.destroy(id)`.
      Then **one** `SortEntities()`, only if anything was reaped. Use `m_Registry` directly throughout —
      `Entity`'s accessors assert on validity and these entities are, by construction, invalid.
- [x] **Call it from `OnFixedUpdate`**, as the last statement, **outside** the `if (m_PhysicsWorld)`
      block so scratch levels reap too.
- [x] **Rework `~Level`**: mark every entity (the existing loop, which is now non-mutating with respect
      to the pool it iterates — a `MetadataComponent` view is untouched by adding a tag to a different
      pool), then reap in a `while (anything pending)` loop. Teardown is not a hot path, and a
      behaviour's `OnDestroy` chaining into another destroy must not strand an entity whose `OnDestroy`
      never runs. This also retires half of RAD-81's `~Level` finding.

### Phase 3 — Nothing iterates a corpse

- [x] **Add `GetLiveEntitiesWith<...>()`** beside `GetAllEntitiesWith`, documented as the default, with
      `GetAllEntitiesWith` re-documented as "includes pending-destroy entities — the reap and teardown
      only".
- [x] **Migrate every gameplay-visible pass** to it: the snapshot pass's three mover views and its
      stale-snapshot sweep, the script-set snapshot, `OnRender`'s camera and sprite views,
      `FindEntityByName`, `GetAssetList`, `OnViewportResize`. The script pass's existing
      `m_Registry.valid(handle)` re-check becomes a live check (an entity can be marked *during* the
      pass, which the view snapshot cannot know about).
- [x] **Leave the physics signal handlers alone** — they run during the mark, before the tag lands.

### Phase 4 — Collapse the defences

- [x] **`DispatchContactEvents`: resolve each side once per event.** Hoist `GetEntityByUUID` for A and B
      above the callback loop; keep the "both gone" skip; delete the per-callback re-resolution and the
      mid-loop `break`. Rewrite the comment to explain *why* one resolution is now enough (validity is
      re-asked at every use, so a destroy inside callback 2 changes what callback 3's handle answers).
      Keep the per-iteration `Active` check — that is about callback removal, a different hazard this
      story does not touch.
- [x] **`NotifyScript`**: the "read `Instance` before the call, never touch it after" dance can go — the
      instance now outlives the call by construction. Keep the live check on `entity` itself, since an
      earlier callback may have marked it.
- [x] **Delete the three caveats**: `EntityBehaviour.h:161-165`, `:181-184`, `GameplayLevel.h:82-85`.
      Deleted, not reworded (AC). Replace with the positive contract: destroying any entity, including
      your own, is safe from any hook and the method runs to completion.
- [x] **Rewrite `Level.h`'s lifecycle contract** (`:26-31`) — creation immediate, destruction deferred,
      the reap point named, and what is eager (physics) vs deferred (storage, instance).
- [x] **Update the move-drain comment and its assert** (`Level.cpp:602-611`): the invariant now holds
      for a stronger reason — a marked entity's body is already gone, so it cannot produce a move event,
      and its map entry is already erased. State that the assert survives *because* of the eager physics
      teardown.

### Phase 5 — The record, and the proof

- [x] **Docs (same change, per the CLAUDE.md doc contract):** `Docs/ECS-And-Levels.md` — the lifecycle
      paragraph, the component table, the temporary-caveat paragraph, and the "Immediate destruction
      costs every caller a guard (RAD-97)" Known Issues entry (which becomes a resolved design note).
      `Docs/Gameplay-Framework.md:214` — the hook caveat.
- [x] **Playbook:** §8.8 gains the systemic answer (deferred reap is why the rule no longer needs a
      per-caller guard for *destruction*, while the add-during-iteration half stands); §1 gains entity
      reaping as the third "one defined point" alongside event processing and contact dispatch.
- [x] **File the RAD-92 scope amendment BEFORE the scaffolding lands** (the rule that card exists to
      enforce), covering the `K` cheat key and `ProbePassenger`'s self-destruct mode.
- [x] **Reaper scaffolding, in its OWN commit**, each piece carrying a `retires with RAD-92` comment:
      `CollisionLogger::OnCollisionBegin` gets work *after* `other.Destroy()` (a log line reading
      `other.IsValid()`, which must print false); `ProbePassenger` gains `RequestSelfDestruct()` whose
      `OnUpdate` calls `GetOwner().Destroy()` and then keeps running — logging, reading its own members,
      and logging `GetOwner().IsValid()` — before returning normally; `K` in `GameLayer::OnKeyPressed`
      requests it.

---

## 6. Ownership & Lifetime Strategy

Nothing changes hands. What changes is *when* one owner lets go.

- **The `Level` owns entity and component storage** (via the private `entt::registry`), before and
  after. The tag is a component like any other and dies with its entity.
- **The `NativeScriptComponent` owns the behaviour instance** by an owning raw pointer (playbook §2,
  §10 — this is why the accessor is named `GetOwner()`). Today `DestroyEntity` performs that delete;
  after this change **the reap** performs it. That is the entire fix for the "last statement" wart: the
  owner does not release until the street is empty. RAD-30 moves this pointer to a Level-owned side
  table, and when it does, the reap is already the natural home for the release.
- **Box2D bodies and shapes** are released at the *mark*, through the unchanged entt `on_destroy`
  signals. Component presence stays the physics binding (playbook §4).
- **The UUID map entry** is released at the mark. It is the level's directory, and Unreal strikes the
  directory entry inside `DestroyActor` for the same reason.
- **`Entity` handles own nothing** and continue to. The window between mark and reap is a window in
  which a handle names a row that exists but is dead; `IsValid()` is what reports that, and it is the
  only thing that needed to learn about it.

**The lifetime hazard to watch:** between the mark and the reap, the behaviour instance is alive but its
entity is invalid. Anything holding a raw `EntityBehaviour*` from before the mark can still call into
it. `Entity::GetBehaviour<T>()` cannot hand one out (it checks `IsValid()` first —
`EntityTemplates.h:96-100`), and its documented contract already forbids storing the pointer across a
fixed step. The one real path is a level-wide collision callback that captured `this`: it stays
registered until the script's `OnDestroy` removes it, which is a large part of why `OnDestroy` runs at
the **mark** and not at the reap (§10, locked decision D1).

---

## 7. Performance Notes

**Removed, and it is the largest single item:** `SortEntities()` is `O(n log n)` over the metadata pool
and currently runs **once per destroyed entity**. Destroying 50 projectiles in one step is 50 sorts
today and 1 after this change. Even at Reaper's scale the current behaviour is the most expensive thing
in the destroy path by an order of magnitude.

**Removed:** `DispatchContactEvents`'s per-callback double re-resolution. With `C` registered level-wide
callbacks and `E` contact events the map lookups go from `2·C·E` to `2·E` — for 5 callbacks and 12
events, 120 → 24 per step.

**Added:** one sparse-set `contains` inside `Entity::IsValid()`. This is the real cost, because
`IsValid()` sits behind every `operator bool`, every `TryGetComponent`/`HasComponent` assert in Debug
and Release, and `GetBehaviour<T>`'s leading check. A `contains` on an `entt` tag pool is a page index
plus one array read — two dependent loads, no allocation, no branch on the common path.
*Rejected micro-optimisation:* caching a `bool m_HasPendingDestroys` to short-circuit the pool check. It
buys two loads and introduces a flag that can desync from the pool, which is exactly the failure class
§4's one-resolver-per-layer rule exists to prevent. Revisit only with a profile.

**Added:** `entt::exclude<PendingDestroyComponent>` on the gameplay-visible views. The per-entity cost is
the same `contains`. The one that runs at frame rate rather than step rate is `OnRender`'s sprite view;
two loads per sprite against a batched quad submission is not measurable.

**Added:** the reap pass itself. On a step where nothing died it is a `size()` check on an empty pool —
effectively free. On a step where things died it is one walk over exactly the entities that died, into a
member vector that stops allocating once capacity settles (the `m_ScriptUpdateList` pattern).

**No new per-frame heap allocations, no GPU resource churn, no new hot-path `std::function`
indirection.**

---

## 8. Logging & Diagnostics

Following playbook §4's rule — *rare and discontinuous verbs log; routine and continuous verbs do not* —
entity destruction is discontinuous but potentially very frequent (a projectile game destroys dozens per
second), so **the happy path stays silent**:

- **Mark, invalid handle:** keep today's `RADIANT_WARN("Level: DestroyEntity called with an invalid
  entity handle")`. A destroy on a garbage handle is still a caller mistake.
- **Mark, already pending:** **silent.** This is the documented idempotent case (UE's early-out); a
  warning here would fire during correct gameplay — two systems independently deciding the same enemy is
  dead in one step is normal, not a bug. This mirrors §10's rule that a *query* answers where a *verb*
  warns: "already dead" is an answer.
- **Reap:** no per-entity log, no per-step log. `RADIANT_PROFILE_FUNCTION()` only, matching
  `DispatchContactEvents`.
- **`~Level` reap loop:** if a pass produces new pending entities, `RADIANT_WARN` naming the count — a
  behaviour spawning during teardown is a genuine misconfiguration worth a line, and the loop still
  converges.
- **Asserts:** the move-drain assert (`Level.cpp:610`) stays, with its comment rewritten to name the
  eager physics teardown as the reason it still holds. Add one assert in the reap: every id in
  `m_ReapList` is still `registry.valid()` at the moment it is destroyed — a failure means something
  freed an entity outside `DestroyEntity`, which is a programmer error, not content.
- **Verification evidence comes from Reaper, not the engine** (Phase 5), which is the established split:
  the engine stays quiet, the scaffolding logs what it expects beside what it got (the RAD-100 probe
  convention).

---

## 9. Scalability Review

**Flag 1 — the exclusion is a per-view edit, and a forgotten one is silent.** Adding
`entt::exclude<PendingDestroyComponent>` to seven views today is fine; the failure mode is the *eighth*
view, written in six months, that forgets it and quietly iterates corpses. That is an
O(N)-edits-per-feature pattern, and review would not necessarily catch it because the code looks correct.

*Fix, now:* make the safe thing the default. `GetLiveEntitiesWith<C...>()` bakes the exclusion in and
becomes the only spelling used by gameplay-visible passes; `GetAllEntitiesWith` stays for the reap and
teardown and is re-documented as the deliberate exception. A new pass then gets correctness by reaching
for the obvious name, and the two remaining raw uses are conspicuous enough to review. This is playbook
§4's "one resolver per layer" applied to iteration.

**Flag 2 — the reap deletes the behaviour instance by hand, which is per-runtime-state-kind work.**
Today there is exactly one kind of runtime side state (the script instance). Add a second — an ability
system instance, an animation state — and the reap grows a second bespoke delete, and so does `~Level`,
and so does anything else that frees an entity.

*Fix, later — this is RAD-30, already filed.* When runtime state moves to Level-owned side tables keyed
by entity, the reap's per-kind deletes collapse into one loop over the registered side tables. Do not
build that abstraction now for a single customer (the rule-of-three; playbook §9's stance on speculative
structures). Note it in the reap's doc comment so the next kind meets a decision rather than a precedent.

**Flag 3 — one reap point assumes one simulation cadence.** If a Level ever steps at a different rate
from another Level, or a future editor tick drives entities outside `OnFixedUpdate`, "end of the fixed
step" needs a second home.

*Assessment:* not now. Each Level owns its own world and its own `OnFixedUpdate` (playbook §4), so "end
of *this* Level's fixed step" already scales to any number of Levels at any number of cadences — the
reap is per-Level by construction. The genuine gap is a Level that is *never* stepped (a paused level, or
an editor level before play). Its pending entities linger until teardown; they are invalid and invisible
the whole time, so the cost is memory, not correctness. Document it in `Level.h`.

**Flag 4 — the engine-private gate is a hand-maintained list.** `IsEngineComponent<T>` (§4, decision D3)
needs a new specialisation every time a runtime-only component is added, and forgetting one is silent:
the component simply stays writable by game code. Two of two are covered today, so the list is complete
and short.

*Fix, later — this is the taxonomy question D3 deliberately scoped out.* The real answer is that
"engine-owned runtime state" should not be a *component* at all: RAD-30's Level-owned side tables are
exactly the mechanism that makes it unreachable by construction rather than by trait, because game code
has no way to name a side table. Until then, the trait is the cheap enforcement and the three-site
contract in `Docs/ECS-And-Levels.md` ("declare in `Components.h`, add to `LevelSerializer`, add to the
inspector") gains a fourth site: *decide whether it is engine-private, and specialise the trait if so*.
Add that line to the doc in this story so the next component meets the decision.

**Already scalable, no change needed:** the marker is a registry-wide mechanism, so a new component type
costs the destruction path zero edits, and a new *entity* costs it zero code. That is the shape we want,
and is why this is a tag rather than a per-type teardown list.

---

## 10. Risks & Edge Cases

**Two decisions, locked 2026-08-26 during the plan walkthrough. Do not relitigate.**

- **D1 — `OnDestroy()` runs at the MARK.** It fires inside `DestroyEntity` exactly as it does today;
  only the `delete` moves to the reap. Three reasons: it preserves today's semantics, so the delta is
  purely "the free is deferred" — smaller and more auditable; it matches Unreal, where `Destroyed()`
  fires inside `DestroyActor` before `MarkAsGarbage()`; and it has strictly better gameplay semantics,
  because a script that unregisters its level-wide collision callback in `OnDestroy` stops receiving
  events *immediately* rather than continuing to be called as a logically-dead subscriber for the rest
  of the batch. The wart it carries — a script destroying itself gets `OnDestroy` re-entrantly while
  `OnUpdate` is still on the stack — is safe, because nothing is freed, and is exactly Unreal's
  behaviour. *Rejected:* at the reap, tidier as a one-sentence contract ("everything about dying happens
  at one moment") but it leaves a dead subscriber live for the rest of the step and hands the reap
  arbitrary gameplay to run.
- **D2 — an entity marked *during* the reap is reaped on the NEXT step.** Single pass; no pass cap, no
  livelock, no warning path. The entity is invalid and invisible to everything from the instant it was
  marked, so "reaped one step later" is a statement about memory, not about behaviour. *Rejected:*
  looping until the pending set is empty, which needs a cap plus a `WARN` to survive two scripts
  destroying each other's spawns forever — and whose failure path has to fall back to "next step"
  regardless. `~Level` is the deliberate exception and does loop, because at teardown an unreaped entity
  means an `OnDestroy` that never runs.
- **D3 — the engine-private component gate lands IN THIS STORY, minimally.** A `IsEngineComponent<T>`
  trait plus a `static_assert` in `Entity`'s three mutating accessors (§4), covering
  `PendingDestroyComponent` and `TransformSnapshotComponent`. Rationale: this story introduces the first
  genuinely dangerous engine-private component, and playbook §10's own precedent is that a rule with a
  hole in it on day one is a rule nobody trusts — which is why `Entity::RefreshCollider` landed in the
  same change that created the dividing rule. The guard ships with the danger. *Rejected:* deferring
  enforcement entirely to a follow-up card (leaves a documented-but-unenforced backdoor), and splitting
  `Components.h` into public/internal headers (`Level.h` still needs `PendingDestroyComponent` visible
  for `entt::exclude`, so it reduces discoverability without adding enforcement over the gate).
  **Deliberately NOT in scope:** the broader taxonomy of which components are game-facing API at all —
  that is component-hygiene design and belongs with RAD-30 / RAD-94. File it as its own card.
- **D4 — the recursion guard is a SECOND state, separate from the death marker** (locked 2026-08-26
  during implementation; see the correction in §5 Phase 1 for how it was found). `m_MarksInProgress` —
  a small LIFO vector of the entities whose `DestroyEntity` is currently on the stack, scanned
  linearly, non-empty only while a mark is executing — is pushed before `OnDestroy` and popped when the
  mark returns. `PendingDestroyComponent` still lands last. They cannot be one flag: the recursion
  guard must be set *before* `OnDestroy`, and the death marker must be set *after* it, because
  `OnDestroy` has to be able to read its own entity — "spawn an effect where I died" is the ordinary
  case, and the physics `on_destroy` handlers reach the entity through checked accessors as well. UE
  splits exactly this pair for exactly this reason (`bActorIsBeingDestroyed`, set with the comment
  `// Prevent recursion` at `LevelActor.cpp:908`, versus the garbage flag set at `:1054`), which is why
  `IsValid(Actor)` remains true throughout `Destroyed()`. *Rejected:* one tag emplaced first, which
  makes `OnDestroy` run against an entity whose every checked accessor asserts.
- **Note the coupling, since it is why D2 is cheap.** D1 landing on *the mark* means the reap executes
  **no gameplay code at all** — it deletes instances and destroys rows. Nothing can become newly-pending
  during a reap except through a behaviour *destructor* performing gameplay, which is pathological. D2
  is therefore a defined corner rather than a hot path, and single-pass costs nothing real.

**Risks and edge cases:**

- **A mark outside a fixed step.** A cheat key, a UI action, or a destroy while the game is paused marks
  an entity that will not be reaped until the next step — possibly never, if the level is never stepped
  again. Mitigation: it is invalid and excluded from every view from the instant of the mark, so it is
  invisible; `~Level` collects it. Document the window in `Level.h`.
- **`SortEntities` between mark and reap would assert.** Its comparator resolves each metadata ID through
  `m_EntityMap` and asserts on a miss (`Level.cpp:864`), and the mark erases the map entry. Mitigation:
  the reap is the only caller, and it sorts *after* destroying. Leave a comment at the sort saying it
  must run with no pending entities outstanding.
- **The tag must never be serialized.** `LevelSerializer` writes only components it has an explicit entry
  for, so absence of an entry is already correct — but verify it explicitly and add the tag to the
  documented runtime-only exception list beside `TransformSnapshotComponent`, or a future "serialize
  everything" refactor writes a condemned notice to disk and loads a level full of corpses.
- **`Level::Copy()` (RAD-52, play-in-editor) must skip pending entities.** Not in scope; note it on the
  tag's doc comment so the copy is written correctly the first time.
- **A `CreateEntityWithUUID` reusing a pending entity's UUID before the reap** would put two entities
  with the same metadata ID in the registry. Only the deserialization path can do this (`GameplayLevel`
  deliberately withholds `CreateEntityWithUUID`), and the existing duplicate-UUID `WARN` covers it. No
  new guard; note it.
- **`GetBehaviour<T>` on a pending entity returns null.** Correct and intended, but it changes an
  observable behaviour within the step: a collision handler that destroys `other` and then asks
  `other.GetBehaviour<Door>()` gets null where today it would get a dangling pointer that happened to
  work. This is the fix, not a regression — but it belongs in the story report, because it reads as a
  break if you were relying on the bug.
- **A behaviour that destroys its own entity twice in one hook** now takes the silent early-out rather
  than the invalid-handle `WARN`. Intended (D1/UE), but it means a genuine double-destroy bug is quieter
  than it was. Accepted: the alternative warns during correct gameplay, which is worse.

---

## 11. Verification (AC → proof)

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| `DestroyEntity` is safe from a collision handler, a script `OnUpdate`, and an entity destroying itself — none require "last statement" discipline | Three probes, one run: `C` (platform destroys the square from `OnCollisionBegin`, with a log line **after** the destroy), `N` (SpawnProbe destroys another entity from inside the script pass — unchanged), `K` (ProbePassenger destroys **itself** mid-`OnUpdate` and keeps running). All three complete; no crash in Debug, Release **or Dist** — Dist is where the current UB would actually bite. |
| `EntityBehaviour`'s "must be the last statement" caveat is **removed**, not reworded — all three instances (the story says `ScriptableEntity`; RAD-99 renamed the class) | Read the three original sites (`EntityBehaviour.h:161-165`, `:181-184`, `GameplayLevel.h:82-85`): each now states the positive contract instead. `grep -rn "last statement" Radiant/Source Reaper/Source Docs` must show the phrase **only in negated or historical form** ("no last-statement discipline", "need NOT be"), never as a requirement — a bare count would fail this criterion for the wrong reason. |
| `DispatchContactEvents` no longer needs per-callback re-resolution of both sides | Read the diff: one `GetEntityByUUID` pair per event, above the callback loop. The `C` cheat run still shows the platform's `OnCollisionEnd` arriving one step later with `<destroyed>` — the surviving side's late-notification path is unchanged. |
| Physics bodies are torn down eagerly; a destroyed entity generates no further contacts in the same step | The `C` cheat log: after `platform destroying Green Square`, no further `BEGIN`/`END` line in that step names the square. The move-drain assert (`Level.cpp:610`) does not fire, which is the same fact from the other side. |
| `Entity::IsValid()` returns false for a pending-kill entity | `CollisionLogger` logs `other.IsValid()` on the line *after* `other.Destroy()` — must print false. `ProbePassenger` logs `GetOwner().IsValid()` after destroying itself — must print false. Both in the same run. |
| `Level.h` and `Docs/ECS-And-Levels.md` state the new contract and the reap point | Read both: `Level.h`'s lifecycle block names mark/reap/eager-physics and the reap point; `ECS-And-Levels.md`'s lifecycle section and Known Issues entry updated; `Docs/Gameplay-Framework.md:214` caveat gone. `/review`'s doc-staleness check passes. |
| This is **not** a tracing collector — no reference graph, no `AddReferencedObjects` equivalent | Read the diff: one tag component, one predicate, one reap function walking one pool. No graph, no traversal, no root set. |
| Compiles clean in Debug/Release/Dist, no new warnings | CLI MSBuild all three configurations per CLAUDE.md's Build & Run block. |
| Reaper runs and is visually verified | Kareem's half: launch, play, press `C` / `N` / `K`, watch the square vanish, the probe cycle, and nothing draw after it dies. Claude's half: headless launch with redirected output and graceful `CloseMainWindow`, scanning the log for `[W]`/`[E]` — noting that a headless run stops at MainMenu and proves teardown, not gameplay. |
| Engine-private components cannot be written by game code (D3) | A deliberately-broken Reaper line — `entity.AddComponent<PendingDestroyComponent>()` — fails to compile with the trait's message, then is deleted. Confirm the engine itself builds untouched, proving `Level`'s own passes never went through `Entity`'s accessors. |
| Ownership documented for new types | `PendingDestroyComponent`'s doc comment states: owned by the registry, no allocation, runtime-only, never serialized, `Level::Copy` must skip it. The reap's doc comment states it is where the behaviour instance's owner releases, and that RAD-30 collapses that per-kind delete. |

**Additional test-plan items from the story:**

- [ ] The `C` cheat still works with the destroy no longer the last statement — covered above.
- [ ] A script destroys its own entity mid-`OnUpdate` and the method runs to completion — the `K` probe
      logs a line before the destroy, the destroy, and two lines after it, in order.
- [ ] An entity destroyed during a step generates no contacts for the remainder of that step — covered
      above.
- [ ] Config check: CLI MSBuild all three configurations.
