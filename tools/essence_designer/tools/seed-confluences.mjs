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
// Two confluences may share a name when it suits both (Bryan, 2026-10-10): Eden, Oasis, Hearth.
const NAMES = {
  // Added 2026-10-10 with Life, Magic and Might.
  'Dark+Earth+Life': 'Marrow',
  'Dark+Fire+Life': 'Cauldron',
  'Dark+Holy+Life': 'Purgatory',
  'Dark+Life+Lightning': 'Reanimator',
  'Dark+Life+Nature': 'Undergrowth',
  'Dark+Life+Shield': 'Revenant',
  'Dark+Life+Water': 'Leech',
  'Earth+Fire+Life': 'Crucible',
  'Earth+Holy+Life': 'Eden',
  'Earth+Life+Lightning': 'Golem',
  'Earth+Life+Nature': 'Grove',
  'Earth+Life+Shield': 'Titan',
  'Earth+Life+Water': 'Oasis',
  'Fire+Holy+Life': 'Rebirth',
  'Fire+Life+Lightning': 'Zeal',
  'Fire+Life+Nature': 'Summer',
  'Fire+Life+Shield': 'Hearth',
  'Fire+Life+Water': 'Hotspring',
  'Holy+Life+Lightning': 'Seraph',
  'Holy+Life+Nature': 'Paradise',
  'Holy+Life+Shield': 'Paladin',
  'Holy+Life+Water': 'Baptism',
  'Life+Lightning+Nature': 'Wildheart',
  'Life+Lightning+Shield': 'Stormheart',
  'Life+Lightning+Water': 'Lifestream',
  'Life+Nature+Shield': 'Evergreen',
  'Life+Nature+Water': 'Verdance',
  'Life+Shield+Water': 'Haven',
  'Dark+Earth+Magic': 'Necropolis',
  'Dark+Fire+Magic': 'Warlock',
  'Dark+Holy+Magic': 'Heresy',
  'Dark+Lightning+Magic': 'Hex',
  'Dark+Magic+Nature': 'Coven',
  'Dark+Magic+Shield': 'Shroud',
  'Dark+Magic+Water': 'Illusion',
  'Earth+Fire+Magic': 'Alchemy',
  'Earth+Holy+Magic': 'Runestone',
  'Earth+Lightning+Magic': 'Lodestone',
  'Earth+Magic+Nature': 'Druid',
  'Earth+Magic+Shield': 'Obelisk',
  'Earth+Magic+Water': 'Crystal',
  'Fire+Holy+Magic': 'Sunfire',
  'Fire+Lightning+Magic': 'Evoker',
  'Fire+Magic+Nature': 'Shaman',
  'Fire+Magic+Shield': 'Battlemage',
  'Fire+Magic+Water': 'Elementalist',
  'Holy+Lightning+Magic': 'Oracle',
  'Holy+Magic+Nature': 'Mystic',
  'Holy+Magic+Shield': 'Sigil',
  'Holy+Magic+Water': 'Seer',
  'Lightning+Magic+Nature': 'Stormcaller',
  'Lightning+Magic+Shield': 'Conduit',
  'Lightning+Magic+Water': 'Squall',
  'Magic+Nature+Shield': 'Enchanter',
  'Magic+Nature+Water': 'Fey',
  'Magic+Shield+Water': 'Mirror',
  'Dark+Earth+Might': 'Ravager',
  'Dark+Fire+Might': 'Destroyer',
  'Dark+Holy+Might': 'Executioner',
  'Dark+Lightning+Might': 'Reaver',
  'Dark+Might+Nature': 'Hunter',
  'Dark+Might+Shield': 'Blackguard',
  'Dark+Might+Water': 'Corsair',
  'Earth+Fire+Might': 'Hammer',
  'Earth+Holy+Might': 'Champion',
  'Earth+Lightning+Might': 'Earthshaker',
  'Earth+Might+Nature': 'Savage',
  'Earth+Might+Shield': 'Colossus',
  'Earth+Might+Water': 'Glacier',
  'Fire+Holy+Might': 'Crusader',
  'Fire+Lightning+Might': 'Berserker',
  'Fire+Might+Nature': 'Barbarian',
  'Fire+Might+Shield': 'Legionnaire',
  'Fire+Might+Water': 'Steel',
  'Holy+Lightning+Might': 'Valkyrie',
  'Holy+Might+Nature': 'Avenger',
  'Holy+Might+Shield': 'Templar',
  'Holy+Might+Water': 'Knight',
  'Lightning+Might+Nature': 'Stampede',
  'Lightning+Might+Shield': 'Ironclad',
  'Lightning+Might+Water': 'Typhoon',
  'Might+Nature+Shield': 'Protector',
  'Might+Nature+Water': 'Rapids',
  'Might+Shield+Water': 'Breakwater',
  'Dark+Life+Magic': 'Lich',
  'Earth+Life+Magic': 'Geomancer',
  'Fire+Life+Magic': 'Elixir',
  'Holy+Life+Magic': 'Divinity',
  'Life+Lightning+Magic': 'Animus',
  'Life+Magic+Nature': 'Faerie',
  'Life+Magic+Shield': 'Wardstone',
  'Life+Magic+Water': 'Fountain',
  'Dark+Life+Might': 'Vampire',
  'Earth+Life+Might': 'Giant',
  'Fire+Life+Might': 'Gladiator',
  'Holy+Life+Might': 'Hero',
  'Life+Lightning+Might': 'Adrenaline',
  'Life+Might+Nature': 'Beast',
  'Life+Might+Shield': 'Fortress',
  'Life+Might+Water': 'Endurance',
  'Dark+Magic+Might': 'Death Knight',
  'Earth+Magic+Might': 'Runesmith',
  'Fire+Magic+Might': 'Flameblade',
  'Holy+Magic+Might': 'Justicar',
  'Lightning+Magic+Might': 'Stormblade',
  'Magic+Might+Nature': 'Totem',
  'Magic+Might+Shield': 'Spellsword',
  'Magic+Might+Water': 'Frostblade',
  'Life+Magic+Might': 'Paragon',
  // The first eight essences.
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
// A confluence may not be called what an essence or a power is called. Another confluence's name is allowed.
const taken = new Set([...essences.map((e) => e.name), ...(data.powers ?? []).map((p) => p.name)].map((n) => n.toLowerCase()))

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
      confluences.push({ essences: ids, name })
      added++
    }

data.confluences = confluences
fs.writeFileSync(dataPath, JSON.stringify(data, null, 2) + '\n')
console.log(`Named ${added} confluences; ${confluences.length} in the file.`)
if (missing.length) console.log('Left without a name:\n  ' + missing.join('\n  '))
