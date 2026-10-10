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

Added 2026-10-04, details to come:

- **Stealth** — monsters fail to notice the player, or lose track of them.
- **Threat / taunting** — controls which target a monster picks.
- **Speed** — faster movement or actions for the player, or slower for a monster.
- **Teleport** — already in the game (Teleport jumps to the pointed spot, Phasing jumps a short random distance); neither belongs to an essence yet.

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

### Stat names and regeneration (changed 2026-10-03)
- **Stat names, display only:** Strength is shown as **Power**, Magic as **Spirit**, Dexterity as
  **Speed**, Vitality as **Recovery**. They work exactly as before and keep their old names in the
  code. Renamed on the character sheet, the new-hero screen, item bonuses ("+5 to power"), elixir
  tooltips and item requirements ("Pow", "Spi", "Spd"). Prefix and suffix names on items are
  unchanged, as are item names such as Elixir of Strength and the shrine messages.
- **Regeneration:** life and mana each come back once a second. The base for both is
  **Recovery / 10 per second**. This replaces the earlier Magic-based mana regen. Spells, abilities
  and buffs will later raise either one; the single place to apply them is
  `GetLifeRegenPerSecond` / `GetManaRegenPerSecond` in `Source/player.cpp`.
- **Essence stats list:** opening the character sheet also shows a plain list to its right with the
  stats the mod adds (health and mana regeneration so far). New stats are added as one line each in
  `DrawExtraStats` in `Source/panels/charpanel.cpp`. To be made prettier later.

### Resistances renamed and re-sorted (changed 2026-10-03)
- **Names:** the three resistances are shown as **Elemental** (was Fire), **Natural** (was Lightning)
  and **Astral** (was Magic) on the character sheet, on items ("Resist Elemental: +20%") and in the
  monster information shown on hover. "Resist All" is unchanged.
- **What each one covers now:** Elemental covers fire, lightning and ice. Natural covers acid (for
  players). Astral covers magic and shadow. Lightning damage is now checked against the Elemental
  slot, for players and monsters alike (`GetResistanceCategory` in `Source/tables/misdat.h`).
- **Item prefixes were not renamed.** The resistance prefixes are gem and colour names (Red,
  Crimson, Garnet, Ruby for the old fire slot; Blue, Azure, Lapis, Cobalt, Sapphire for the old
  lightning slot; White, Pearl, Ivory, Crystal, Diamond for the old magic slot; Topaz, Amber, Jade,
  Obsidian, Emerald for all), so none mention fire or lightning. The "of flame", "of lightning" and
  similar suffixes add fire or lightning *damage*, which are still real damage types.
- **Still to do (monster table pass):** monsters marked resistant or immune to lightning now resist
  Natural, which no player spell deals yet, and they no longer resist lightning; monsters resistant
  to fire now also resist lightning and ice. Undead are still immune to Astral.

### Races instead of classes (changed 2026-10-03)
The hero creation screen says "Choose Race", and the four choices are named after races from the
books. Display names only; each keeps its original art, starting stats and class skill.

| Was | Now |
|---|---|
| Warrior | Human |
| Rogue | Celestine |
| Sorcerer | Runic |
| Monk | Leonid |

The names live in the first column of `classdat.tsv` (both the base and Hellfire copies). Bard and
Barbarian, Hellfire's two hidden classes, are not renamed.

### Special attacks and weapon buffs (first ones built 2026-10-03)
Two kinds of ability that work through the weapon. Both are learned from awakening stones, level
from use, and cost mana, exactly like spells.

- **Special attack:** "casting" it makes a weapon attack with an added effect. Right-clicking a
  monster makes the character walk over and swing as for a left click, and that one swing carries
  the effect. Distinct from spells. Code: `Source/special_attacks.cpp`; the swing is resolved in
  `PlrHitMonst` in `Source/player.cpp`. The mana is paid when the swing happens, hit or miss. Melee
  only for now; with a bow the character says "I can't do that".
  - **Flame Strike** (spell 57, icon 33, 2 mana): the whole weapon hit becomes fire damage, plus 3.
    The +3 scales by the default 12.5% per level. Being fire damage, it is resisted as Elemental.
    The hit's full value earns Flame Strike its XP.
- **Weapon buff:** a persistent buff (until death) that adds to every weapon hit. Code: `Source/buffs.cpp`.
  - **Flaming Weapon** (spell 56, icon 31, 10 mana, castable in town): every weapon hit that lands
    also deals 3 fire damage, scaled 12.5% per level. Applies to whatever weapon is held and stacks
    with fire or lightning properties on the weapon itself (those may be removed from the game
    later). It earns the full XP value of the fire damage it adds.
- Both show the small fire burst on the monster, as a flaming weapon does, and the monster flashes red.
- The spellbook lists special attacks alongside spells for now.
- **XP rule (decided 2026-10-03):** anything that deals damage earns the full XP value of that
  damage: spells, special attacks (Flame Strike), damage auras (Fire Aura) and weapon buffs
  (Flaming Weapon). Only **passive buffs**, which deal no damage of their own (Strength), use the
  one-tenth rule: a tenth of every XP gain the player makes.

### Cooldowns (first one built 2026-10-03: Inferno Strike)
- Code: `Source/cooldowns.h/.cpp`. Most abilities have no cooldown. One that does cannot be used
  again until it runs out. Cooldowns are the local player's only: memory only, never saved, never
  sent to other players. They keep counting in town.
- A special attack's cooldown starts only when its swing lands. A miss still costs the mana but
  leaves the ability ready to use again (decided 2026-10-03).
- **Inferno Strike** (spell 58, special attack, icon 34, 6 mana, 12-second cooldown): the whole
  weapon hit becomes fire damage and is increased by 200%, so three times the normal hit. The 200%
  grows 12.5% per spell level.
- **The clock sweep:** every place a spell icon is drawn (the active spell button, the spell
  selection list, the spellbook) shows the cooldown. The icon turns a washed-out red, and a clock
  hand sweeps clockwise from 12 o'clock, restoring the usable colour behind it. A full turn means
  ready. Done inside `DrawLargeSpellIcon` / `DrawSmallSpellIcon` in `Source/panels/spell_icons.cpp`,
  so any new place that draws an icon gets it automatically.
- **The tracker:** a row of small icons at the bottom-right of the play area, just above the mana
  orb, one per ability on cooldown, soonest-ready nearest the corner. When an ability becomes ready
  its icon grows 5% for a moment and disappears.

---

## Essences and the spellbook (first version built 2026-10-03)
Supersedes the earlier "essence pages" sketch and tab layout where they differ.

**The five tabs**
1. **Racial abilities** — six planned; the page says "Not yet implemented". Class skills (Item
   Repair and the rest) are removed from the game.
2. to 4. **Essences** — the character's three essences, in the order they were absorbed.
5. **Confluence** — a special set that depends on the three essences. Not designed yet; the page
   says "Not yet implemented". The combinations will be kept deliberately few.

**Rules**
- A character absorbs up to **three essences**. The first goes on tab 2, the second on tab 3, the
  third on tab 4. An essence cannot be absorbed twice.
- Each essence holds **five abilities**: the first five awakening stones (or books) of that essence
  the character uses, in that order.
- An awakening stone can only be used if the character holds its essence and that essence is not
  full. Otherwise it stays in the bag and a message says why.
- Essences and abilities are **permanent**. There is no swapping or unlearning; the game becomes
  roguelike in that sense.
- An ability with no essence assigned cannot be learned by anyone. Only Fire and Lightning exist so
  far, so every other ability is unlearnable until it is given a home.
- The only abilities a character knows are the ones held under their essences. A class's starting
  spell, or anything learned before essences, is forgotten.

**Essences so far** (`GetSpellEssence` in `Source/essences.cpp`)
- **Fire:** Firebolt, Fireball, Fire Wall, Flame Wave, Inferno, Fire Aura, Flaming Weapon, Flame
  Strike, Inferno Strike.
- **Lightning:** Lightning, Chain Lightning, Charged Bolt, Flash, Nova.

**Items.** An essence is an item: the cube gem (picture 27) in the essence's colour (Fire red,
Lightning yellow). Like an awakening stone it is a row in the item table; the spell named in its
row says which essence it grants. Pepin's "Buy essences" list sells them for 1 gold for testing.

**Page layout.** An essence page is a header row (the essence, and how many of its five slots are
filled) followed by five ability rows. Dropping from seven rows to six gives each ability three
lines: name and level; mana cost, cooldown and damage; a short description and XP towards the next
level; then the XP bar. Layout is in `Source/panels/spell_book.cpp`.

**Storage.** Essences and their abilities are saved in the sidecar file (`E` and `A` lines). They
belong to the local player only; other players cannot see them.

---

## Threat and stealth (decided 2026-10-04, not built)
How targeting works today: a monster wakes when its tile is in a player's line of sight, picks the
nearest player (same room beats a different room), and re-picks each time it stands still
(`UpdateEnemy` in `Source/monster.cpp`). A staggering hit switches it to the hitter until the next re-pick.

Decision: each player has one **distance ratio** (100 = normal).
- **Targeting:** the monster sees the player's distance multiplied by the ratio. Below 100 is
  threatening (seems closer), above 100 is stealthy (seems farther). The amount is tunable per ability
  and level, not a flat double or half.
- **Waking:** at 100 or below, monsters wake on sight as today. Above 100 the wake-up range shrinks as
  the ratio grows, with a floor of a couple of tiles.
- **No taunt.** Forcing monsters off other players is out; it asks too much of the front-line player.
- **Speed** and **Teleport** need no design work: Bryan knows how speed works, and teleport already exists.

Multiplayer: every PC keeps a copy of each player's ratio. A player announces it only when it changes
(about three bytes) and once when someone joins. Brief disagreements settle through the existing
monster sync, which already carries each monster's target. Needs two players to test.

Added 2026-10-05: stealth and threat come only from buffs (never auras). A buff *sets* the ratio;
buffs do not add up, and the strongest one active wins. Stealth and threat active together is undecided.

---

## Green and purple tints (measured 2026-10-04, parked on the "maybe someday" list)
The ten tint ramps live in palette slots 128-255, which are the same on every level. There is no
green or purple ramp. Slots 1-127 change per area and hold scenery colours.

Measurement (scripts in `graphics_tool/palette_scan/`, read-only):
- Monsters, missiles, dropped items, players and screen panels use **no** slots in 1-127. Only
  scenery and area-specific objects (doors, Crypt furniture) do. One monster recolour
  (`mage\cnselbk`) maps onto slots 1-4.
- No 16 slots are unused in every area, but each area has 16 it barely uses. Borrowing a
  different 16 per area would repaint: Crypt 0% (31 slots unused), Caves 0.003%, Catacombs 0.01%,
  Town 0.03%, Cathedral 0.08%, Hell 0.2%, Nest 0.9% of scenery pixels, each with a near-identical
  replacement colour. The same 16 everywhere would touch 1-6%, so per-area lists are the way.
- Animated slots must be avoided: Caves and Crypt cycle 1-31, Nest cycles 1-15.

Work if picked up: per-area slot lists and a scenery/object repaint at level load; install two
8-shade ramps (also on mid-level palette swaps); lighting-table entries for them; two new tints
and text colours; add both to the Essence Designer. Not measured: how the lighting table copes.
Suggested first step: a proof in the Cathedral only.

---

## Rules for drafting powers (for Claude, when asked to fill an essence)
Content lives in `designer/essence_data.json`, edited with the Essence Designer. Bryan sets up each
essence (tag lines, pool size, description); Claude invents the power names and the powers from
these rules. **Wait for Bryan to call for it** before drafting. Expect many rounds of adding,
deleting and remaking. Drafted powers are saved as drafts; nothing counts until Bryan accepts it.
If the rules and an essence's lines ask for something impossible or contradictory, say so instead
of quietly bending a rule.

### The essence's five lines
- **All**: every power has these tags. **Most**: most powers. **Some**: a fair share.
  **Never**: no power may have these, including everything an "All other <group>" entry sweeps in.
  Tags on no line count as "a few": allowed, but rare.
- Target shares, counted per tag: All 100%; **Most about 60%; Some about 30%**; Never 0%.
  (Most was 70% until Bryan lowered it on 2026-10-05: the higher figure was bottlenecking the
  sets, because several Most and Some tags compete for the same 20 powers.)
- **Finite** (added 2026-10-05, sits between Some and Never): the essence must have **at least one
  power with this tag, two at most**. These tags are important to the essence and must not be
  missed, but should not spread. A Finite tag is never swept up by an "All other <group>" entry.
  This is the proper way to ask for the single stealth or threat power, a lone resurrect, and the
  like.
- A tag on no line is **not** capped at a small share. It simply follows the tag group rules below.
  A tag the rules require (a Damage Style on every attack, a Target on everything) appears as
  often as the rules demand. A tag nothing requires stays rare, about 10% or less. Use judgment.

### How closely to hit the targets (calibrated on Holy, 2026-10-05)
Bryan approved the second Holy set as hitting the targets "just fine". It is the reference for how
loose the shares may be when an essence lists more than 20 powers can carry: Most tags landed at
60-70% (target 70%), Some tags at 15-30% (target 30%), and Instant at 80% because the rules require
a duration. Distinct powers matter more than exact shares; do not pad tags to reach a number.
Resurrect at 3 of 20 was accepted. Report the actual shares and the reasons for any miss.

### An essence does not have to cover its own weaknesses (Bryan, 2026-10-05)
A player holds three essences plus a confluence, so a monster immune to one essence's damage can
usually be handled with another. Do not add powers just to patch an essence's blind spot (a
fire-immune monster against Fire, say). A power that does so is welcome when it is a good power in
its own right (Bryan liked Kindling), but it is not a requirement.

### First-pass numbers (Claude's model, 2026-10-05; for testing, not balanced)
Bryan asked for every power to be given numbers so they can be looked at and tested. They come
from one model in `essence_designer/tools/fill-numbers.mjs` (re-runnable; it only touches drafts).
All numbers are for power level 1 and grow 12.5% per level like the game's spells.
- Anchors taken from the game as built: Firebolt 6 mana for about 6 damage; Flame Strike 2 mana
  for +3 damage; Fire Aura 1 damage every 2 seconds; Flaming Weapon +3 per hit; Inferno Strike
  6 mana, +200%, 12 seconds.
- Baselines: a spell attack is 6 mana for 6 damage; a special attack 2 mana for +3 on the hit; a
  heal 8 mana for 12; a shield 8 mana for 14; a cleanse 4 mana; a lasting buff 10 mana, once.
- Area: Splash gives each target 70% for 1.5x the mana; Blanket 50% for 2x; Chain 80% then less;
  Cone 70% for 1.5x; Everyone 40% for 2.5x.
- Cooldown buys power at similar cost: Skirmish 1.5x, Battle 3x (for 1.5x mana), Ultimate 8x (for
  3x mana).
- Efficient: 0.6x mana for 0.9x effect. Inefficient: 3x mana for 1.8x effect. Over Time: 1.3x in
  total. A power that both harms and heals gives 60% of each.
- Lasting buff on one person: +10 armor, +10% resistances, +10% maximum life or mana, +30% faster
  regeneration, +10% speed, +10% damage. In an aura: half. On a cooldown (Timed): three times.
  For Everyone: half.
- **Radius (Bryan's figures):** a damaging aura 2, growing with level; Splash 2 (Fireball included); Blanket 4, or 6 on an Ultimate; an
  aura that only buffs, heals or debuffs 8.
- Known gap: the original spells kept in Fire use the model's numbers, not what the game charges
  today (Fireball is 16 mana for about 26 damage in the game; the model says 9 mana for 4 to each
  target). Overhauling the old spells' numbers is still to be decided.

### No two powers alike (given by Bryan 2026-10-05)
Two powers may share an identical tag list, but they must still play differently. Ways to tell
similar powers apart, roughly in order of preference:
- A different **cooldown** tier. A cooldown can buy more *effect*, or it can buy more *efficiency*
  (a heal with a much better healing-per-mana ratio, on a short 6-second cooldown so it cannot be
  used constantly).
- **Size** at the same ratio: a small effect for small mana against a huge effect for huge mana.
  This does not show in the tags; say it plainly in the description.
- **Efficiency** tags, used sparingly.
- A different mechanic (heals more on a nearly dead ally, bounces, also cleanses...).
Hybrids Bryan liked and wants more of: one power that harms enemies and heals allies at once
(Dawnburst: a blanket on the ground; Atonement: a strike whose damage heals a wounded ally).

### Tag group rules (given by Bryan 2026-10-05)
Words used below: an **attack** is any power that deals damage and is not an aura.

**Element** (several allowed)
- Assume every power needs at least one element; every attack certainly does. It is flavour.
- A pure essence (Fire, Water...) has its element on All and "All other Elements" on Never.
- A power may carry two elements only when the essence does not bar the second one.

- Naming: Water covers water itself and frozen water. Names may use ice, frost and glacial, which
  suit its attacks better than splashing someone with water.

**Target** (exactly one; every power has one)
- Attacks: Enemy or Ground.
- Buffs (including Defensive): Self or Ally.
- Debuffs: Enemy or Ground.
- Direction: used by cones. Area of Effect "Cone" requires Target "Direction".
- Healing and Shielding: Self or Ally, like buffs. Ground-effect healing also exists.
- Auras always target Self (see Aura).
- Nothing affects "every enemy". Anything that affects enemies needs a centre point (a target or a
  ground spot) and a radius.

**Area of Effect** (exactly one; every power has one)
- Single Target: one target picked. Very common.
- Splash: one target picked, and it also hits everything close to that target. A small area.
- Blanket: a much larger area than Splash. Less damage, more reach (or reach that grows).
  More likely aimed at the Ground, but may be aimed at an Enemy or an Ally.
- Everyone (allies only): the caster targets Self and every player in the game is affected. No
  range, no stepping out of it. Used for buffs, and also for "heal everyone" powers, which must be
  weaker than ranged heals or be a heal over time. Never for anything that affects enemies.
  (This is not an aura.)
- Chain: hits a target, then bounces to another, and so on (Chain Lightning, chain heals).
  The number of bounces is a value to set.
- Cone: starts at the caster and fires out in a direction (Inferno, Charged Bolt). Needs Target
  "Direction".
- Aura: its own special area (moved into this group by Bryan 2026-10-05). It has a radius, always
  targets Self and is always Permanent. See Aura below.

**Balance targets for new powers** (given by Bryan 2026-10-06; they apply going forward and do not
call for changing powers already accepted)
- **Attacks: half with no cooldown, half with one.** Count every attack in the essence, special and
  spell attacks together. Aim for an even split and lean toward a few more with no cooldown, so a
  player always has a good choice of attacks that are simply available. (When he asked, the
  essences other than Fire had clearly more cooldown attacks than not.)
- **Lasting buffs: half self-only, half for others.** Count the lasting, no-cooldown buffs, leaving
  auras out. About half should be castable on yourself only. The other half are the ones that can
  go on an ally or on the whole party; those two kinds count together. His worry is everyone
  carrying a great pile of buffs at all times.
- **Buffs with a cooldown are outside that count.** A buff you can put on yourself or one ally, but
  only every so often, is a choice about who gets it, which he likes. Leave those out of the
  balance entirely.
- He may go back over the existing buffs himself and make some self-only. Leave that to him.

**Purpose** (several allowed): the basic idea of the power
- Attack (deals damage; this tag was called Damage until 2026-10-05); Healing; Shielding
  (preemptive, temporary health); Buff (strengthens players); Debuff (weakens enemies).
- Shielding, defined (Bryan 2026-10-05): the magical effect of putting *temporary health* on
  someone. It is a buffer, pre-healing: it is lost first when they are hit, and once used up it is
  gone. The Shield essence's aura is this effect: every 10 seconds a very small shield on everyone
  in the aura.
- Resurrect: brings someone back to life. Normally an Ally power. The one exception is a
  self-resurrect, which must be an Ultimate with a super long cooldown (10 to 20 minutes) and must
  be super rare. Every Resurrect power is also tagged Healing, since it gives life back (Bryan,
  2026-10-07); this lets a healing awakening stone reach them.
- Cleanse: removes debuffs that monsters have put on allies. Usually Instant, never Permanent.
  Self, Ally, Splash, Blanket and Everyone all apply. Strength varies (remove one stack, all stacks
  of one thing, everything). How monsters apply debuffs is not designed yet, so keep the wording
  loose.

**Stat Effect** (several allowed) (added by Bryan 2026-10-05)
Says *what a buff or debuff changes*. Every buff and debuff should carry one; nothing else does.
- Specific: Power, Speed, Recovery, Spirit (the four stats), Damage, Accuracy, HP, MP,
  Health Regen, Mana Regen. "Damage" here means a change to damage dealt; a power that deals
  damage carries the purpose tag Attack.
- General: Defensive (helps allies live longer, or makes the enemy worse at killing) and Offensive
  (helps allies kill faster, or makes the enemy die faster).
- **Tag the most specific one only.** A mana-regeneration buff is tagged Mana Regen, not
  Defensive + Recovery + Mana Regen. Use Defensive or Offensive on a power only when no specific
  tag fits (a cheat-death buff, an immunity to debuffs).
- **On an essence line, a Stat Effect tag is a focus, not a quantity** (global rule, 2026-10-05).
  Mana Regen on Most does not ask for more buffs; it says this essence's buffs and debuffs should
  mostly be about mana regeneration. How many buffs and debuffs there are comes only from the Buff
  and Debuff tags. The essence summary will therefore show a low share for such a tag; that is
  not a miss.
- Defensive and Offensive are mainly for *asking*: on an essence line, Defensive means "give this
  essence buffs and debuffs of the defensive kind" (Health, Health Regen, Recovery and the like),
  and the powers themselves then carry the specific tags. Debuff plus Defensive on an essence
  means it debuffs enemies to protect the party.

**Duration** (several allowed; every power has at least one)
- Instant: very common. Most attacks are Instant.
- Over Time: the effect itself is spread out in ticks (about every 2 seconds) for the duration:
  damage over time, healing over time. Avoid it unless the essence lists it on Most or Some, or
  Bryan calls for it.
- **Every damage-over-time effect ticks every 2 seconds and lasts 20 seconds** (Bryan
  2026-10-05). Shorter is too hard to keep up without better tracking. Healing over time is not
  covered by this yet.
- **A damage-over-time effect is shared, not owned by a power.** Burn is Burn: every Fire power
  that burns adds to the same Burn on the monster, and any new application restarts the 20 seconds
  for all of it. Each application adds its own amount (a higher-level power adds more), so the
  total compounds for as long as the player keeps it up. **There is no limit to the stacks**; going
  very high on a long boss fight is the point. Each power earns the XP for the part it added.
  Corruption is the same kind of effect for Shadow. A power's potency for such an effect is its
  total over the 20 seconds; similar powers should add similar amounts (1 per tick is the baseline).
- **Buffs are Permanent by default** (Bryan 2026-10-05, replacing the earlier "most buffs are
  Timed"). Juggling several cooldown buffs, especially ones cast on other people, is a nightmare
  to manage, and an essence full of them makes it hard to avoid picking up three or four. So:
  - Most buffs in an essence are lasting buffs: cast once, they stay until death or the end of the
    session (nothing is saved). Claude assigns Permanent freely to buffs. A permanent buff is
    naturally a smaller effect than a cooldown one, and normally has No CD.
  - Cooldown (Timed) buffs are used **sparingly**: a small handful per essence. Cooldowns are good
    for attacks, and only occasionally for buffs.
  - Of those, **favour Self buffs**. At most **one or two Timed buffs per essence that go on other
    people** (Ally or Everyone), the Ultimate included.
- **Every buff can be cast in town**, cooldown ones included (Bryan 2026-10-05).
- **Debuffs also tend towards Permanent** (Bryan 2026-10-05): once on a monster it stays until
  the monster dies. Unlike a lasting buff, a lasting debuff *may* have a cooldown, and often
  should: the cooldown is the wait before it can be put on the next enemy or group. (A lasting
  buff with a cooldown makes no sense: it is cast once and the cooldown never matters again.)
  Short stuns and freezes stay Timed.
- The aim behind all of this: a character has 20 powers, and Bryan hopes about half turn out to be
  passive, lasting buffs that never need attention in a fight. Buffs and debuffs are things you
  apply and move on from; the active juggling is for attacks and heals.
- Timed: a buff, debuff or shield that is there for a period and then goes away.
  - Most last 10 to 30 seconds; 20 seconds is common. As a guide the effect lasts about a third of
    its cooldown (Ultimates excepted).
  - Avoid Skirmish-cooldown buffs. If one is used, it lasts 5 or 6 seconds at most.
  - Shields of temporary health and debuffs on enemies are not part of the juggling problem and
    may be Timed as needed.
- Permanent: buffs (the default, above), some debuffs, and every aura. Never damage, healing or
  shielding. Permanent means until death or the end of the session; a permanent debuff on a
  monster lasts until it dies. Stealth and Threat buffs are always Permanent.
- Not exclusive: a power may be Instant and Over Time (a hit that also leaves damage over time).

**Efficiency** (at most one; most powers have neither)
- No tag means ordinary efficiency, which is the norm.
- Efficient: low output, but better than average output per mana. Cheap and weak, good ratio.
- Inefficient: a lot of power, quickly, for much more mana than it is worth. The heavy hitters
  that drain mana fast.
- The essence listing one calls for it. Otherwise they may be used sparingly as one way to make
  two similar powers differ.

**Cooldown** (exactly one; mutually exclusive) (given by Bryan 2026-10-05)
A cooldown trades being always available for more effect. The longer the wait, the more powerful
the power may be for its cost. It adds variety and big moments without unbalancing things.
- No Cooldown (tag "No CD"): always available. The baseline for power.
- Skirmish: about 15 seconds or less. Back for every fight or every other fight. A little more
  powerful than baseline.
- Battle: roughly 30 seconds to a minute. "I have entered a room full of monsters and use this
  once." More powerful than Skirmish.
- Ultimate: 2 to 5 minutes. The defining moment-of-glory power; very powerful. Five minutes is the
  normal ceiling. A very few niche, astonishing powers (a self-resurrect, say) may sit at 10 to 20
  minutes.
- **Every essence gets at least one Ultimate.** One is the norm. Two should not be common. Three is
  the absolute maximum and should be rare.

**Weapon** (Sword, Axe, Bow, Staff, Shield, Blunt) (added by Bryan 2026-10-05)
A power with a Weapon tag only works while that kind of weapon is equipped.
- **Use these only when the essence calls for them.** They are for weapon-locked essences (a Sword
  essence, an Axe essence, the Shield essence). Never add one to any other essence's powers.
- **Weapon tags go on Special Attacks only.** A weapon tag on an essence's *All* line does not mean
  every power requires that weapon. It means every *Special Attack* in the essence requires it.
  Buffs, shields, spells and the aura in that essence carry no weapon tag and work with anything
  equipped.
- Example, the Shield essence: Shield on All, Special Attack on Most, Attack on Some. It is
  primarily a defensive buffing essence with only some attacks, and every attack it does have is a
  shield slam or shield bash that requires a shield. "Special Attack on Most" is the usual lean:
  its attacks are special attacks, not spells.
- The game already has a shield-strike animation (shield and no weapon), so shield attacks can be
  drawn; see graphics_tool/extracted/shield_only_attack_warrior.png.

**Awareness** (Stealth, Threat)
- Only on buffs. Not on auras as a rule, with one exception Bryan made on 2026-10-06: the Fire
  Aura carries threat, because Fire had no threat power. The threat is the owner's alone, as the
  aura itself is: it hurts enemies near them and gives allies nothing. Ask before adding
  stealth or threat to any other aura.
- **Always Permanent, never Timed** (Bryan 2026-10-05: temporary stealth or threat adds more
  complication than he wants). A stealth or threat power is a lasting buff, like Flaming Weapon.
- Bryan puts them on Never for most essences. **If one is deliberately left off an essence's Never
  line, that is a quiet request for exactly one power with it** (corrected 2026-10-05: Nature left
  Stealth off Never and expected one stealth power). Listing it on Some or Most asks for more.
- **Never a power by itself** (Bryan 2026-10-06: a power that is only stealth or only threat is
  painful to spend a slot on). Stealth or threat always comes as one half of a two-part buff: paired
  with a second, modest effect that suits the essence (Cloak of Night: stealth and immunity to
  blindness; Unyielding: threat and no stagger). The second effect carries its own Stat Effect tag.
  Camouflage and Stalwart were made before this rule and are still solo.
- An essence may have two threat or stealth powers; they do not stack (the strongest wins).
- A Stealth or Threat buff needs no Stat Effect tag; the Awareness tag already says what it changes.
- In the game these set the player's distance ratio (see "Threat and stealth"). They do not add
  up: the strongest one active wins (a 50% threat buff beats a 70% one). A stealth buff and a
  threat buff active together is undecided.

**Damage Style** (Special Attack, Spell Attack)
- Every power that deals damage, **except auras**, is exactly one of the two. This covers
  attacks, damage over time and damaging debuffs. Powers that deal no damage carry neither.
- About 50/50 by default. If the essence lists one on Some or Most, lean that way; it is a lean,
  not an exclusion.
- Future, not built: racial bonuses will tilt this. Example: a human "special attack" racial adds
  the Special Attack tag to every awakening stone the player uses that has no damage style of its
  own, so it prefers special attacks unless a Spell Attack stone is used.

**Aura**
- Every essence gets exactly one aura, and only one. It is the power most aligned with the
  essence: build it from as many of the essence's Most tags (and All tags) as possible, and keep
  "a few" tags off it. Its Area of Effect is the Aura tag, so it carries no other area even when
  the essence has Splash or Blanket on Most. Bryan will go through each aura.
- An aura is a self-buff that emanates from the caster: Target is always Self, Duration is always
  Permanent, and it pulses around the caster within a radius. What the pulse does is flexible: hurt enemies, curse enemies,
  buff allies, heal allies.
- Auras never carry Stealth or Threat, and carry no Damage Style.
- Idea to try for a shielding essence: an aura that puts a very small shield on everyone nearby on
  a slow timer (about every 10 seconds).
- Game rule, not built: a character can hold only one aura power. The first aura a character
  unlocks locks them out of every other aura.

## Buff stacking (decided 2026-10-06)

- Different buffs to the same stat add up.
- The very same buff can never be on a player twice. Cast again, a lasting buff keeps whichever cast is stronger; a timed buff takes the newest cast, weaker or not, and starts its time again.
- The very same aura from two players does not double: the stronger one counts, and only that one is shown.
- Damaging auras are separate sources of damage, so two players' copies both tick.
- In the buff bar, a buff or aura that came from another player is drawn yellow; your own are blue. Where debuffs from enemies are shown is not decided.
- A player can learn only one aura power, whatever essences they hold. Once they know one, every other aura stone is refused. (Decided 2026-10-06.)

## Reducing monster resistance (decided and built 2026-10-07)

A resistant monster shrugs off 75% of the damage. Bryan wanted powers that lower that by points
(10 points makes it 65%), as a way to do far better against resistant monsters without handing
everyone a blanket damage increase. His rules:

- **Reducing resistance only reduces resistance.** It does nothing to an immune monster.
- **Stripping immunity is its own thing**, and says so: Soul Rend (Astral) and Kindling (Elemental)
  remove resistance and immunity outright, on a cooldown. Keep those as full strips.
- **Never below zero.** A monster with no resistance left takes normal damage, not extra.
- **Debuffs and auras with different names add up.** The same one twice counts once.
- A power can reduce one resistance (Elemental, Natural or Astral) or all three.

In the game: an Aura or a Curse with the stat ElementalResistCut, NaturalResistCut,
AstralResistCut or AllResistCut. Nothing is saved; it lasts until the monster dies or the game
ends. Pall of Shadow is the first: 10 points off Astral resistance for monsters within 8 tiles.

## Iron Rank and the Filthy debuff (Bryan, 2026-10-08)

From the books: on gaining a fourth essence a person reaches Iron Rank and the contamination of
their old self pours out of them. In the game, filling the fourth row (accepting the confluence,
or taking a fourth essence in its place) sets off a burst of sludge and the character's death cry,
and leaves them Filthy. It lasts until they use a Crystal Wash, which Pepin always stocks for 10
gold. Filthy does nothing but show: halving the stats was tried and was too punishing. It is kept in the sidecar file, shown as a
greyed icon at the end of the buff bar, and other PCs are told (`CMD_FILTHY`). No cleanse removes it.

## A little luck in every number (Bryan, 2026-10-09)

The old game's spells and every weapon roll dice; the powers made in the designer used to deal one
exact number. Now damage, healing and shields that land all at once fall anywhere from 20% under
their number to 20% over (`RollVariance`). Anything that arrives over time is steady from beat
to beat: damage over time, damage zones, damage auras, heals over time and healing ground. The
size of a tick is how a player reads how many stacks are on a monster, so it must not wobble.
Averages are unchanged, so the balance numbers stand. Buff strengths, percentages, durations and cooldowns
do not vary. The original spells no longer grow with character level; each keeps what it dealt at
level 1 and grows with its own level and Spirit like everything else.

## What a longer cooldown buys (Bryan, 2026-10-09)

A general rule, above all for powers made from here on. A longer cooldown should give slightly
more damage for the mana. More importantly, it should give the power more layers: more separate
effects in one cast. An Ultimate may carry three or four (damage, a hold, a heal, a cleanse, a
curse and so on), because it is an Ultimate. A no-cooldown power does one thing.

How attack powers are valued when their numbers are set (the balance passes of 2026-10-09):
a radius 2, cone or full-strength chain power is counted as hitting 3 monsters; a chain that
loses a quarter each leap as 2.31; a bigger area as 5; damage over time at half its total; a
special attack by its bonus alone, since the weapon swing is free. On that footing, in damage for
a point of mana at level 0: no-cooldown spells 1.2 to 2.4 (Efficient ones near 3, Inefficient
ones near 0.5 with much higher damage); Skirmish spells 2.5 to 3.5; a plain no-cooldown special
attack adds 2 for 2 mana. A spell should do about twice what an equal special attack adds, the
weapon supplying the other half.

## Starting sizes for percentage buffs (Bryan, 2026-10-08)

A first pass, to be balanced in play. These are the numbers at level 0.

- A lasting buff on yourself: 10%.
- A lasting buff you give to others, and an aura: 5%.
- A buff on a cooldown, for yourself: 20% on a Battle cooldown, up to 30% on an Ultimate.
- A buff on a cooldown that reaches everyone: about 20%, even on an Ultimate. 30% is for personal
  Ultimates only.
- Stealth and threat never grow with level. A power that carries one must carry a second effect
  that does grow; several do not yet.
- Damage and healing keep the rule of an eighth more each level, compounding (3.25 times at level 10).
- Mana cost grows a tenth each level, compounding (2.59 times at level 10), for every power. It
  is kept in 64ths of a point. The old game's rule of spells getting cheaper with level is gone.
- A power with nothing else to gain from a level can have its cooldown shrink instead: Teleport
  (30 seconds), Kindling (60) and Soul Rend (45) each lose 2 seconds a level; Ward of Purity (60)
  loses 3, Lay on Hands (120) loses 5 and Absolution (12) loses 1.
- A small number of points gains one a level: Ironbark's resistance and Hold the Line's armor
  run 5 to 15.
- Physical damage reduction gains half a point a level when it starts small (Bedrock 1 to 6,
  Stoneskin 2 to 7, Stone Wall 3 to 8) and a whole point when it starts at 5 or more (Dig In 5
  to 15). Stoneskin's armor gains a point a level too (10 to 20).
- Cleansing removes stacks of harmful effects, of any kind, the most damaging first. A power
  starts at a number of stacks and gains some each level (quarters and halves add up; only whole
  stacks count): the plain cleanses and Monsoon 1 +1 a level; the cheap heals that also cleanse
  (Purify, Wash Away, Remedy) and Renewal 1 +0.5; Sacred Grove 1 each tick +0.25; World Tree 2
  each tick +1; Great Flood and Miracle 3 +1. Absolution and Lay on Hands remove everything and
  grow by a shorter cooldown instead. Nothing puts harmful effects on players yet, so none of
  this can be seen working; Renewal's and Ward of Purity's own effects are not built either.
- Quickening takes 1 frame off each step, 2 from level 5 and 3 from level 10. Its mana cost
  moves only at those two levels, catching up to what it would have been.
- Radiuses do not grow with level, the Fire Aura's included (it stays at 2). The sizes in use are
  2 (splash), 4 (blanket), 6 (big blanket or ultimate) and 8 (aura). At the game's normal view a
  radius of 6 reaches the sides of the screen and 8 the top, so there is little room to grow into.
- A percentage buff, aura or curse grows by a set step each level, by where it starts: 5% gains
  1% a level (15% at level 10), 10% gains 1.5% (25%), 20% gains 2% (40%), 30% gains 2.5% (55%).
  Half percents add up across levels and only whole percents count. Kept small on purpose, so
  the big buffs do not get out of control.
- Attack and casting speed buffs are their own case. One given to others starts at 5% and gains
  2% a level (25% at level 10): Fan the Flames. A self-only one starts at 10% and gains 3% (40%);
  none exists yet, and that size is being saved for them.
- Haste: a speed buff of X% takes frames off an attack or cast in proportion to its length
  (frames x X / (100 + X), rounded down), so the action is X% faster to within a frame and a long
  animation gains a frame more often than a short one. At least one frame is left before the hit
  or spell. The four attack speed suffixes on gear are switched off so they cannot stack with it;
  eight unique weapons still carry attack speed.

Some buffs' numbers are not bonuses of this kind and sit outside the rule: Oathbound (the share of
an ally's damage you take) and Guardian Angel (the share of life an ally rises with) start at 20%.

## Buffs that add stat points (Bryan, 2026-10-07)

A buff or aura that adds points to Power, Speed, Recovery or Spirit starts small and grows by one
point for each level of the power, not by the usual eighth. A self-only buff starts at +3 (so +13
at level 10). A lasting buff that can be cast on other players, or one that reaches a group, starts
at +1 (+11 at level 10). The starting number is the power's potency in the designer. Each point is
worth 10% under the stat rules below, which is why these are kept small.

## What the four stats do (Bryan, 2026-10-07)

Starting stats, as Power / Speed / Recovery / Spirit: Human 15 / 10 / 10 / 10, Celestine 5 / 15 / 15 / 10,
Runic 10 / 5 / 10 / 20, Leonid 15 / 15 / 10 / 5.

A stat of 10 is the baseline: where a stat multiplies something, the multiplier is the stat times
10 percent (`ScaleByStat` in `dots.cpp`).

- **Power (Strength):** multiplies weapon attacks and special attacks. 3 life a point. It no longer
  adds damage to a hit.
- **Speed (Dexterity):** multiplies bow shots, and special attacks made with a bow, in place of
  Power. To hit: +1 a point for melee, +2 for bows. 1 armor for every 3. Twice the old share of block.
- **Recovery (Vitality):** life and mana regeneration of Recovery / 10 a second, as before. 1 life a point.
- **Spirit (Magic):** 2 mana a point. Spells get +2 to hit a point. Multiplies spell attacks, damage
  over time, healing and shields. Buffs and auras are not changed by it. The original spells no
  longer add Spirit into their own damage; each keeps what it had at 10 Spirit.

Life and mana come from the stats alone: nothing flat, nothing for a level, the same for every
race. A character is brought to this on loading (`RecalculateBaseLifeAndMana`). No race has a cap
of its own; a character's own points stop at 255 (one byte in the save) and 750 with gear.

Left for later, by his choice: accuracy (the 5 to 95 percent limits and each race's flat starting
to-hit), weapon and armour requirements, how stat points are earned, the size of stat buffs and of
stat bonuses on gear, the remaining race differences from the old classes, and monster balance.

## Races and racial powers (2026-10-09)

Different starting stats threw the game out of balance, so **every race starts with 10 in each of
the four stats**. What tells the races apart is their racial powers: things a character is born
with, never cast and never growing. They are made on the designer's Races page, exported to
`txtdata/classes/racial_powers.tsv` and read by `Source/races.cpp`. A race is found by its name,
which is the name its class goes by in `classdat.tsv`. The hero screen lists a race's racial
powers by name where the four stats used to be.

- **Human** (Warrior): Human Ambition (10% more experience), Special Attack Aptitude, Essence Gifts
  (5% more damage, healing and shields from the powers of each essence held; the confluence gives
  nothing yet, undecided).
- **Celestine** (Rogue): Holy Affinity (10% to Holy damage, healing, shields), Mana Recovery (10%
  faster mana), Innate Speed (10% Speed).
- **Runic** (Sorcerer): Spellborn (10% larger mana pool), Wellspring (10% faster mana), Spell
  Aptitude, Magic Affinity (5% spell damage), Adaptive Resistance (5 points on all resistances).
- **Leonid** (Monk): Ancestral Strength (10% Power), Ancestral Swiftness (10% Speed), Ancestral
  Stamina (10% larger health pool).
- **Elf** (a second Rogue, in the Bard's place): Spell Aptitude, Nature Affinity, Life Affinity
  (no Life essence yet), Mystic Bloodline (10% larger mana pool), Grace (10% Speed).

An aptitude: a stone tagged Attack that names neither Special Attack nor Spell Attack is treated as
carrying the race's kind as well, so the race leans that way; a stone that names a kind is obeyed.

A stat percentage applies to the character's own stat only, rounded down; points from gear and boons are added after it. A character made before
this loses the old head start of their class once (sidecar line `B 1`) and keeps what they earned.
Multiplayer: all of it follows from the class alone except a Human's gifts, which touch only the
Human's own numbers on their own PC.
