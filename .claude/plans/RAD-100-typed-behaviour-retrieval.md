# Implementation Plan — RAD-100: Typed behaviour retrieval: recover a concrete script from an Entity

| Field | Value |
|-------|-------|
| **Jira** | [RAD-100](https://hndredgames.atlassian.net/browse/RAD-100) |
| **Epic** | RAD-98 — Gameplay Framework: engine base classes for game code |
| **Story status** | To Do |
| **Dependencies** | RAD-95 (In Review), RAD-99 (In Review) — both landed on `dev` |
| **Planned** | 2026-08-25 |

---

## 0. The Problem, Ground Up

### What the code does today

A behaviour — a `CollisionLogger`, a `CameraController` — is a C++ object the engine
creates for you and hangs off an entity. The engine stores it inside a component as a
pointer to the **base** class, because the engine has no idea what your subclass is:

```cpp
struct NativeScriptComponent
{
    EntityBehaviour* Instance;   // <- points at a CollisionLogger, but doesn't say so
};
```

"Base class" here means the common parent every behaviour inherits from. The engine can
call `OnUpdate()` on it — that works through **virtual dispatch**, C++'s mechanism for
"call whatever the real object's version is". What the engine *cannot* do is hand the
pointer back to you as the type you actually wrote, because the type information was
thrown away at the moment of storage.

So game code that needs the real thing does this — `Reaper/Source/Layers/GameLayer.cpp:329-332`:

```cpp
auto* nsc = platform.TryGetComponent<NativeScriptComponent>();
static_cast<CollisionLogger*>(nsc->Instance)->SetDestroyOnContact(true);
```

Two problems stacked. First, the caller had to know that behaviours live in a component
called `NativeScriptComponent` — that is engine plumbing leaking into game code. Second,
and much worse: `static_cast` is C++ for **"trust me"**. The compiler does not check it.
It emits no code at all beyond adjusting the pointer.

### The failure, concretely

Rename that entity, or rebind it to a different behaviour, and the line above becomes:

```cpp
static_cast<CollisionLogger*>(/* actually a CameraController* */)->SetDestroyOnContact(true);
```

Nothing crashes at that line. The program takes the `CameraController` object's memory,
pretends it is a `CollisionLogger`, and looks up `SetDestroyOnContact` at whatever byte
offset a `CollisionLogger` keeps its `m_DestroyOnContact` flag at. In a
`CameraController` that offset holds — let's say — the low byte of `m_ZoomSpeed`, a
`float`. Writing `true` (the byte `0x01`) into it turns `2.5f` (bytes `00 00 20 40`)
into `01 00 20 40`, which is `2.5000002f`. **The game keeps running.** Nothing logs,
nothing asserts, no memory tool fires — the write was inside a live object you own.

Three weeks later the camera has a barely-wrong zoom and nobody can explain it. If the
offsets had happened to land on a pointer member instead of a float, you get a crash
somewhere else entirely, in a system that never touched this code. This class of bug —
*wrong-type reinterpretation that corrupts a neighbour* — is the reason Naughty Dog's
engine tooling and Unreal both refuse to let gameplay hand-cast: it is the single hardest
category of bug to trace back to its cause, because the crash site and the guilty line
are unrelated.

### The fix, in one everyday image

Think of the base pointer as a **sealed parcel on a shelf**. `static_cast` is slicing it
open and *declaring* what is inside — if you are wrong you have already cut into whatever
is really there. What we want instead is a **label the parcel carries itself**: you ask
"is this a CollisionLogger?" and the parcel answers, truthfully, before anything is
opened. If the answer is no, you get nothing back and you carry on.

C++ already has that label. Any class with a virtual function carries a small runtime
tag describing its real type — this is called **RTTI** ("run-time type information"), and
the operator that reads it is `dynamic_cast`. `dynamic_cast<T*>(p)` returns a valid `T*`
if `p` really points at a `T`, and **`nullptr` if it does not**. Checked, not trusted.

### How Unreal solves it

Identically in shape, differently in mechanism. `AActor::FindComponentByClass<T>()`
searches the actor's component list, asks each candidate `IsA(TargetClass)`, and returns
`nullptr` when nothing matches — never an assert, because "it isn't that" is a normal
answer to a question. UE does not use `dynamic_cast` for this: it has a reflection system
(every `UCLASS` registers a `UClass` object with a parent chain), so it can answer
"is this a T?" by walking its own class chain, which it controls and can make fast. We do
not have reflection (RAD-72 is parked at the Phase 4 gate), so `dynamic_cast` **is** our
`IsA`. See §2 for the source.

### What we gain, and what we deliberately do not

**Gain:** one query — `entity.GetBehaviour<CollisionLogger>()` — that returns the real
pointer or `nullptr`, with no `NativeScriptComponent` at the call site and no unchecked
cast anywhere in the game.

**Do not gain, and should not be sold as gaining:**

- **No speed.** `dynamic_cast` is *slower* than `static_cast`, which compiles to nothing.
  We are buying correctness with cycles, at a call frequency where the cycles do not
  matter (§7).
- **No help with a stale pointer.** The returned `T*` dies with the entity, exactly like
  the `Entity` handle it came from. Storing one across a fixed step is the same bug it
  always was; this API cannot detect that and does not try (§6).
- **No composition.** One behaviour per entity is still the rule until RAD-101. This card
  makes the *query* exist; it does not make the *list* exist.

### What we have / what we're building

**What we have:** the engine remembers your behaviour only as a base-class pointer, so
game code recovers the real type with an unchecked `static_cast` that silently corrupts
memory the day it is wrong.

**What we're building:** one templated query on `Entity` that asks the object what it
really is and returns `nullptr` when the answer is "not that" — so a wrong guess becomes
a null check instead of a haunted float three systems away.

---

## 1. Architecture Decision

**The mechanism is `dynamic_cast`, and the shape is a template method on `Entity`.**

The card's Technical Notes already recommended `dynamic_cast` over a hand-rolled type-id
registry; RAD-99's planning comment (2026-08-03) strengthened it and this plan treats it
as settled, not re-argued. The reasoning in one line: a hand-rolled registry is the
O(N)-edits-per-type pattern playbook §9 / RAD-94 forbid, and it would be thrown away
twice — once when it proves that, and again when `entt::meta` (already vendored, verified
in `Radiant/Vendor/entt/include/entt.hpp`) lands with RAD-72. `dynamic_cast` is the choice
that gets *replaced*, not *reworked*. RTTI is on in all three configs (§11 verifies it),
and `EntityBehaviour` is already polymorphic — it has a virtual destructor — so **the
engine needs no new type, no new file, and no change to `EntityBehaviour` itself.**

Three design decisions carry the plan, each with its rejected alternative:

**(a) It joins the generic-query family, not the named-verb family.** `Entity` currently
has two distinct groups of members: the *named gameplay verbs* (`GetLocation`,
`SetTransform`, `Teleport`) which are governed by RAD-99's two-gate rule, and the
*generic template queries* (`GetComponent<T>`, `TryGetComponent<T>`, `HasComponent<T>`)
which are not. `GetBehaviour<T>` belongs to the second group: it is parameterised by type,
so it costs `Entity.h` exactly one declaration no matter how many behaviour types exist —
it is bounded **by construction**, which is precisely what the two gates exist to achieve
by discipline. Filing it under the gates instead would dilute a rule whose value is that
it is short and mechanical. *Rejected alternative:* widen the semantic gate's clause (a)
from "the equivalent raw component **write** is silently wrong" to "…**access**…" so
`GetBehaviour` passes as a named verb. It would pass — the unchecked `static_cast` is the
most silently-wrong access in the codebase — but widening a rule to admit a member that
does not need it makes the rule vaguer for zero benefit. The `Entity.h` doc gains one
sentence saying the gates govern named verbs, so the next reader does not have to
re-derive this.

**(b) Every negative answer is a silent `nullptr`; nothing asserts.** Four ways to get
nothing back — invalid handle, no `NativeScriptComponent`, instance not lazily created
yet, type mismatch — and all four are *normal answers to a question*. In particular
`OnCollisionEnd`'s documented contract is that the partner `Entity` **may be invalid**, so
`other.GetBehaviour<Door>()` on a dead partner is the designed path, not a mistake; a
WARN there would fire on normal gameplay and get tuned out (playbook §4's "routine verbs
do not log"). This is a deliberate departure from the sibling accessors:
`TryGetComponent` asserts `IsValid()`. Consequence for the implementation — `GetBehaviour`
must check `IsValid()` **first** and return early, rather than delegating to
`TryGetComponent` and inheriting its assert. *Rejected alternative:* assert on an invalid
handle for consistency with `TryGetComponent`. Rejected because it makes the engine
hostile on the exact call site the API exists for.

**(c) `T* GetBehaviour() const` — one overload, mutable pointer from a const handle.**
This follows the precedent `GetPhysicsBody() const` and `GetLevel() const` set and
documented: `const` on a handle constrains *the handle*, not the thing it names, exactly
as with a pointer. *Rejected alternative:* a `const T*` overload mirroring
`GetComponent`'s pair. Rejected because a behaviour is a live object with identity, not
stored data, and because the const overload would have no caller — RAD-94's "adopt when
the need exists, never before".

Playbook sections in force: **§10** (the gameplay seam and the dividing rule — a verb
naming one entity lives on `Entity`), **§2** (transient handles; the returned pointer is a
non-owning observer), **§3** (components stay plain data — this card adds nothing to
`NativeScriptComponent`), **§9** (no hand-rolled machinery where the standard library or
an existing dependency answers), **§8.7** (the bug class being closed).

---

## 2. UE Reference

### The search — `AActor::FindComponentByClass<T>()`

`Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:3823-3830`:

```cpp
	/** Templatized version of FindComponentByClass that handles casting for you */
	template<class T>
	T* FindComponentByClass() const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UActorComponent>::Value, "'T' template parameter to FindComponentByClass must be derived from UActorComponent");

		return (T*)FindComponentByClass(T::StaticClass());
	}
```

Three things we adopt directly:

1. **A `static_assert` constraining `T`, with a sentence-long message.** Our
   `std::is_base_of_v<EntityBehaviour, T>` is the same guard; without it a wrong `T`
   produces a page of template errors instead of one line.
2. **The template is a thin wrapper over a non-templated search.** UE's real work is in a
   normal function; the template only casts. Ours is the same shape — the lookup is
   type-independent, only the final cast is not.
3. **It returns `nullptr`, never asserts.** `CastChecked` exists separately for callers who
   want the assert. We ship only the query half, per this card's AC.

The non-templated half, `Engine/Source/Runtime/Engine/Private/Actor.cpp:3991-4008`:

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

**This is the shape RAD-101 turns ours into**: a linear scan over the owned list,
`IsA` per element, first match wins. Worth internalising now — the *signature and contract*
we write today survive that change untouched, which is the whole reason the query goes on
`Entity` rather than exposing the component.

### The mechanism — why UE does *not* use `dynamic_cast`

`Engine/Source/Runtime/CoreUObject/Public/Templates/Casts.h:93-171`, trimmed to the load-bearing branch:

```cpp
// Dynamically cast an object type-safely.
template <typename To, typename From>
inline TCopyQualifiersFromTo_T<From, To>* Cast(From* Src)
{
	static_assert(sizeof(From) > 0 && sizeof(To) > 0, "Attempting to cast between incomplete types");
	...
			else if constexpr (std::is_base_of_v<To, From>)
			{
				return Src;                       // upcast: free, no check needed
			}
			else
			{
				if (((const UObject*)Src)->IsA<To>())
				{
					return (To*)Src;              // checked by reflection, then C-cast
				}
			}
	...
	return nullptr;
}
```

And, at the very bottom of the same header (`Casts.h:591`):

```cpp
#define dynamic_cast UE::CoreUObject::Private::DynamicCast
```

UE **redefines the C++ keyword itself** so that any `dynamic_cast` involving a `UObject`
is rerouted through `Cast<>` and its reflection chain, falling back to the real
`dynamic_cast` only for non-`UObject` types (`Casts.h:540-548`). That is how strongly UE
prefers its own type check over the compiler's.

### Why UE's `Cast` is genuinely faster — and the trigger that would make us want one

Worth understanding rather than assuming, because the answer decides whether we build an
equivalent. UE's cast is fast in three tiers:

**Tier 0 — the upcast is free.** `Casts.h:152-155` uses `if constexpr (std::is_base_of_v<To, From>)`
to delete the check at compile time. `dynamic_cast` optimises this case too, so this tier is a wash.

**Tier 1 — a 64-bit bitmask for ~64 privileged types.** `Casts.h:465-468`:

```cpp
#define DECLARE_CAST_BY_FLAG(ClassName) \
	class ClassName; \
	template <> \
	constexpr inline EClassCastFlags UE::CoreUObject::Private::TCastFlags_V<ClassName> = CASTCLASS_##ClassName;
```

`CASTCLASS_AActor = 0x0000001000000000` (`ObjectMacros.h:372`); each `UClass` stores the OR of its
own bit and its ancestors', so `Cast<AActor>` reduces to `(ClassCastFlags & Flag) != 0`
(`Class.h:4546-4549`) — one AND, one compare.

**Tier 2 — the general `IsA` is O(1), not a walk.** `Class.h:430-434`:

```cpp
	[[nodiscard]] inline bool IsChildOfUsingStructArray(const FStructBaseChain& Parent) const
	{
		int32 NumParentStructBasesInChainMinusOne = Parent.NumStructBasesInChainMinusOne;
		return NumParentStructBasesInChainMinusOne <= NumStructBasesInChainMinusOne && StructBaseChainArray[NumParentStructBasesInChainMinusOne] == &Parent;
	}
```

Every class carries its **full ancestry as a flat root-first array plus its own depth**, so "am I a
child of X?" is "is X's depth ≤ mine, and is slot[X.depth] of my ancestry exactly X?" — constant time
regardless of hierarchy depth. The naive walk it replaced is still in the tree, cross-checked against
it under `ensureMsgf` (`Class.cpp:2791-2807`).

UE then `#define`s the `dynamic_cast` keyword itself to reroute into this (`Casts.h:591`), which is
how strongly it prefers its own path.

**Why we still do not build one.** The speed does not come from `Cast` — it comes from `UClass`.
Every tier above is a thin read of a runtime class object holding a flag mask, an ancestry array and
a depth. Radiant has no such object, so a `Radiant::Cast<T>` today would either be a rename of
`dynamic_cast` (§9's wrapper-for-no-reason) or the start of hand-building `UClass`, which is the
O(N)-edits-per-type registry this card already rejected, answers only *exact* type without the
ancestry array, and pays RAD-98's declined reflection tax for one of its many benefits.

**The trigger that reopens this: RAD-72 landing `entt::meta`.** That supplies the class object from a
dependency already vendored, at which point a `Cast<T>` becomes a reader of a real data structure
rather than a rename — the same reasoning that makes `dynamic_cast` the choice that gets *replaced*
rather than *reworked*. Any measurement claim before then is gated on RAD-67 / RAD-83, not asserted.

**What we adopt:** the contract (checked, returns null), the `static_assert`, the
upcast-is-free observation — `dynamic_cast` already handles that case optimally, so we get
it without the `if constexpr`.

**What we deliberately do not adopt, and why our scale justifies it:**

- **No reflection-backed `IsA`.** UE's mechanism is affordable because it *already* pays
  for `UClass` registration for serialization, Blueprints, replication and the GC. We pay
  none of that (RAD-98 declined the trade explicitly), so building a class chain solely to
  beat `dynamic_cast` would be paying UE's cost for one of UE's many benefits.
- **No `Cast<T>` free-function wrapper.** UE needs one because it has something faster to
  route to. We would be renaming a language feature — the exact wrapper-for-no-reason smell
  §9 warns about. Gameplay that holds a raw `EntityBehaviour*` can write `dynamic_cast`
  itself; in practice nothing does, because it goes through `Entity`.
- **No `ExactCast` / `CastChecked` variants.** Both fail RAD-94's customer gate today. Note
  `ExactCast` exists to answer a *different* question — "is it exactly T, not a subclass" —
  which is precisely why a stored-type-id scheme (§9) cannot substitute for `IsA`.

---

## 3. File Plan

```text
Radiant/Source/Radiant/ECS/
├── Entity.h            (modify) — declare GetBehaviour<T>(); doc the query family vs the verb gates
├── EntityTemplates.h   (modify) — define GetBehaviour<T>(): IsValid guard, lookup, dynamic_cast
└── Components.h        (modify) — NativeScriptComponent doc: Instance is reached via GetBehaviour<T>

Radiant/Source/Radiant/Gameplay/
└── EntityBehaviour.h   (modify) — doc: how a concrete behaviour is recovered; the virtual dtor is load-bearing twice

Reaper/Source/Layers/
└── GameLayer.cpp       (modify) — C and N cheats use the new API; new B probe cheat (own commit)

Docs/
└── Gameplay-Framework.md (modify) — "Recovering a concrete behaviour" section; Known Issues update

.claude/references/
└── radiant-playbook.md (modify) — §10: the query family is not governed by the verb gates
```

| Action | Path | Description |
|--------|------|-------------|
| Modify | `Radiant/Source/Radiant/ECS/Entity.h` | Declaration + Doxygen contract: returns null on all four negatives, transient pointer, never assert. One sentence added to the verb-gate block scoping the gates to *named* verbs. |
| Modify | `Radiant/Source/Radiant/ECS/EntityTemplates.h` | Definition. `static_assert` on `T`, `IsValid()` early-out, `try_get`, `dynamic_cast`. |
| Modify | `Radiant/Source/Radiant/ECS/Components.h` | `NativeScriptComponent` doc points at `Entity::GetBehaviour<T>()` as the supported way to reach `Instance`. |
| Modify | `Radiant/Source/Radiant/Gameplay/EntityBehaviour.h` | Class doc gains the recovery sentence and the note that the virtual destructor is now load-bearing for RTTI as well as deletion. |
| Modify | `Reaper/Source/Layers/GameLayer.cpp` | `C` and `N` cheats drop `NativeScriptComponent` + `static_cast`; new `B` cheat probes the three negative cases. |
| Modify | `Docs/Gameplay-Framework.md` | New Architecture subsection + Known Issues line retired. |
| Modify | `.claude/references/radiant-playbook.md` | §10 gains the query-family/verb-gate distinction. |

**No new files.** Worth stating explicitly: `Entity.h` lives in `ECS/` and `EntityBehaviour`
lives in `Gameplay/`, and the layering rule says `Gameplay/ → ECS/`, never the reverse.
This change does not violate it, because `Entity.h` names `EntityBehaviour` only through
the **forward declaration that already exists** in `Components.h:131` — a name, not an
include, so no dependency edge is added to the build graph. The template *definition* also
needs no include: a template body is only compiled when instantiated, and any call site
must already have the complete `T`, which cannot be complete unless its base is
(see §10 for the one case where that reasoning could bite).

---

## 4. Type Design

No new types. One new member on an existing one.

### `Entity::GetBehaviour<T>()`

- **Kind:** public template member function on `class Entity` (declared `Entity.h`, defined `EntityTemplates.h`)
- **Signature:** `template<typename T> T* GetBehaviour() const;`
- **Responsibility:** answer "is this entity running a behaviour of type `T`, and if so where is it?" — checked, in one call, with no engine plumbing named at the call site.
- **Ownership:** returns a **non-owning observer pointer**. The `NativeScriptComponent` owns the instance and deletes it (a known plain-data violation moving to a Level-owned side table with RAD-30). Nothing about the returned pointer extends its life.
- **Lifetime & threading:** the pointer is **transient**, with the same lifetime as the `Entity` handle it came from — it dangles the moment the entity is destroyed, the behaviour is rebound, or the Level dies. Never store it across a fixed step; store the `Entity`, or the `UUID` if it must survive longer. Main-thread only, like every `Entity` accessor.
- **Contract — returns `nullptr`, silently, in all four negative cases:**
  1. this handle is invalid (dead entity, or no Level);
  2. the entity has no `NativeScriptComponent`;
  3. it has one, but the instance is not created yet (lazily built on the first `Level::OnFixedUpdate` after `Bind<T>()`);
  4. the instance exists but is not a `T` (nor derived from `T`).
- **Never asserts.** Compile-time only: `static_assert(std::is_base_of_v<EntityBehaviour, T>, "…")`.
- **Playbook patterns:** §10 (dividing rule — names one entity, so it lives on `Entity`), §2 (transient, non-owning), §3 (adds nothing to the component), §4 by analogy (a routine query does not log).

Shape of the definition (concept-level — the engineer writes the real one):

```cpp
template<typename T>
T* Entity::GetBehaviour() const
{
    static_assert(/* T derives from EntityBehaviour */);

    // IsValid first, deliberately: TryGetComponent ASSERTS on a dead handle,
    // and a dead handle is a normal answer here, not a programmer error
    if (!IsValid())
        return nullptr;

    const auto* nsc = /* try_get the script component */;
    if (!nsc || !nsc->Instance)
        return nullptr;

    return dynamic_cast<T*>(nsc->Instance);   // the checked cast; null if not a T
}
```

---

## 5. Implementation Steps

### Phase 1 — Verify the mechanism before building on it
- [x] **Confirm RTTI is enabled in all three configs.** `Build.lua` and the project premake files set no `rtti` directive, so premake emits no `<RuntimeTypeInfo>` element and MSVC's default `/GR` applies — verified absent from the generated `Radiant.vcxproj` / `Reaper.vcxproj`. Confirm positively by building **Dist** with a `dynamic_cast` present and observing it links and returns non-null at runtime (§11). Record the finding on the story — it is an explicit AC.

### Phase 2 — The engine query
- [x] **Declare `GetBehaviour<T>()` in `Entity.h`.** Place it with the generic query family (`TryGetComponent` / `HasComponent`), not among the named verbs. Full Doxygen block: what it returns, the four null cases, the transient-pointer contract, and the "never asserts, this is a query" sentence.
- [x] **Add one sentence to `Entity.h`'s verb-gate block** scoping the two gates to *named* verbs, with the reason (template queries are bounded by construction). Keeps the next addition meeting a decision rather than a precedent — playbook §10's own rule about itself.
- [x] **Define `GetBehaviour<T>()` in `EntityTemplates.h`.** `static_assert` first; `IsValid()` early-out with the comment saying *why* it cannot delegate to `TryGetComponent`; then lookup and `dynamic_cast`.

### Phase 3 — Documentation chores (Claude)
- [x] **`Components.h`** — `NativeScriptComponent`'s doc gains: `Instance` is reached through `Entity::GetBehaviour<T>()`; touching it directly is the unchecked-cast trap.
- [x] **`EntityBehaviour.h`** — class doc gains the recovery line, and the note that the virtual destructor is now load-bearing twice: polymorphic deletion *and* the RTTI that makes the query possible. Deleting it would silently break `GetBehaviour` at compile time in a confusing place.
- [x] **`Docs/Gameplay-Framework.md`** — new subsection under Architecture ("Recovering a concrete behaviour"), problem-first per the docs convention, carrying the UE `FindComponentByClass` comparison and the `dynamic_cast`-vs-reflection decision. Retire the "No typed retrieval (RAD-100)" bullet from Known Issues and replace it with the RAD-101 forward note (linear search, duplicate types).
- [x] **`.claude/references/radiant-playbook.md` §10** — one bullet: the two gates govern named verbs; generic template queries are a separate, self-bounding family; `GetBehaviour` is the worked example.

### Phase 4 — Reaper (separate commit, per RAD-92's rule)
- [x] **Post a scope amendment to RAD-92** *before* the scaffolding lands, enrolling the new `B` cheat for retirement — the rule RAD-92 exists to enforce, applied to itself (as RAD-90 and RAD-95 did).
- [x] **Rewrite the `C` cheat** (`GameLayer.cpp:328-343`) to `platform.GetBehaviour<CollisionLogger>()`. The `nsc` local and the `NativeScriptComponent` include-dependency disappear; the warn-and-recover branch stays, now covering all four null cases with one check.
- [x] **Rewrite the `N` cheat** (`GameLayer.cpp:354-366`) the same way, deleting the `// The unchecked cast RAD-100 exists to replace` comment along with the cast it describes.
- [x] **Add the `B` probe cheat** — one key, three `GAME_TRACE` lines, **no new entities**: positive (`Platform` → `CollisionLogger*`, non-null), wrong-type (`Platform` → `KinematicPlatform*`, null), and scriptless (`Reaper` → `CollisionLogger*`, null). Marked "retires with RAD-92". `B` is free — the used set is Escape/T/M/G/Space/V/R/C/N plus the camera's WASD/QE and UILayer's F1–F3.

### Phase 5 — Verification
- [x] **Build all three configs** via the CLI MSBuild recipe in CLAUDE.md; zero new warnings.
- [x] **Run Reaper in Debug** and press `B`, `C`, `N`; check against §11.
- [x] **Run Reaper in Dist** and press `C` — the square must visibly vanish on contact, and the `GAME_WARN` arm line must appear in `Reaper/Radiant.log`. Do NOT expect `B` to print here; see §11's Dist note for why.

---

## 6. Ownership & Lifetime Strategy

Nothing about ownership changes — that is the point, and it is worth being explicit about
because a function returning a raw pointer is exactly where ownership questions get
fudged.

- **Who creates the instance:** `Level::OnFixedUpdate`, lazily, on the first fixed step
  after `Bind<T>()` (`Level.cpp:548-561`). Before that step, `GetBehaviour` legitimately
  returns null on a correctly-bound entity — case (3) in §4, and the reason the existing
  cheats already carry a "created lazily on the first fixed update" comment.
- **Who owns it:** the entity's `NativeScriptComponent`, by an owning raw pointer. The
  known plain-data violation (playbook §3), tracked by RAD-30.
- **Who deletes it:** `Level::DestroyEntity` (`Level.cpp:105-116`) runs `OnDestroy` and
  then `DestroyScript`.
- **What `GetBehaviour` returns:** a **non-owning observer**. It does not extend life, does
  not participate in refcounting, and has no `Ref`/`Scope` story of its own — playbook §2's
  "one ownership story per type" is satisfied by the pointer having *no* ownership story,
  stated at the declaration.

**The lifetime hazard, stated plainly for the header:** the returned `T*` is transient with
exactly the lifetime of the `Entity` handle that produced it. It dangles when the entity is
destroyed, when the component is replaced (`AddOrReplaceComponent<NativeScriptComponent>`
deletes the old instance — Reaper does this at `GameLayer.cpp:56`), or when the Level dies.
Re-query rather than store; the query is cheap enough (§7) that caching it is a net loss in
safety for a rounding error in speed. RAD-101 makes this *worse*, not better — with several
behaviours on one entity, a sibling's actions can invalidate a stored pointer, so the
contract written now is the one that must hold then.

---

## 7. Performance Notes

**No hot-path impact, and no per-frame cost introduced** — nothing calls this from a loop.

Cost per call, honestly accounted:

- One `IsValid()` (an entt validity check — an index compare against the pool's generation).
- One `try_get<NativeScriptComponent>` — a sparse-set indirection, a couple of dependent loads.
- One `dynamic_cast`. On MSVC this lowers to a call into the CRT's `__RTDynamicCast`, which
  walks the object's RTTI base-class descriptor table looking for a match. It is a real
  function call with a loop in it — not free, and *orders of magnitude* more expensive than
  the `static_cast` it replaces, which compiles to nothing at all. That is the trade this
  card is making, deliberately, and it should be stated that way rather than hidden.

Why it is nonetheless the right trade here: the intended call sites are event-shaped, not
frame-shaped — a bullet asking what it hit, a cheat key, a trigger resolving a door once in
`OnCreate`. At that frequency the cost is unmeasurable. **If a caller ever wants it per
frame per entity, the correct fix is to cache the `Entity` and query once in `OnCreate`,
not to cache the pointer** (§6) — the same guidance `GameplayLevel::FindEntityByName`
already carries for the same reason.

Forward-looking, both directions:

- **RAD-101 makes it worse by a constant factor**: the single lookup becomes a linear scan
  over the entity's behaviour list with a `dynamic_cast` per element. With the handful of
  behaviours per entity a 2D game carries, still irrelevant; the shape is UE's and UE ships
  it at far larger scale.
- **RAD-72 makes it better**: `entt::meta` type ids replace `__RTDynamicCast` with an
  integer compare against a cached parent chain — UE's mechanism, arrived at from a
  dependency already in the build. The signature does not change when that happens, which
  is the payoff for putting the query on `Entity` instead of exposing the component.

No allocations. No GPU implications. Nothing in a render path.

---

## 8. Logging & Diagnostics

**The engine query logs nothing, at any severity.** This is a deliberate policy decision,
not an oversight, and the header must say so — playbook §10's rule that a verb defined by
its silence cannot be verified by an absent log line applies directly to how we test it
(§11).

The reasoning, case by case:

| Case | Behaviour | Why |
|------|-----------|-----|
| Invalid handle | silent `nullptr` | `OnCollisionEnd`'s documented contract is that the partner may be invalid — this is the normal path, and a WARN would fire on correct gameplay. |
| No `NativeScriptComponent` | silent `nullptr` | "Does this thing have a behaviour?" is a legitimate question with a legitimate "no". |
| Instance not yet created | silent `nullptr` | Timing, not error. Documented; callers that care check on a later step. |
| Type mismatch | silent `nullptr` | The entire purpose of the query. |
| `T` not derived from `EntityBehaviour` | **`static_assert`** | A programmer error the compiler can catch, so it is caught there and never reaches runtime. |

Contrast with playbook §4's physics resolvers, which WARN on a stale id: there the caller
asked for an *action* on a thing it believed existed, so the mismatch is news. Here the
caller asked a *question*, and "no" is an answer.

`RADIANT_ASSERT` appears nowhere in this code path — the AC requires it. Note the
implementation consequence again because it is easy to get wrong: routing through
`TryGetComponent` would import its `RADIANT_ASSERT(IsValid(), …)` and violate this table in
Debug and Release while passing in Dist.

**Game-side diagnostics** carry the whole verification burden (§11): the `B` cheat emits
three `GAME_TRACE` lines naming the case and the outcome, so a single keypress produces a
readable record of all three answers.

---

## 9. Scalability Review

**The chosen approach is the most scalable option available today, and the flags below are
about what it must *not* become.**

- **`GetBehaviour<T>` is O(1) edits per new behaviour type — by construction.** A new
  behaviour subclass requires zero engine edits: no registration, no header touch, no
  serializer entry. This is the property that disqualified the alternative and it should be
  the first thing checked if anyone proposes changing the mechanism.

- **FLAG — a stored type-id would be O(N) edits *and* answer the wrong question.** The
  tempting optimisation is to stash `entt::type_hash<T>::value()` in `NativeScriptComponent`
  at `Bind<T>()` and integer-compare before casting. Reject it now, for two reasons, the
  second being the one that actually kills it: (1) it puts type machinery back into a
  component that playbook §3 wants to be plain data; (2) an **exact** type-id compare
  cannot answer "is it a `T` **or derived from** `T`", which is the question `IsA` and
  `dynamic_cast` answer. UE keeps `ExactCast` as a *separate* function precisely because
  these are different questions. A query that silently means "exactly T" would fail the day
  someone writes `class PatrollingGuard : public Guard` and asks for a `Guard`.

- **FLAG — do not add per-behaviour-type accessors on `Entity`.** `entity.GetCollisionLogger()`
  is RAD-94 category A wearing a new coat, and it would make `Entity.h` grow per behaviour
  type — the exact failure the template exists to prevent. Already covered by RAD-94's
  written rejection; noted here because `GetBehaviour` is where the pressure will appear.

- **Not a flag, but the known reshape:** RAD-101 turns the single lookup into a linear scan
  and must define duplicate-type semantics. Its own AC names this; **reject-at-attach** is
  the answer that keeps *this* card's contract honest, because it is what makes "**the**
  behaviour of type `T`" a total, unambiguous phrase rather than "some behaviour, whichever
  we hit first". Writing the contract in the singular today is safe *because* RAD-101 will
  enforce that — the dependency runs from that decision to this wording, and both cards
  should say so.

- **Deliberately not built (RAD-94's customer gate):** `HasBehaviour<T>()` (it is
  `GetBehaviour<T>() != nullptr`, and the pointer form is strictly more useful);
  `GameplayLevel::FindEntitiesWithBehaviour<T>()` (a *world query* — RAD-76's territory,
  and it wants a different implementation entirely, not a wrapper around this);
  `GetBehaviourChecked<T>()` (UE's `CastChecked` equivalent — no caller). Each is a one-liner
  the day something needs it; none should land speculatively.

---

## 10. Risks & Edge Cases

- **Header self-containment (playbook §10's last bullet).** The template definition in
  `EntityTemplates.h` uses `EntityBehaviour` as an incomplete type at *declaration* time and
  requires it complete at *instantiation* time. That is safe in every real call — `T` cannot
  be a complete type unless its base is — but it means a call site that somehow named `T`
  without `EntityBehaviour.h` in scope would get a confusing error. Mitigation: the
  `static_assert` fires first with a readable message, and `Radiant.h` already exports
  `EntityBehaviour.h` for game code. **Do not add an include of `Gameplay/EntityBehaviour.h`
  to `ECS/Entity.h` to "fix" this** — that inverts the layering direction the module split
  exists to establish.

- **RTTI could be disabled later without anyone noticing this depends on it.** No premake
  file mentions `rtti` today, so the dependency is invisible in the build config. Mitigation:
  the header doc states the dependency in words, and the `EntityBehaviour.h` note records
  that the virtual destructor is load-bearing for RTTI. If Dist ever adopts `/GR-` for size,
  this breaks loudly at compile time, not silently — acceptable.

- **A future engine DLL boundary.** `dynamic_cast` across module boundaries is reliable on
  MSVC within one process and the same CRT, and today Radiant is a **StaticLib** linked into
  Reaper, so there is no boundary at all. The real hazard is a *hot-reloaded* game DLL
  defining behaviour types (a Phase 5 editor concern): reloading it invalidates the RTTI
  descriptors instances were created against. Out of scope; noted so RAD-72 / the editor
  work inherits the question rather than rediscovering it.

- **Rebinding invalidates outstanding pointers silently.** `AddOrReplaceComponent<NativeScriptComponent>`
  deletes the old instance — Reaper does exactly this at `GameLayer.cpp:56`. Any `T*` held
  across that call dangles. Covered by the transient-pointer contract; no code guard is
  possible without ownership machinery this card deliberately does not add.

- **The `B` probe's "wrong type" case must stay wrong.** It asks `Platform` for a
  `KinematicPlatform*`, which is null only because `Platform` is bound to `CollisionLogger`.
  If someone rebinds the debug `Platform` entity, the probe silently starts asserting
  nothing. Mitigation: the probe logs the *expected* answer alongside the actual one, so a
  drifted fixture reads as a mismatch rather than as a pass.

- **`GetBehaviour<EntityBehaviour>()` is legal and returns any bound instance.** Not a bug —
  it is the base-class query, and `dynamic_cast` handles it as a free upcast. Worth one
  sentence in the doc so it is understood as designed rather than accidental.

---

## 11. Verification (AC → proof)

| Acceptance Criterion | How to verify |
|----------------------|---------------|
| A concrete behaviour is recoverable from an `Entity` with no `friend`, no reach into `NativeScriptComponent`, no unchecked cast | Grep `Reaper/` for `NativeScriptComponent` after the change: the only survivors are `AddComponent<…>().Bind<T>()` attach sites (RAD-101's territory). Zero `static_cast` on a behaviour anywhere in `Reaper/`. |
| A type mismatch returns nullptr rather than asserting or returning a wrong-typed pointer | `B` cheat, line 2: `Platform` queried for `KinematicPlatform*` logs `null (expected null)`. Run in **Debug** specifically — an assert would fire there and not in Dist. |
| An entity with no script, or a script not yet instantiated, returns nullptr | `B` cheat, line 3: `Reaper` (no behaviour bound) queried for `CollisionLogger*` logs `null (expected null)`. The not-yet-instantiated case is covered by the existing lazy-creation path — the `C`/`N` cheats' recover branch fires if pressed before the first fixed step. |
| The mechanism choice (RTTI vs type-id) is recorded with reasoning and Dist verification | §1, §2 and §11's Dist note below. **Corrected 2026-08-25 during `/review`** — see the note; the `B` probe cannot verify Dist. |
| Reaper's `C` cheat uses it, replacing the raw `static_cast` | Press `C` after the level loads: `Collision cheat: platform will destroy the next thing that touches it` still logs, the Green Square teleports and is destroyed on contact, and the platform's `OnCollisionEnd` with `<destroyed>` arrives one step later — the RAD-29 behaviour, unchanged. |
| Lifetime documented: the pointer is transient, never stored | `Entity.h` doc block review during `/review`; §6 of this plan is the long form. |
| Compiles clean in Debug/Release/Dist, no new warnings | CLI MSBuild all three configs per CLAUDE.md; diff the warning count against the pre-change baseline. |
| Reaper runs and is visually verified | **Debug:** press `B`, `C`, `N` — three probe lines matching their stated expectations, the `C` destroy sequence, and the `N` spawn/destroy toggle still alternating. **Dist:** press `C` only — `B` and `N`'s diagnostics are `GAME_TRACE` and compile out (see the Dist note). The square must visibly vanish on contact, and `Reaper/Radiant.log` must carry the `GAME_WARN` arm line. |

### The Dist note (corrected 2026-08-25 during `/review`)

The original version of this table asked for the Dist proof by **pressing `B` and reading line 1**. That is impossible, and the correction is worth keeping visible rather than quietly rewriting, because the reasoning generalises.

`GAME_TRACE` does not exist in Dist (`Core/Log.h:45-55`):

```cpp
#ifndef RD_DIST
	#define GAME_TRACE(...)       ::Radiant::Log::GetGameLogger()->trace(__VA_ARGS__)
#else
	#define GAME_TRACE(...)       ((void)0)
#endif
```

The `B` probe logs exclusively through `GAME_TRACE`, so in Dist all three lines — and the `GetBehaviour` calls inside them — compile away to nothing. **`B` is a Debug/Release instrument.** That is the right shape for it: its job is the two *negative* cases, and the failure it guards against (an assert on a dead handle, or on a type mismatch) fires in Debug and Release and never in Dist, so Debug is precisely where it must run.

**The Dist runtime proof is the `C` cheat**, for two reasons that have to hold together:

1. Its success message is `GAME_WARN`, which survives Dist deliberately (`Log.h:43-44`: *"a player's log of what went wrong is often the only diagnostic a shipped build produces"*).
2. The file sink in `Log::Init` (`Core/Log.cpp:25`) is **unconditional**, so that line reaches `Reaper/Radiant.log` even though Dist is a `WindowedApp` with no console attached (`Reaper/premake5.lua:57`).

And the primary evidence needs no logging at all — it is the **visual consequence**, which is the RAD-99 rule (playbook §10) applied again: a build with RTTI stripped would have `GetBehaviour<CollisionLogger>()` return null, the platform would never be armed, and the Green Square would land on it and sit there. Press `C`, watch the square vanish on contact. The bug's signature is the square *surviving*.

Observed while correcting this (2026-08-25): a clean Dist launch writes `Reaper/Radiant.log` as an **empty file** — the sink is constructed, and nothing on the startup path is above INFO. That is a happy accident for this check rather than a problem: in Dist the log stays empty unless something genuinely warns, so the `C` cheat's line arrives as the *only* line in the file and cannot be missed in the noise.

Static evidence, gathered once and recorded here so it is not re-derived:

| Signal | Result |
|---|---|
| `C4541` ("dynamic_cast used with /GR-") in any config | **absent** — `/GR-` still *compiles* `dynamic_cast`, so the absence of this warning is the positive signal, not the successful build |
| `__RTDynamicCast` in `bin/Dist-.../Reaper.exe` | present |
| RTTI type descriptors in the Dist image | `.?AVCollisionLogger@@`, `.?AVKinematicPlatform@@`, `.?AVSpawnProbe@@`, `.?AVCameraController@@`, `.?AVProbePassenger@@`, `.?AVEntityBehaviour@Radiant@@` |
| `rtti` directive in any premake file / `<RuntimeTypeInfo>` in either `.vcxproj` | none — MSVC's default `/GR` applies to all three configs |

**Additional check not in the AC, worth running:** the `N` cheat's rewrite must keep the
RAD-95 property it exists to prove — after pressing `N`, `[spawn-probe] passenger OnCreate`
must still appear on the step *after* the spawn line. `GetBehaviour` sits outside the script
pass, so it cannot affect this; confirming it is how we know the rewrite changed only the
lookup.