# Implementation Plan — RAD-29: Collision event queue with validity checks

| Field | Value |
|-------|-------|
| **Jira** | [RAD-29](https://hndredgames.atlassian.net/browse/RAD-29) |
| **Epic** | RAD-2 — Phase 2, Simulation Foundation |
| **Story status** | To Do |
| **Dependencies** | RAD-60 (Done), RAD-27 (Done), RAD-28 (Done) |
| **Planned** | 2026-07-29 |

---

## 0. The Problem, Ground Up

### What the engine does today

Nothing. That is the whole problem.

Radiant simulates physics correctly — crates fall, they land on platforms, they stack and go to sleep — but the simulation never *tells anyone* that two things touched. Gameplay code has no way to find out. Reaper cannot detect that the player landed, that a bullet hit a wall, that a pickup was collected. Every one of those is the same missing sentence: **"these two entities started touching."**

Here is the current fixed step, stripped to its bones:

```text
Level::OnFixedUpdate(fixedDelta):
    snapshot transforms          # for render interpolation
    run native scripts           # gameplay
    physics.Step(fixedDelta)     # Box2D advances the world
    drain move events            # "these bodies moved" -> ECS transforms
    # <- nothing here says "these bodies touched"
```

### What it used to do, and why that was worse

Before the Box2D v3 port (RAD-27), the engine *did* have collision notification, and its shape is worth understanding because it is the design this story exists to avoid.

A **callback** is a function you hand to somebody else so they can call it later, at a moment of their choosing. Box2D v2 took a callback object — a "contact listener" — and invoked it *from inside the physics step*, at the exact instant the solver noticed two shapes touching. Radiant stored those callbacks as `std::function` members on `RigidBody2DComponent`, one for begin and one for end.

That design fails three ways at once, and they compound:

**Failure 1 — you cannot change the world from inside the callback.** Box2D is walking its internal contact arrays when it calls you. The natural thing for a game to do on collision is destroy something ("bullet hits wall → delete bullet"). Destroying a body while Box2D iterates its own contact list is the classic use-after-free: the library is mid-walk over an array you just shortened. Box2D's own manual states the rule outright — *"You cannot create/destroy Box2D entities inside these callbacks"* (`Radiant/Vendor/box2d/docs/simulation.md`). Unity hit the same wall and solved it the same way: `Destroy()` is *deferred* to the end of frame precisely so `OnCollisionEnter` handlers can call it safely.

**Failure 2 — nobody validated anything.** The old `CollisionListener2D` read the entity out of the body's user data and invoked the stored `std::function` with no check that the entity still existed. A mid-destruction entity produced either an assert or silent undefined behaviour.

**Failure 3 — a `std::function` cannot be saved to disk.** A `std::function` is a chunk of executable code plus captured variables. There is no way to write that into a `.rdlvl` YAML file and read it back. Any component holding one breaks two engine rules simultaneously: components are plain serializable data (playbook §3), and `Level::Copy` — required for play-in-editor in Phase 5 — must be able to shallow-copy every component safely.

The v3 port deleted all of it. `CollisionListener2D` is gone, the `std::function` members are gone, and `Docs/Physics.md` records the gap in writing so nobody builds on it. **This story fills the gap properly.**

### One concrete failure, with real numbers

Picture Reaper with the old design, six months from now. Twelve bullets in flight, each a small dynamic body. In one fixed step the solver resolves them in its own internal order and finds four impacts. It calls our listener four times, from inside the step:

```text
b2World_Step
 └─ solver finds touching pairs
     ├─ listener(bullet#3, wall)   -> game deletes bullet#3   <-- world mutated mid-step
     ├─ listener(bullet#7, crate)  -> game deletes crate      <-- and again
     ├─ listener(bullet#9, crate)  -> crate is already gone; the entity handle
     │                                 in this event now points at freed memory
     └─ listener(bullet#3, floor)  -> bullet#3 is already gone
```

Two of the four notifications reference objects that a *previous notification in the same batch* destroyed. Nothing in the old code noticed. In Debug you get an assert; in Dist — where asserts compile out — you get a read of freed memory, which usually looks like nothing at all until it looks like a crash three seconds later in an unrelated system. This is the single hardest bug class in gameplay physics, and it is entirely structural: it exists because the notification happened *during* the walk instead of *after* it.

### The fix, as an everyday thing

Think of a **letterbox**.

Right now the physics engine is a courier who bangs on your door mid-delivery and waits, holding the rest of the mail, while you decide what to do — and if what you do is rearrange the furniture, the courier trips over it.

The fix: the courier drops everything through the slot and leaves. Later, when the delivery van is safely gone, you open the letterbox, read the letters one at a time, and rearrange as much furniture as you like. Rearranging cannot trip anyone, because nobody is standing in the hall any more.

Two rules make the letterbox work, and they are the whole story:

1. **Copy the letters out before reading them.** The courier's satchel gets reused for tomorrow's round. If you kept a letter *in the satchel* and read it tomorrow, you would be reading tomorrow's mail. So we copy each note into our own format the moment it arrives.
2. **Re-check each name as you reach it.** Letter #1 might say "the tenant in flat 3 moved out." By the time you reach letter #4, addressed to flat 3, that name is stale. You check the name against the current tenant list *at the moment you act on it* — not once at the start.

Rule 1 is the queue. Rule 2 is the validity check. The story title is literally these two rules.

### What Box2D v3 already gives us

Here is the good news, and it is the reason this card shrank: **v3 deleted contact callbacks from the library.** There is no listener interface to implement. The library records touches into arrays during the step and hands them over afterwards:

```c
// Radiant/Vendor/box2d/include/box2d/types.h
// "Events are used to collect events that occur during the world time step. These events
//  are then available to query after the time step is complete. This is preferable to callbacks
//  because Box2D uses multithreaded simulation.
//  Also when events occur in the simulation step it may be problematic to modify the world..."
```

Erin Catto rebuilt the API around the letterbox because v3 simulates on multiple threads, where a mid-step callback is not merely risky but meaningless — you would not know which thread you were on. We get the correct architecture handed to us; our job is the part Box2D cannot do for us: **translating shape ids into Radiant entities, checking those entities are still alive, and choosing where gameplay hears about it.**

We have already rehearsed this exact idiom once. RAD-28's move-event drain is the same three moves — copy out of Box2D's transient array, translate to engine types, act afterwards — for the question "which bodies moved." This story answers "which bodies touched" the same way. If the move drain makes sense to you, you already understand 70% of this.

### What we gain, and what we deliberately do not

**Gain:** gameplay can react to collisions at all, for the first time; handlers may destroy entities, teleport things, and spawn things without any restriction, because the physics step is long over by the time they run; and the notification path holds no `std::function`, so `RigidBody2DComponent` and `BoxCollider2DComponent` stay plain data and `Level::Copy` stays writable.

**Not a gain — say it plainly:**

- **No latency win.** Notifications arrive at the same simulation step they always would have. The work moved in *space* (out of the step, into the drain) — not in *time*.
- **No performance win over the v2 design.** Roughly the same amount of work, arranged safely. It is not free either: each event costs a couple of hash lookups. Both are noise.
- **End notifications for a destroyed entity arrive one step late.** That is Box2D's design, not ours (see §1, the double-buffered end array), and it is why validity checks are mandatory rather than defensive.
- **No collision *response* control.** We are reporting what happened, not filtering or modifying it. No collision channels, no filtering matrix, no contact modification. Those are separate systems and separate stories.

**What we have / what we're building, in two sentences.** Today physics simulates touches perfectly and tells nobody, because the only mechanism that ever told anyone did it from inside the step, where nothing is safe to change. We are building a letterbox: the step records touches, we copy them into engine-typed records the instant the step ends, and then — with the physics engine idle — we walk that list, re-checking each entity is still alive before we hand it to gameplay.

---

## 1. Architecture Decision

**The shape:** `PhysicsWorld2D::Step` drains `b2World_GetContactEvents` into a reusable buffer of engine-typed `ContactEvent` PODs — exactly the pattern `BodyMoveEvent` established in RAD-28 (playbook §4). `Level` then dispatches those records to gameplay in a dedicated pass inside `OnFixedUpdate`, after the move drain. Box2D types never cross the `Physics/` boundary; `Level` never sees a `b2ShapeId`.

**The five decisions inside that shape:**

**1 — The record identifies entities, not shapes.** Each body carries its entity UUID in `userData` (stamped at `CreateBody` since RAD-27, explicitly for this story). At drain time we walk shape → body → userData → `UUID`. UUIDs are the durable identity (`Entity` handles dangle; playbook §2), so the queue can safely outlive any handle. A side we could not resolve is recorded as `UUID(0)` — the same "none" sentinel `AssetHandle 0` already uses across the codebase.

**2 — Validity is checked three times, at three different moments, for three different reasons.** This is the load-bearing part of the story and the reason it is not a fifteen-line change:

| When | What is checked | Why it can fail |
|------|-----------------|-----------------|
| Translation (in `Step`) | `b2Shape_IsValid` on each id | An **end** event may name a shape destroyed *last* frame — v3 delivers those one step late (below) |
| Resolution (in `Level`) | UUID present in `m_EntityMap` | The entity died since the step — including during *this very dispatch loop* |
| Dispatch (per side, per event) | the side is re-resolved immediately before its hook runs | An earlier event's handler destroyed it — the exact bug from §0 |

The contrast with RAD-28's move drain is deliberate and worth internalising: there, a missing entity is an **asserted invariant break** (nothing can destroy entities between step and drain, because scripts run *before* the step). Here, a missing entity is **routine and survivable**, because dispatch *is* gameplay code and gameplay code destroys things. Same lookup, opposite verdict — because the surrounding invariants differ.

**3 — Why end events can name dead shapes: v3's double-buffered end array.** Box2D keeps *two* end-event buffers and swaps them at the end of each step (`Radiant/Vendor/box2d/src/world.c:807-810`); `b2World_GetContactEvents` reads the buffer that is *not* currently being written (`world.c:1538-1542`). Consequence: end events produced by user code between steps — a `b2DestroyBody` from `Level::DestroyEntity`, say — land in the write buffer and are reported after the **next** step, by which time the shape is provably gone. The library's own header says so: *"this shape may have been destroyed … @see b2Shape_IsValid"* (`types.h:1067-1078`). Validity checking here is not defensive coding; it is following the API contract.

**4 — Contact events are opt-in per collider, defaulting to on.** `b2DefaultShapeDef()` zero-initialises `enableContactEvents` to **false** (`src/types.c:55-65`), so today's shapes report nothing. We surface it as a serialized `BoxCollider2DComponent::EnableContactEvents` field defaulting to `true`. Rationale: the cost of reporting scales with *transitions*, not population — a settled 500-crate pile generates zero events per step, exactly like move events — so defaulting off buys no measurable performance and creates UE's most notorious ergonomic trap (`bNotifyRigidBodyCollision` defaults false; every UE developer has spent an afternoon on "why isn't my hit event firing"). The field still exists so a future debris system can opt out of *dispatch* cost. Box2D's semantics are OR: a contact reports if **either** shape has the flag (`src/contact.c:253`).

**5 — Gameplay hears about it through two channels: a Level-wide observer list and per-entity script hooks.** (Decided 2026-07-29: the observer list was initially scoped out as speculative and was pulled in on the named-customer rule — GAS is a planned consumer that needs collision notification without being attached to a specific entity's script.)

- **Observers** — `Level::AddCollisionObserver(std::function<void(const CollisionEvent&)>) -> CollisionObserverHandle`, mirroring UE's world-level `UPhysicsCollisionHandler`. Called **once per event with both participants**, because a global system cares about the pair, not about whose perspective it is. This is the channel GAS, audio, VFX, and damage bookkeeping will use.
- **Script hooks** — `ScriptableEntity::OnCollisionBegin(Entity other)` / `OnCollisionEnd(Entity other)`, beside the existing `OnCreate`/`OnUpdate`/`OnDestroy`, called **once per live side** because "something hit *me*" is inherently one-sided.

Observers run before script hooks, matching UE (`DispatchPendingCollisionNotifies` calls the world handler first, then per-actor dispatch): global systems observe the fullest picture before per-entity gameplay starts destroying things.

`CollisionObserverHandle` is `{Index, Generation}` — the same slot-plus-generation shape as `TimerHandle` (playbook §2; `Core/TimerManager.h:16`), so a stale handle is a benign no-op rather than a removal of whoever recycled the slot. Level state, not component state, so the `std::function` is legal (playbook §3 bans callables *in components*).

Rejected: a **batch** observer signature (`void(const std::vector<CollisionEvent>&)`, which is literally UE's `HandlePhysicsCollisions_AssumesLocked(TArray<FCollisionNotifyInfo>&)`). It saves N-1 calls, but it hands the subscriber a list of entities that go stale *as the subscriber walks it* — re-validation would become every subscriber's problem, which is exactly the bug this story exists to eliminate. Per-event dispatch lets `Level` re-resolve both sides immediately before each call. Subscribers that need rate limiting (UE's impact-sound cooldown is the canonical case) keep their own state.

Script instances move to a Level-owned side table in RAD-30; that changes *where the instance pointer lives*, not this dispatch shape.

**Rejected alternative — reuse `EventQueue`.** Superficially attractive (it is a queue; it already exists). Wrong on three counts: `EventQueue` drains once per *frame* at frame start, while collision events must drain once per *fixed step* — several times per frame under a slow frame; its `QueuedEvent` variant is a closed list of window/input events with a 64-byte size contract; and its `Event` base class carries `Handled`/propagation semantics that mean nothing for a physics fact. Same word, different domain, different lifetime.

**Rejected alternative — dispatch inside `PhysicsWorld2D`.** The physics module would need a `Level*` back-pointer to resolve entities, re-creating the exact upward dependency that v3's event model let us delete (`Docs/Physics.md`, architecture section). The dependency stays one-way: `Level → PhysicsWorld2D → Box2D`.

Playbook sections applied: **§4** (physics integration — the "record during, dispatch after, with validity checks" rule this story finally implements), **§3** (components stay plain data — no `std::function` returns), **§2** (UUIDs as generation handles), **§1** (drains inside the fixed step, never in render), **§8.4/§8.8** (no cached failures; no view mutation while iterating).

---

## 2. UE Reference

Unreal solves the identical problem with the identical architecture, at a scale that makes every one of its choices load-bearing. Chaos records collisions during the physics tick into a pending array on the scene, then dispatches them on the game thread afterwards.

**The record.** UE's queue entry keeps a *per-side dispatch flag* and *weak* references to both participants:

```cpp
// Engine/Source/Runtime/Engine/Public/PhysicsPublic.h:61
struct FRigidBodyCollisionInfo
{
    /** Actor involved in the collision */
    TWeakObjectPtr<AActor>                  Actor;
    /** Component of Actor involved in the collision. */
    TWeakObjectPtr<UPrimitiveComponent>     Component;
    ...
};

// Engine/Source/Runtime/Engine/Public/PhysicsPublic.h:90
/** One entry in the array of collision notifications pending execution at the end of the physics engine run. */
struct FCollisionNotifyInfo
{
    /** If this notification should be called for the Actor in Info0. */
    bool                        bCallEvent0;
    /** If this notification should be called for the Actor in Info1. */
    bool                        bCallEvent1;
    FRigidBodyCollisionInfo     Info0;
    FRigidBodyCollisionInfo     Info1;
    FCollisionImpactData        RigidCollisionData;
    ...
    /** Check that is is valid to call a notification for this entry. Looks at the IsValid() flags on both Actors. */
    ENGINE_API bool IsValidForNotify() const;
};
```

`TWeakObjectPtr` is UE's **generation handle** — index + serial number, resolved through a validity check, exactly the category our `UUID` + `m_EntityMap` lookup falls into and exactly why playbook §2 says Radiant has no `WeakRef` type. UE cannot store a raw `AActor*` in a queue for the same reason we cannot store an `Entity` handle: the queue outlives the thing it names.

**The dispatch loop.** This is the snippet to internalise — note where the validity check sits:

```cpp
// Engine/Source/Runtime/Engine/Private/PhysicsEngine/Experimental/PhysScene_Chaos.cpp:1150
void FPhysScene_Chaos::DispatchPendingCollisionNotifies()
{
    ...
    // Fire any collision notifies in the queue.
    for (FCollisionNotifyInfo& NotifyInfo : PendingCollisionNotifies)
    {
        if (NotifyInfo.bCallEvent0)
        {
            if (AActor* Actor = NotifyInfo.Info0.Actor.Get())     // <-- re-checked INSIDE the loop
            {
                Actor->DispatchPhysicsCollisionHit(NotifyInfo.Info0, NotifyInfo.Info1, NotifyInfo.RigidCollisionData);
            }
        }
    }
    PendingCollisionNotifies.Reset();
}
```

`Actor.Get()` returns null once the actor is gone, and it is called **per entry, immediately before dispatching that entry** — not once up front. That is precisely because a handler invoked earlier in this same loop may have destroyed a later entry's actor. Our `GetEntityByUUID` re-resolution per side is the same move in Radiant's vocabulary.

**What we adopt:** the pending-array-plus-post-step-dispatch shape; generation handles rather than raw pointers in the queue; per-side dispatch flags; re-validation inside the dispatch loop.

**What we deliberately simplify:**

| UE has | We do | Why our scale justifies it |
|--------|-------|----------------------------|
| `UPhysicsCollisionHandler` — a single, game-overridable, world-wide handler class (`PhysicsCollisionHandler.h:44`) | A **list** of plain-callable observers on `Level` | We adopt the channel, not the shape: UE's is one `UObject` subclass per world (a reflection/Blueprint-era constraint), which forces unrelated systems to share one override. A handle-keyed subscriber list lets GAS, audio, and damage subscribe independently. |
| Batch dispatch — the handler receives the whole `TArray<FCollisionNotifyInfo>` | Per-event dispatch | See §1 decision 5: batching moves re-validation into every subscriber |
| `FCollisionImpactData` — contact points, normals, `DeltaVelocity`, per-contact physical materials | Entity pair + phase only | No customer; the A/B ordering semantics of a normal deserve their own design pass (see §9) |
| `ContactPairToPendingNotifyMap` deduplicating multiple contacts per pair per solver time | Nothing | Box2D reports one begin and one end per *contact*, not per manifold point — deduplication is already done by the library |
| Async/sync scenes, `SolverTime` stamping, threaded solver marshalling | One world, one thread | Single-threaded fixed step; the whole reason UE needs `SolverTime` does not exist here |

---

## 3. File Plan

> **Deviation found in implementation (2026-07-30):** `ContactPhase`/`ContactEvent` could not be nested in `PhysicsWorld2D` as planned. `Level::CollisionEvent` needs to name `ContactPhase`, but `PhysicsWorld2D.h` includes `<box2d/id.h>` for its `b2WorldId` member, and including it from `Level.h` would put a Box2D header in a public engine header — the boundary RAD-27 established. The records moved to a vendor-free `Physics/ContactEvent.h` that both headers include. Rejected alternative: a duplicate phase enum in the ECS layer plus a translation switch — two enums that can drift, to avoid one small header.

```text
Radiant/Source/Radiant/Physics/
    ContactEvent.h          (new)    — ContactPhase enum + ContactEvent record, no vendor includes
    PhysicsWorld2D.h        (modify) — includes ContactEvent.h; GetContactEvents() + buffer
    PhysicsWorld2D.cpp      (modify) — enable events on shape create/refresh; translate + validate the drain

Radiant/Source/Radiant/ECS/
    Components.h            (modify) — BoxCollider2DComponent::EnableContactEvents
    ScriptableEntity.h      (modify) — OnCollisionBegin / OnCollisionEnd virtuals
    Level.h                 (modify) — CollisionEvent, CollisionObserverHandle, Add/RemoveCollisionObserver,
                                       DispatchContactEvents(), observer slots + re-entrancy tripwire
    Level.cpp               (modify) — observer add/remove/slot recycling; dispatch pass after the move drain
    LevelSerializer.cpp     (modify) — (de)serialize EnableContactEvents, missing key defaults true

Reaper/Source/
    CollisionLogger.h       (new)    — verification script: logs begin/end, destroys on demand
    Layers/GameLayer.cpp    (modify) — bind the logger; register an observer; cheat for destroy-inside-handler

Docs/
    Physics.md              (modify) — "Collision reporting" replaces the deliberate-gap note

.claude/references/
    radiant-playbook.md     (modify) — §4 collision bullet marked landed
```

| Action | Path | Description |
|--------|------|-------------|
| Modify | `Radiant/Source/Radiant/Physics/PhysicsWorld2D.h` | `ContactPhase` enum, `ContactEvent` POD, `m_ContactEvents` buffer, `GetContactEvents()`; doc the transient-buffer contract |
| Modify | `Radiant/Source/Radiant/Physics/PhysicsWorld2D.cpp` | `shapeDef.enableContactEvents` at create, `b2Shape_EnableContactEvents` at refresh, contact drain with translation-time validity in `Step` |
| Modify | `Radiant/Source/Radiant/ECS/Components.h` | `bool EnableContactEvents = true` on the collider, documented as per-transition cost |
| Modify | `Radiant/Source/Radiant/ECS/ScriptableEntity.h` | Two protected virtuals; contract states `other` may be invalid |
| Modify | `Radiant/Source/Radiant/ECS/Level.h` / `.cpp` | `CollisionEvent` payload, `CollisionObserverHandle`, `Add`/`RemoveCollisionObserver`, observer slot storage + free list, `DispatchContactEvents()` after the move drain, `m_DispatchingContacts` tripwire and deferred observer release |
| Modify | `Radiant/Source/Radiant/ECS/LevelSerializer.cpp` | Write the flag; read it with a `true` default so existing `.rdlvl` files load unchanged |
| Create | `Reaper/Source/CollisionLogger.h` | Native script proving begin/end fire, and that destroying inside a handler is safe |
| Modify | `Reaper/Source/Layers/GameLayer.cpp` | Bind the logger; register a Level observer (proves the GAS-facing channel + observers-before-scripts order); destroy-in-handler cheat |
| Modify | `Docs/Physics.md` | Replace the gap section with the landed architecture (doc update contract) |

---

## 4. Type Design

*(Both records live in `Physics/ContactEvent.h` at namespace scope, not nested in `PhysicsWorld2D` — see the deviation note in §3.)*

### ContactPhase
- **Kind:** `enum class : uint8_t`
- **Responsibility:** Which edge of a touch a record describes — `Begin` or `End`.
- **Why an enum and not two vectors:** one ordered list preserves the begin-before-end guarantee (§6) and keeps the dispatch loop a single pass. Adding `Hit` later means adding a value plus its own payload vector, not restructuring this one.

### ContactEvent
- **Kind:** `struct` (POD, trivially copyable)
- **Responsibility:** One "these two entities started/stopped touching" record, in engine types.
- **Ownership:** Value inside a `PhysicsWorld2D`-owned `std::vector`; consumers read a `const&` and never store it.
- **Lifetime & threading:** Valid until the next `Step` refills the buffer — consume within the same fixed update. Main-thread only.
- **Key Members:**
  - `ContactPhase Phase` — begin or end.
  - `UUID EntityA` / `UUID EntityB` — the participants; **`0` when that side's shape was already destroyed** (possible on `End` only). Order is Box2D's shape ordering and carries no meaning.
- **Playbook Patterns:** §4 (record during, dispatch after), §2 (UUID as generation handle), §3 (plain data on the wire between modules).

### PhysicsWorld2D — additions
- `const std::vector<ContactEvent>& GetContactEvents() const` — the drained batch for the last `Step`.
- `std::vector<ContactEvent> m_ContactEvents` — reusable buffer; `clear()` keeps capacity, so steady state allocates nothing (same idiom as `m_MoveEvents`).

### Level::CollisionEvent
- **Kind:** `struct` (value, passed by `const&`)
- **Responsibility:** The gameplay-facing form of a contact record — the *resolved* twin of `PhysicsWorld2D::ContactEvent`.
- **Key Members:**
  - `PhysicsWorld2D::ContactPhase Phase`
  - `Entity A` / `Entity B` — **either may be invalid** when that side is already destroyed; never both (the event is skipped in that case).
- **Why two types and not one:** `ContactEvent` is the transport record across the module boundary and speaks in UUIDs, because `Physics/` has no entity map and must not have one. `CollisionEvent` is the consumer payload and speaks in `Entity`, because that is what gameplay can act on. Collapsing them would drag entity resolution into `Physics/` and give it the `Level*` back-pointer RAD-27 deleted.

### Level::CollisionObserverHandle
- **Kind:** `struct` (POD, copyable), `{uint32_t Index, uint32_t Generation}`
- **Responsibility:** Opaque identity for one registered observer.
- **Why generation, not a bare index:** slots recycle. A stale handle whose slot was reused would otherwise remove somebody else's observer — a bug that presents as "GAS randomly stops hearing collisions." Identical reasoning and identical layout to `TimerHandle` (`Core/TimerManager.h:16`), playbook §2.

### Level — additions
- `CollisionObserverHandle AddCollisionObserver(std::function<void(const CollisionEvent&)>)` — registers a Level-wide observer. Asserts on an empty callable (programmer error) and returns an invalid handle, matching `TimerManager::SetTimer`.
- `void RemoveCollisionObserver(CollisionObserverHandle&)` — releases the callable and its captures immediately, bumps the slot generation, resets the handle. Stale/invalid handles are benign no-ops — the correct idiom for "remove if still registered." Removal *during* dispatch defers the release until the batch finishes (below).
- `void DispatchContactEvents()` — **private.** Walks the world's batch; per event, re-resolves both sides and calls every live observer, then each live side's script hook.
- `std::vector<CollisionObserver> m_CollisionObservers` + `std::vector<uint32_t> m_FreeObserverSlots` — slot storage with a free list; slots never shrink, so a handle's index stays meaningful forever. Same structure as `TimerManager`'s pool.
- `bool m_DispatchingContacts = false` — re-entrancy tripwire, and the flag that makes observer removal deferred. `OnFixedUpdate` asserts it is clear on entry: a handler that re-entered the fixed update would call `Step`, which clears the very vector the dispatch loop is walking. Same role as `EventQueue::m_IsProcessingEvents`.

**Observer callback lifetime contract** (mirrors `TimerManager`'s, and belongs verbatim in the header doc): *the Level owns the callback by value. A callback capturing an object (an `Entity`, a system pointer, `this`) outlives its target unless the owner removes the handle in its teardown path — `RemoveCollisionObserver` releases the callable and its captures immediately.* GAS will capture its ability-system pointer here; if it forgets to remove, the Level calls into a dead object.

### ScriptableEntity — additions
- `virtual void OnCollisionBegin(Entity other)` / `virtual void OnCollisionEnd(Entity other)` — **protected**, `Level` is already a friend. Default implementations are empty (unlike the existing `INFO`-logging defaults, which would flood: every crate landing would log).
- **Contract to document:** runs during the fixed step, after physics has advanced and transforms are current; `other` **may be invalid** — check it before use; safe to destroy/teleport/spawn anything, including `other` and yourself.

### BoxCollider2DComponent — addition
- `bool EnableContactEvents = true` — serialized. Off means this shape's contacts are never reported *unless the other shape opts in* (Box2D ORs the flag).

---

## 5. Implementation Steps

### Phase 1 — The queue (Physics module)

- [x] **Declare the record.** `ContactPhase` + `ContactEvent` nested in `PhysicsWorld2D`, beside `BodyMoveEvent`, with the same doc-comment shape. State in the comment that `EntityA`/`EntityB` may be `0` on `End`, and that the buffer is refilled per `Step`.

- [x] **Opt shapes in.** Add `EnableContactEvents` to `BoxCollider2DComponent`; set `shapeDef.enableContactEvents` in `CreateBoxShape`; call `b2Shape_EnableContactEvents` in `UpdateBoxShape`. **Gotcha to comment:** the contact's flag is captured when the *contact* is created (`src/contact.c:253`), so toggling at runtime does not affect contacts that already exist — a shape switched off while touching will still emit its end event, and one switched on while touching never emits a begin. Box2D warns about this in `box2d.h:580`; the honest fix is "set it at spawn," and the doc comment says so.

- [x] **Serialize the flag.** Write it in `LevelSerializer::Serialize`; read it in `Deserialize` **with a `true` default when the key is absent** — existing `.rdlvl` files predate the field and must load unchanged (same courtesy the removed `RestitutionThreshold` key gets).

- [x] **Drain contacts in `Step`.** Immediately after the existing move drain: `b2World_GetContactEvents`, `clear()` + `reserve(beginCount + endCount)`, then **begins first, then ends** (§6 explains why that order). Per side: `b2Shape_IsValid` → `b2Shape_GetBody` → `b2Body_GetUserData` → `UUID`; an invalid shape yields `UUID(0)`. **Assert** on an invalid shape in a *begin* event — nothing can destroy a shape between `b2World_Step` returning and this line, so that would be a broken invariant — and **survive silently** on an invalid shape in an *end* event, which is the documented normal case.

### Phase 2 — Dispatch (ECS module)

- [x] **Add the script hooks.** `OnCollisionBegin` / `OnCollisionEnd` on `ScriptableEntity`, protected, empty defaults, full contract in the doc comment (when they run, what `other` means, what is safe to do).

- [x] **Build the observer registry.** `CollisionEvent`, `CollisionObserverHandle`, `Add`/`RemoveCollisionObserver`, slot storage + free list + generation bump on release. Copy `TimerManager`'s structure deliberately rather than inventing a second idiom, and carry over its callback-lifetime doc block. Removal while `m_DispatchingContacts` is set marks the slot inactive and queues the release until the batch ends — releasing immediately would destroy the `std::function` currently executing if an observer removes itself.

- [x] **Write `Level::DispatchContactEvents`.** For each record, in this order:
  1. **Observers**, once per event: re-resolve A and B, skip the event entirely if both are dead, and call each active observer with `{Phase, A, B}` — **re-resolving between observers**, because an observer may destroy either side.
  2. **Script hooks**, once per live side: resolve A; if live, call its hook with B resolved *at that moment*; then resolve B **again** (the previous handler may have destroyed it) and call its hook with A re-resolved.

  A script notification requires a `NativeScriptComponent` with a live `Instance` and `MetadataComponent::IsActive` (matching how `OnUpdate` is gated). `GetEntityByUUID(UUID(0))` naturally returns an invalid `Entity`, so the zero sentinel needs no special case anywhere.

  Iterate observers **by index with the count captured at entry**: an observer registered during dispatch does not receive the current batch (deterministic, and the same rule `EventQueue` uses for events pushed during processing), and index-based access survives the vector reallocating under a `push_back`.

- [x] **Wire it into the step.** Call it from `OnFixedUpdate` **after** the move drain — handlers must see current transforms — and set/clear `m_DispatchingContacts` around it, asserting at `OnFixedUpdate` entry that it is clear. Flush deferred observer releases when the flag clears.

### Phase 3 — Proof and records

- [x] **Reaper verification script.** `CollisionLogger` (`ScriptableEntity`) logging `GAME_TRACE` on begin/end with both entity names; plus a mode that calls `Level::DestroyEntity(other)` from inside `OnCollisionBegin` — the §0 scenario, which must not crash and must deliver a subsequent `OnCollisionEnd` with an invalid `other` to the survivor. Bind it in `GameLayer` on both level-load paths.

- [x] **Reaper verification observer.** Register a `GameLayer` observer logging every pair, and remove it in `OnDetach` — which is both the proof that the GAS-facing channel works and the worked example of the removal contract. Its log lines must precede the script hooks' for the same event (the observers-first order), and a self-removing observer must not crash.

- [x] **Docs + playbook, same change.** Replace the "deliberate gap until RAD-29" section of `Docs/Physics.md` with the landed architecture (drain, three validity checks, opt-in flag, dispatch order, the one-step-late end note); update the per-step flow diagram; mark playbook §4's collision bullet landed with the date. `/review` treats a stale doc as an ERROR.

- [ ] **Build and run.** All three configs via the CLI MSBuild command in CLAUDE.md, then the Reaper smoke run — launch, exercise the collision cheats, close via `CloseMainWindow`, scan the log for `[W]`/`[E]`.

---

## 6. Ownership & Lifetime Strategy

**Who owns what.** `PhysicsWorld2D` owns `m_ContactEvents` exactly as it owns `m_MoveEvents`: created empty with the world, refilled every `Step`, destroyed with the world. `Level` reads a `const&` and walks it; it stores no contact records. Nobody stores a `ContactEvent` beyond the fixed update that produced it, and the doc comment says so.

`Level` does own one new thing: the observer slots, and through them the observer callables *by value*. A `std::function` owns its captures, so registering an observer transfers a copy of whatever it captured into the Level. Two consequences worth stating in the header: releasing a handle frees those captures immediately (matters when a capture holds a `Ref<T>`), and a subscriber that dies without removing its handle leaves the Level calling into a dead object. The Level cannot detect that — there is no back-channel from a `std::function` to its owner's lifetime — which is exactly why `TimerManager` states the same contract in the same words rather than trying to be clever about it.

**The three-way lifetime hazard, and why each is closed.**

1. *Box2D's arrays.* `b2World_GetContactEvents` returns pointers into world-internal storage that the next step overwrites — `types.h` says "do not store a reference to this data." Closed by copying into engine PODs inside `Step`, before any other code runs.
2. *Shape ids in end events.* May name shapes destroyed a step ago (§1, double buffer). Closed by `b2Shape_IsValid` before every dereference, with `UUID(0)` as the honest "unresolvable" answer rather than a fabricated id.
3. *Entities during dispatch.* Handlers destroy things; a later record may name a just-destroyed entity. Closed by re-resolving each side through `m_EntityMap` immediately before its hook fires — the `Entity` handle is *derived at the point of use* and never carried across a handler call. This mirrors UE's `Actor.Get()` inside its dispatch loop.

**Ordering guarantees inside one batch.** Begins are dispatched before ends. Consider an entity standing on platform A (overlap count 1) when A is destroyed and the entity simultaneously lands on B: the batch holds `Begin(B)` and `End(A)`. Begins-first yields 1 → 2 → 1 and the entity is never momentarily "airborne"; ends-first yields 1 → 0 → 1 and any `count == 0` reaction fires spuriously for one step. Box2D gives us no cross-array timestamp, so we choose the order that cannot produce a false zero-crossing, and we document it as a guarantee gameplay may rely on.

**Interaction with `Level::DestroyEntity`.** Destroying an entity during dispatch removes its collider then its rigidbody (existing order), destroying the Box2D body. That happens *outside* a step, so the world is unlocked and it is legal. The end events it generates land in v3's write buffer and surface after the next step — which is exactly why the survivor's `OnCollisionEnd` arrives one step later with an invalid `other`.

**Interaction with `~Level`.** Teardown destroys every entity, generating end events into a buffer nobody will ever drain. Harmless — the world dies immediately after.

---

## 7. Performance Notes

**Per fixed step, added cost:**

- One `b2World_GetContactEvents` call — returns three pointers and three counts, copies nothing.
- One pass over `beginCount + endCount` entries. Per entry: up to two `b2Shape_IsValid` (a bounds check plus a generation compare), two `b2Shape_GetBody` / `b2Body_GetUserData` (array indexing), and one `push_back` of a 24-byte POD into a pre-`reserve`d vector.
- Per dispatched event: `2 × (observerCount + 2)` `m_EntityMap` hash lookups — both sides re-resolved before every observer and before each script hook — plus a `TryGetComponent` and an `IsActive` read per live side. With three observers that is ten lookups per *transition*; the re-resolution is the price of check 3 and it is the right trade.

**Allocations:** none in steady state. `clear()` preserves capacity and `reserve()` runs before the fill, so the buffer grows to the level's busiest step once and never again — the pattern RAD-28 already proved for move events. Observer registration allocates once at subscribe time (the `std::function`, if its captures outgrow the small-buffer optimisation); dispatch copies no callables, so nothing allocates per event. Observer slots are never freed, only recycled — so a system that subscribes and unsubscribes repeatedly does not grow the vector.

**The property that actually matters:** *events are transitions, not states.* Box2D emits a begin when a pair starts touching and an end when it stops; a pile of 500 crates resting on each other emits **zero** events per step forever. Cost scales with how much the world is *changing*, not with how much is *in* it — the same activity-proportional shape as the move drain, and the reason defaulting `EnableContactEvents` to true is not a performance decision.

**What could go wrong at scale, and the tripwire for it:** a pathological case is many small fast bodies skimming a surface, flickering begin/end every step. That shows up as a growing `m_ContactEvents` size, not as a hidden cost — and if it ever appears, `EnableContactEvents = false` on the offending collider is already the fix, which is the second reason the field exists.

**No render-path impact whatsoever.** Everything here lives inside `OnFixedUpdate`; `OnRender` is untouched (playbook §1).

---

## 8. Logging & Diagnostics

**Asserts (programmer errors):**
- Invalid shape id in a **begin** event — impossible unless the drain moved away from being the first thing after `b2World_Step`.
- `m_DispatchingContacts` already set on entering `OnFixedUpdate` — re-entrant fixed update.

**Warn-and-recover (content/config mistakes):** none needed in the engine dispatch path. Every failure here — unresolvable end-event side, entity destroyed mid-batch, script-less entity, inactive entity — is a *legitimate runtime state*, and logging it would produce noise indistinguishable from normal play. This is a deliberate departure from the move drain, which asserts on the same lookup miss; the difference is that dispatch runs gameplay code and the move drain does not.

**Trace:** no per-event engine logging — a busy step would emit dozens of lines and drown the log. Verification logging lives in Reaper's `CollisionLogger` as `GAME_TRACE`, where it can be attached to exactly one entity and removed when the story closes.

**Diagnostics we get for free:** `b2SetAssertFcn` (installed in RAD-27) routes any Box2D-internal assertion — including anything we trip by mutating the world at the wrong moment — through Radiant's log-then-break machinery with file and line.

---

## 9. Scalability Review

| Pattern | Verdict |
|---------|---------|
| Drain-and-translate loop in `Step` | **Scales.** Adding hit or sensor events is a new buffer and a new loop, not a change to this one. |
| `ContactEvent` = phase + two UUIDs | **Scales for now, with a known ceiling.** One collider per entity today, so entity identity is sufficient. The moment an entity carries several shapes ("hurtbox vs feet sensor"), gameplay must know *which* shape touched, and the record needs a shape identity too. That is a multi-collider story, not this one — but the record should be extended, never duplicated. |
| Dispatch to consumers | **Addressed in this story.** A handle-keyed observer list means a new consumer kind (GAS, audio, VFX, damage) subscribes instead of editing `DispatchContactEvents` — the O(N)-changes-per-feature pattern this section exists to catch is closed before it opens. Script hooks remain a second, per-side channel because "something hit me" is genuinely a different question from "A and B touched." |
| Second hand-rolled generation-handle slot pool | **Flag: watch for the third.** `TimerManager` has one, `Level`'s observers now have another, and playbook §9 already names `SlotMap<T>` as a gated learning container to extract "at the rule-of-three moment — RAD-30 side tables, Phase 3 GPU resource pools." This is occurrence two, so duplicating deliberately is correct today. RAD-30's script side table is the likely third; whoever writes it should extract rather than write a fourth. Note it there, not here. |
| `EnableContactEvents` as a bool on the collider | **Scales to the next step, then wants a system.** A bool answers "report or not"; real games ask "report *to whom*" — collision channels and a filtering matrix (UE's `ECollisionChannel`, Box2D's `b2Filter`, which we currently leave at default). That is a physics-filtering story of its own; the bool is not on its critical path. |
| Begin-then-end ordering | **Scales.** A documented guarantee, not an implementation detail — hit and sensor events slot in as additional phases under the same rule. |

**Follow-ups to file (see §11 close-out):** contact manifold data (normal/point) + hit events with `approachSpeed`; sensor events and a sensor component; collision filtering/channels.

---

## 10. Risks & Edge Cases

- **A handler destroys the entity the handler belongs to.** `Level::DestroyEntity` deletes the script instance whose method is currently on the stack — a use-after-free the moment the hook returns and touches a member. *Mitigation:* the dispatch loop must hold no reference to the instance across the call, and the hook's doc comment must state that destroying *yourself* is legal only as the last statement. Worth an explicit test in `CollisionLogger`, because it is the sharpest edge in the story.
- **A handler adds or removes a physics component.** That fires entt signals → `b2CreateBody`/`b2DestroyBody` outside a step → legal (world unlocked). Safe, but only because dispatch is not inside a registry view — it walks a `std::vector` of PODs. Do not "optimise" the loop into a view later.
- **An observer removes itself from inside its own callback.** Releasing the slot immediately would destroy the `std::function` whose body is currently executing — use-after-free on return. *Mitigation:* removal during dispatch marks the slot inactive and defers the release to the end of the batch. Must be tested explicitly; it is the observer-channel twin of "a script destroys its own entity."
- **An observer registers another observer mid-dispatch.** The vector may reallocate. *Mitigation:* index-based iteration with the count captured at entry — the new observer is skipped for this batch and receives the next one, matching `EventQueue`'s "pushed during processing → processed next round" rule.
- **A subscriber forgets to remove its handle.** The Level keeps calling into a dead object. No mitigation is possible from the Level side (§6); the contract is documented on the API and demonstrated by `GameLayer::OnDetach` removing its own observer.
- **Toggling `EnableContactEvents` at runtime.** Existing contacts keep the flag captured at contact creation (`src/contact.c:253`), so a mid-touch toggle produces an unpaired begin or end. Documented; recommended usage is spawn-time only.
- **Both sides unresolvable.** Two entities destroyed in the same step produce an end event with two `UUID(0)`s. Loop does nothing. No log — it is normal.
- **A `Level` with no physics world** (scratch levels, or failed world creation). `DispatchContactEvents` sits inside the existing `if (m_PhysicsWorld)` block; unchanged behaviour.
- **Several fixed steps per frame.** Under a slow frame the accumulator runs `OnFixedUpdate` multiple times; each step drains and dispatches its own batch. Correct by construction — and the reason this cannot live at frame scope.
- **Old `.rdlvl` files.** Missing `EnableContactEvents` key must default true, not false, or previously-working content silently stops reporting.
- **Dist build.** Both new asserts must contain no side effects — the drain and the tripwire must behave identically with `RADIANT_ASSERT` compiled out (playbook §8.5).

---

## 11. Verification (AC → proof)

> **Verification history (2026-08-01).** Briefly re-scoped to block on RAD-67's doctest harness, then reverted: this story verifies in Reaper as planned below, and the accumulated Reaper scaffolding is retired later under **RAD-92** once RAD-67 provides durable tests. RAD-67 is a follow-up, not a blocker — the trade is explicit and has a receipt.
>
> **Run performed 2026-08-01 (Debug), all rows below confirmed.** The decisive observation: with the platform destroying its partner from `OnCollisionBegin`, the partner's own hook never fired — re-resolved, found dead, skipped — and its `OnCollisionEnd` arrived a step later with `<destroyed>`. Zero `[E]` lines, zero stale-ticket warnings, no Box2D asserts, bodies still reaching sleep (contact persistence intact), clean teardown.

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| Contact events are recorded during the step and dispatched after it — never from inside a Box2D callback | Code review: the only Box2D contact call is `b2World_GetContactEvents` in `Step`; no listener type exists in the codebase (`grep` for `CollisionListener` returns docs only) |
| Gameplay receives begin and end notifications | Reaper: `CollisionLogger` on the falling square logs `OnCollisionBegin` exactly once as it lands on the platform, and `OnCollisionEnd` exactly once when the `T` teleport cheat lifts it away |
| Both participants are notified | The same landing logs begin from the square's script and from a logger bound to the platform, with each naming the other |
| Level-wide observers receive every collision, before per-entity scripts | `GameLayer`'s observer logs the pair for each landing, and its line precedes the scripts' lines for that event |
| Observer handles are safe against slot reuse and self-removal | Remove an observer, register a new one (recycling the slot), then call `RemoveCollisionObserver` on the stale handle: benign no-op, the new observer keeps firing. An observer that removes itself mid-callback does not crash in Debug or Dist |
| Destroying an entity from inside a handler is safe | Cheat mode: `CollisionLogger` calls `DestroyEntity(other)` on its first begin. Debug + Dist: no crash, no Box2D assert, the destroyed entity disappears, and the survivor receives `OnCollisionEnd` with an invalid `other` on a later step |
| Events referencing destroyed shapes/entities are dropped, not dereferenced | Same run: the drain resolves the dead side to `UUID(0)`, dispatch skips it, and no `[E]`/`[W]` lines appear |
| Notifications carry no `std::function`; components stay plain data | `Components.h` review: `BoxCollider2DComponent` gains one `bool`; no component holds a callable |
| Box2D types never leave `Physics/` | `grep` for `b2` outside `Radiant/Source/Radiant/Physics/` returns nothing new |
| Contact reporting is opt-in per collider and round-trips through the serializer | Set `EnableContactEvents = false` on the square, save, reload: no notifications; existing `.rdlvl` files without the key still report |
| No per-step allocations in the drain | Review: `clear()` + `reserve()` before fill, POD `push_back`, buffer reused — the RAD-28 pattern |
| Builds clean in all three configs; Reaper runs | CLI MSBuild for Debug/Release/Dist with no new warnings, then the smoke run (launch → exercise cheats → `CloseMainWindow` → log scan for `[W]`/`[E]`) |
| `Docs/Physics.md` reflects the landed design | The "deliberate gap" section is replaced; the per-step flow diagram includes the dispatch pass; playbook §4 marked landed |