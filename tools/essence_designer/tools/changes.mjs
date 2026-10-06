// A change log of what has been edited in the Essence Designer since the game was last brought
// up to date with it.
//
// Usage: node tools/changes.mjs            compares with the snapshot kept at the last export
//        node tools/changes.mjs --git      compares with the last commit instead
//
// Every time export-game.mjs writes the game's files it saves a copy of the designer's data
// (designer/last_export.json). This script compares the designer's data now with that copy and
// lists what was added, removed or edited. Each line says whether the next export carries the
// change into the game by itself, or whether it needs doing by hand (marked NEEDS WORK).
//
// What the export handles by itself: a power's mana cost, cooldown, potency and radius, and its
// Ally, Enemy, Everyone and Weapon tags. Everything about what a power *does* is written by hand
// in export-game.mjs (the BEHAVIOUR, ICONS and SHORT_TEXT lists, all keyed by the power's name),
// and an essence's damage type and colours are typed into the game's source.
import fs from 'node:fs'
import path from 'node:path'
import { execFileSync } from 'node:child_process'
import { fileURLToPath } from 'node:url'

const here = path.dirname(fileURLToPath(import.meta.url))
const REPO = path.resolve(here, '../../..')
export const DATA_FILE = path.join(REPO, 'designer/essence_data.json')
export const SNAPSHOT_FILE = path.join(REPO, 'designer/last_export.json')

/** Tags the export reads straight off a power. A change to any other tag needs a look. */
const isHandledTag = (name, group) => ['Ally', 'Enemy'].includes(name) || name.startsWith('Everyone') || group === 'Weapon'

/**
 * The names of the powers that already have behaviour written in export-game.mjs, read from its
 * BEHAVIOUR and ALREADY_IN_GAME lists. (Read as text, because importing that script runs it.)
 */
export function builtPowerNames() {
  const text = fs.readFileSync(path.join(here, 'export-game.mjs'), 'utf8')
  const names = new Set()
  for (const list of ['BEHAVIOUR', 'ALREADY_IN_GAME']) {
    const start = text.indexOf('const ' + list + ' = {')
    if (start < 0) continue
    const lines = text.slice(start).split(/\r?\n/)
    // The list ends at the first line that is only a closing brace.
    const body = lines.slice(0, lines.findIndex((line) => line === '}')).join(' ')
    // Each entry starts with its name in quotes, followed by a colon. A name in double quotes is
    // one that contains an apostrophe.
    for (const match of body.matchAll(/'([^']+)':|"([^"]+)":/g)) names.add(match[1] ?? match[2])
  }
  return names
}

const byId = (list) => new Map((list ?? []).map((x) => [x.id, x]))
const same = (a, b) => JSON.stringify(a ?? null) === JSON.stringify(b ?? null)

/**
 * Compares two copies of the designer's data.
 * @returns a list of { area, name, text, needsWork } lines, in a sensible reading order.
 */
export function listChanges(before, after) {
  const built = builtPowerNames()
  const lines = []
  const add = (area, name, text, needsWork = false) => lines.push({ area, name, text, needsWork })
  const tagName = (data, id) => data.tags.find((t) => t.id === id)?.name ?? '(deleted tag)'
  const groupOf = (data, id) => data.tagGroups.find((g) => g.id === data.tags.find((t) => t.id === id)?.groupId)?.name ?? ''
  const essenceName = (data, id) => data.essences.find((e) => e.id === id)?.name ?? '(deleted essence)'
  const typeName = (data, id) => data.damageTypes.find((t) => t.id === id)?.name ?? 'none'

  // --- Powers ---
  const oldPowers = byId(before.powers), newPowers = byId(after.powers)
  for (const [id, power] of newPowers) {
    const old = oldPowers.get(id)
    const where = power.essenceIds?.length ? power.essenceIds.map((e) => essenceName(after, e)).join('/') : 'no essence'
    if (!old) {
      if (built.has(power.name)) add('Power', power.name, `new (${where}, ${power.status}). Its behaviour is already written.`)
      else add('Power', power.name, `new (${where}, ${power.status}). It has no behaviour in the game until one is written.`, true)
      continue
    }
    if (old.name !== power.name)
      add('Power', power.name, `renamed from "${old.name}". Its behaviour, icon and spellbook line are looked up by name, so they must be renamed in export-game.mjs too.`, true)
    if (old.status !== power.status) add('Power', power.name, `status ${old.status} -> ${power.status}.`)
    if (!same(old.essenceIds, power.essenceIds))
      add('Power', power.name, `moved from ${old.essenceIds?.map((e) => essenceName(before, e)).join('/') || 'no essence'} to ${where}. Its essence, colour and damage type in the game follow the essence.`, true)
    for (const [field, label] of [['manaCost', 'mana cost'], ['cooldown', 'cooldown'], ['potency', 'potency'], ['radius', 'radius']]) {
      if (!same(old[field], power[field])) add('Power', power.name, `${label} ${old[field] ?? '-'} -> ${power[field] ?? '-'}. Carried over by the export.`)
    }
    if (old.description !== power.description)
      add('Power', power.name, 'description rewritten. Check whether what the power does has changed; the game keeps its old behaviour until told otherwise.', true)
    if ((old.potencyNote ?? '') !== (power.potencyNote ?? ''))
      add('Power', power.name, `note changed to "${power.potencyNote ?? ''}". Second numbers in a note (a rider's amount, a duration) are typed by hand in export-game.mjs.`, true)
    const gone = (old.tags ?? []).filter((t) => !(power.tags ?? []).includes(t))
    const came = (power.tags ?? []).filter((t) => !(old.tags ?? []).includes(t))
    if (gone.length || came.length) {
      const handled = [...gone.map((t) => [tagName(before, t), groupOf(before, t)]), ...came.map((t) => [tagName(after, t), groupOf(after, t)])].every(([n, g]) => isHandledTag(n, g))
      const text = [came.length ? 'tags added: ' + came.map((t) => tagName(after, t)).join(', ') : '', gone.length ? 'tags removed: ' + gone.map((t) => tagName(before, t)).join(', ') : ''].filter(Boolean).join('; ')
      add('Power', power.name, text + (handled ? '. Carried over by the export.' : '. These change what the power is; its behaviour may need rewriting.'), !handled)
    }
  }
  for (const [id, old] of oldPowers) {
    if (!newPowers.has(id))
      add('Power', old.name, old.number != null
        ? `deleted. It is in the game as number ${old.number}; its number and stone stay there, since saved characters may refer to them. Decide what becomes of it.`
        : 'deleted. It was never in the game, so nothing else is needed.', old.number != null)
  }

  // --- Essences ---
  const oldEssences = byId(before.essences), newEssences = byId(after.essences)
  const LINES = [['tagsAll', 'All'], ['tagsMost', 'Most'], ['tagsSome', 'Some'], ['tagsFinite', 'Finite'], ['tagsNever', 'Never']]
  for (const [id, essence] of newEssences) {
    const old = oldEssences.get(id)
    if (!old) { add('Essence', essence.name, 'new. It is not in the game until its powers are drafted and built.'); continue }
    if (old.name !== essence.name) add('Essence', essence.name, `renamed from "${old.name}". The game knows it by the old name.`, true)
    if (old.damageTypeId !== essence.damageTypeId)
      add('Essence', essence.name, `damage type ${typeName(before, old.damageTypeId)} -> ${typeName(after, essence.damageTypeId)}. Its damage type and item colour are typed into the game.`, true)
    if (old.rarity !== essence.rarity) add('Essence', essence.name, `rarity ${old.rarity} -> ${essence.rarity}. Nothing in the game uses rarity yet.`)
    if (old.poolSize !== essence.poolSize) add('Essence', essence.name, `pool size ${old.poolSize} -> ${essence.poolSize}.`)
    if ((old.description ?? '') !== (essence.description ?? '')) add('Essence', essence.name, 'description changed.')
    for (const [field, label] of LINES) {
      const gone = (old[field] ?? []).filter((t) => !(essence[field] ?? []).includes(t))
      const came = (essence[field] ?? []).filter((t) => !(old[field] ?? []).includes(t))
      if (gone.length || came.length)
        add('Essence', essence.name, `${label} line: ${[came.length ? '+ ' + came.map((t) => tagName(after, t)).join(', ') : '', gone.length ? '- ' + gone.map((t) => tagName(before, t)).join(', ') : ''].filter(Boolean).join('  ')}. This guides drafting; powers already made are not changed by it.`)
    }
    if (!same(old.neverOtherGroups, essence.neverOtherGroups)) add('Essence', essence.name, '"All other ..." entries on the Never line changed. This guides drafting only.')
  }
  for (const [id, old] of oldEssences) if (!newEssences.has(id)) add('Essence', old.name, 'deleted. If it is in the game, it and its powers are still there.', true)

  // --- Damage types ---
  const oldTypes = byId(before.damageTypes), newTypes = byId(after.damageTypes)
  for (const [id, type] of newTypes) {
    const old = oldTypes.get(id)
    if (!old) { add('Damage type', type.name, 'new. The game does not have it until it is added there.', true); continue }
    if (old.name !== type.name) add('Damage type', type.name, `renamed from "${old.name}".`, true)
    for (const [field, label] of [['category', 'resistance category'], ['itemColor', 'item colour'], ['textColor', 'text colour'], ['effectColor', 'effect colour']]) {
      if (old[field] !== type[field]) add('Damage type', type.name, `${label} ${old[field]} -> ${type[field]}. Colours and categories are typed into the game.`, true)
    }
  }
  for (const [id, old] of oldTypes) if (!newTypes.has(id)) add('Damage type', old.name, 'deleted.', true)

  // --- Tags and groups ---
  const oldTags = byId(before.tags), newTags = byId(after.tags)
  for (const [id, tag] of newTags) {
    const old = oldTags.get(id)
    if (!old) add('Tag', tag.name, `new, in ${groupOf(after, id) || 'no group'}.` + (groupOf(after, id) === 'Stat Effect' ? ' The game has to be taught what this stat does before a power can use it.' : ''), groupOf(after, id) === 'Stat Effect')
    else if (old.name !== tag.name) add('Tag', tag.name, `renamed from "${old.name}".`)
    else if (old.groupId !== tag.groupId) add('Tag', tag.name, `moved to ${groupOf(after, id)}.`)
  }
  for (const [id, old] of oldTags) if (!newTags.has(id)) add('Tag', old.name, 'deleted.')
  const oldGroups = byId(before.tagGroups), newGroups = byId(after.tagGroups)
  for (const [id, group] of newGroups) {
    const old = oldGroups.get(id)
    if (!old) add('Tag group', group.name, 'new.')
    else if (old.name !== group.name) add('Tag group', group.name, `renamed from "${old.name}".`)
  }
  for (const [id, old] of oldGroups) if (!newGroups.has(id)) add('Tag group', old.name, 'deleted.')

  return lines
}

/** Prints the change log. Returns how many lines need work by hand. */
export function printChanges(before, after, sinceWhat) {
  const lines = listChanges(before, after)
  if (lines.length === 0) {
    console.log(`Designer: nothing has changed since ${sinceWhat}.`)
    return 0
  }
  const needing = lines.filter((l) => l.needsWork).length
  console.log(`Designer: ${lines.length} change${lines.length === 1 ? '' : 's'} since ${sinceWhat}; ${needing} need${needing === 1 ? 's' : ''} work by hand.`)
  for (const area of ['Power', 'Essence', 'Damage type', 'Tag', 'Tag group']) {
    const mine = lines.filter((l) => l.area === area)
    if (mine.length === 0) continue
    console.log(`\n${area}s`)
    for (const line of mine) console.log(`  ${line.needsWork ? 'NEEDS WORK' : '          '}  ${line.name}: ${line.text}`)
  }
  return needing
}

/** The copy of the designer's data kept at the last export, or null if there has never been one. */
export const readSnapshot = () => (fs.existsSync(SNAPSHOT_FILE) ? JSON.parse(fs.readFileSync(SNAPSHOT_FILE, 'utf8')) : null)

// Run directly: print the change log.
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const after = JSON.parse(fs.readFileSync(DATA_FILE, 'utf8'))
  if (process.argv.includes('--git')) {
    const committed = JSON.parse(execFileSync('git', ['show', 'HEAD:designer/essence_data.json'], { cwd: REPO, maxBuffer: 64 * 1024 * 1024 }).toString())
    printChanges(committed, after, 'the last commit')
  } else {
    const snapshot = readSnapshot()
    if (!snapshot) console.log('There is no snapshot yet. One is saved the next time export-game.mjs writes the game\'s files.')
    else printChanges(snapshot, after, 'the last export')
  }
}
