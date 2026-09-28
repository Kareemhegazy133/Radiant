# Reaper — Game Design Document

**Status:** Draft v0.6 — 2026-09-27
**One line:** A Hollow Knight-style metroidvania with Elden Ring's progression depth.

---

## 0. Decision History

Recorded so these are not relitigated. Each was a deliberate change of direction, not drift.

| Date | Decision | Outcome |
|---|---|---|
| 2026-09-27 | Perspective: top-down vs side-view | **Side-view**, locked. Art cost (one facing vs eight) and silhouette readability decided it. |
| 2026-09-27 | Reference balance | **Hollow Knight is primary. Elden Ring supplies selected systems.** Not a 50/50 blend. |
| 2026-09-27 | Stamina | **Cut.** See §2.2. |
| 2026-09-27 | World gating | **Ability-gated (metroidvania)**, with lethality-gated optional content. |
| 2026-09-27 | Classes | **Yes — Dark Souls-style starting classes**, not locked classes. See S6.1. |
| 2026-09-27 | Dungeons | **Yes — the regions already are dungeons**; add Elden Ring-style optional *minor* dungeons. See S9.1. |
| 2026-09-27 | Puzzles | **Yes — spatial and mechanism only.** Item/logic puzzles rejected. See S11. |
| 2026-09-27 | Boss count | **Capped at 4 main + 3 mini for v1.0.** Beyond that, reuse rigs. See S8.1. |
| 2026-09-27 | Dialogue | **Yes — Tier 2 (flag-gated linear), not branching trees.** Reverses an earlier "no dialogue system" call, which was over-trimming. See S10. |
| 2026-09-27 | NPCs | **Yes — ~12 total, sparse and stationary, lines change with world flags.** See S10. |
| 2026-09-27 | Shops | **Yes — fixed finite stock, souls as the only currency.** See S12. |
| 2026-09-27 | Loot | **Hand-placed only. No random drops, no rarity tiers, no rolled stats.** See S12. |
| 2026-09-27 | Map | **Added — was an oversight.** Hollow Knight's gating model, hand-drawn art. See S13. |
| 2026-09-27 | Slice scope | **None of the above is in the slice.** See §7 and §7.1. |

---

## 1. The North Star vs. The Deliverable

**Reaper is a Hollow Knight-like.** That is the game: a side-view, hand-authored, interconnected world you explore, with precise platforming, readable combat, and a death loop with real stakes.

**From Elden Ring we take the things Hollow Knight does not have** — specifically progression depth and weapon variety (§2.1). We are not making a soulslike with a map; we are making a metroidvania with a soulslike's sense of consequence and an RPG's sense of build.

**The scope ladder:**

| Tier | What it is | Size |
|---|---|---|
| **The Slice** | "The First Fog Gate" — one bench, one route, one two-phase boss | ~15 min of play |
| **v1.0 Reaper** | Three regions, four bosses, six weapons, two traversal upgrades, one ending | ~4 hours of play |
| **The North Star** | Hollow Knight's density and craft | never "done", always the bar |

**The board only ever holds The Slice, plus the next region.**

---

## 2. Perspective: Side-View — LOCKED 2026-09-27

**Side-view, in the lineage of Hollow Knight, Blasphemous, Ender Lilies, and Nine Sols.**

Top-down (Hyper Light Drifter / Death's Door) was proposed and rejected. Reasoning on the record:

1. **Art cost, and it is not close.** Top-down needs 8-directional — or minimum 4-directional — sprite sets for every animation on every character. Side-view needs one facing, mirrored. A 4–8x reduction in the most expensive asset category in the project. For a solo developer, art is the bottleneck, not engine cards.
2. **Silhouette readability.** A side-view sprite telegraphs an attack far more legibly — the whole body reads against the background. Telegraph readability is load-bearing for the entire genre.
3. **Verticality is a design axis.** Metroidvania topology — interconnection, shortcuts, one-way drops — is far richer in side-view, and interconnection is the heart of what we are building.
4. **Deeper genre precedent.** Five shipped, heavily-studied reference games to learn from.

**Accepted cost:** the character controller is substantially harder — gravity, variable-height jumps, coyote time, jump buffering, one-way platforms, slope handling, corner correction, and later wall-cling and double-jump. That is the single largest engine card in the project.

**We accept it because it is the point.** A kinematic 2D character controller is the most educational artifact in gameplay programming — every "feel" technique in the discipline lives inside it. This project exists to build principal-level experience, so a system that is harder and more instructive is a feature.

**Physics consequence:** the player is **not** a Box2D dynamic body. Side-scrolling controllers fight dynamic solvers — sticky walls, slope jitter, no control over jump arcs. The player is a **kinematic body with manual collide-and-slide**. Enemies, props, and projectiles stay dynamic Box2D bodies. This split is what every shipped 2D action game does.

---

## 2.1 What We Take From Elden Ring

Hollow Knight is a nearly perfect game with three structural gaps. Elden Ring fills exactly those.

| # | Import | Why — what gap it fills |
|---|---|---|
| **1** | **Souls as XP + currency, with stats** | **The single biggest win.** Hollow Knight has *no levelling at all* — power comes only from exploration pickups, so killing an enemy you have already beaten is worthless. Souls-as-runes makes every fight pay, gives the player a build identity, and creates the risk/reward tension of carrying a large unbanked sum. Cheap to build, enormous design return. |
| **2** | **Weapon variety with real movesets** | Hollow Knight has one nail that gets numerically stronger. Six weapons with distinct reach, speed, arc, and poise damage give genuine playstyle expression and replayability — and weapons are the most content-scalable system in the game. Proven in 2D by Salt and Sanctuary and Blasphemous. |
| **3** | **Poise / stagger** | Adds a second axis to combat beyond HP. Some enemies can be stunlocked, some cannot, and heavy weapons earn their slowness. Cheap to implement, large depth return. |
| **4** | **Boss framing: fog gate, phase 2, the walk-back** | Hollow Knight already does most of this. Elden Ring's framing sharpens it: a sealed arena, a named bar, a real phase transition, and the walk-back as deliberate cost rather than friction. |
| **5** | **Lore by item description** | Both games do this. Adopted because it means we never build a dialogue, quest, or cutscene system. |

### What we deliberately leave behind

| Left in Elden Ring | Why |
|---|---|
| **Stamina** | See §2.2 — the most important cut in this document. |
| **Equip load / armour weight** | A whole gear system to serve one stat. Weapons already carry build identity. |
| **Lethality-only world gating** | We are a metroidvania. The map opens through ability, Hollow Knight's way. |
| **Weapon Arts / Ashes of War** | Hollow Knight's spell system does the same job more simply. See S7. |
| **Multiplayer, invasions, messages** | Out of scope permanently. |

---

## 2.2 The Stamina Cut

**Decision: Reaper has no stamina bar.**

This is the highest-consequence call in the document, so the reasoning is explicit.

**Why cut it.** Hollow Knight's identity is *freedom of movement* — you dash, jump, and swing as much as you like, and the difficulty comes from precision and pattern-reading rather than resource management. A stamina bar layered onto constant platforming does not read as tension; it reads as exhaustion. Blasphemous and Salt and Sanctuary carry stamina because they are soulslikes with a map. We are the other thing.

**What replaces it as the commitment mechanism.** Pillar 1 survives intact, on a better mechanism for this game:

- **Attack recovery frames.** Every attack has wind-up, active, and recovery. You cannot cancel out of recovery except into a dash, and only after the active frames. Mashing gets you hit — the punishment is positional, not numerical.
- **Healing costs Focus, and Focus is earned by fighting** (S2). You cannot heal unless you have already landed hits, and channelling roots you in place. That is a sharper risk/reward loop than a stamina bar, because it is *earned* rather than merely *spent*.

**What we gain.** One fewer attribute, one fewer UI bar, one fewer tuning axis fighting the platforming, and a combat rhythm that matches the reference game.

**Reversibility:** cheap during the slice, expensive after. If combat proves shallow in playtesting, stamina is the first thing to reconsider.

---

## 3. Fiction & Tone

You are a **Reaper** — not a hero, an instrument. Reapers harvest souls to sustain a mechanism at the centre of a dying world. The mechanism is failing. The things you are sent to harvest are increasingly things that used to be people.

**Tone:** melancholy, not grim-cool. The world already lost; you are late. Nothing explains itself. Everything is the ruin of something that used to work.

**Storytelling method:** three channels, no cutscenes, no quest log, no voice acting. See S10 for the full treatment.

1. **Item descriptions** — a sword's text tells you who carried it and how they failed
2. **Environmental storytelling** — the ruin itself is the evidence
3. **NPC dialogue** — sparse, stationary characters whose lines change as the world does

**Why this matters mechanically:** we never build a cutscene system, a quest system, or a branching dialogue tree. Dialogue is linear text gated by world flags — the flags we already need for save/load. The fiction rides on infrastructure the game requires anyway.

**The fiction earns the mechanics.** A reaper harvests souls — so souls are literally the currency, the XP, and the thing you lose on death. Focus is the reaper's channelled power, built by harvesting and spent to sustain yourself. The systems and the story say the same thing.

---

## 4. Design Pillars

Three. Everything must serve one, or it gets cut.

### Pillar 1 — Precision & Commitment
Platforming and combat both demand exactness. Attacks have recovery frames you cannot cancel out of. Difficulty comes from reading and executing, never from resource attrition. **A death must always be legible as your own mistake.**

### Pillar 2 — Meaningful Death
Death drops the souls you were carrying and resets the world. You get exactly one chance to recover them. Death is the teaching mechanism and the tension generator — never a slap on the wrist, never a permanent loss of progress.

### Pillar 3 — The Map Is the Reward
The world is one continuous, hand-authored, interconnected space. The payoff is not a cutscene; it is the moment a locked door opens from the far side and the geography clicks into place. Shortcuts, one-way drops, and the slow conversion of hostile territory into safe routes.

---

## 5. Core Loops

**Moment-to-moment (seconds):**

```
approach -> read telegraph -> dash through or jump over it
   -> punish in the recovery window -> build Focus -> reposition
```

**Encounter (minutes):**

```
enter room -> read the vertical layout -> isolate and pull one enemy
   -> land hits to bank Focus -> heal in a safe window, or press on
```

**Session (tens of minutes):**

```
rest at bench (heal, respawn world) -> spend souls on levels
   -> venture out -> harvest -> die, or reach the next bench
   -> recover the bloodstain -> find a shortcut -> fog gate -> boss -> new region
```

The three loops nest, and that nesting is the genre.

---

## 6. Systems

### S1 — Character Controller

**Intent:** movement is precise, responsive, and never floaty. You always know exactly where you will land. This is the most important system in the game and deserves the most tuning time by a wide margin.

**Ground movement**
- Acceleration and friction curves, not instant velocity — responsive without being twitchy.
- **Facing** flips on direction input, and locks during attacks.
- No sprint. Hollow Knight has one movement speed, and it is correct — a sprint button in a game with this much platforming is a button you hold permanently.

**Jump** — every technique below is a standard game-feel tool, and implementing them is a large part of why side-view was chosen:

| Technique | What it does |
|---|---|
| **Variable height** | Releasing jump early cuts upward velocity — short tap, short hop |
| **Coyote time** | ~5 frames of grace to still jump after walking off a ledge |
| **Jump buffering** | ~6 frames of grace to queue a jump pressed just before landing |
| **Apex float** | Slightly reduced gravity near the top of the arc — makes aiming a landing feel controlled |
| **Fast fall** | Higher gravity on descent than ascent — kills the floaty moon-jump |
| **Corner correction** | Nudge the player sideways when they clip a corner by a pixel or two |

**Dash** — the primary defensive verb. Horizontal, fixed distance, short cooldown.

| Phase | Frames | Behaviour |
|---|---|---|
| Startup | 1–2 | Committed, no i-frames |
| Invulnerable | 3–10 | i-frames, velocity locked to the dash vector |
| Recovery | 11–16 | Vulnerable, cannot act, can buffer the next input |

Shorter and snappier than a soulslike roll — this is Hollow Knight's dash, not Elden Ring's. The cooldown, not a stamina cost, is what stops it being spammed.

**One-way platforms:** jump up through, press down + jump to drop through. Load-bearing for S9.

**Traversal upgrades (post-slice, v1.0):** wall cling and slide, then double jump. Each is a controller state, and each meaningfully enlarges the reachable map.

**Engine needs:** fixed-timestep frame counting (have it), input action mapping, input buffering, kinematic body with manual collide-and-slide, slope and one-way platform resolution.

---

### S2 — Focus

**Intent:** the resource that makes healing a decision. This is Hollow Knight's SOUL, and it is a better fit for this game than a stamina bar.

- **Focus is earned by landing hits.** A fixed amount per connected attack, more for heavy hits.
- **Focus is spent on healing and on spells.** That is the entire tension: heal now, or bank it for a spell?
- **Healing is a channel.** Hold the button, root in place for ~1.0s, restore one health. Getting hit interrupts it and **wastes the Focus**.
- **Focus does not persist through death**, and it does not carry between areas. It is a combat resource, not a currency.

**Why this is better than stamina here (§2.2):** you cannot heal unless you have already fought well, and healing demands you find a safe window in a fight rather than back off and wait for a bar to refill. It rewards aggression and punishes panic — which is exactly the rhythm Hollow Knight's combat has.

**Engine needs:** attribute system, UI meter.

---

### S3 — Melee Combat

**Intent:** hits must feel like they *land*. Impact is roughly 50% animation, 30% hit-stop and screenshake, 20% audio. The damage number is the least important part.

**The attack set** — side-view gives a directional moveset for free, and it is much of what makes 2D combat expressive:

| Input | Move | Purpose |
|---|---|---|
| Attack | **Light attack** | Fast, chains to a 3-hit combo with tightening input windows |
| Hold attack | **Heavy attack** | Slow, high damage, high poise damage, breaks guards |
| Up + attack | **Up-slash** | Hits flying and elevated enemies |
| Down + attack (airborne) | **Pogo** | See below — the signature 2D verb |

**The pogo.** A downward air attack that, on connect, **bounces the player upward**. Simultaneously a combat verb (safe damage from above), a traversal verb (chain bounces across hazards), and a skill-expression ceiling. Hollow Knight builds half its late game on this. Cheap to implement, enormous value — in from the slice.

**Attack structure:** wind-up -> active (hitbox exists) -> recovery.

**Cancel rules:** an attack cancels into a dash *only after the active frames*, never during wind-up. This is the commitment rule, and with stamina cut (§2.2) it is now the sole mechanism of Pillar 1. Tune it carefully.

**Hit-stop:** on a successful hit, freeze attacker and victim for 3–6 frames scaled by damage. This single feature does more for game feel than any other code in the project.

**Poise / stagger** *(Elden Ring import)*: every entity has poise. Poise damage accumulated within a window staggers the victim into a long recovery. Heavy and charged attacks build it fast. Heavy enemies have enough poise that they cannot be stunlocked — the player must respect them.

**Engine needs:** physics overlap queries, sprite animation with frame events, time-scale/freeze, particles, audio, screenshake.

---

### S4 — Health, Death, Recovery

**Intent:** death is a tax, not a punishment.

- **HP** comes from the **Vitality** stat, displayed as discrete masks rather than a bar — discrete health reads instantly and makes every hit legible.
- **Healing** is by channelling Focus (S2). There are no flask charges — Focus *is* the limiter, and it is earned rather than refilled.
- **Death:** drop all carried souls at the death location as a **Bloodstain**. Respawn at the last bench. All non-boss enemies respawn.
- **Bloodstain recovery:** touch it to recover everything. **Die again before reaching it and it is gone permanently.** One chance, always.
- **Death by falling into a pit:** the bloodstain drops at the last safe ground position, not in the pit. Non-negotiable quality-of-life.

**Engine needs:** attribute system, save/load, level reset, spawn templates.

---

### S5 — Enemies & AI

**Intent:** every enemy is a readable pattern. The player must always be able to say "that was my fault."

**The telegraph rule:** every attack has a wind-up long enough to react to, with a distinct animation and audio cue. An unreadable attack is a bug, not difficulty.

**AI state machine:**

```
Idle -> Perceive -> Approach -> Telegraph -> Attack -> Recover -> Reposition
             ^                                                        |
             +--------------------------------------------------------+
```

- **Perception:** horizontal sight range plus a facing check, plus a hearing radius. Aggro is sticky with a leash that returns them home.
- **Ledge awareness:** ground enemies must not walk off platforms while chasing. A ground-probe raycast ahead of the feet, checked before committing to movement. Cheap, and its absence is instantly visible as stupidity.
- **Reposition:** enemies back off and re-approach rather than standing in your face — this creates the spacing game.
- **Poise:** enemies stagger; heavy ones cannot be stunlocked.

**Slice archetypes (three, enough to test everything):**

| Archetype | Role | Teaches |
|---|---|---|
| **Husk** | Fast, low HP, low poise, 2-hit combo, ground-bound | Basic timing and dash direction |
| **Bell-Keeper** | Ranged, holds a ledge above you, forces you to close or pogo | Vertical threat, positioning under pressure |
| **Warden** | Slow, high poise, huge telegraphed overhead | Patience, and that some things cannot be stunlocked |

Placing the Bell-Keeper *above* the player is the deliberate test that the vertical axis is doing design work.

**Engine needs:** AI framework (state machines + perception), physics queries and raycasts, spawn templates, animation.

---

### S6 — Stats & Progression *(Elden Ring import #1)*

**Intent:** fix Hollow Knight's one structural gap — that killing an enemy you have already beaten is worth nothing.

- **Souls** drop from every enemy, and are simultaneously the levelling currency and the shop currency. Carrying a large unbanked sum is a real risk.
- Level up **only at a bench**. The cost curve is superlinear, so levels get progressively more expensive.

| Stat | Governs |
|---|---|
| **Vitality** | Health masks |
| **Attunement** | Max Focus, and Focus gained per hit |
| **Strength** | Scaling on heavy weapons |
| **Dexterity** | Scaling on fast weapons |

Four stats, not eight. Every stat must have an obvious build identity, or it is noise.

### S6.1 — Starting Classes

**Decision: Dark Souls-style starting classes, not Diablo-style locked classes.**

The distinction is the entire cost difference, so it is stated plainly:

| Model | What it is | Cost |
|---|---|---|
| **Starting class** (Dark Souls, Elden Ring) | A named starting *condition* — stat spread, starting weapon, starting spell. Builds converge as you level. | **Near zero.** A data table and a menu. |
| **Locked class** (Diablo, most MMOs) | Permanent identity with its own abilities and progression tree. | **N times the content.** N movesets, N ability sets, N balance passes. |

We take the first. A class is a **row in a YAML table** — name, starting stats, starting weapon, starting spell, flavour text — plus a selection screen. Once S6 and S7 exist, a class costs roughly an afternoon, and it buys immediate build identity and the cheapest replayability lever in the game.

**v1.0 classes (five):**

| Class | Stats | Starting weapon | Identity |
|---|---|---|---|
| **Warden** | Str-heavy, high Vitality | Reaper's Edge | The balanced default. Recommended for a first run. |
| **Husk** | Dex-heavy, low Vitality | Husk Daggers | Fast, fragile, punishes tiny windows |
| **Bellringer** | Attunement-heavy | Bell Spear + starting spell | Ranged pressure, Focus-hungry |
| **Gravedigger** | Str extreme, no Attunement | Warden's Greatsword | Slow, enormous poise damage, no spells |
| **The Hollow** | All stats minimum, level 1 | Broken blade | The deprived. For the second playthrough. |

**Why classes fit this game specifically:** they are an Elden Ring import that costs nothing, they make the first ten minutes feel authored rather than generic, and in a metroidvania where everyone eventually finds the same items, the *starting* condition is the only cheap differentiator available.

**Engine needs:** attribute system, save/load, UI, a class definition asset type (trivial — it is a stat block).

---

### S7 — Weapons & Spells *(Elden Ring import #2)*

**Intent:** the weapon *is* the build. Changing weapons should change how you play, not just your damage number. This is the deepest single addition over Hollow Knight, which ships one nail.

Each weapon defines: damage, attack speed, range, arc, poise damage, scaling letters (Str/Dex), and **its own moveset** including up-slash and pogo.

**v1.0 weapon set (six, one per playstyle):**

| Weapon | Identity |
|---|---|
| **Reaper's Edge** | Starting blade. Balanced. The tutorial for every mechanic. |
| **Husk Daggers** | Very fast, tiny range, low poise damage. Dex scaling. Punishes tiny windows. |
| **Warden's Greatsword** | Slow, enormous poise damage, wide arc. Str scaling. |
| **Bell Spear** | Long reach, thrust moveset, safe poking. Excellent pogo reach. |
| **Chained Censer** | Swinging arc, hits above and around cover, awkward timing. Str scaling. |
| **Pale Scythe** | The reaper's own weapon. Wide sweep, harvests extra souls on kill. |

**Spells** (Hollow Knight's approach, not Elden Ring's Ashes of War): three spells, each costing Focus, each found rather than bought — a ranged bolt, a downward area attack, a screen-clearing scream. They compete with healing for the same resource, which is the entire point.

**Engine needs:** data-driven weapon definitions as an asset type, animation per moveset, attribute system.

---

### S8 — Bosses

**Intent:** the punctuation. Everything the player has learned, tested at once.

- **Fog gate** — a trigger volume that seals the arena and starts the encounter.
- **Named health bar** across the bottom of the screen.
- **Arena shape:** wide, flat, mostly featureless for the first boss — the player should be reading the boss, not the terrain. Later bosses earn platforms and hazards.
- **Phase 2 at 50% HP** — a transition animation (invulnerable), then new moves added to the pool. Never merely "more damage."
- **On death:** the fog gate remains; the player respawns at the bench and walks back. The walk-back is deliberate — the cost of failure, and the time to think about what went wrong.
- **On victory:** a large soul reward, the fog clears permanently, and a new region opens.

**Slice boss — "The First Warden":** a scaled-up Warden.

- **Phase 1:** three attacks — overhead slam, horizontal sweep, step-back recover. All heavily telegraphed. Teaches dash timing and punish windows.
- **Phase 2 (50%):** adds a delayed second slam — the classic "wait for it" bait — and a charging shoulder that must be jumped rather than dashed. Teaches the player to *read* rather than pattern-match, and that both defensive verbs matter.

### S8.1 — The Cost of a Boss, and How to Afford More

Boss fights are already the core of S8, so the real question is not *whether* but *how many* — and bosses are **the most expensive content type in the entire game.**

A genuinely new boss needs a unique sprite set, a unique animation set (idle, three-plus attacks, telegraphs, transition, stagger, death), a bespoke AI state machine, a purpose-built arena, and its own audio. Solo, that is realistically **two to three weeks each**. Four main bosses plus three mini-bosses is already an ambitious v1.0.

**Three ways to afford more, in order of value:**

1. **Reuse the rig, change the moveset.** A boss that returns later with two new attacks and a palette shift costs roughly **20%** of a new one, and reads as an escalation rather than a repeat. Elden Ring does this shamelessly and it works.
2. **Promote an elite enemy.** Take an existing enemy, scale it, give it one new telegraphed attack and a health bar. This is what the slice's First Warden already is — a scaled-up Warden — and it is why the slice is affordable.
3. **Arena as the variable.** The same boss in a different arena — narrow ledges, a hazard, a one-way drop — plays differently for almost no art cost.

**Rule:** every new boss beyond the four main ones must justify itself as a *new pattern to learn*, not as new art. If it does not teach the player something, reuse a rig instead.

**Engine needs:** trigger volumes, boss UI, multi-phase state machine, camera framing, audio, persisted boss-defeated flags.

---

### S9 — World Structure

**Intent:** Hollow Knight's interconnection. One continuous hand-authored space, not a level select.

- **Benches:** save point, level-up point, health restore, world respawn. Sparse — their scarcity is what makes the space between them tense.
- **Shortcuts:** the signature unlock — a ladder kicked down, a gate opened from the far side, a drop that becomes a loop. Every region gets at least one, and they are the primary reward of exploration.
- **One-way drops:** you can always fall *down* into somewhere dangerous; climbing back out requires finding the route. This is how the world stays open while still having topology.
- **Ability gating:** wall cling and double jump open regions. **Both are v1.0, neither is in the slice.**
- **Lethality gating where it fits:** at least one area is reachable early and will simply kill you — the Elden Ring flavour, applied as seasoning rather than structure.

**v1.0 regions (three):**

| Region | Identity | Boss |
|---|---|---|
| **The Harvest Fields** | Open, horizontal, gentle. Teaches the loop. | The First Warden |
| **The Bell Cloister** | Vertical ruined towers, ranged pressure from above, one-way drops | The Bell Mother |
| **The Mechanism** | The failing machine at the world's centre. Moving platforms, hazards. | The Last Reaper |

### S9.1 — Dungeons

**Reframe: the three regions already *are* dungeons.** Hollow Knight's Crystal Peak, Deepnest and City of Tears are dungeons — themed areas with a gimmick, an enemy set, and a boss at the end. That is exactly what S9 describes, so the main-path ask is already satisfied.

What is genuinely worth *adding* is Elden Ring's **minor dungeon** — the catacombs, caves and tunnels. These are:

- **Short** — four to eight rooms, ten minutes
- **Self-contained** — one entrance, one exit, sealed off from the main map
- **Built around one gimmick** — a hazard, a mechanism, an enemy type used in an unusual way
- **Capped by a mini-boss** — very often a *reused* boss with a new moveset or a palette change
- **Optional** — they never gate the critical path, so they never need balancing against required progression
- **Rewarding** — a weapon, a spell, an upgrade material, or a lore item

**Why this is the single most valuable content structure in the game:** a minor dungeon costs **zero new engine systems**. It is a tilemap, a trigger, existing enemies, and a reward — all of which the main path already required. It is how a solo developer scales a world without scaling the engine, and it is precisely how Elden Ring padded a 60-hour map with a team that could not hand-author every metre of it.

**v1.0 target: two minor dungeons per region, six total.** They come after all three regions exist, never before — content breadth is the last thing added, not the first.

**Engine needs:** tilemap with collision generation, one-way platforms, level load/unload, trigger volumes, save/load. **Minor dungeons add nothing to this list.**

---

### S10 — Lore, Dialogue & NPCs

**Intent:** make the world feel inhabited and already-lost. The player should meet people who are not quest-givers — they are survivors with their own situation, who happen to talk to you.

#### The dialogue system: pick the cheap tier deliberately

The expensive part of dialogue is never the text box. It is **state tracking and authoring tooling**. A branching tree without an editor becomes unmaintainable YAML within twenty lines. So the tier is chosen up front:

| Tier | What it is | Cost | Verdict |
|---|---|---|---|
| **1 — Linear** | An ordered list of lines; the last one repeats forever | ~0.5 card | Not enough alone |
| **2 — Flag-gated linear** | Line *sets* selected by world flags; linear within a set | **~0.75 card** | **This one** |
| **3 — Branching trees** | Player choices, response nodes, per-node state | +2–3 cards, plus an authoring tool | Rejected for v1.0 |
| **4 — Questlines** | NPCs that move, multi-stage state, failure states | Very high | Rejected |

**Tier 2 is what Hollow Knight actually does**, and it delivers ~90% of that game's narrative effect. Elderbug's lines change as Hallownest empties. Myla's dialogue degrades as the infection takes her — and she is one of the most affecting characters in the medium, built entirely from *swapping which line set is active*.

**Crucially, Tier 2 needs no new state infrastructure.** It reads the same world flags that save/load (E14) already requires: bosses defeated, regions entered, items found. Dialogue rides on infrastructure the game needs anyway.

**Format:** one YAML file per NPC — a list of line sets, each with a flag condition and an ordered list of strings. Authorable by hand, diffable, no tool required.

#### The craft rules

These are what make sparse dialogue land, and they are cheaper than writing more of it:

1. **Sparse.** Roughly a dozen NPCs in the entire game. Scarcity is what makes meeting one feel like an event.
2. **Stationary, at memorable places.** At benches, at crossroads, at the edge of somewhere dangerous. The character and the location become one memory.
3. **They change as the world changes.** The same NPC, new lines, after you kill a boss or enter a region. This is what makes the world feel like it is moving without animating anything.
4. **They never explain the plot.** They talk about *themselves* — what they wanted, what they lost, what they are still doing here. The lore is the residue.
5. **Several have arcs that end.** An NPC who disappears, or whose last line set is their last, costs nothing extra and is where the emotional weight lives.

#### The v1.0 cast

| NPC | Where | Role |
|---|---|---|
| **The Keeper** | The first bench, permanently | The anchor. Warm, tired, stopped harvesting long ago. Comments on your progress. Your measure of how far you have come. |
| **The Wanderer** | A different bench in each region, always ahead of you | Cheerful in a doomed way. Has an arc, and an ending. |
| **The Cartographer** | Found lost somewhere in each region | Sells the region map (S13). Gives the map system a face. |
| **The Sister** | The hub | Merchant — consumables and upgrade materials (S12) |
| **The Hollowed** | A dead end in region two | A reaper who stayed too long. Speaks only in fragments, and the fragments degrade each time you return. |
| **The Smith** | The hub | Applies weapon upgrades. Barely speaks. |

**No voice acting.** Text only, permanently.

**Engine needs:** an interaction verb (shared with benches, shops and pickups — see S12), a text box UI, a dialogue component reading flag-gated line sets, world flags (E14).

---

### S11 — Puzzles & Environmental Interaction

**Intent:** break up combat pacing, make the world feel mechanical rather than decorative, and reward observation.

**The governing rule: puzzles are spatial, not cerebral.** The question is always *"how do I reach that?"* and never *"what is the code?"* A puzzle that stops the player to think in the abstract breaks Pillar 1's rhythm and belongs in a different genre. A puzzle that makes them look at the room and then execute a jump *is* Pillar 1.

**Three tiers, cheapest first:**

**Tier 1 — Traversal puzzles.** Already the metroidvania core loop and already in the design: a ledge you cannot reach yet, a gap that needs a pogo chain, a route visible but not yet accessible. **Cost: zero.** These are level design, not systems.

**Tier 2 — Mechanism puzzles.** Levers, pressure plates, timed doors, moving platforms, breakable walls, crushers. A lever opens a gate; a plate holds it only while weighted; a platform runs a timed circuit you must ride.

> **Cost: about half a card.** Every piece already exists — trigger volumes (E13), kinematic movers (shipped with RAD-90), and collision events (shipped with RAD-29). What is missing is only a small, data-driven **activation graph**: emitters (lever, plate, enemy death) wired to receivers (door, platform, hazard). That is a component with a target list, not a new subsystem.

These are also the natural home for shortcuts (S9) — a lever that opens a gate from the far side is a mechanism puzzle and a shortcut simultaneously.

**Tier 3 — Item and logic puzzles.** Zelda-style: find object A, combine with B, use on C. **Rejected.** They require an inventory UI, a use-item verb, and per-object interaction logic, and they pace like an adventure game rather than an action game. Nothing in the pillars asks for them.

**One exception worth keeping:** a small number of **secret walls and hidden rooms**, found by observation rather than logic. Effectively free — a tile flag and a fade-out — and they are a cornerstone of both reference games.

**Engine needs:** trigger volumes (E13), kinematic movers (have it), collision events (have it), plus a small activation-graph component. **No new subsystem.**

---

### S12 — Items, Loot & Economy

#### The loot philosophy — authored, never random

**Decision: every item in the world is hand-placed. There are no random drops, no rarity tiers, and no randomised stats.**

This is a genre boundary, not a preference. Randomised loot belongs to Diablo and its descendants, and importing it here would actively break Pillar 3:

> If the reward for reaching a dangerous ledge is a dice roll, exploration stops being **discovery** and becomes **grinding**. The player farms the roll instead of reading the map. In a hand-authored world, the reward has to be a specific, memorable, authored thing — *the* weapon on *that* corpse behind *that* waterfall.

Both reference games have exactly zero random loot, and that is not a coincidence — it is what makes their worlds worth exploring twice.

**What enemies do drop:**
- **Souls**, always — a fixed amount per enemy type (S6)
- **Consumables**, occasionally — a small, fixed chance of a throwable or a cure. Flavour, never a progression path.

That is the entire drop system. It needs no loot tables, no rarity rolls, and no stat generation.

#### Item categories

| Category | Examples | Stackable | In inventory? |
|---|---|---|---|
| **Weapons** | The six of S7 | No | Yes, equippable |
| **Spells** | The three of S7 | No | Yes, equippable |
| **Consumables** | Throwables, buffs, cures | Yes | Yes, quick-select |
| **Upgrade materials** | Weapon +1…+5 components | Yes | Yes, passive |
| **Key items** | Map fragments, lore objects | No | Yes, read-only |

Every item carries description text, and that text is a primary narrative channel (§3).

#### The hidden cost: inventory

**This is the part that is easy to underestimate.** Asking for "shops and loot" sounds like asking for a shop; the real cost is the **item data model and the inventory UI** underneath both. A shop is a list view and a transaction — perhaps half a card. An inventory is a persistent, save-serialised, navigable, categorised UI that every other system then talks to.

Kept deliberately minimal: a single scrollable list with five category tabs, an equip action, and a quick-select bar for consumables. **No grid, no drag-and-drop, no weight limit, no sorting options.** Those are MMO features and they serve nothing in the pillars.

#### Shops

- **Souls are the only currency** — the same resource as levelling. That is the point.
- **Two sinks, one pool.** Every visit to a bench is the question *"level up, or buy that weapon?"* — a genuinely good tension, imported wholesale from Elden Ring, and it costs nothing because both systems already exist.
- **Fixed, finite stock.** Each merchant has an authored inventory that never restocks and never randomises. When the Sister runs out of cures, that is a world state, not a timer.
- **Prices are tuned against the souls economy**, so that buying always costs a real number of levels.

**Engine needs:** the shared interaction verb, inventory data model + UI, a shop view, transaction logic against the souls attribute, save/load of inventory and stock state.

---

### S13 — The Map

**This section exists because its absence was an oversight, and in a metroidvania the map is a core system — not a UI afterthought.** A hand-authored interconnected world that the player cannot see a picture of is a world they cannot navigate, and Pillar 3 ("The Map Is the Reward") is unbuildable without it.

#### Hollow Knight's gating model, adopted wholesale

Three restrictions that together turn a convenience feature into a design pillar:

1. **You do not start with the map.** You must find the Cartographer somewhere in the region, lost and humming, and buy it from him. Until then you navigate by memory — which is precisely when you actually *learn* the space.
2. **The map only updates when you rest at a bench.** Explore a hundred rooms and the map still shows where you last sat. It converts "where am I?" into real tension and makes benches valuable beyond healing.
3. **Markers are bought separately.** Bench locations, shop locations, and custom pins are incremental purchases — another souls sink, another reason to explore.

This is one of the best designs in the genre, it is free to copy, and it makes the map *content* rather than overhead.

#### Implementation: hand-drawn, not procedural

**The map is authored art, one image per region — not generated from the tilemap.** This matters:

- A procedurally-derived map is a large engine feature (geometry simplification, region labelling, readable layout) and it always looks worse.
- A hand-drawn map is a PNG. The engine's job shrinks to *masking undiscovered areas* and *drawing a marker*.
- Hollow Knight's maps are hand-drawn, and their character is a large part of why the game is remembered.

**What the engine actually provides:** per-room discovered flags (part of E14's world flags), a full-screen map view that composites the region image against a discovery mask, a player-position marker, and pin placement.

**Engine needs:** world flags (E14), a UI view, and a texture composite. **~1 card** — most of the cost is authoring art, not code.

---

## 7. The Vertical Slice — "The First Fog Gate"

**This is the only thing on the roadmap until it is done and it feels good.**

### Content

One bench. A short route with three enemies (2x Husk on the ground, 1x Bell-Keeper on a ledge above), one one-way drop, and one Warden guarding the fog gate. Beyond it, a two-phase boss arena, and a second bench past that.

**About fifteen minutes of play.**

### The player must be able to

1. Run, jump with variable height, and dash with real i-frames
2. Light attack, chain three hits, heavy attack, up-slash, and **pogo off an enemy**
3. Build Focus by landing hits
4. **Channel to heal — and get interrupted, wasting it, by healing at the wrong moment**
5. Kill enemies and collect souls
6. **Die, lose the souls, walk back, and recover the bloodstain**
7. **Die again before reaching it, and lose those souls permanently**
8. Take a one-way drop into an area they cannot climb back out of
9. Rest at the bench — heal, and respawn the world
10. Spend souls to level Vitality or Attunement, and feel the difference immediately
11. Walk through the fog gate
12. Fight a boss with readable telegraphs and a real phase 2 demanding both dash and jump
13. Lose, walk back, and win

### Why this is the correct slice

It contains **every pillar** and **every core loop**, using one route, three enemies, and one boss. If these fifteen minutes feel right, the game works, and everything after is content and breadth. If they do not feel right, no amount of map will save it.

It is also the right *engineering* slice: it exercises the character controller, Focus, hitboxes, hit-stop, AI telegraphs, the death and respawn loop, world reset, progression, save/load, and boss structure — the entire spine of the engine's gameplay layer, proven end to end against something playable.

### What the slice explicitly does NOT include

**Classes. Minor dungeons. Mechanism puzzles. Any boss beyond the first. NPCs and dialogue. Shops. Inventory. The map.** Multiple weapons. Spells. Traversal upgrades. Upgrade materials. A second region. Poise. Any lore text beyond a single item. Any menu beyond the level-up screen.

That list is long on purpose. **None of it affects whether the fifteen minutes feel good**, which is the only question the slice exists to answer. A map you cannot get lost without, a merchant with nothing worth buying, and an NPC commenting on progress you have not made are all things that can only be evaluated *after* the combat and movement are proven.

Every one of these is approved for v1.0 (§7.1). None of them is in the slice, and the reason is the whole point of having a slice: **the slice exists to prove the fifteen minutes feel good.** Content added before that proof is content built on an unvalidated foundation, and it is exactly how the previous roadmap produced three months with nothing playable at the end.

---

## 7.1 Content Budget

Engine cost and content cost are different budgets, and conflating them is how solo projects die. The four systems added in v0.5 cost almost nothing in *engine* terms — and that is genuinely good news — but they are not free.

| Addition | Engine cost | Content cost | Verdict |
|---|---|---|---|
| **Starting classes** (S6.1) | ~0 — a YAML table and a menu | Low — five stat blocks, five flavour texts | **Cheapest value in the document** |
| **Minor dungeons** (S9.1) | **0** — no new systems at all | Medium — 6 dungeons x ~6 rooms + a reward each | **The scaling lever.** Do it last. |
| **Mechanism puzzles** (S11) | ~0.5 card — an activation graph | Low-medium — per-room authoring | **Yes.** Also gives shortcuts their mechanism. |
| **More bosses** (S8.1) | 0 — S8 already covers it | **Very high — 2-3 weeks each** | **Cap at four main + three mini for v1.0.** Beyond that, reuse rigs. |
| **Dialogue + NPCs** (S10) | ~0.75 card — text box + flag-gated line sets | Medium — writing ~12 characters well | **Yes.** Rides on save flags we need anyway. |
| **Shops + inventory** (S12) | **~1.25 cards** — inventory is the real cost, not the shop | Low — authored stock lists | **Yes.** Gives souls a second sink. |
| **Hand-placed loot** (S12) | ~0 — a trigger and a grant | Low — but every placement is a decision | **Yes.** Random loot **rejected** — it breaks Pillar 3. |
| **The map** (S13) | ~1 card — discovery mask + marker | Medium — hand-drawn art per region | **Yes, and it was missing.** Core system in this genre. |

**Total new engine work across every addition: roughly 3.5 cards.** The first four asks cost about half a card between them; the world layer (dialogue, shops, inventory, map) is the larger share, and inventory plus map are most of it.

They stay cheap *because* the systems layer was designed before the content was requested — which is the argument for having done it in that order.

**The real risk is the content budget, not the engine budget.** v1.0 is now: 3 regions, 4 main bosses, 3 mini-bosses, 6 minor dungeons, 6 weapons, 3 spells, 5 classes, 2 traversal upgrades, ~12 NPCs with written arcs, 2 merchants, and 3 hand-drawn region maps.

That is an ambitious but coherent solo scope *provided* it is built in this order:

> **slice → region 1 complete (incl. the world layer) → region 2 → region 3 → classes → minor dungeons → polish**

The world layer (S10, S12, S13) lands with **region 1**, not later — a region without a map, a merchant, or anyone to talk to does not prove the game, and building three regions before discovering that would repeat the original mistake at a larger scale.

**Designated cuts if the schedule slips, in order:** minor dungeons, then classes, then the third region. All three are additive, and nothing depends on them.

---

## 8. Engine Dependency Graph

### Already shipped

Fixed timestep with accumulator, event queue, per-Level Box2D v3 world, collision events, entity behaviours with composition, typed behaviour retrieval, gameplay dynamics verbs, deferred entity destruction, the batched sprite renderer, YAML levels, handle-based assets, and a basic camera controller.

**Every one of those is load-bearing for this slice.** The gameplay framework work of the last three months was not wasted — it was *unmotivated*. Those are different problems, and only the second needs fixing.

### Required for the slice, in dependency order

| # | Engine capability | What in the game needs it | Board |
|---|---|---|---|
| E1 | Pawn / Controller possession | Player and AI drive the same movement code | RAD-102 |
| E2 | Input action mapping | Jump / dash / attack as actions, rebindable | RAD-69 (icebox) |
| E3 | Input buffering + frame windows | i-frames, active frames, jump buffer, coyote time, cancel rules | **new** |
| E4 | **Kinematic character controller** | Gravity, collide-and-slide, slopes, one-way platforms, corner correction | **new — the big one** |
| E5 | Physics overlap queries + raycasts | Attack hitboxes, AI perception, ledge probes, ground checks | RAD-76 (icebox) |
| E6 | Sprite animation + frame events | Telegraphs; hitboxes fired from animation frames | RAD-64 (icebox) |
| E7 | Camera framework + screenshake | Horizontal lookahead, **vertical smoothing that ignores jump arcs**, room bounds, impact feel | RAD-75 (icebox) |
| E8 | Time scale / freeze | Hit-stop, and real pause | RAD-31 |
| E9 | Attribute system (GAS-lite) | Health, Focus, poise, damage, scaling | RAD-63 (icebox) |
| E10 | AI framework | State machines, perception, leashing, ledge awareness | RAD-78 (icebox) |
| E11 | Spawn templates / prefabs | Enemies, the boss, the bloodstain | RAD-74 (icebox) |
| E12 | Game UI framework | Health masks, Focus meter, souls counter, boss bar | RAD-71 (icebox) |
| E13 | Trigger volumes | Fog gate, bench activation, one-way drop markers | **new** |
| E14 | Save / load game state | Bench rest, souls, level, world flags | **new** |
| E15 | Audio | Impact, telegraph cues, jump, dash | RAD-65 (icebox) |
| E16 | Particles / VFX | Hit sparks, landing dust, soul absorption | RAD-70 (icebox) |
| E17 | Tilemap + collision generation | Authoring the route and the arena, one-way platforms, slopes | RAD-73 (icebox) |

### Required for region 1 — the world layer, immediately after the slice

Deliberately separated. These are what turn a proven combat prototype into a *game*, and none of them belongs in the slice.

| # | Engine capability | What in the game needs it | Board |
|---|---|---|---|
| W1 | **Interaction verb + prompt** | Benches, NPCs, shops, pickups, levers — one shared primitive | **new** |
| W2 | Dialogue: flag-gated line sets + text box | NPCs (S10) | **new** |
| W3 | Item data model + inventory UI | Weapons, spells, consumables, materials, key items (S12) | **new** |
| W4 | Shop view + transaction | Merchants (S12) | **new** |
| W5 | Map view: discovery mask + marker | The map (S13) | **new** |
| W6 | Activation graph (emitter → receiver) | Mechanism puzzles and shortcuts (S11) | **new** |
| W7 | Class definition asset + selection screen | Starting classes (S6.1) | **new** |

**W1 is the one to notice.** Benches, NPCs, shops, item pickups and levers are all *the same interaction*: stand near a thing, see a prompt, press a button, something happens. Built once as a shared primitive it is cheap; built five separate times it is five times the work and five inconsistent behaviours. This is exactly the kind of unification that only shows up if the content is designed before the code.

### The finding

**Thirteen of the seventeen slice capabilities already exist as cards.** The board correctly identified *what* the engine needs. It got *when* catastrophically wrong — nearly all sit in the Icebox, behind Vulkan, behind the asset pipeline, behind the editor.

The replan is therefore not "invent new work." It is **promote the Icebox, defer the layers, and let the slice order the cards.**

**The one genuinely large new card is E4, the kinematic character controller** — and it is the card most worth writing by hand, because it is where 2D game feel actually lives.

---

## 9. Open Decisions

| # | Decision | Status |
|---|---|---|
| D1 | Side-view vs top-down | **LOCKED: side-view** (§2) |
| D2 | Reference balance | **LOCKED: Hollow Knight primary, Elden Ring supplies systems** (§2.1) |
| D3 | Stamina | **LOCKED: cut** (§2.2). Reversible cheaply during the slice only. |
| D4 | World gating | **LOCKED: ability-gated, with lethality-gated optional content** (S9) |
| D5 | Player collision | **LOCKED: kinematic, manual collide-and-slide** (§2) |
| D6 | Art direction | **Recommend pixel art**, low resolution, small palette | 
| D7 | Animation | **Recommend flipbook**, not skeletal — far cheaper engine-side |
| D8 | Block / parry as a third defensive verb? | **Recommend no.** Dash and jump are enough. Revisit only if combat proves shallow |
| D9 | Health display: masks or bar? | **Recommend discrete masks** — every hit stays legible |
| D10 | Does the walk-back after a boss death stay? | **Yes.** It is a pillar, not friction |

---

## 10. Where This Document Lives

This is the game's design, so it lives with the game (`Reaper/Design/`), not in `Docs/`, which holds engine system architecture. Engine docs describe how Radiant works; this describes what Reaper is. The two must never merge — the engine never knows about the game.

**Update contract:** this document is the source of truth for game scope. If a Jira card contradicts it, one of the two is wrong, and it gets resolved here first.
