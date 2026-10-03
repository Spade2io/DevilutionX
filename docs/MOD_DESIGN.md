# Essence Mod Design — Classless Diablo (DevilutionX)

Living design document. Ideas here are direction, not final decisions — open questions are marked.

---

## Core vision
Diablo 1 is already mostly classless: any character can learn any spell from books, and class mainly
affects stats and one class skill. This mod leans into that. **Your build is defined by the essences
you equip, the abilities you slot, how you've used them, and your stats** — not by a fixed class.

Existing classes and their sprites stay as body/appearance choices (plus palette recolors). No new
character art is planned; identity comes from abilities and effects instead.

**Inspiration:** the *He Who Fights With Monsters* books. Essences, awakening stones and the Astral
category come from there. The aim is for play to feel like that world.

**Guiding principle (2026-10-03): keep it quick and simple.** This is a fun project, not a rebuild
of the game. A few major changes and a lot of small ones, each simple and rewarding on its own.
When there is a choice, reuse what Diablo already does over building a new system, and prefer the
smaller version of an idea. Bigger options noted elsewhere in this document (a dedicated server,
drawing in more colours, percentage resistances) are recorded as possibilities, not plans.

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

---

## Multiplayer and enemy debuffs (discussion notes, 2026-10-03)
Multiplayer is a firm goal. Nothing in the mod has been tested with two players yet.

**How the game syncs today (read from `Source/sync.cpp`, `Source/nthread.cpp`):** there is no server.
Every PC runs its own copy of every monster. Each PC sends a bundle to the others every 2 ticks
(10 times a second), capped at 512 bytes: the player's actions first, then 9-byte monster
corrections (position, health, target) for the monsters most in need. On receipt, the version from
the player closest to the monster wins; a copy within 2 tiles walks to the reported spot, otherwise
it snaps there.

**Enemy debuffs (curses, damage over time, slows):**
- Not fixed slots. Each monster gets a list of active effects that grows and shrinks, with a high
  safety cap (around 32). Twenty or more effects per monster costs nothing noticeable.
- Each effect records: type, strength, stacks, time left, caster, and the spell it came from (for XP).
- Same effect from the same caster refreshes; different casters' effects run side by side;
  different effects coexist. Whether damage over time stacks or only refreshes is still open.
- Memory only, never saved; gone when the level is left.
- **Multiplayer rule: the caster's PC decides, then announces.** One small message when an effect
  is applied or changed, stating the *result* ("monster 37's Burn is now 3 stacks, 8 seconds left"),
  not the change, so a missed or reordered message corrects itself. One more message for early
  removal. Every PC runs the timers itself; ticks are never announced. Only the caster's PC deals
  damage-over-time damage; the existing health sync carries it to the others.
- Each effect is owned by its caster; only the owner announces it. Effects from different casters
  stay separate, so one player's spell does not modify another's effects (revisit if a spell needs it).
- A burst (a curse landing on 30 monsters) should be one compact message: effect details once, then
  the list of monsters.
- Works well for damage over time, stat curses and buffs; acceptably for slows. Effects that change
  monster decisions (fear, confuse, taunt) and effects that jump between monsters are the weak spot
  of "every PC runs its own copy".

**Possible later steps, in order:** (1) debuff announcements as above; (2) one PC in charge of
monsters on a level, building on the existing "level owner" idea; (3) a host that runs with no
player and no screen, i.e. a dedicated server. Each step is useful alone and none is wasted by the
next. Diablo 2 and modern co-op games (e.g. Enshrouded) use a server that owns the world; getting
all the way there means separating "what decides" from "what draws" across the codebase.

**Later item:** explore raising the 512-byte bundle cap and the 10-per-second rate, both chosen for
1996 modems.

**Known single-player-only gaps so far:** levels of spells numbered 52 and up are not shared with
other players (the spell-level message covers 0–51).

### Decision (2026-10-03): damage over time is private to the caster's PC
Supersedes the damage-over-time parts of the notes above.

- **How damage is shared today:** each hit sends a 7-byte message, "monster N took D damage"
  (`CMD_MONSTDAMAGE`). Other PCs subtract it but hold the monster at 1 health; only the PC that lands
  the killing blow sends the death message.
- **Damage-over-time effects (bleed, burn, poison) are never announced.** Each player's PC keeps its
  own effects, deals the ticks, and reports only the damage through the existing message.
- **Every player has their own version of an effect.** Two players putting the same bleed on one
  monster run independently and both deal damage. No ownership or "whose is stronger" rules.
- **Only debuffs that change a monster's numbers or behaviour are announced** (takes more damage,
  weaker armour, slow), using the "caster's PC decides, then announces the result" rule.
- **Tick rate:** once or twice a second. The budget is about 5,000 bytes a second per player; 30
  monsters ticking once a second is about 4% of it, and ticking every game tick would be about 80%.
  If faster ticks are wanted, total the damage per monster and send one combined report per bundle.
- **Given up for now:** other players cannot see your effect on a monster, and spells cannot react
  to another player's damage-over-time effects. Both can be added later with a display-only
  announcement, without changing how the damage flows.

---

## Damage types and resistances (decided 2026-10-03)
**Two layers.** A spell has a *damage type* (as many as wanted: fire, ice, lightning, earth, air,
bleed, poison, decay, light, darkness, arcane, ...). Every damage type belongs to exactly one of
three *resistance categories*, and only the categories have resistances. New damage types can be
added freely without touching gear, item properties, the character sheet or monster resistances.

| Category | Covers | Replaces today's slot |
|---|---|---|
| **Elemental** | fire, ice, lightning, earth, air and the like | Fire |
| **Natural** | bleeds, poisons, decay | Lightning |
| **Astral** | light, darkness, arcane; the foundational, ephemeral things | Magic |

- **Physical stays outside all three.** It has no resistance. Armour is left exactly as it is: it
  reduces the chance to be hit and never reduces damage.
- **What the game has today:** five damage types (Physical, Fire, Lightning, Magic, Acid). Monsters
  have on/off switches (resistant = quarter damage, immune = none) for Magic, Fire and Lightning,
  plus Acid immunity. Players have three percentages from gear (Magic, Fire, Lightning), not saved.
- **The rename needs a re-sort of the data,** not just new labels: monster resistances in the
  monster table were tuned for the old meanings (a "lightning immune" monster would become "Natural
  immune"), existing resistance items change meaning, and every existing spell needs filing into a
  category. Likely filing: fire and lightning spells to Elemental, Acid to Natural; Holy Bolt, Bone
  Spirit, Blood Star, Elemental and Apocalypse still to be placed.
- **Monster resistances stay as they are (decided 2026-10-03):** the existing on/off system, where a
  monster is normal, resistant (quarter damage) or immune to each category. No percentages, no
  weaknesses and no damage bonuses. The game already works on "full damage, and down from there".
- **Save format is not a limit here.** Monster resistances come from the monster's type, so anything
  beyond the original saved switches can live in memory and be refilled from the monster table on
  load. That also allows percentages and weaknesses later, which the on/off switches cannot express.
- **Floating damage numbers** are on (the built-in "Floating Numbers - Damage" script); floating XP
  numbers stay off. Text comes in a fixed set of roughly eight to ten colours, so that is about how
  many damage types can each have a distinct colour.

### Damage over time (first one built 2026-10-03: Corruption)
- Code: `Source/dots.h/.cpp`. Each monster has a list of active effects, held in memory on this PC
  only; cleared when a level is set up. Only the damage is reported to other players.
- **How every damage-over-time effect works in this mod:** each cast adds a stack and refreshes the
  duration. Damage per tick is per stack, so it starts far below a direct-damage spell and builds
  with repeated casts.
- **Corruption** (spell 54, icon 30, Magic casting animation, 6 mana): cast on a monster like Stone
  Curse, with no projectile. A burst, a flash and "Corruption xN" text show it landed; "Immune" shows
  if it cannot. It cannot miss and has no line-of-sight check. It does no damage
  when it lands and attaches one stack. 1 shadow damage per stack every 2 seconds, for 20 seconds
  (the 20 seconds is a placeholder). Damage is kept in 64ths, so fractions work: a resistant monster
  takes a quarter (0.25 per stack) and an immune one takes none.
- **Shadow** is the first new damage type. It belongs to the Astral category, which is checked
  against today's Magic resistance (`GetResistanceCategory` in `Source/tables/misdat.h`).
- **One colour per spell** (`GetSpellTint` in `Source/essence_tint.cpp`): the projectile, the impact,
  the awakening stone (in the bag and on the ground) and the flash on the monster all use it.
  Shadow's colour is the dark two-thirds of the slate-blue ramp, the nearest the shared palette has
  to purple. Floating numbers for shadow use the game's "blue" text, which is that same ramp.
- **The flash:** a monster is drawn in the spell's colour for about a third of a second each time an
  effect ticks. Effects keep the rhythm they started with, so several different effects on one
  monster flash at staggered moments in their own colours.

- **Ice** (added 2026-10-03) is the second new damage type, in the Elemental category (checked
  against today's Fire resistance). Frostbolt deals ice damage. Its colour is the bright blue: bolt,
  impact, awakening stone and floating numbers. The numbers use a new bright blue text colour
  (`ColorIce`, `assets/fonts/ice.trn`).

### Default damage scaling (decided 2026-10-03)
Unless Bryan says otherwise, **every new spell scales its damage by 12.5% per spell level after the
first, compounding** (`ScaleDamageForSpellLevel` in `Source/dots.cpp`). This is the rule Fireball,
Flash and Nova already use. Level 1 is the base, level 5 is 1.6x, level 10 is 2.9x, level 15 is 5.2x.
Damage is held in 64ths, so the fractions are real. Corruption and Fire Aura both use it. Frostbolt
copies Firebolt's own formula. Stats do not add to damage yet.

### Damage auras (first one built 2026-10-03: Fire Aura)
- A damage aura is a persistent buff (`Source/buffs.cpp`) that pulses. It lasts until death.
- **Fire Aura** (spell 55, icon 32, Fire casting animation, 10 mana, castable in town): every 2
  seconds, 1 fire damage (scaled by level) to each monster within reach that it has a clear line to;
  it does not reach through walls. Reach is 2 tiles at level 1 and 1 more every two levels.
- Each pulse goes through the same path as a damage-over-time tick: resistances, spell XP, floating
  numbers, and the monster flashing in the spell's colour (red). It wakes whatever it touches.
- A damage aura earns XP from its own damage only; it does not take the tenth share that other
  buffs get.
- **Visual:** the Flash burst, turned red, drawn under the characters and lowered to foot level. It
  only plays on a pulse when at least one monster is in reach, to keep exploring quiet.
- **Multiplayer:** casting is already announced, so every PC knows who has the aura and shows the
  burst. Only the owner's PC deals damage, reported with the normal damage message.

### Difficulty names (changed 2026-10-03)
The three difficulties are named after the books' ranks: **Iron** (was Normal), **Bronze** (was
Nightmare) and **Silver** (was Hell). Only the displayed names changed; the rules, the level
requirements for multiplayer (20 and 30) and the internal names in the code are untouched.
