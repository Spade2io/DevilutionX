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
- **Item graphics (decided 2026-10-02, replaces the rune idea below):** essences use item picture **27**
  (the cube gem) and awakening stones use picture **26** (the round gem). Both are unused by any
  vanilla item. Each is tinted per essence or stone by remapping palette ramps at draw time; ten
  single-ramp colours exist (vivid red, orange, yellow, blue; muted red, orange, gold, steel blue,
  rose, grey). There is no green or purple in the fixed half of the palette. On the ground both
  currently show the shared rune animation (`runes1`, a small tumbling cube); picture 26 should be
  pointed at the Blood Stone animation (`bldstn`) in `ItemCAnimTbl` so it is round there too.
  Ten colours are enough for now. Kept in reserve: a few shades of purple (rose and blue mixed pixel
  by pixel) and a brighter orange (red and yellow mixed), plus two-tone stones. True green or purple
  would need either repainting part of the shared palette or drawing in more than 256 colours; not
  ruled out, just not now.
  Sample sheets and the scripts that make them are in `graphics_tool` (outside the repo).
- Earlier idea, superseded: essence items use the Hellfire rune graphics. Six rune
  graphics exist: Rune of Fire, Greater Rune of Fire, Rune of Lightning, Greater Rune of Lightning,
  Rune of Stone, and the Rune Bomb quest item (`ICURS_RUNE_*` in `Source/tables/itemdat.h`).

**Controller / hotkey proposal:** all 4 pages are equipped, but **one page is active at a time**.
The player cycles pages (like a weapon swap), so only 5 abilities are on the bar at once.
Switching essences mid-fight becomes a skill (e.g., curse a pack from the Death page, flip to Flame to finish).

**Tab layout (decided 2026-10-02):** the mod targets Hellfire, whose spellbook has 5 tabs. Tabs 1–4
are the four essence pages (5 abilities each). **Tab 5 holds non-essence skills** the character can learn.

**Page layout (decided 2026-10-02, part of "spell books for essences"):** remap each spellbook page
from 7 entries to **5**. Use the freed space for text on each spell: a description and the XP
required for its next level. Today's page is laid out in `Source/panels/spell_book.cpp`
(`SpellBookPageEntries = 7`, 43 pixels per entry).

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

## 7. Removing levels
Character levels will be ditched at some point, possibly with something else in their place. Not
designed yet, and not now. This section lists everything that depends on character level, so it
can be found when that time comes.

- **Spell XP level modifier (in effect).** Spell XP uses the game's existing kill-XP adjustment:
  10% more or less per level of difference between the monster and the **character's** level
  (deliberately not the spell's level). See `Source/spell_xp.cpp`. Needs a replacement when levels go.
- **Spell level-up table (in effect).** Spells level up on the character level XP table: spell level
  1 → 2 costs what character level 1 → 2 costs, and so on. Chosen because those numbers are already
  balanced around character growth. Also needs a replacement when levels go.

---

## Spell XP rule (decided 2026-10-02)
Spell XP = monster's kill XP × (damage dealt ÷ monster's total health) × level modifier.

- Damage beyond the monster's remaining health earns nothing, so one kill never pays more than 100%.
- Tracked in 64ths of a point so small hits still add up.
- Separate from, and in addition to, the character's normal kill XP.
- Level modifier: see "Removing levels" above.
- Only learned spells earn XP. Casting from a staff or scroll does not teach or level a spell.
- Level-ups use the character level XP table (see "Removing levels"). Maximum spell level is still 15.
- **Books only teach.** Reading a book of a spell you already know does nothing and the book is not
  used up. A book can never raise a spell's level.
- XP is saved in a sidecar text file next to the save (`essence_single_<slot>_<ext>.txt`), written
  whenever the hero is saved and deleted with the character. Spells are identified by number in that
  file, so the format needs revisiting when the spell list is overhauled.
- Known gap: loading an older saved game does not rewind spell XP to that save's moment.

## Spell list overhaul (planned)
Many spells and abilities will be added, and some current ones removed. Size unknown.

- **All characters will start with no spells** (decided 2026-10-02, not built yet). Today a Sorcerer
  starts with Firebolt at level 2; spell XP counts from the start of whatever level a spell already has.
- Learn-by-doing currently covers the damage spells that hit through a projectile (Firebolt, Charged
  Bolt, Holy Bolt, Lightning, Flash, Fire Wall, Fireball, Flame Wave, Nova, Inferno, Elemental, Blood
  Star, Bone Spirit, Apocalypse, Lightning Wall, Immolation). Guardian, Chain Lightning and Ring of
  Fire fire another spell's projectile, so their damage is credited to Firebolt, Lightning and Fire Wall.
- **Healing and Heal Other (decided 2026-10-02):** the healed player counts as an average monster of
  their own level. XP = average monster kill XP for that level × (hit points actually restored ÷ the
  target's maximum hit points), then the usual level modifier (target level vs caster level, so zero
  for self-healing). Healing at full health earns nothing. The average comes from a curve fitted to
  Hellfire's monster table: 5L² + 10L + 40 (`AverageMonsterExperienceForLevel` in `Source/spell_xp.cpp`).
  It depends on character level, so it is also affected by "Removing levels".
- Other non-damage spells (Mana Shield, Teleport, Stone Curse, Golem, and so on) earn nothing yet;
  each needs its own measure of effective use.

## 8. Randomized spell learning (to be designed)
A book never gives you a spell you already have, but it might give you a different one. How the
replacement spell is picked is undecided. Until this is built, a book of a known spell does nothing.

---

## Current roadmap (decided 2026-10-02)
The order we are actually building in. Supersedes the build order below where they differ.

1. **Mana regeneration** — Magic-based. Vanilla has no passive player mana regen (confirmed in code).
2. **Spell learn-by-doing** — one spell first (Firebolt), then level-ups, sidecar save, progress bar, all spells.
3. **Stat increases from spell increases** — rules to be defined.
4. **Essences (cube gem, picture 27)** — essence items use picture 27, tinted; behaviour to be defined. Includes remapping spellbook pages to 5 entries.
5. **Awakening stones (round gem, picture 26)** — stones use picture 26, tinted; behaviour to be defined.
6. **Removing levels** — later; see section 7 for what depends on character level.
7. **Randomized spell learning** — to be designed; see section 8.
11. **New effect testing** — current focus after the first awakening stones; see the "New effect testing" section.
10. **Player pets** — very low priority; see the "Player pets" section.
9. **Database and editor tool for essences and awakening stones** (next up) — one data file the game reads, plus a tool showing every entry in a big chart with "new essence" / "new awakening stone" buttons and in-place editing.
8. **Controller overhaul** — several spells available at once, each on its own button. The game already has 12 quick-spell slots (`QuickSpell1`–`12`) and controller button mapping, but a slot only selects the active spell; casting directly on the press is the new part.

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

---

## Temporary test tools
- **Pepin's test shop** (`Source/stores.cpp`, search "Essence Mod"). Pepin's menu has two extra lists:
  "Buy essences" (empty placeholder) and "Buy awakening stones", which lists every spell the player
  has not learned for 1 gold and teaches it directly on purchase, with no item handed over. For
  testing new spells only; remove or rework before the mod is real.
- **Books have no Magic requirement** (`Source/items.cpp`). Other requirements will replace it later.
- Debug builds also have a console (backtick key): `dev.player.spells.setLevel(1)` teaches every spell.

---

## How new spells are added (worked example: Frostbolt)
Awakening stones are the new spell books: right-click one to learn its spell. Later a stone will go
into one of an essence's five slots, with some randomness; for now it simply teaches the spell.

- **Spell numbers.** "Known spells" is 64 on/off bits, so there can be at most 64 spells. Hellfire
  uses 0–51; new spells take 52–63 and existing numbers are never changed. Frostbolt is 52.
- **Spellbook.** The spellbook is a live list of whatever the player knows, in spell-number order;
  no spell owns a fixed slot (`Source/panels/spell_book.cpp`). The final order is undecided.
- **Saving.** The original save only stores levels for spell numbers 0–46. Levels of higher
  numbers are kept in the sidecar file (`L <spell> <level>` lines).
- **Awakening stone items.** Each stone is its own row in `assets/txtdata/items/itemdat.tsv` with
  type `AWAKENINGSTONE` and its spell written in, like a scroll. Nothing about it is rolled from a
  seed, so it cannot change on reload. Picture 26; on the ground it uses the Blood Stone animation.
  Drop rate 0: stones are only sold by Pepin's test shop for now.
- **Casting animation rule (decided 2026-10-03):** every new spell uses the **Magic** casting
  animation (the `Magic` flag in `spelldat.tsv`) unless Bryan explicitly says it is a fire or
  lightning spell. The game has only three casting animations (Fire, Lightning, Magic), and the glow
  is painted into the character frames, so tinting it would recolour the character too. The same flag
  sets the colour of the spell's book. A better method for per-essence casting effects is still to come.
- **Steps for a new projectile spell:** add the spell and its projectile(s) to the enums and name
  parsers in `Source/tables/spelldat.h/.cpp`; add rows to Hellfire's `spelldat.tsv` and `misdat.tsv`
  (row position is the number); add a spellbook icon entry in `Source/panels/spell_icons.cpp`; add
  damage text in `GetDamageAmt` and the XP mapping in `GetSpellForMissile`; add its stone row.
- **Tints.** `Source/essence_tint.cpp` builds colour lookup tables; `DrawMissilePrivate` in
  `Source/engine/render/scrollrt.cpp` draws Frostbolt and its impact through the vivid blue one.
- **Known limits.** Frostbolt exists only in Hellfire (plain Diablo's tables are untouched). Its
  damage type is still Fire, since the game has no cold damage type. In multiplayer, the packet that
  shares a character's spell levels covers numbers 0–51 only, so other players would not see it.

---

## Player pets (very low priority, to be designed)
Summons beyond the single Golem. What the code does today, for when this is picked up:

- Golem is the only summon. The spell creates a monster; the monster system then runs it.
- One reserved monster slot per player, so "one summon each" is built into the structure.
- Stats are fixed at summoning: health = 10 per spell level + two-thirds of maximum mana; damage
  2 x (level + 4) to 2 x (level + 8); armour 25. See `InitGolem` in `Source/monster.cpp`.
- It attacks the nearest monster in melee, wakes nearby monsters, wanders when idle, takes no orders,
  and does not follow between dungeon levels.
- Kills are credited to the player, but pet damage is not a projectile, so it earns no spell XP yet.
- In multiplayer only the level owner's PC creates the monster and tells the others.

---

## New effect testing (decided 2026-10-03)
Before building the essence and awakening stone systems, try out individual spell effects one at a
time, each as a test spell with its own awakening stone. The first batch:

- **Persistent buff** — an effect on the caster that stays until it runs out or is removed (existing model: Mana Shield).
- **Buff aura** — a buff that applies to everyone near the caster.
- **Damage over time** — a hit that keeps hurting its target afterwards. New: nothing does this to monsters today.
- **Damage aura** — hurts monsters near the caster on a pulse (closest existing shape: Flash).
- **Cooldowns** — a wait before a spell can be cast again. New: mana is the only limit today.
- **Leech** — damage dealt gives back life or mana (existing model: life-steal and mana-steal items, weapon hits only).
- **Health regeneration** — life that comes back over time (model: the mana regen already built).

Buffs, auras and damage-over-time are temporary state: held in memory, never saved (hard rule 1).

### Persistent buffs (first one built 2026-10-03: Strength)
- Code: `Source/buffs.h/.cpp`. A buff is switched on by casting its spell and lasts until the player
  dies. Held in memory only; a character always loads with none.
- **Strength** (spell 53, icon 47, Magic casting animation, 10 mana, castable in town): +10 Strength
  at spell level 1, +5 per level after that. Applied where the game totals stats (`CalcPlrItemVals`).
- **Buff XP (tentative):** every active buff's spell earns one-tenth of the XP a damaging spell just
  earned. Every XP source feeds buffs, healing included (decided 2026-10-03). Weapon hits (melee, arrows, and spells cast from a staff or
  scroll) are valued the same way as spell damage and feed the buffs their tenth; no ability earns
  the attack's own XP yet. Melee abilities that level like spells are planned.
- **XP message:** one line per gain, e.g. `Firebolt 50 XP, Strength 5 XP`.
- **Buff bar:** a row of spell icons across the top-left of the screen, one per active buff. Hovering
  an icon shows what it gives, e.g. `Strength: +10 Strength`. Icons will be replaced later.
