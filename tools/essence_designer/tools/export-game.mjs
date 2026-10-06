// Writes the designer's powers into the files the game reads.
// Usage: node tools/export-game.mjs [--check] [essence names...]     (default: Fire and Water)
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

const here = path.dirname(fileURLToPath(import.meta.url))
const REPO = path.resolve(here, '../../..')
const DATA_FILE = path.join(REPO, 'designer/essence_data.json')
const POWERS_FILE = path.join(REPO, 'assets/txtdata/spells/essence_powers.tsv')
const ITEMS_FILE = path.join(REPO, 'assets/txtdata/items/itemdat.tsv')
const check = process.argv.includes('--check')
const wanted = process.argv.slice(2).filter((a) => !a.startsWith('--'))
const essenceNames = wanted.length ? wanted : ['Fire', 'Water']

// Powers the game already has as hand-written spells. They keep their original numbers and are
// not exported; the name on the right is the game's own name for the spell.
const ALREADY_IN_GAME = {
  'Firebolt': 'Firebolt', 'Fireball': 'Fireball', 'Flame Wave': 'FlameWave', 'Inferno': 'Inferno', 'Fire Wall': 'FireWall',
  'Fire Aura': 'FireAura', 'Flaming Weapon': 'FlamingWeapon', 'Flame Strike': 'FlameStrike', 'Inferno Strike': 'InfernoStrike',
  'Guardian': 'Guardian', 'Elemental': 'Elemental', 'Immolation': 'Immolation', 'Ring of Fire': 'RingOfFire',
  'Rune of Fire': 'RuneOfFire', 'Rune of Immolation': 'RuneOfImmolation', 'Frostbolt': 'Frostbolt', 'Holy Bolt': 'HolyBolt',
}
// Which picture each power borrows from the game's sheet of spell icons, until it has its own.
// The numbers are positions in that sheet (see SpellIcon in Source/panels/spell_icons.hpp).
const ICONS = {
  'Smolder': 32, 'Wildfire': 46, 'Meteor': 24, 'Ignite': 22, 'Flame Cleave': 47, 'Searing Brand': 31, 'Melt Armor': 30,
  'Stoke the Flames': 18, 'Fan the Flames': 33, 'Kindling': 37, 'Phoenix': 40,
  'Soothing Waters': 9, 'Healing Rain': 1, 'Glacial Spike': 29, 'Ice Burst': 10, 'Hailstorm': 45, 'Icebreaker': 35,
  'Riptide': 13, 'Springwater': 40, 'Clarity': 43, 'High Tide': 50, 'Reservoir': 12, 'Mana Tide': 28,
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
}
const FIRST_NUMBER = 100

const data = JSON.parse(fs.readFileSync(DATA_FILE, 'utf8'))
const tagNames = (power) => power.tags.map((id) => data.tags.find((t) => t.id === id)?.name).filter(Boolean)
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
  const inTown = ['Buff', 'Heal', 'Mana'].includes(BEHAVIOUR[power.name]?.effect)
  const flags = [element, ...(tags.includes('Enemy') ? ['Targeted'] : []), ...(inTown ? ['AllowedInTown'] : [])].join(',')
  const mana = Math.min(250, Math.max(0, Math.round(power.manaCost ?? 0)))
  const behaviour = BEHAVIOUR[power.name]
  // Amounts are in 64ths of a hit point, the unit the game counts damage in.
  const amount = behaviour?.effect === 'Burn' ? Math.round((power.potency ?? 0) * 64 / TICKS_PER_EFFECT)
    : ['Burst', 'GroundBurst', 'Strike', 'Heal', 'Mana'].includes(behaviour?.effect) ? Math.round((power.potency ?? 0) * 64)
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
    power.gameKey, power.number, power.name, essence.name, element === 'Lightning' ? 'CastLightning' : element === 'Magic' && ['Heal', 'Mana', 'Buff'].includes(behaviour?.effect) ? 'CastHealing' : 'CastFire',
    mana, flags, behaviour?.missile ?? '', 0, mana, power.icon,
    Math.round(power.cooldown ?? 0), shortLine(power),
    behaviour?.effect ?? '', amount, radius, behaviour?.rider ?? '', riderAmount,
    behaviour?.stat ?? '', behaviour?.duration ?? 0,
    // Heals and buffs tagged Ally can be aimed at another player.
    ['Heal', 'Buff', 'Mana'].includes(behaviour?.effect) && tags.includes('Ally') ? 1 : 0,
  ].join('\t')
}
const header = ['id', 'number', 'name', 'essence', 'soundId', 'manaCost', 'flags', 'missiles', 'manaMultiplier', 'minMana', 'icon', 'cooldown', 'description', 'effect', 'amount', 'radius', 'rider', 'riderAmount', 'stat', 'duration', 'ally'].join('\t')
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
  fs.writeFileSync(POWERS_FILE, powersText)
  fs.writeFileSync(ITEMS_FILE, itemLines.join('\r\n') + '\r\n')
  console.log('Written.')
}
