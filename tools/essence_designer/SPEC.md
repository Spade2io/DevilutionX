# Essence Designer: Spec for Claude Code

A standalone designer app (outside the game) for authoring the content of a Diablo 1 / DevilutionX mod. The mod's magic system is based on *essences*. This app is where the content gets defined: tags, essences, powers, confluences, damage types, and awakening stones.

## How to work with me

- I'm newer to coding. Please explain what you're doing and why as you go, not just hand over finished code.
- Work one step at a time and confirm with me before moving to the next step.
- Build everything described below as a first pass. I will organize and rearrange the actual content (tags, groups, etc.) myself in the UI, so **do not pre-populate tags or groups** beyond what's needed to test.

## Game concepts (context)

- **Essence**: an equippable spell page. A player equips several (design currently assumes 4 pages, 5 ability slots each). Each essence holds a pool of powers.
- **Power**: a single ability (spell or special attack) living inside one or more essences. A power can appear in multiple essences (many-to-many).
- **Tags**: every power carries multiple tags (roughly 2-6). Tags drive build identity and the awakening stone matching logic.
- **Awakening stone**: an item carrying tags. When used, it scans the player's *unfilled/open* power slots across their equipped essences and picks a matching power, using a best-match cascade (see below).
- **Confluence**: a name for each combination of three essences. Mechanically it is not a new essence with its own power pool. It draws randomly from the player's other three equipped essences. A small number of hand-made flavor powers may exist for thematic combinations.

## Awakening stone matching logic

1. Try to find a power in the player's open slots that matches **all** the stone's tags.
2. If none, fall back to powers matching one fewer tag, and so on, down to a single tag.
3. The cascade always finds something as long as at least one tag matches.
4. Higher-rarity stones carry more tags (common ~1 up to legendary ~4-5), so they give much better odds of landing a specific desired power. It is never guaranteed.
5. Parent/child tags count at both levels (see Tags). A power tagged `defensive-buff` also counts as `buff`.

Design notes already decided:
- Generic tags (AOE, efficient, etc.) are reusable across heal, damage, buff, debuff. Do **not** split them into heal-specific vs damage-specific variants. The best-match-first logic favors the right powers naturally (e.g. a healer's open slots will have heal+AOE+efficient matches that damage powers can't fully match).
- Powers with more tags get matched more often by high-rarity stones. This is accepted: extra narrow/specific tags only help when a stone is hunting that specific tag, so the effect is self-limiting.

## Pages to build

### 1. Tags

- **Tag groups**: create groups; each group has a checkbox for **mutually exclusive** (a power can hold only one tag from that group if checked; multiple if not).
- Tags live inside groups. UI must support:
  - Creating a tag group
  - **Drag and drop** tags between groups
  - Deleting a tag group **only when it's empty**
- **Parent/child tags** (many tags, not all): e.g. `buff` with children `defensive-buff`, `throughput-buff`; `special-attack` with weapon-type children. A child automatically implies its parent for matching.
- Tags from different trees can co-occur freely on one power (e.g. `buff` + `sword` means a buff that affects sword use). That is different from parent/child nesting.
- Spells do *not* get the weapon-type sub-layer that special attacks have. Spells stay flatter.
- The tag type / group data and the tags themselves can be two tables internally but should be managed on one page (a sub-panel or modal for group management is fine), not a separate top-level screen.
- Primary/secondary tag weighting is **no longer a built-in system**. Tag groups replace it.

Tags I expect to exist (for my reference, not for you to pre-populate):
- Role: tank, heal, buff, self-buff, group-buff, debuff, damage
- Spell vs special-attack (mutually exclusive)
- Hit pattern: single-target, splash, blanket (AOE tiers)
- Timing: instant, DoT
- Efficiency: efficient / inefficient (mutually exclusive)
- Elements (not mutually exclusive, a power can be fire and lightning): fire, lightning, ice, shadow, etc.
- Long-cooldown: for ultimates with multi-minute cooldowns
- Weapon types (sword, axe, club, bow, staff): **parked, see open questions**

The damage tags are independent axes: hit pattern, timing, and element. DoT is a timing tag, not a damage type. Instant and DoT are **not** strictly exclusive because some abilities do instant damage and also leave a DoT, so allow both on one power.

### 2. Essences

- Name, description
- **Rarity**: simple drop-rate value only, no other mechanical meaning
- Tags for the essence (identity hints, e.g. a tank essence leans self-buff/defensive). Since primary/secondary weighting was dropped, see open questions for how this works now.
- The list of powers it contains (many-to-many with powers)

### 3. Powers

Per power:
- Name
- Targeting type: self / ally / enemy (exact mechanism TBD, see open questions)
- Potency: damage or healing amount
- Mana cost
- Per-level scaling: what changes as the power levels up (see effect engine below)
- Damage type (see Damage Types page)
- Full tag list

Power browser must be **sortable and filterable** by tag, essence, and similar fields.

Example spells for testing the tag model:
- **Corruption**: spell, shadow, damage, single-target, DoT, no efficiency modifier (baseline)
- **Flame Strike (spell)**: spell, fire, damage, single-target
- **Flame Strike (sword special attack)**: fire, damage, single-target, special-attack, instant (sword tag parked)

### 4. Confluences

- A grid of **every three-essence combination**, each with one editable **name** field.
- That's all. This page holds no power content.

### 5. Confluence Powers

- A separate, rarely used screen for the handful of hand-made flavor powers tied to specific confluences (e.g. a volcano-themed confluence).
- Kept apart from the main power list because they're their own special tier.
- Same field list as regular powers, likely identical schema.

### 6. Damage Types

- Add new damage types and assign each to one of three categories (used to simplify resistances):
  - **Elemental**: fire, lightning, ice, earth, air, etc.
  - **Astral**: light, darkness, galactic/primordial/god-tier damage
  - **Natural**: poison, toxins, bleeding, mundane effects
- Each damage type has three color assignments from Diablo 1's palette (roughly 10-13 colors), shown as dropdowns so I can see the color:
  1. Color for the awakening stones and essences
  2. Color of the damage text when this type is dealt
  3. Color of the magic effects
- **Pull the actual palette from the DevilutionX source** so the list matches what the engine can render. I'll coordinate this with you.

### 7. Awakening Stones

- Create form: name + tag picker.
- **Rarity is derived automatically from the number of tags** (add or remove a tag and the rarity changes). It is never set manually.
- Summary view with counts by rarity so I can spot imbalances ("too many rares").
- Filterable search/report: e.g. how many stones include tag X, or a combination of tags. This doubles as a content-planning view for finding gaps.

## Effect engine (later, but plan for it)

Powers will eventually need a way to define what they actually do mechanically: damage amounts, stat debuffs of a specific type and magnitude, durations, and which parts scale per level. This will be the most complex part of the app and the game. I want a builder interface where I program an effect and flag parts of it for growth per level. **Don't build this in the first pass**, but keep the powers schema from boxing it out.

## Open questions (please flag these as you go, don't silently decide)

1. **Tag behavior in code**: how should the game understand what each tag *does*? My instinct is that most tags are just metadata for matching and filtering, while mechanical ones (instant vs DoT, efficient vs inefficient) should be real fields on the power. Please weigh in on how to code tags, how powers work in the engine, and how to build the interface for implementing effects.
2. **Targeting**: how Diablo 1 / DevilutionX handles self vs ally vs enemy targeting. I haven't checked yet.
3. **Weapon-type tags**: parked for now. Problem: a sword wielder could be offered an axe-only power and it'd feel bad. Two ideas: filter eligibility against the equipped weapon, and/or later add weapon-mastery essences (e.g. a "sword essence") that unlock that weapon's tagged powers. Whether a power can have multiple weapon tags is also undecided. Diablo 1 facts: axes are two-handed only; swords and clubs/maces come in one- and two-handed (e.g. the Maul is a two-handed club-class weapon).
4. **Essence tags**: I dropped primary/secondary weighting. Do essences still carry tags as simple identity hints (or just derive them from their powers)? I want the tag-group system to carry most of the structure. Suggest a simple approach.
5. **Tech stack**: my usual tools are React, Node.js, and Supabase. Confirm that's a good fit here or suggest something else, and tell me why.
6. **Per-level scaling**: how to represent it in the data model without overbuilding.
