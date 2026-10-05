// Adds drafted powers to the designer's data file.
// Usage: node tools/add-drafts.mjs <essence name> <file of powers.json> [--replace] [--check]
// Use the essence name "none" to add powers that belong to no essence.
// Each power in the file is { name, tags: [tag names], description, radius? }.
// Every tag name must already exist. Powers arrive as drafts, belonging to the named essence,
// and each one is run through the same rule checks the designer shows.
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { checkPower, essenceSummary } from '../src/rules.js'

const here = path.dirname(fileURLToPath(import.meta.url))
const DATA_FILE = path.resolve(here, '../../../designer/essence_data.json')
const [essenceName, powersFile] = process.argv.slice(2)

const data = JSON.parse(fs.readFileSync(DATA_FILE, 'utf8'))
data.powers ??= []
const unassigned = essenceName.toLowerCase() === 'none'
const essence = unassigned ? null : data.essences.find((e) => e.name.toLowerCase() === essenceName.toLowerCase())
if (!essence && !unassigned) throw new Error('No essence called ' + essenceName)
// A tag is named in full, or by the start of its name when that is unambiguous
// ("Battle" finds "Battle (<60)").
const tagId = (name) => {
  const wanted = name.toLowerCase()
  const exact = data.tags.find((t) => t.name.toLowerCase() === wanted)
  if (exact) return exact.id
  // "Ultimate" also finds that tag while it is still spelled "Utlimate".
  const starts = data.tags.filter((t) => t.name.toLowerCase().startsWith(wanted) || (wanted === 'ultimate' && /^u[lt]{2}imate/i.test(t.name)))
  if (starts.length !== 1) throw new Error((starts.length ? 'More than one tag starts with ' : 'No tag called ') + name)
  return starts[0].id
}

// --replace: first remove this essence's existing drafts. Accepted powers are never removed.
if (process.argv.includes('--replace') && essence) {
  const before = data.powers.length
  data.powers = data.powers.filter((p) => !(p.status !== 'accepted' && (p.essenceIds ?? []).includes(essence.id)))
  console.log('Removed ' + (before - data.powers.length) + ' existing drafts from ' + essence.name)
}

let problems = 0
for (const draft of JSON.parse(fs.readFileSync(powersFile, 'utf8'))) {
  if (data.powers.some((p) => p.name.toLowerCase() === draft.name.toLowerCase())) throw new Error('Already a power called ' + draft.name)
  const power = {
    id: 'p_' + crypto.randomUUID().slice(0, 8),
    name: draft.name,
    description: draft.description,
    status: 'draft',
    essenceIds: essence ? [essence.id] : [],
    tags: draft.tags.map(tagId),
    potency: null,
    manaCost: null,
    cooldown: null,
    radius: draft.radius ?? null,
    growth: 12.5,
  }
  const warnings = checkPower(power, data)
  problems += warnings.length
  for (const w of warnings) console.log('WARNING', power.name + ':', w)
  data.powers.push(power)
}

if (!essence) console.log(`Unassigned: ${problems} warnings`)
const summary = essence ? essenceSummary(essence, data) : { powers: [], auras: 0, share: () => 0 }
const name = (id) => data.tags.find((t) => t.id === id)?.name
if (essence) console.log(`${essence.name}: ${summary.powers.length} powers, ${summary.auras} aura(s), ${summary.ultimates} ultimate(s), ${problems} warnings`)
for (const [field, label] of [['tagsAll', 'All'], ['tagsMost', 'Most'], ['tagsSome', 'Some']])
  for (const id of essence ? essence[field] : []) console.log(`  ${label.padEnd(4)} ${name(id).padEnd(16)} ${Math.round(summary.share(id) * 100)}%`)
const counts = {}
for (const p of summary.powers) for (const id of p.tags) counts[name(id)] = (counts[name(id)] ?? 0) + 1
for (const id of essence?.tagsFinite ?? []) {
  const count = summary.powers.filter((p) => p.tags.includes(id)).length
  console.log('  Finite ' + name(id).padEnd(14) + ' ' + count + (count >= 1 && count <= 2 ? '' : '   <-- should be 1 or 2'))
}
if (essence) console.log('  every tag:', Object.entries(counts).sort((a, b) => b[1] - a[1]).map(([n, c]) => `${n} ${c}`).join(', '))

if (process.argv.includes('--check')) {
  console.log('Check only: nothing was saved.')
} else {
  // Keep a safety copy of the file as it was before these drafts went in.
  const backups = path.join(path.dirname(DATA_FILE), 'backups')
  fs.mkdirSync(backups, { recursive: true })
  const stamp = new Date().toISOString().replace(/[:T]/g, '-').slice(0, 19)
  fs.copyFileSync(DATA_FILE, path.join(backups, `essence_data-${stamp}-before-drafts.json`))
  fs.writeFileSync(DATA_FILE, JSON.stringify(data, null, 2) + '\n')
  console.log('Saved.')
}
