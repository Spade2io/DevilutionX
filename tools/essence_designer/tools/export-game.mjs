// Writes the designer's powers into the files the game reads.
// Usage: node tools/export-game.mjs [--check] [essence names...]     (default: Fire)
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
const essenceNames = wanted.length ? wanted : ['Fire']

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
}
// The short line the spellbook shows under each power. There is room for about 28 letters.
const SHORT_TEXT = {
  'Ignite': 'Strike that leaves a burn', 'Flame Cleave': 'Hits target and neighbours', 'Melt Armor': 'Target takes more damage',
  'Stoke the Flames': '+30% damage for 20s', 'Phoenix': 'Rise again on death', 'Smolder': 'Stacking burn from range',
  'Wildfire': 'Burn that spreads', 'Meteor': 'Heavy hit, radius 2', 'Kindling': 'Removes fire resistance',
  'Searing Brand': 'All hits deal extra fire', 'Fan the Flames': 'Ally acts 10% faster',
}
// What each built power does in the game. "missile" is the game projectile that carries the cast;
// "effect" names what lands. A power not listed here has no behaviour yet: it can be learned, and
// casting it says "not built yet".
//   Burn: adds to the shared Burn on the target (and on everything within the power's radius).
//         Every damage-over-time effect ticks every 2 seconds for 20 seconds, so the power's
//         potency, its total over that time, is spread over 10 ticks.
const TICKS_PER_EFFECT = 10
const BEHAVIOUR = {
  'Smolder': { missile: 'Corruption', effect: 'Burn' },
  'Wildfire': { missile: 'Corruption', effect: 'Burn' },
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
    if (power.icon == null) power.icon = ICONS[power.name] ?? 26
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
  const flags = [element, ...(tags.includes('Enemy') ? ['Targeted'] : [])].join(',')
  const mana = Math.min(250, Math.max(0, Math.round(power.manaCost ?? 0)))
  const behaviour = BEHAVIOUR[power.name]
  // Amounts are in 64ths of a hit point, the unit the game counts damage in.
  const amount = behaviour?.effect === 'Burn' ? Math.round((power.potency ?? 0) * 64 / TICKS_PER_EFFECT) : 0
  return [
    power.gameKey, power.number, power.name, essence.name, element === 'Lightning' ? 'CastLightning' : 'CastFire',
    mana, flags, behaviour?.missile ?? '', 0, mana, power.icon,
    Math.round(power.cooldown ?? 0), shortLine(power),
    behaviour?.effect ?? '', amount, behaviour ? Math.round(power.radius ?? 0) : 0,
  ].join('\t')
}
const header = ['id', 'number', 'name', 'essence', 'soundId', 'manaCost', 'flags', 'missiles', 'manaMultiplier', 'minMana', 'icon', 'cooldown', 'description', 'effect', 'amount', 'radius'].join('\t')
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
