// Gives a first name to every confluence (every set of three essences) that has none yet.
// Names already in the data file are never touched, so this is safe to run again after editing.
//   node tools/seed-confluences.mjs
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const here = path.dirname(fileURLToPath(import.meta.url))
const dataPath = path.resolve(here, '../../../designer/essence_data.json')
const data = JSON.parse(fs.readFileSync(dataPath, 'utf8'))

// Essence names in alphabetical order, joined with "+".
const NAMES = {
  'Fire+Holy+Nature': 'Dawn',
  'Fire+Holy+Water': 'Redemption',
  'Earth+Fire+Holy': 'Forge',
  'Dark+Fire+Holy': 'Reckoning',
  'Fire+Holy+Lightning': 'Wrath',
  'Fire+Holy+Shield': 'Crusader',
  'Fire+Nature+Water': 'Seasons',
  'Earth+Fire+Nature': 'Primal',
  'Dark+Fire+Nature': 'Pyre',
  'Fire+Lightning+Nature': 'Firestorm',
  'Fire+Nature+Shield': 'Hearth',
  'Earth+Fire+Water': 'Geyser',
  'Dark+Fire+Water': 'Veil',
  'Fire+Lightning+Water': 'Maelstrom',
  'Fire+Shield+Water': 'Equilibrium',
  'Dark+Earth+Fire': 'Brimstone',
  'Earth+Fire+Lightning': 'Cataclysm',
  'Earth+Fire+Shield': 'Anvil',
  'Dark+Fire+Lightning': 'Havoc',
  'Dark+Fire+Shield': 'Tyrant',
  'Fire+Lightning+Shield': 'Vanguard',
  'Holy+Nature+Water': 'Sanctuary',
  'Earth+Holy+Nature': 'Eden',
  'Dark+Holy+Nature': 'Twilight',
  'Holy+Lightning+Nature': 'Firmament',
  'Holy+Nature+Shield': 'Warden',
  'Earth+Holy+Water': 'Covenant',
  'Dark+Holy+Water': 'Requiem',
  'Holy+Lightning+Water': 'Deluge',
  'Holy+Shield+Water': 'Mercy',
  'Dark+Earth+Holy': 'Reliquary',
  'Earth+Holy+Lightning': 'Ascension',
  'Earth+Holy+Shield': 'Bastion',
  'Dark+Holy+Lightning': 'Revelation',
  'Dark+Holy+Shield': 'Inquisitor',
  'Holy+Lightning+Shield': 'Templar',
  'Earth+Nature+Water': 'Oasis',
  'Dark+Nature+Water': 'Quagmire',
  'Lightning+Nature+Water': 'Thunderhead',
  'Nature+Shield+Water': 'Haven',
  'Dark+Earth+Nature': 'Barrow',
  'Earth+Lightning+Nature': 'Savage',
  'Earth+Nature+Shield': 'Stronghold',
  'Dark+Lightning+Nature': 'Predator',
  'Dark+Nature+Shield': 'Bramble',
  'Lightning+Nature+Shield': 'Sentinel',
  'Dark+Earth+Water': 'Abyss',
  'Earth+Lightning+Water': 'Torrent',
  'Earth+Shield+Water': 'Rampart',
  'Dark+Lightning+Water': 'Leviathan',
  'Dark+Shield+Water': 'Undertow',
  'Lightning+Shield+Water': 'Breakwater',
  'Dark+Earth+Lightning': 'Ruin',
  'Dark+Earth+Shield': 'Tomb',
  'Earth+Lightning+Shield': 'Colossus',
  'Dark+Lightning+Shield': 'Dreadnought',
}

const key = (ids) => [...ids].sort().join('+')
const essences = data.essences ?? []
const confluences = data.confluences ?? []
const named = new Set(confluences.filter((c) => c.name).map((c) => key(c.essences)))
const taken = new Set([...confluences.map((c) => c.name), ...essences.map((e) => e.name), ...(data.powers ?? []).map((p) => p.name)].map((n) => n.toLowerCase()))

let added = 0
const missing = []
for (let a = 0; a < essences.length; a++)
  for (let b = a + 1; b < essences.length; b++)
    for (let c = b + 1; c < essences.length; c++) {
      const trio = [essences[a], essences[b], essences[c]]
      const ids = trio.map((e) => e.id).sort()
      if (named.has(key(ids))) continue
      const name = NAMES[trio.map((e) => e.name).sort().join('+')]
      if (!name) { missing.push(trio.map((e) => e.name).join(' + ')); continue }
      if (taken.has(name.toLowerCase())) { missing.push(`${trio.map((e) => e.name).join(' + ')}: "${name}" is already used`); continue }
      taken.add(name.toLowerCase())
      confluences.push({ essences: ids, name })
      added++
    }

data.confluences = confluences
fs.writeFileSync(dataPath, JSON.stringify(data, null, 2) + '\n')
console.log(`Named ${added} confluences; ${confluences.length} in the file.`)
if (missing.length) console.log('Left without a name:\n  ' + missing.join('\n  '))
