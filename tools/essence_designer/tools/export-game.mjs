// Writes the designer's powers into the files the game reads.
// Usage: node tools/export-game.mjs [--check] [essence names...]     (default: Fire, Water, Holy, Nature, Shield, Dark and Earth)
//
// What it writes:
//   assets/txtdata/spells/essence_powers.tsv  one row per power the game does not already have
//   assets/txtdata/items/itemdat.tsv          an awakening stone for each of those powers
//
// Each exported power is given a number (100 and up) the first time it is exported. The number is
// saved back into the designer's data file and never changes or gets reused, because characters'
// side files refer to powers by number. Stones are only ever added at the end of the item table,
// for the same reason: saved items refer to their row by position.
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { printChanges, readSnapshot, SNAPSHOT_FILE } from './changes.mjs'

const here = path.dirname(fileURLToPath(import.meta.url))
const REPO = path.resolve(here, '../../..')
const DATA_FILE = path.join(REPO, 'designer/essence_data.json')
const POWERS_FILE = path.join(REPO, 'assets/txtdata/spells/essence_powers.tsv')
const ITEMS_FILE = path.join(REPO, 'assets/txtdata/items/itemdat.tsv')
const check = process.argv.includes('--check')
const wanted = process.argv.slice(2).filter((a) => !a.startsWith('--'))
const essenceNames = wanted.length ? wanted : ['Fire', 'Water', 'Holy', 'Nature', 'Shield', 'Dark', 'Earth']

// Powers the game already has as hand-written spells. They keep their original numbers and are
// not exported; the name on the right is the game's own name for the spell.
const ALREADY_IN_GAME = {
  'Firebolt': 'Firebolt', 'Fireball': 'Fireball', 'Flame Wave': 'FlameWave', 'Inferno': 'Inferno', 'Fire Wall': 'FireWall',
  'Fire Aura': 'FireAura', 'Flaming Weapon': 'FlamingWeapon', 'Flame Strike': 'FlameStrike', 'Inferno Strike': 'InfernoStrike',
  'Guardian': 'Guardian', 'Elemental': 'Elemental', 'Immolation': 'Immolation', 'Ring of Fire': 'RingOfFire',
  'Rune of Fire': 'RuneOfFire', 'Rune of Immolation': 'RuneOfImmolation', 'Frostbolt': 'Frostbolt', 'Holy Bolt': 'HolyBolt',
  'Corruption': 'Corruption',
}
// Which picture each power borrows from the game's sheet of spell icons, until it has its own.
// The numbers are positions in that sheet (see SpellIcon in Source/panels/spell_icons.hpp).
const ICONS = {
  'Smolder': 32, 'Wildfire': 46, 'Meteor': 24, 'Ignite': 22, 'Flame Cleave': 47, 'Searing Brand': 31, 'Melt Armor': 30,
  'Stoke the Flames': 18, 'Fan the Flames': 33, 'Kindling': 37, 'Phoenix': 40,
  'Soothing Waters': 9, 'Healing Rain': 1, 'Glacial Spike': 29, 'Ice Burst': 10, 'Hailstorm': 45, 'Icebreaker': 35,
  'Riptide': 13, 'Springwater': 40, 'Clarity': 43, 'High Tide': 50, 'Reservoir': 12, 'Mana Tide': 28,
  'Radiance': 8, 'Mend': 1, 'Divine Light': 3, 'Patient Prayer': 9, 'Lay on Hands': 40, 'Purify': 4, 'Absolution': 21,
  'Chorus of Light': 15, 'Second Dawn': 40, 'Guardian Angel': 44, 'Miracle': 24, 'Ward of Purity': 12, 'Vigil': 43, 'Benediction': 18,
  'Smite': 41, 'Judgment': 34, 'Atonement': 33, 'Searing Light': 10, 'Dawnburst': 37,
  'Ironbark': 8, 'Healing Touch': 1, 'Nourish': 9, 'Full Bloom': 40, 'Pollinate': 15, 'Verdant Tide': 3, 'Fairy Ring': 10,
  'Rejuvenation': 43, 'Spring Shower': 50, 'Sacred Grove': 37, 'Remedy': 4, 'Renewal': 21, 'Fresh Breeze': 27, 'New Growth': 44,
  'Barkskin': 12, 'Canopy': 45, 'Ironwood': 7, 'Oakheart': 18, 'Camouflage': 19, 'World Tree': 24,
  'Aegis': 8, 'Shield Bash': 47, 'Shield Slam': 22, 'Shield Sweep': 46, 'Concussive Blow': 7, 'Guarded Strike': 44, 'Bulwark Slam': 24,
  'Barrier': 12, 'Reinforce': 16, 'Shield Wall': 45, 'Phalanx': 48, 'Bulwark': 25, 'Spell Ward': 21, 'Hold the Line': 33, 'Stout Heart': 18,
  'Resilience': 43, 'Oathbound': 40, 'Stalwart': 31, 'Brace': 30, 'Unbreakable': 34,
  'Pall of Shadow': 8, 'Creeping Rot': 32, 'Shadow Bolt': 35, 'Blight': 46, 'Plague Wind': 13, 'Contagion': 15, 'Drain': 29, 'Black Sun': 24,
  'Umbral Strike': 22, 'Night Blade': 47, 'Rupture': 34, 'Blind': 30, 'Murk': 31, 'Night Terrors': 33, 'Wither': 37, 'Soul Rend': 36,
  'Cloak of Night': 19, 'Malice': 18, "Night's Edge": 44,
  'Bedrock': 8, 'Stone Spike': 10, 'Tremor': 37, 'Earthquake': 45, 'Landslide': 46, 'Upheaval': 24, 'Stone Fist': 47, 'Boulder': 35, 'Shockwave': 22,
  'Earthshatter': 7, 'Crushing Blow': 34, 'Mire': 31, 'Stoneskin': 12, 'Dig In': 30, 'Stone Wall': 48, 'Earthen Might': 18, 'Monolith': 25, 'Unyielding': 33,
  'Wellspring': 8, 'Tidal Wave': 13, 'Deep Freeze': 7, 'Wash Away': 4, 'Purging Rain': 21, 'Monsoon': 3, 'Great Flood': 24,
}
// The short line the spellbook shows under each power. There is room for about 28 letters.
const SHORT_TEXT = {
  'Ignite': 'Strike that leaves a burn', 'Flame Cleave': 'Hits target and neighbours', 'Melt Armor': 'Target takes more damage',
  'Stoke the Flames': '+30% damage for 20s', 'Phoenix': 'Rise again on death', 'Smolder': 'Stacking burn from range',
  'Wildfire': 'Burn that spreads', 'Meteor': 'Heavy hit, radius 2', 'Kindling': 'Removes fire resistance',
  'Soothing Waters': 'Heals an ally, or you', 'Healing Rain': 'Heals allies, radius 4',
  'Glacial Spike': 'Heavy ice hit, one enemy', 'Ice Burst': 'Ice damage, radius 2', 'Hailstorm': 'Ice damage, radius 4',
  'Icebreaker': 'Strike dealing ice damage', 'Riptide': 'Hurts foes, heals allies', 'Springwater': 'Heals ally, half to nearby',
  'Clarity': 'Ally regains mana faster', 'High Tide': 'All allies: mana regen 20s', 'Reservoir': 'Ally has more maximum mana',
  'Mana Tide': 'Gives an ally 15 mana',
  'Wellspring': 'Aura: mana regen, radius 8', 'Tidal Wave': 'Wave of ice damage', 'Deep Freeze': 'Holds foes still 6s, rad 2',
  'Wash Away': 'Cleanses and heals an ally', 'Purging Rain': 'Cleanses allies, radius 4', 'Monsoon': 'Big heal + cleanse, rad 4',
  'Great Flood': 'Hurts foes, heals, radius 6',
  'Bedrock': 'Aura: -1 physical damage', 'Stone Spike': 'Earth damage, radius 2', 'Tremor': 'Ground shakes 10s, rad 2',
  'Earthquake': 'Ground shakes 20s, rad 4', 'Landslide': 'Earth damage, radius 4', 'Upheaval': 'Heavy hit + hold, rad 6',
  'Stone Fist': 'Strike dealing earth damage', 'Boulder': 'Thrown rock, one enemy', 'Shockwave': 'Hits target and neighbours',
  'Earthshatter': 'Strike; holds foe 2s', 'Crushing Blow': 'Heavy; foe takes more', 'Mire': 'Holds foes 4s, radius 2',
  'Stoneskin': '-2 physical dmg, +10 armor', 'Dig In': '-5 physical dmg for 20s', 'Stone Wall': 'All allies: -3 phys, 15s',
  'Earthen Might': '+10 Power', 'Monolith': 'Threat; +10% max life', 'Unyielding': 'Threat; never staggered',
  'Pall of Shadow': 'Aura: foes resist less', 'Creeping Rot': 'Cheap Corruption', 'Shadow Bolt': 'Hit that leaves Corruption',
  'Blight': 'Corruption, radius 2', 'Plague Wind': 'Corruption in a cone', 'Contagion': 'Corruption that leaps',
  'Drain': 'Corruption that heals you', 'Black Sun': 'Hit + Corruption, radius 6', 'Umbral Strike': 'Strike leaving Corruption',
  'Night Blade': 'Cheap shadow strike', 'Rupture': 'Strike: more per Corruption', 'Blind': 'Foe misses more',
  'Murk': 'Foes miss a lot for 12s', 'Night Terrors': 'Corruption; foe misses more', 'Wither': 'Foe takes more over time',
  'Soul Rend': 'Removes Astral resistance', 'Cloak of Night': 'Monsters notice you less', 'Malice': 'Your Corruption hits harder',
  "Night's Edge": 'Weapon hits add Corruption',
  'Aegis': 'Aura: small shield per 10s', 'Shield Bash': 'Shield strike', 'Shield Slam': 'Heavy; armor adds damage',
  'Shield Sweep': 'Hits target and neighbours', 'Concussive Blow': 'Strike; holds foe 3s', 'Guarded Strike': 'Strike; +15 armor for 5s',
  'Bulwark Slam': 'Shield per enemy struck', 'Barrier': 'Shield of 14 on an ally', 'Reinforce': 'Big shield and +15 armor',
  'Shield Wall': 'Shields ally and nearby', 'Phalanx': 'Shields every ally', 'Bulwark': 'You have more armor',
  'Spell Ward': 'Ally: +10 resistances', 'Hold the Line': 'All allies: +5 armor', 'Stout Heart': 'Ally has more maximum life',
  'Resilience': 'You regain life faster', 'Oathbound': 'You take 30% of ally damage', 'Stalwart': 'Monsters prefer you',
  'Brace': '+30 armor and resist, 20s', 'Unbreakable': 'All allies: half damage 12s',
  'Ironbark': 'Aura: resistances, rad 8', 'Healing Touch': 'Heals an ally, or you', 'Nourish': 'Cheap heal; more with a HoT',
  'Full Bloom': 'Restores 70% of ally life', 'Pollinate': 'Heal that leaps to others', 'Verdant Tide': 'Heals every ally anywhere',
  'Fairy Ring': 'Heals allies, radius 2', 'Rejuvenation': 'Heals an ally over 12s', 'Spring Shower': 'Heals all allies over 10s',
  'Sacred Grove': 'Healing ground, 20s, rad 4', 'Remedy': 'Cleanses and heals an ally', 'Renewal': 'Cleanses an ally over 10s',
  'Fresh Breeze': 'Cleanses every ally', 'New Growth': 'Raises ally, then regrows', 'Barkskin': 'Ally: +10 resistances',
  'Canopy': 'All allies: resist for 20s', 'Ironwood': 'Ally has more armor', 'Oakheart': 'Ally has more maximum life',
  'Camouflage': 'Monsters notice you less', 'World Tree': 'Healing ground, 30s, rad 6',
  'Radiance': 'Aura: heals allies, rad 8', 'Mend': 'Small, cheap heal', 'Divine Light': 'Huge heal, huge cost',
  'Patient Prayer': 'Cheap medium heal', 'Lay on Hands': 'Restores 70% of ally life', 'Purify': 'Cleanses and heals an ally',
  'Absolution': 'Cleanses ally and nearby', 'Chorus of Light': 'Heal that leaps to others', 'Second Dawn': 'Raises a fallen ally',
  'Guardian Angel': 'Ally rises once on death', 'Miracle': 'Raises and heals everyone', 'Ward of Purity': 'Cleanse; wards for 20s',
  'Vigil': 'Ally regains life faster', 'Benediction': 'Ally has more maximum life', 'Smite': 'Strike dealing holy damage',
  'Judgment': 'Heavy; more to undead/demon', 'Atonement': 'Strike that heals an ally', 'Searing Light': 'Holy damage, radius 2',
  'Dawnburst': 'Hurts foes, heals, radius 4',
  'Searing Brand': 'All hits deal extra fire', 'Fan the Flames': 'Ally acts 10% faster',
}
// What each built power does in the game. "missile" is the game projectile that carries the cast;
// "effect" names what lands. A power not listed here has no behaviour yet: it can be learned, and
// casting it says "not built yet".
//   Burn: adds to the shared Burn on the target (and on everything within the power's radius).
//         Every damage-over-time effect ticks every 2 seconds for 20 seconds, so the power's
//         potency, its total over that time, is spread over 10 ticks.
//   Burst: every enemy within the power's radius of the target takes the power's potency as damage
//         at once, in the power's element.
//   Strike: a weapon attack. The power's potency is added to the swing, and the whole hit lands in
//         the power's element. "spread" carries the same hit to every other enemy within the
//         power's radius of the target. "rider" adds a damage-over-time effect to the target;
//         riderTotal is that effect's total over its 20 seconds. The rider "Vulnerable" makes the
//         target take more damage until it dies; riderPercent is how much more.
//         The rider "Branded" makes every direct hit on the target deal extra fire damage until
//         it dies; riderDamage is how much.
//   Kindle: the target and everything within the power's radius lose their resistance and
//         immunity to fire until they die.
//   Rebirth: passive. When the player would die they rise at once with the power's potency as a
//         percentage of their life, and the power goes on cooldown. It is never cast.
//   Heal: restores the power's potency in life to the player aimed at, or to the caster if no
//         player was aimed at. Works on other players in multiplayer. With a radius it falls on
//         a spot instead and heals every player within the radius of it.
//   GroundBurst: a Burst that lands on the spot chosen rather than on a monster, so it can be
//         cast at empty ground. The rider "Heal" also heals every player within the radius;
//         riderHeal is how much.
//   Heal on a power tagged Ally with a radius: the ally aimed at is healed in full and every
//         other player within the radius of them for half.
//   Mana: gives the power's potency in mana to the player aimed at.
//   A Buff tagged Everyone reaches every player in the game. Buff stats: Damage, Speed,
//         ManaRegen, MaxMana.
//   Aura: a lasting buff on the caster that also helps every living player within the power's
//         radius of them, for as long as they stay there. It takes a stat like a Buff.
//   Wave: the game's Flame Wave, in the power's colour and damage type, dealing its potency.
//   Freeze: holds the target and every monster within the radius still for the potency in
//         seconds (the game's Stone Curse). The time does not grow with the power's level.
//   Cleanse: removes harmful effects from every player within the radius of the spot. As a
//         rider on a Heal ("Cleanse") or a GroundBurst ("HealCleanse") it is added to the heal.
//         Monsters put nothing harmful on players yet, so there is nothing for it to remove.
//   HealPercent: a Heal whose potency is a percentage of the target's maximum life.
//   ChainHeal: a Heal that then leaps to the nearest wounded player, and again, a quarter less
//         each time. Two leaps, one more for every two levels.
//   Resurrect: raises the fallen player nearest the spot chosen, with the potency as a percentage
//         of their life. Tagged Everyone it raises every fallen player, and the living are healed
//         for riderHeal.
//   An Aura with the stat HealPulse heals everyone it reaches for its potency every 2 seconds.
//   More Buff stats: LifeRegen, MaxLife, Rebirth (the player rises once when they would die,
//         with the potency as a percentage of their life), Ward (nothing yet).
//   More Strike riders: Bane (riderPercent more damage to undead and demons), HealAlly (the most
//         wounded player within 8 tiles is healed for riderHeal).
//   A Buff with the stat HealPulse and "overTime" is a heal over time: the potency is the total,
//         spread over one pulse every 2 seconds of the duration.
//   Zone: healing ground at the spot chosen, for the duration. Every 2 seconds each wounded
//         player within the radius is healed; the potency is the total over the whole time. A
//         rider naming "Raise" also raises each fallen player lying in it, once, with 30% life.
//   More Buff stats: Resist (points on all three resistances), Armor (points), Distance (how far
//         away the player seems to monsters, as a percentage: above 100 is stealth), Renewal
//         (nothing yet).
//   More riders: Nourish on a Heal (riderPercent more if the target has one of the caster's heals
//         over time), Regrow on a Resurrect (riderPercent of life regained over the duration).
//   Shield: temporary health on the player aimed at (or around them, or on everyone, as a Heal
//         would reach), for the duration. It takes damage before their own life does.
//   An Aura with the stat ShieldPulse renews a shield of its potency on everyone it reaches
//         every 10 seconds.
//   More Buff stats: DamageTaken (the percentage less damage the player takes), Oath (the caster
//         takes that percentage of the player's damage instead; one player at a time).
//         Distance below 100 is threat. A buff's rider can name a second stat it changes, by
//         riderPercent (Brace: Armor and Resist; Reinforce: a Shield and Armor while it lasts).
//   More Strike riders: Freeze (holds the target riderPercent seconds), ArmorBonus (riderPercent
//         of the striker's armor as extra damage), Guard (the striker gains the power's buff, at
//         riderPercent strength), ShieldPerHit (a shield of riderHeal for each monster struck).
//   A power with a Weapon tag needs that weapon in hand. Only Shield is checked by the game.
//   Corruption works as Burn does, for Shadow. Riders on either: Leech (the caster gains
//         riderDamage of life every 2 seconds for the duration), Accuracy (the monster also has
//         riderPercent less chance to hit, for the duration), Wither (every effect over time on
//         the monster deals riderPercent more until it dies).
//   A Burst or GroundBurst whose rider names an effect over time leaves riderTotal of it behind.
//   Cone and Chain lay down the effect over time their rider names, the potency being its total:
//         a cone on everything within 6 tiles the way the caster aims, a chain on the target and
//         then on the nearest other monster, and again, a quarter less each leap.
//   Curse: a debuff on the target, and on monsters within the radius, that every PC is told of.
//         Stat Accuracy: potency percent less chance to hit, for the duration, or with none
//         until it dies. Stat AstralStrip: no Astral resistance or immunity until it dies.
//   An Aura with the stat AstralCurse makes monsters within it take potency percent more from
//         Astral damage.
//   More Buff stats: DotDamage (percent more from the caster's effects over time), WeaponDot
//         (each weapon hit adds the effect over time its rider names; potency is the total).
//   The Strike rider Rupture adds riderPercent of the Corruption still to tick on the target.
//   Bolt (missile PowerBolt): a projectile like Firebolt, in the power's colour, dealing its
//         potency in its damage type. It can miss. A rider naming an effect over time leaves
//         riderTotal of it on whatever the bolt hits.
//   Buff stat DmgReduction: potency points off every physical hit the player takes (monster
//         melee and arrows), never below 1. Spells and other elements are not reduced.
//   DamageZone: ground at the spot chosen that hurts every monster standing on it, every 2
//         seconds for the duration; the potency is the total over the whole time.
//   GroundFreeze: a Freeze aimed at a spot instead of a monster. A GroundBurst with the rider
//         Freeze also holds what it hits, for riderPercent seconds.
//   More Buff stats: Power (points), NoStagger (hits never interrupt the player). A buff's
//         rider can also be Distance, giving a buff about something else threat or stealth.
//   Buff: switches a buff on for the caster, or for the player aimed at if the power is tagged Ally. "stat" is what it changes; the power's potency is how
//         much (30 is +30%); "duration" is seconds, or 0 for a lasting buff.
const TICKS_PER_EFFECT = 10
const BEHAVIOUR = {
  'Smolder': { missile: 'Corruption', effect: 'Burn' },
  'Wildfire': { missile: 'Corruption', effect: 'Burn' },
  'Meteor': { missile: 'Corruption', effect: 'Burst' },
  'Ignite': { effect: 'Strike', rider: 'Burn', riderTotal: 5 },
  'Flame Cleave': { effect: 'Strike', spread: true },
  'Melt Armor': { effect: 'Strike', rider: 'Vulnerable', riderPercent: 15 },
  'Searing Brand': { effect: 'Strike', rider: 'Branded', riderDamage: 2 },
  'Kindling': { missile: 'Corruption', effect: 'Kindle' },
  'Phoenix': { effect: 'Rebirth' },
  'Stoke the Flames': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Damage', duration: 20 },
  'Fan the Flames': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Speed', duration: 0 },
  'Soothing Waters': { missile: 'StrengthBuff', effect: 'Heal' },
  'Healing Rain': { missile: 'StrengthBuff', effect: 'Heal' },
  'Glacial Spike': { missile: 'Corruption', effect: 'Burst' },
  'Ice Burst': { missile: 'Corruption', effect: 'GroundBurst' },
  'Hailstorm': { missile: 'Corruption', effect: 'GroundBurst' },
  'Icebreaker': { effect: 'Strike' },
  'Riptide': { missile: 'Corruption', effect: 'GroundBurst', rider: 'Heal', riderHeal: 8 },
  'Springwater': { missile: 'StrengthBuff', effect: 'Heal' },
  'Clarity': { missile: 'StrengthBuff', effect: 'Buff', stat: 'ManaRegen', duration: 0 },
  'High Tide': { missile: 'StrengthBuff', effect: 'Buff', stat: 'ManaRegen', duration: 20 },
  'Reservoir': { missile: 'StrengthBuff', effect: 'Buff', stat: 'MaxMana', duration: 0 },
  'Mana Tide': { missile: 'StrengthBuff', effect: 'Mana' },
  'Wellspring': { missile: 'StrengthBuff', effect: 'Aura', stat: 'ManaRegen', duration: 0 },
  'Tidal Wave': { missile: 'FlameWaveControl', effect: 'Wave' },
  'Deep Freeze': { missile: 'Corruption', effect: 'Freeze' },
  'Wash Away': { missile: 'StrengthBuff', effect: 'Heal', rider: 'Cleanse', riderPercent: 1 },
  'Purging Rain': { missile: 'StrengthBuff', effect: 'Cleanse' },
  'Monsoon': { missile: 'StrengthBuff', effect: 'Heal', rider: 'Cleanse', riderPercent: 99 },
  'Great Flood': { missile: 'Corruption', effect: 'GroundBurst', rider: 'HealCleanse', riderHeal: 29 },
  'Bedrock': { missile: 'StrengthBuff', effect: 'Aura', stat: 'DmgReduction', duration: 0 },
  'Stone Spike': { missile: 'Corruption', effect: 'GroundBurst' },
  'Tremor': { missile: 'StrengthBuff', effect: 'DamageZone', duration: 10 },
  'Earthquake': { missile: 'StrengthBuff', effect: 'DamageZone', duration: 20 },
  'Landslide': { missile: 'Corruption', effect: 'GroundBurst' },
  'Upheaval': { missile: 'Corruption', effect: 'GroundBurst', rider: 'Freeze', riderPercent: 4 },
  'Stone Fist': { effect: 'Strike' },
  'Boulder': { missile: 'PowerBolt', effect: 'Bolt' },
  'Shockwave': { effect: 'Strike', spread: true },
  'Earthshatter': { effect: 'Strike', rider: 'Freeze', riderPercent: 2 },
  'Crushing Blow': { effect: 'Strike', rider: 'Vulnerable', riderPercent: 15 },
  'Mire': { missile: 'Corruption', effect: 'GroundFreeze' },
  'Stoneskin': { missile: 'StrengthBuff', effect: 'Buff', stat: 'DmgReduction', duration: 0, rider: 'Armor', riderPercent: 10 },
  'Dig In': { missile: 'StrengthBuff', effect: 'Buff', stat: 'DmgReduction', duration: 20 },
  'Stone Wall': { missile: 'StrengthBuff', effect: 'Buff', stat: 'DmgReduction', duration: 15 },
  'Earthen Might': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Power', duration: 0 },
  'Monolith': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Distance', duration: 0, rider: 'MaxLife', riderPercent: 10 },
  'Unyielding': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Distance', duration: 0, rider: 'NoStagger', riderPercent: 1 },
  'Pall of Shadow': { missile: 'StrengthBuff', effect: 'Aura', stat: 'AstralCurse', duration: 0 },
  'Creeping Rot': { missile: 'Corruption', effect: 'Corruption' },
  'Shadow Bolt': { missile: 'PowerBolt', effect: 'Bolt', rider: 'Corruption', riderTotal: 4 },
  'Blight': { missile: 'Corruption', effect: 'Corruption' },
  'Plague Wind': { missile: 'Corruption', effect: 'Cone', rider: 'Corruption', riderPercent: 0 },
  'Contagion': { missile: 'Corruption', effect: 'Chain', rider: 'Corruption', riderPercent: 0 },
  'Drain': { missile: 'Corruption', effect: 'Corruption', rider: 'Leech', riderDamage: 0.5, stat: 'HealPulse', duration: 20 },
  'Black Sun': { missile: 'Corruption', effect: 'GroundBurst', rider: 'Corruption', riderTotal: 40 },
  'Umbral Strike': { effect: 'Strike', rider: 'Corruption', riderTotal: 4 },
  'Night Blade': { effect: 'Strike' },
  'Rupture': { effect: 'Strike', rider: 'Rupture', riderPercent: 50 },
  'Blind': { missile: 'Corruption', effect: 'Curse', stat: 'Accuracy', duration: 0 },
  'Murk': { missile: 'Corruption', effect: 'Curse', stat: 'Accuracy', duration: 12 },
  'Night Terrors': { missile: 'Corruption', effect: 'Corruption', rider: 'Accuracy', riderPercent: 15, duration: 20 },
  'Wither': { missile: 'Corruption', effect: 'Corruption', rider: 'Wither', riderPercent: 25 },
  'Soul Rend': { missile: 'Corruption', effect: 'Curse', stat: 'AstralStrip', duration: 0 },
  'Cloak of Night': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Distance', duration: 0 },
  'Malice': { missile: 'StrengthBuff', effect: 'Buff', stat: 'DotDamage', duration: 0 },
  "Night's Edge": { missile: 'StrengthBuff', effect: 'Buff', stat: 'WeaponDot', duration: 0, perTick: true, rider: 'Corruption', riderPercent: 0 },
  'Aegis': { missile: 'StrengthBuff', effect: 'Aura', stat: 'ShieldPulse', duration: 0 },
  'Shield Bash': { effect: 'Strike' },
  'Shield Slam': { effect: 'Strike', rider: 'ArmorBonus', riderPercent: 25 },
  'Shield Sweep': { effect: 'Strike', spread: true },
  'Concussive Blow': { effect: 'Strike', rider: 'Freeze', riderPercent: 3 },
  'Guarded Strike': { effect: 'Strike', rider: 'Guard', riderPercent: 15, stat: 'Armor', duration: 5 },
  'Bulwark Slam': { effect: 'Strike', spread: true, rider: 'ShieldPerHit', riderHeal: 18, stat: 'Shield', duration: 15 },
  'Barrier': { missile: 'StrengthBuff', effect: 'Shield', stat: 'Shield', duration: 15 },
  'Reinforce': { missile: 'StrengthBuff', effect: 'Shield', stat: 'Shield', duration: 20, rider: 'Armor', riderPercent: 15 },
  'Shield Wall': { missile: 'StrengthBuff', effect: 'Shield', stat: 'Shield', duration: 15 },
  'Phalanx': { missile: 'StrengthBuff', effect: 'Shield', stat: 'Shield', duration: 15 },
  'Bulwark': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Armor', duration: 0 },
  'Spell Ward': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Resist', duration: 0 },
  'Hold the Line': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Armor', duration: 0 },
  'Stout Heart': { missile: 'StrengthBuff', effect: 'Buff', stat: 'MaxLife', duration: 0 },
  'Resilience': { missile: 'StrengthBuff', effect: 'Buff', stat: 'LifeRegen', duration: 0 },
  'Oathbound': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Oath', duration: 0 },
  'Stalwart': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Distance', duration: 0 },
  'Brace': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Armor', duration: 20, rider: 'Resist', riderPercent: 30 },
  'Unbreakable': { missile: 'StrengthBuff', effect: 'Buff', stat: 'DamageTaken', duration: 12 },
  'Ironbark': { missile: 'StrengthBuff', effect: 'Aura', stat: 'Resist', duration: 0 },
  'Healing Touch': { missile: 'StrengthBuff', effect: 'Heal' },
  'Nourish': { missile: 'StrengthBuff', effect: 'Heal', rider: 'Nourish', riderPercent: 50 },
  'Full Bloom': { missile: 'StrengthBuff', effect: 'HealPercent' },
  'Pollinate': { missile: 'StrengthBuff', effect: 'ChainHeal' },
  'Verdant Tide': { missile: 'StrengthBuff', effect: 'Heal' },
  'Fairy Ring': { missile: 'StrengthBuff', effect: 'Heal' },
  'Rejuvenation': { missile: 'StrengthBuff', effect: 'Buff', stat: 'HealPulse', duration: 12, overTime: true },
  'Spring Shower': { missile: 'StrengthBuff', effect: 'Buff', stat: 'HealPulse', duration: 10, overTime: true },
  'Sacred Grove': { missile: 'StrengthBuff', effect: 'Zone', duration: 20, rider: 'Cleanse', riderPercent: 1 },
  'Remedy': { missile: 'StrengthBuff', effect: 'Heal', rider: 'Cleanse', riderPercent: 1 },
  'Renewal': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Renewal', duration: 10 },
  'Fresh Breeze': { missile: 'StrengthBuff', effect: 'Cleanse' },
  'New Growth': { missile: 'StrengthBuff', effect: 'Resurrect', stat: 'HealPulse', duration: 15, rider: 'Regrow', riderPercent: 60 },
  'Barkskin': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Resist', duration: 0 },
  'Canopy': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Resist', duration: 20 },
  'Ironwood': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Armor', duration: 0 },
  'Oakheart': { missile: 'StrengthBuff', effect: 'Buff', stat: 'MaxLife', duration: 0 },
  'Camouflage': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Distance', duration: 0 },
  'World Tree': { missile: 'StrengthBuff', effect: 'Zone', duration: 30, rider: 'CleanseRaise', riderPercent: 1 },
  'Radiance': { missile: 'StrengthBuff', effect: 'Aura', stat: 'HealPulse', duration: 0 },
  'Mend': { missile: 'StrengthBuff', effect: 'Heal' },
  'Divine Light': { missile: 'StrengthBuff', effect: 'Heal' },
  'Patient Prayer': { missile: 'StrengthBuff', effect: 'Heal' },
  'Lay on Hands': { missile: 'StrengthBuff', effect: 'HealPercent', rider: 'Cleanse', riderPercent: 99 },
  'Purify': { missile: 'StrengthBuff', effect: 'Heal', rider: 'Cleanse', riderPercent: 1 },
  'Absolution': { missile: 'StrengthBuff', effect: 'Cleanse' },
  'Chorus of Light': { missile: 'StrengthBuff', effect: 'ChainHeal' },
  'Second Dawn': { missile: 'StrengthBuff', effect: 'Resurrect' },
  'Guardian Angel': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Rebirth', duration: 0 },
  'Miracle': { missile: 'StrengthBuff', effect: 'Resurrect', rider: 'HealCleanse', riderHeal: 15 },
  'Ward of Purity': { missile: 'StrengthBuff', effect: 'Buff', stat: 'Ward', duration: 20, rider: 'Cleanse', riderPercent: 1 },
  'Vigil': { missile: 'StrengthBuff', effect: 'Buff', stat: 'LifeRegen', duration: 0 },
  'Benediction': { missile: 'StrengthBuff', effect: 'Buff', stat: 'MaxLife', duration: 0 },
  'Smite': { effect: 'Strike' },
  'Judgment': { effect: 'Strike', rider: 'Bane', riderPercent: 50 },
  'Atonement': { effect: 'Strike', rider: 'HealAlly', riderHeal: 11 },
  'Searing Light': { missile: 'Corruption', effect: 'Burst' },
  'Dawnburst': { missile: 'Corruption', effect: 'GroundBurst', rider: 'Heal', riderHeal: 11 },
}
const FIRST_NUMBER = 100
// The powers whose awakening stones Pepin's test shelf sells: the ones being tested now. Use the
// names as the designer shows them. Everything else stays in the game but is not for sale.
const ON_SALE = ['Bedrock', 'Stone Spike', 'Tremor', 'Earthquake', 'Landslide', 'Upheaval', 'Stone Fist', 'Boulder', 'Shockwave', 'Earthshatter',
  'Crushing Blow', 'Mire', 'Stoneskin', 'Dig In', 'Stone Wall', 'Earthen Might', 'Monolith', 'Unyielding']
const SHOP_FILE = path.join(REPO, 'assets/txtdata/spells/stone_shop.tsv')

const data = JSON.parse(fs.readFileSync(DATA_FILE, 'utf8'))
// First, the change log: what has been edited in the designer since the last export. Anything
// marked NEEDS WORK is not carried into the game by this script and has to be done by hand.
if (readSnapshot()) {
  printChanges(readSnapshot(), data, 'the last export')
  console.log('')
}
const tagNames = (power) => power.tags.map((id) => data.tags.find((t) => t.id === id)?.name).filter(Boolean)
const weaponGroup = data.tagGroups.find((g) => g.name === 'Weapon')?.id
const weaponOf = (power) => power.tags.map((id) => data.tags.find((t) => t.id === id)).find((t) => t && t.groupId === weaponGroup)?.name ?? ''
const keyFor = (name) => name.replace(/[^A-Za-z0-9 ]/g, '').split(' ').filter(Boolean).map((w) => w[0].toUpperCase() + w.slice(1)).join('')

const exported = []
for (const essenceName of essenceNames) {
  const essence = data.essences.find((e) => e.name.toLowerCase() === essenceName.toLowerCase())
  if (!essence) throw new Error('No essence called ' + essenceName)
  for (const power of data.powers.filter((p) => p.essenceIds.includes(essence.id))) {
    if (ALREADY_IN_GAME[power.name]) continue
    if (power.number == null) {
      power.number = Math.max(FIRST_NUMBER - 1, ...data.powers.map((p) => p.number ?? 0)) + 1
      power.gameKey = keyFor(power.name)
    }
    // 26 is the blank stand-in given to a power exported before it had a picture chosen.
    if (power.icon == null || power.icon === 26) power.icon = ICONS[power.name] ?? 26
    if (power.shortText == null && SHORT_TEXT[power.name]) power.shortText = SHORT_TEXT[power.name]
    exported.push({ power, essence })
  }
}
exported.sort((a, b) => a.power.number - b.power.number)

// The short line the spellbook shows under a power: its headline number and what it measures.
const shortLine = (power) => {
  if (power.shortText) return power.shortText
  const text = (power.potency != null ? power.potency + ' ' : '') + (power.potencyNote ?? '').split(/[;,(]/)[0].trim()
  return text.length <= 30 ? text : text.slice(0, 30).replace(/\s+\S*$/, '')
}
const row = ({ power, essence }) => {
  const tags = tagNames(power)
  const element = tags.includes('Lightning') ? 'Lightning' : tags.includes('Fire') ? 'Fire' : 'Magic'
  // Every buff and heal can be cast in town, cooldown ones included.
  const inTown = ['Buff', 'Heal', 'Mana', 'Aura', 'Cleanse', 'HealPercent', 'ChainHeal', 'Resurrect', 'Zone', 'Shield'].includes(BEHAVIOUR[power.name]?.effect)
  const flags = [element, ...(tags.includes('Enemy') ? ['Targeted'] : []), ...(inTown ? ['AllowedInTown'] : [])].join(',')
  const mana = Math.min(250, Math.max(0, Math.round(power.manaCost ?? 0)))
  const behaviour = BEHAVIOUR[power.name]
  // Amounts are in 64ths of a hit point, the unit the game counts damage in.
  const amount = behaviour?.perTick || ['Burn', 'Corruption', 'Cone', 'Chain'].includes(behaviour?.effect) ? Math.round((power.potency ?? 0) * 64 / TICKS_PER_EFFECT)
    : behaviour?.effect === 'Curse' ? Math.round(power.potency ?? 0)
    : behaviour?.effect === 'Bolt' ? Math.round((power.potency ?? 0) * 64)
    : behaviour?.overTime || ['Zone', 'DamageZone'].includes(behaviour?.effect) ? Math.round((power.potency ?? 0) * 64 / (behaviour.duration / 2))
    : ['HealPulse', 'ShieldPulse'].includes(behaviour?.stat) && behaviour.effect === 'Aura' ? Math.round((power.potency ?? 0) * 64)
    : behaviour?.effect === 'Shield' ? Math.round((power.potency ?? 0) * 64)
    : ['Burst', 'GroundBurst', 'Strike', 'Heal', 'Mana', 'Wave', 'ChainHeal'].includes(behaviour?.effect) ? Math.round((power.potency ?? 0) * 64)
    : behaviour?.effect === 'Cleanse' ? Math.round(power.potency ?? 99)
    : ['Aura', 'Freeze', 'GroundFreeze', 'HealPercent', 'Resurrect'].includes(behaviour?.effect) ? Math.round(power.potency ?? 0)
    : behaviour?.effect === 'Buff' || behaviour?.effect === 'Rebirth' ? Math.round(power.potency ?? 0)
    : 0
  // A strike only reaches past its target when it is marked to spread.
  // 99 stands for "everyone in the game".
  const radius = !behaviour ? 0 : behaviour.effect === 'Strike' && !behaviour.spread ? 0 : tags.some((t) => t.startsWith('Everyone')) ? 99 : Math.round(power.radius ?? 0)
  const riderAmount = !behaviour?.rider ? 0
    : behaviour.riderPercent != null ? behaviour.riderPercent
    : behaviour.riderDamage != null ? Math.round(behaviour.riderDamage * 64)
    : behaviour.riderHeal != null ? Math.round(behaviour.riderHeal * 64)
    : Math.round(behaviour.riderTotal * 64 / TICKS_PER_EFFECT)
  return [
    power.gameKey, power.number, power.name, essence.name, element === 'Lightning' ? 'CastLightning' : element === 'Magic' && ['Heal', 'Mana', 'Buff', 'Aura', 'Cleanse', 'HealPercent', 'ChainHeal', 'Resurrect', 'Zone', 'Shield'].includes(behaviour?.effect) ? 'CastHealing' : 'CastFire',
    mana, flags, behaviour?.missile ?? '', 0, mana, power.icon,
    Math.round(power.cooldown ?? 0), shortLine(power),
    behaviour?.effect ?? '', amount, radius, behaviour?.rider ?? '', riderAmount,
    behaviour?.stat ?? '', behaviour?.duration ?? 0,
    // Heals and buffs tagged Ally can be aimed at another player.
    ['Heal', 'Buff', 'Mana', 'Cleanse', 'HealPercent', 'ChainHeal', 'Shield'].includes(behaviour?.effect) && tags.includes('Ally') ? 1 : 0,
    // The weapon the power needs in hand, from its Weapon tag.
    behaviour ? weaponOf(power) : '',
  ].join('\t')
}
const header = ['id', 'number', 'name', 'essence', 'soundId', 'manaCost', 'flags', 'missiles', 'manaMultiplier', 'minMana', 'icon', 'cooldown', 'description', 'effect', 'amount', 'radius', 'rider', 'riderAmount', 'stat', 'duration', 'ally', 'weapon'].join('\t')
const powersText = [header, ...exported.map(row)].join('\r\n') + '\r\n'

// Stones. Keep every existing row exactly where it is; drop the stand-in test stone if it is
// still the last row; add a stone at the end for each power that has none.
let itemLines = fs.readFileSync(ITEMS_FILE, 'utf8').split('\r\n')
if (itemLines.at(-1) === '') itemLines.pop()
const SPELL_COLUMN = 20
if (itemLines.at(-1).split('\t')[SPELL_COLUMN] === 'TestBolt') itemLines.pop()
const stonesAdded = []
for (const { power } of exported) {
  if (itemLines.some((line) => line.split('\t')[SPELL_COLUMN] === power.gameKey)) continue
  itemLines.push(['', 0, 'Misc', 'Unequippable', 'AWAKENING_STONE', 'Misc', 'NONE', 'Awakening Stone of ' + power.name, 'Stone', 1, 0, 0, 0, 0, 0, 0, 0, 0, '', 'AWAKENINGSTONE', power.gameKey, 'true', 1].join('\t'))
  stonesAdded.push(power.name)
}

for (const { power } of exported) console.log(String(power.number).padStart(4), power.gameKey.padEnd(16), String(power.manaCost ?? '-').padStart(3) + ' mana', String(power.cooldown ?? 0).padStart(4) + 's', ' icon ' + String(power.icon).padStart(2), ' ', shortLine(power).padEnd(28), BEHAVIOUR[power.name] ? '[' + BEHAVIOUR[power.name].effect + ']' : '[not built]')
console.log(`${exported.length} powers for ${essenceNames.join(', ')}; ${stonesAdded.length} stones added`)
if (check) {
  console.log('Check only: nothing was written.')
} else {
  fs.writeFileSync(DATA_FILE, JSON.stringify(data, null, 2) + '\n')
  // Keep a copy of the designer's data as exported, for the next change log to compare against.
  fs.writeFileSync(SNAPSHOT_FILE, JSON.stringify(data, null, 2) + '\n')
  fs.writeFileSync(POWERS_FILE, powersText)
  const saleKeys = ON_SALE.map((name) => {
    const key = ALREADY_IN_GAME[name] ?? data.powers.find((p) => p.name === name)?.gameKey
    if (!key) throw new Error('ON_SALE names a power the game does not have: ' + name)
    return key
  })
  fs.writeFileSync(SHOP_FILE, ['id', ...saleKeys].join('\r\n') + '\r\n')
  console.log('Stones on sale: ' + (ON_SALE.join(', ') || 'none'))
  fs.writeFileSync(ITEMS_FILE, itemLines.join('\r\n') + '\r\n')
  console.log('Written.')
}
