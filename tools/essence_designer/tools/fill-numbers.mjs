// Fills in first-pass numbers for every power from one simple model, so they can be looked at
// and tested. Usage: node tools/fill-numbers.mjs [--check]
// The model is written out in devilutionX/docs/MOD_DESIGN.md ("First-pass numbers").
// Numbers are for power level 1. Re-running overwrites potency, mana, cooldown, radius and the
// note on every power that is still a draft; accepted powers are left alone.
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const here = path.dirname(fileURLToPath(import.meta.url))
const DATA_FILE = path.resolve(here, '../../../designer/essence_data.json')
const data = JSON.parse(fs.readFileSync(DATA_FILE, 'utf8'))
const round = (n) => Math.max(1, Math.round(n))

// Per-power adjustments the tags cannot express.
const SIZE = {
  'Mend': 0.6, 'Divine Light': 2.5, 'Purify': 0.4, 'Wash Away': 0.4, 'Remedy': 0.4, 'Judgment': 2, 'Shield Slam': 2,
  'Meteor': 1.5, 'Atonement': 1.5, 'Inferno': 1.5, 'Concussive Blow': 0.5, 'Guarded Strike': 0.8, 'Melt Armor': 0.8,
}
// Powers whose numbers are fixed: either they already exist in the game with these values, or
// their effect is not a plain amount.
const FIXED = {
  'Fire Aura': { potency: 1, note: 'fire damage every 2 seconds (as built in the game)', mana: 10 },
  'Flaming Weapon': { potency: 3, note: 'fire damage added to each weapon hit (as built in the game)', mana: 10 },
  'Flame Strike': { potency: 3, note: 'bonus fire damage on the hit (as built in the game)', mana: 2 },
  'Inferno Strike': { potency: 200, note: '% extra weapon damage, so three times the hit (as built in the game)', mana: 6, cooldown: 12 },
  'Lay on Hands': { potency: 70, note: "% of the ally's maximum life restored", mana: 6 },
  'Full Bloom': { potency: 70, note: "% of the ally's maximum life restored", mana: 8 },
  'Second Dawn': { potency: 30, note: '% of life the ally returns with', mana: 30 },
  'New Growth': { potency: 10, note: '% of life on return; regrows a further 60% over 15 seconds', mana: 30 },
  'Guardian Angel': { potency: 30, note: '% of life the ally rises with', mana: 30 },
  'Phoenix': { potency: 50, note: '% of life you rise with', mana: 0 },
  'Miracle': { potency: 30, note: '% of life the fallen return with; everyone else is healed for 15', mana: 60 },
  'World Tree': { potency: 60, note: 'healing in total over 30 seconds, to each ally in its shade', mana: 60 },
  'Ward of Purity': { potency: 20, note: 'seconds of immunity to new harmful effects', mana: 18 },
  'Kindling': { potency: null, note: 'removes fire resistance and fire immunity', mana: 15 },
  'Deep Freeze': { potency: 6, note: 'seconds held', mana: 18 },
  'Stalwart': { potency: 50, note: '% apparent distance: monsters treat you as half as far away', mana: 10 },
  'Camouflage': { potency: 150, note: '% apparent distance: monsters treat you as half again as far away', mana: 10 },
  'Oathbound': { potency: 30, note: "% of the ally's damage that is dealt to you instead", mana: 10 },
  'Unbreakable': { potency: 50, note: '% less damage taken by every ally', mana: 40 },
  'Mana Tide': { potency: 15, note: 'mana given to the ally (you spend 20)', mana: 20 },
  'Ignite': { potency: 2, note: 'bonus damage on the hit, then 4 burning over 6 seconds; each strike adds another layer', mana: 2 },
  'Searing Brand': { potency: 3, note: 'bonus damage on the hit; from then on every hit on that enemy deals 2 extra fire damage', mana: 2 },
  'Renewal': { potency: 5, note: 'harmful effects removed in all, one every 2 seconds', mana: 6 },
  'Aegis': { potency: 2, note: 'shield on each ally every 10 seconds', mana: 10 },
  'Radiance': { potency: 2, note: 'healing to each ally every 2 seconds', mana: 10 },
  // Lightning. The five original spells keep the numbers the game gives them today.
  "Lightning": { potency: null, note: "damage to everything in its path (the original spell, with the game's own numbers)", mana: 10 },
  "Chain Lightning": { potency: null, note: "damage to every enemy in sight (the original spell, with the game's own numbers)", mana: 30 },
  "Charged Bolt": { potency: null, note: "damage for each bolt that lands (the original spell, with the game's own numbers)", mana: 6 },
  "Teleport": { potency: null, note: "moves you to the spot chosen (the original spell, with the game's own cost)", mana: 35 },
  "Spark": { potency: 5, note: "damage", mana: 4 },
  "Arc": { potency: 5, note: "damage to the first target, a quarter less with each leap", mana: 9 },
  "Forked Lightning": { potency: 7, note: "damage to each of three targets", mana: 9, cooldown: 10 },
  "Thunderstrike": { potency: 9, note: "damage", mana: 6, cooldown: 8 },
  "Ball Lightning": { potency: 14, note: "damage on the hit, then a quarter less with each leap", mana: 14, cooldown: 30 },
  "Tempest": { potency: 30, note: "damage to the first target, a tenth less with each of up to 8 leaps", mana: 36, cooldown: 180 },
  "Shocking Grasp": { potency: 4, note: "bonus damage on the hit; 3 lightning to each of the two nearest other enemies", mana: 3, cooldown: 8 },
  "Storm's Focus": { potency: 10, note: "Spirit, lasting", mana: 10 },
  "Amplify": { potency: 10, note: "% more damage dealt, lasting", mana: 10 },
  "Charged Air": { potency: 5, note: "% more spell attack damage, for you and allies in the aura (not special attacks)", mana: 10 },
  "Overcharge": { potency: 20, note: "% more damage dealt for 20 seconds", mana: 15, cooldown: 60 },
  "Quickening": { potency: 1, note: "frame less for each step you walk (8 frames down to 7, about an eighth faster), lasting", mana: 10 },
  "Dynamo": { potency: 30, note: "% faster mana regeneration, lasting", mana: 10 },
  // Earth. Physical damage reduction is in points off each physical hit, kept very small.
  "Bedrock": { potency: 1, note: "point off every physical hit, for you and allies in the aura", mana: 10 },
  "Stone Spike": { potency: 4, note: "damage to each target", mana: 9 },
  "Tremor": { potency: 9, note: "damage in total over 10 seconds to each enemy standing on it", mana: 9, cooldown: 10 },
  "Earthquake": { potency: 12, note: "damage in total over 20 seconds to each enemy standing on it", mana: 18, cooldown: 45 },
  "Landslide": { potency: 3, note: "damage to each target", mana: 12 },
  "Upheaval": { potency: 24, note: "damage to each target; held for 4 seconds", mana: 36, cooldown: 180 },
  "Stone Fist": { potency: 3, note: "bonus damage on the hit", mana: 2 },
  "Boulder": { potency: 6, note: "damage", mana: 6 },
  "Shockwave": { potency: 2, note: "bonus damage on the hit to each target", mana: 3 },
  "Earthshatter": { potency: 4, note: "bonus damage on the hit; held for 2 seconds", mana: 3, cooldown: 10 },
  "Crushing Blow": { potency: 9, note: "bonus damage on the hit; the enemy takes 15% more damage until it dies", mana: 3, cooldown: 45 },
  "Mire": { potency: 4, note: "seconds held, for each target", mana: 12, cooldown: 40 },
  "Stoneskin": { potency: 2, note: "points off every physical hit you take, lasting", mana: 10 },
  "Granite Guard": { potency: 1, note: "point off every physical hit the ally takes, lasting", mana: 10 },
  "Dig In": { potency: 5, note: "points off every physical hit you take, for 20 seconds", mana: 15, cooldown: 60 },
  "Stone Wall": { potency: 3, note: "points off every physical hit, for every ally, for 15 seconds", mana: 25, cooldown: 60 },
  "Ironhide": { potency: 10, note: "armor, lasting", mana: 10 },
  "Earthen Might": { potency: 10, note: "Power, lasting", mana: 10 },
  "Monolith": { potency: 50, note: "% apparent distance: monsters treat you as half as far away", mana: 10 },
  "Unyielding": { potency: null, note: "hits no longer stagger you, lasting", mana: 10 },
  // Dark. Its damage over time is Corruption: every 2 seconds for 20 seconds, like every such effect.
  "Pall of Shadow": { potency: 10, note: "% less Astral resistance for every enemy in the aura", mana: 10 },
  "Corruption": { potency: 10, note: "Corruption in total over 20 seconds (as built in the game)", mana: 6 },
  "Creeping Rot": { potency: 9, note: "Corruption in total over 20 seconds", mana: 4 },
  "Shadow Bolt": { potency: 3, note: "damage at once, then 4 Corruption over 20 seconds", mana: 6 },
  "Blight": { potency: 7, note: "Corruption in total over 20 seconds to each target", mana: 9 },
  "Plague Wind": { potency: 10, note: "Corruption in total over 20 seconds to each target", mana: 9, cooldown: 10 },
  "Contagion": { potency: 24, note: "Corruption over 20 seconds on the first target, a quarter less with each leap", mana: 22 },
  "Drain": { potency: 5, note: "Corruption in total over 20 seconds; heals you for 5 over the same time", mana: 4 },
  "Black Sun": { potency: 24, note: "damage at once to each target, then 40 Corruption over 20 seconds", mana: 36, cooldown: 180 },
  "Umbral Strike": { potency: 3, note: "bonus damage on the hit, then 4 Corruption over 20 seconds", mana: 2 },
  "Night Blade": { potency: 3, note: "bonus damage on the hit", mana: 1 },
  "Rupture": { potency: 5, note: "bonus damage on the hit, plus half of the Corruption still to tick on the enemy", mana: 2, cooldown: 8 },
  "Blind": { potency: 10, note: "% less chance to hit, until the enemy dies", mana: 6, cooldown: 6 },
  "Murk": { potency: 30, note: "% less chance to hit for 12 seconds, for each target", mana: 18, cooldown: 40 },
  "Night Terrors": { potency: 10, note: "Corruption in total over 20 seconds; 15% less chance to hit while it ticks", mana: 6, cooldown: 10 },
  "Wither": { potency: 5, note: "Corruption in total over 20 seconds; damage over time on the enemy deals 25% more until it dies", mana: 6, cooldown: 12 },
  "Soul Rend": { potency: 100, note: "% of the enemy's Astral resistance and immunity removed, until it dies", mana: 15, cooldown: 45 },
  "Cloak of Night": { potency: 150, note: "% apparent distance: monsters treat you as half again as far away", mana: 10 },
  "Malice": { potency: 10, note: "% more damage from your damage over time, lasting", mana: 10 },
  "Night's Edge": { potency: 2, note: "Corruption added by each weapon hit that lands, lasting", mana: 10 },
}
// What a lasting buff on one person gives, by the stat it changes.
const STAT = {
  'Armor': [10, 'armor'], 'Resistance': [10, '% to all three resistances'], 'Elemental Resist': [20, '% elemental resistance'],
  'Astral Resist': [20, '% astral resistance'], 'Natural Resist': [20, '% natural resistance'], 'HP': [10, '% maximum life'],
  'MP': [10, '% maximum mana'], 'Health Regen': [30, '% faster life regeneration'], 'Mana Regen': [30, '% faster mana regeneration'],
  'Speed': [10, '% faster attacks and casting'], 'Damage': [10, '% more damage dealt'], 'Power': [5, 'Power'], 'Spirit': [5, 'Spirit'],
  'Recovery': [5, 'Recovery'], 'Accuracy': [10, '% chance to hit'], 'Defensive': [10, '% less damage taken'], 'Offensive': [15, '% more damage taken by the enemy'],
}
const AREA = { // [share of the amount each target gets, mana multiplier, words]
  'Single Target': [1, 1, ''], 'Splash': [0.7, 1.5, ' to each target'], 'Blanket': [0.5, 2, ' to each target'],
  'Chain': [0.8, 1.5, ' to the first target, less with each leap'], 'Cone': [0.7, 1.5, ' to each target'],
  'Everyone (allies only)': [0.4, 2.5, ' to each ally'], 'Aura': [1, 1, ''],
}

function numbersFor(power) {
  const names = power.tags.map((id) => data.tags.find((t) => t.id === id)?.name).filter(Boolean)
  const has = (n) => names.includes(n)
  const starts = (n) => names.some((x) => x.startsWith(n))
  const tier = starts('Skirmish') ? 'skirmish' : starts('Battle') ? 'battle' : names.some((x) => /^u[lt]{2}imate/i.test(x)) ? 'ultimate' : 'none'
  const seconds = (re, fallback) => { const m = (power.description ?? '').match(re); return m ? Number(m[1]) : fallback }
  const cooldownText = (power.description ?? '').match(/Cooldown of about (\d+) (second|minute)|Cooldown of about a minute|Cooldown of (\d+) seconds/)
  let cooldown = { none: 0, skirmish: 10, battle: 60, ultimate: 180 }[tier]
  if (cooldownText && tier !== 'none') cooldown = cooldownText[1] ? Number(cooldownText[1]) * (cooldownText[2] === 'minute' ? 60 : 1) : cooldownText[3] ? Number(cooldownText[3]) : 60
  if (power.name === 'Phoenix') cooldown = 900

  const areaName = Object.keys(AREA).find(has) ?? 'Single Target'
  const [share, areaMana, areaWords] = AREA[areaName]
  const attack = has('Attack'), healing = has('Healing'), shielding = has('Shielding'), aura = has('Aura')
  const damagingAura = aura && attack
  const radius = aura ? (damagingAura ? 2 : 8) : areaName === 'Splash' ? 2 : areaName === 'Blanket' ? (power.name === 'Fire Wall' ? null : tier === 'ultimate' ? 6 : 4) : null

  if (FIXED[power.name]) {
    const f = FIXED[power.name]
    return { potency: f.potency, potencyNote: f.note, manaCost: f.mana, cooldown: f.cooldown ?? cooldown, radius }
  }

  const size = SIZE[power.name] ?? 1
  const cdMana = { none: 1, skirmish: 1, battle: 1.5, ultimate: 3 }[tier]
  const cdPotency = { none: 1, skirmish: 1.5, battle: 3, ultimate: 8 }[tier]
  const effMana = has('Efficient') ? 0.6 : has('Inefficient') ? 3 : 1
  const effPotency = has('Efficient') ? 0.9 : has('Inefficient') ? 1.8 : 1
  const overTime = has('Over Time') && !has('Instant') ? 1.3 : 1
  const duration = seconds(/(?:for|over|lasts) about (\d+) seconds/, 8)
  const hybrid = attack && (healing || shielding) ? 0.6 : 1
  const amount = (base, ratio) => round(base * size * ratio * share * cdPotency * effPotency * overTime * hybrid)
  const timeWords = has('Over Time') && !has('Instant') ? ` in total over ${duration} seconds` : ''

  const plainBuff = (has('Buff') || has('Debuff')) && !attack && !shielding && !has('Instant') && (has('Permanent') || has('Timed'))
  if (!plainBuff && (attack || healing || shielding)) {
    const special = has('Special Attack')
    const base = attack ? (special ? 2 : 6) : 8
    const manaCost = round(base * size * areaMana * cdMana * effMana)
    const heal = amount(8, 1.5), shield = amount(8, 1.8), damage = special ? amount(2, 1.5) : amount(6, 1)
    let potency, note
    if (attack) {
      potency = damage
      note = (special ? 'bonus damage on the hit' : 'damage') + timeWords + areaWords
      if (healing) note += `; heals ${heal}`
      if (shielding) note += `; ${shield} shield for each enemy struck`
    } else if (healing) {
      potency = heal
      note = 'healing' + timeWords + areaWords
    } else {
      potency = shield
      note = 'shield of temporary health' + areaWords
    }
    const stat = Object.keys(STAT).find(has)
    if (stat && (has('Buff') || has('Debuff'))) {
      const [value, words] = STAT[stat]
      note += has('Debuff') && stat === 'Speed' ? `; held for ${seconds(/for about (\d+) seconds/, 3)} seconds` : `; also ${has('Timed') ? round(value * 1.5) : value} ${words}`
    }
    if (has('Cleanse')) note += '; also removes harmful effects'
    return { potency, potencyNote: note, manaCost, cooldown, radius }
  }

  if (has('Cleanse') && !plainBuff) {
    const all = /every harmful effect/.test(power.description ?? '')
    return { potency: all ? null : 1, potencyNote: all ? 'removes every harmful effect' + areaWords.replace(' to ', ' from ') : 'harmful effect removed' + areaWords.replace(' to ', ' from '), manaCost: round(4 * areaMana * cdMana * effMana), cooldown, radius }
  }

  if (has('Buff') || has('Debuff')) {
    const stats = Object.keys(STAT).filter(has)
    const stat = stats[0] ?? 'Defensive'
    const [value, words] = STAT[stat]
    const strength = (aura ? 0.5 : 1) * (has('Timed') ? 3 : 1) * (areaName === 'Everyone (allies only)' ? 0.5 : 1)
    const extra = stats.slice(1).map((s) => `${round(STAT[s][0] * strength)} ${STAT[s][1]}`).join(' and ')
    return {
      potency: round(value * strength),
      potencyNote: words + (extra ? ', and ' + extra : '') + (has('Timed') ? ` for ${seconds(/for about (\d+) seconds/, 20)} seconds` : ', lasting') + (areaName === 'Everyone (allies only)' ? ', for every ally' : aura ? ', for allies in the aura' : ''),
      manaCost: aura ? 10 : has('Timed') ? round(12 * cdMana) : 10, cooldown, radius,
    }
  }
  return { potency: null, potencyNote: '', manaCost: null, cooldown, radius }
}

let changed = 0
for (const power of data.powers) {
  if (power.status === 'accepted') continue
  Object.assign(power, numbersFor(power), { growth: power.growth ?? 12.5 })
  changed++
}

const essenceName = (p) => p.essenceIds.map((id) => data.essences.find((e) => e.id === id)?.name).join('+') || 'unassigned'
for (const p of [...data.powers].sort((a, b) => essenceName(a).localeCompare(essenceName(b)) || a.name.localeCompare(b.name)))
  console.log(`${essenceName(p).padEnd(10)} ${p.name.padEnd(18)} mana ${String(p.manaCost ?? '-').padStart(3)}  cd ${String(p.cooldown ?? '-').padStart(4)}  r ${String(p.radius ?? '-').padStart(2)}  ${String(p.potency ?? '-').padStart(4)} ${p.potencyNote}`)

if (process.argv.includes('--check')) {
  console.log(`Check only: ${changed} powers would change.`)
} else {
  const backups = path.join(path.dirname(DATA_FILE), 'backups')
  fs.mkdirSync(backups, { recursive: true })
  fs.copyFileSync(DATA_FILE, path.join(backups, 'essence_data-' + new Date().toISOString().replace(/[:T]/g, '-').slice(0, 19) + '-before-numbers.json'))
  fs.writeFileSync(DATA_FILE, JSON.stringify(data, null, 2) + '\n')
  console.log(`Saved numbers on ${changed} powers.`)
}
