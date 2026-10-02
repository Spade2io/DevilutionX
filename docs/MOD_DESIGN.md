# Essence Mod Design — Classless Diablo (DevilutionX)

Living design document. Ideas here are direction, not final decisions — open questions are marked.

---

## Core vision
Diablo 1 is already mostly classless: any character can learn any spell from books, and class mainly
affects stats and one class skill. This mod leans into that. **Your build is defined by the essences
you equip, the abilities you slot, how you've used them, and your stats** — not by a fixed class.

Existing classes and their sprites stay as body/appearance choices (plus palette recolors). No new
character art is planned; identity comes from abilities and effects instead.

---

## 1. Essence pages (spellbook rework)
Replace the current spellbook pages with **essence pages**.

- An **essence** is equipped to the character and defines a page (e.g., Death, Flame, Frost, Blood, Arcane).
- The character has **4 essence pages**, each with **5 ability slots** → 20 abilities total.
- Players **custom-design each page** by slotting abilities they've learned.
- Spell books **teach** an ability into the player's personal library; slotting decides what's usable.
- Essences themselves are loot (rare drops).

**Controller / hotkey proposal:** all 4 pages are equipped, but **one page is active at a time**.
The player cycles pages (like a weapon swap), so only 5 abilities are on the bar at once.
Switching essences mid-fight becomes a skill (e.g., curse a pack from the Death page, flip to Flame to finish).

**Tab layout (decided 2026-10-02):** the mod targets Hellfire, whose spellbook has 5 tabs. Tabs 1–4
are the four essence pages (5 abilities each). **Tab 5 holds non-essence skills** the character can learn.

**Open questions**
- What counts as a non-essence skill on tab 5, and how many slots does that tab have?
- What does an essence do itself?
  - (a) just a themed container,
  - (b) container + passive bonus (e.g., +fire damage, +curse duration),
  - (c) **modifies what's slotted in it** (Plague essence makes slotted abilities spread; Frost adds slow).
  Option (c) multiplies build variety without needing more spells.
- Can any ability go on any page, or must it match the essence theme?
- When can loadouts change — anywhere, town only, or for a cost?

**Persistence:** essence loadouts are permanent character data → **sidecar save file**.

---

## 2. Curse system
Build curses as **one general system**, then make group curses different delivery methods.

### Data model
Each monster gets a few **curse slots** (in memory only, not saved):
```cpp
// sketch, not real DevilutionX code
struct ActiveCurse {
    CurseType type;
    int strength;   // may scale with caster's Magic
    int ticksLeft;
    int casterId;   // credit + multiplayer
};
ActiveCurse curses[4]; // per monster
```

### Effect categories and where they hook in
| Curse kind | Example | Hook point |
|---|---|---|
| Stat change | Take +25% damage, armor down | Damage calculation |
| Behavior | Slow, fear, confuse | Monster AI / movement |
| Damage over time | Rot, bleed | Per-tick monster update |
| Triggered | Explode on death, heal attacker | Death / hit handlers |

### Group curse delivery methods
- **Burst** — missile impact applies the curse to all monsters in a radius.
- **Field** — lingering cursed ground; monsters are cursed while inside, fades when they leave.
- **Contagion** — on death (or periodically) the curse jumps to the nearest uncursed monster.
- **Pack curse** — curse a unique leader and its minion pack is cursed too (uses D1's existing leader/minion packs).
- **Linked fate** — damage to one linked monster is partly shared across the group (most complex).

### Rules
- Recasting the same curse **refreshes** it; different curses coexist up to the slot limit.
- Bosses/uniques get shorter durations or immunity to behavior curses (no permanent lockdown).
- Scaling example: Magic → strength, spell level → duration.
- Clear **visual feedback** on cursed monsters (tint or overhead icon).

### Implementation note
Most D1 spell effects are implemented as **missiles** (even non-projectiles like Mana Shield and
Fire Wall). New curses = new missile types + the curse-slot system.

---

## 3. Other spell effects (auras, buffs)
- **Damage aura** — a missile that follows the player and periodically damages monsters in a radius.
  Hellfire's Immolation and wall spells are good references. Easiest of the new effect types.
- **Buffs** — modeled on Mana Shield: set a player flag/stat, expire after a duration, and hook into
  the relevant calculations. Temporary → not saved.

---

## 4. Learn-by-doing progression
Abilities improve from **effective use**, not from reading duplicate books.

- **What counts:** results, not casts — damage dealt, monsters affected, curse time spent on enemies,
  damage prevented by buffs. Casting in town or at nothing earns nothing.
- **Challenge scaling:** XP depends on monster level vs. ability level, so farming weak monsters is pointless.
- **Credit tracking:** D1 missiles already carry their source player and spell, so damage can be credited back to an ability.
- **Essences level too**, from use of abilities slotted on their page (could unlock stronger page effects).
- **Milestones:** each level adds a little power; every few levels the ability gains a new behavior
  (e.g., curse starts spreading at rank 5, becomes a field at rank 10).
- **Books** teach new abilities; rare **tomes** could grant a burst of XP.
- **Swap tension:** to keep loadouts flexible — unslotted abilities keep their level, new abilities
  level faster early, and some XP may be shared within an essence.
- **UI:** progress bar per ability in the spellbook.

**Open question:** should progress mainly mean bigger numbers, or mainly new behaviors at milestones?

**Persistence:** ability XP and essence levels → **sidecar save file**.

---

## 5. Stat-driven spells
Rewrite spell formulas so stats matter to spells. Examples:
- Fireball damage scales with **Magic**
- Fire Wall / aura duration scales with **Vitality**
- Chain Lightning jump count scales with **Dexterity**
- Melee buffs scale with **Strength**; curse strength with **Magic**
- Mana Shield efficiency improves with Magic

Remember to update spellbook **tooltips** so displayed numbers match. Expect heavy balance tuning,
especially with +stat items.

---

## 6. Mana regeneration
Small, self-contained change in the per-tick player update. If computed from stats/essences, it needs
**no new saved data**.

Options:
- **Stat-based** — scales with Magic.
- **Combat-aware** — fast out of combat, slow in combat.
- **Earned** — mana on kill/hit (D1 already has mana-steal affixes to build from).
- **Essence-driven** — Death: mana when cursed enemies die; Blood: spend life when out of mana;
  Flame: regen accelerates while continuously casting; Arcane: strong regen while not taking damage.
- **Learn-by-doing** — regen improves the more mana you spend.

Balance watch-outs: mana potions lose value; Mana Shield becomes a second life bar with fast regen.

**Open question:** universal regen rule, or mana behavior as part of each essence's identity?

---

## Current roadmap (decided 2026-10-02)
The order we are actually building in. Supersedes the build order below where they differ.

1. **Mana regeneration** — Magic-based. Vanilla has no passive player mana regen (confirmed in code).
2. **Spell learn-by-doing** — one spell first (Firebolt), then level-ups, sidecar save, progress bar, all spells.
3. **Stat increases from spell increases** — rules to be defined.
4. **Spell books for essences** — to be defined.
5. **Spell books for awakening stones** — to be defined; "awakening stone" is a new concept not yet described here.

## Build order
Each step builds the foundation the next one needs.

1. **Mana regen** — simple Magic-based regen with an out-of-combat boost. Learn where player state and the per-tick update live.
2. **Trace an existing spell** (Mana Shield or Fire Wall) end to end, cast → missile → effect.
3. **One single-target stat curse** (Amplify-Damage style) — proves curse slots, the damage hook, and duration countdown.
4. **Make it a burst** — first group curse.
5. **Field** curse, then **contagion** or **pack** curse.
6. **Stat-driven formula** for one spell (e.g., Fireball uses Magic) + tooltip update.
7. **Learn-by-doing for one ability** — XP from damage dealt, saved to a sidecar file, progress bar in UI.
8. **Essence pages** — spellbook UI rework, loadouts, active-page cycling, sidecar persistence.

---

## To verify in the code (don't assume)
- How many spellbook pages exist in Diablo vs. Hellfire, and how the panel is laid out.
- Vanilla mana regeneration behavior (believed: life doesn't regen, mana regens very slowly).
- Where spell/missile data is defined now (C++ tables vs. newer data files).
- How DevilutionX stores its stash outside the main save — a possible model for the sidecar file.
- Where spell damage formulas and spellbook tooltip numbers are calculated.
- Current state of the Lua mod API, and whether any of this could be Lua instead of C++.
