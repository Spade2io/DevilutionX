import { useState } from 'react'
import { AddBox, DeleteButton, NameInput, NumberInput, TextArea, newId, sameName } from './parts.jsx'
import { TINTS, orbStyle } from './palette.js'
import TagInput, { DRAG_TYPE } from './TagInput.jsx'
import { RARITIES, rarityOf } from './rarity.js'
import { essenceSummary } from './rules.js'

// The four stats an essence can be bound to.
const PRIMARY_STATS = ['Power', 'Spirit', 'Recovery', 'Speed']

// Data shape used on this page:
//   essence: { id, name, description, rarity, damageTypeId,   (poolSize is still stored but no longer shown)
//              tagsAll, tagsMost, tagsSome, tagsFinite, tagsNever, neverOtherGroups }
// The four tag lists hold tag ids. A tag sits on at most one of the four; a tag on none of them
// counts as "a few". They say what the essence's powers should lean towards, and are what Claude
// reads when asked to draft powers for the essence.
// neverOtherGroups holds tag group ids. Listing a group there means "every tag in this group that
// is not on the All, Most or Some line is a Never". It follows the group as tags are added to it.

// Makes a group name plural for the "All other ..." entries: Element -> Elements,
// Efficiency -> Efficiencies, Area of Effect -> Areas of Effect.
const pluralWord = (w) =>
  /[^aeiou]y$/i.test(w) ? w.slice(0, -1) + 'ies' : /(s|x|ch|sh)$/i.test(w) ? w + 'es' : w + 's'
const plural = (name) => {
  const words = name.trim().split(' ')
  const of = words.findIndex((w) => w.toLowerCase() === 'of')
  const at = of > 0 ? of - 1 : words.length - 1
  words[at] = pluralWord(words[at])
  return words.join(' ')
}

const TAG_LINES = [
  { field: 'tagsAll', label: 'All', help: 'every power has these' },
  { field: 'tagsMost', label: 'Most', help: 'most powers have these' },
  { field: 'tagsSome', label: 'Some', help: 'a fair share of powers have these' },
  { field: 'tagsFinite', label: 'Finite', help: 'one power has this, two at most' },
  { field: 'tagsNever', label: 'Never', help: 'no power may have these' },
]

export default function EssencesPage({ data, update }) {
  const essences = data.essences ?? []
  const damageTypes = data.damageTypes ?? []
  const [selectedId, setSelectedId] = useState(null)
  // The list runs Common first, then Rare, Epic, Legendary. Within a rarity they run alphabetically.
  const rank = (e) => RARITIES.indexOf(rarityOf(e.rarity))
  const sorted = [...essences].sort((a, b) => rank(a) - rank(b) || a.name.localeCompare(b.name))
  const selected = essences.find((e) => e.id === selectedId) ?? sorted[0] ?? null

  const nameTaken = (name, exceptId) => essences.some((e) => e.id !== exceptId && sameName(e.name, name))

  const add = (name) => {
    const id = newId('e')
    update((d) => ({
      ...d,
      essences: [
        ...(d.essences ?? []),
        { id, name, description: '', rarity: 'common', poolSize: 20, damageTypeId: null, tagsAll: [], tagsMost: [], tagsSome: [], tagsFinite: [], tagsNever: [], neverOtherGroups: [] },
      ],
    }))
    setSelectedId(id)
  }

  const change = (id, fields) =>
    update((d) => ({ ...d, essences: d.essences.map((e) => (e.id === id ? { ...e, ...fields } : e)) }))

  const remove = (id) => update((d) => ({ ...d, essences: d.essences.filter((e) => e.id !== id) }))

  // Putting a tag on one line takes it off the other three.
  const addTag = (essence, field, tagId) => {
    const fields = {}
    for (const line of TAG_LINES) fields[line.field] = (essence[line.field] ?? []).filter((t) => t !== tagId)
    fields[field] = [...fields[field], tagId]
    change(essence.id, fields)
  }

  const removeTag = (essence, field, tagId) => change(essence.id, { [field]: (essence[field] ?? []).filter((t) => t !== tagId) })

  const colorOf = (essence) => {
    const type = damageTypes.find((t) => t.id === essence.damageTypeId)
    return TINTS.find((c) => c.id === type?.itemColor)
  }

  return (
    <>
      <header className="page-head">
        <h2>Essences</h2>
        <AddBox placeholder="New essence name" button="Add essence" onAdd={add}
          check={(name) => (nameTaken(name) ? 'An essence with that name already exists.' : '')} />
      </header>
      {essences.length === 0
        ? <p className="empty">No essences yet. Add one above to get started.</p>
        : (
          <div className="split">
            <ul className="side-list">
              {sorted.map((essence, index) => {
                const color = colorOf(essence)
                const newGroup = index > 0 && rarityOf(essence.rarity).id !== rarityOf(sorted[index - 1].rarity).id
                return (
                  <li key={essence.id} className={newGroup ? 'rarity-break' : undefined}>
                    <button className={essence.id === selected.id ? 'side-item active' : 'side-item'} onClick={() => setSelectedId(essence.id)}>
                      {color ? <span className="orb" style={orbStyle(color)} /> : <span className="orb empty-orb" />}
                      <span>{essence.name}</span>
                    </button>
                  </li>
                )
              })}
            </ul>
            <Editor key={selected.id} essence={selected} data={data} nameTaken={nameTaken} change={change}
              addTag={addTag} removeTag={removeTag} onDelete={() => { remove(selected.id); setSelectedId(null) }} />
            <TagShelf essence={selected} data={data} change={change} />
          </div>
        )}
    </>
  )
}

// Every tag not yet on one of the essence's five lines, laid out by group, so none has to be
// remembered. Drag one onto a line to use it; it then leaves the shelf. Drag a tag from a line
// back onto the shelf to take it off that line. A tag barred by an "All other ..." entry stays
// on the shelf, struck through, because it can still be dragged onto a line to bring it back.
function TagShelf({ essence, data, change }) {
  const [dropping, setDropping] = useState(false)
  const onLine = new Set(TAG_LINES.flatMap((line) => essence[line.field] ?? []))
  const sweptGroups = essence.neverOtherGroups ?? []
  const groups = data.tagGroups
    .map((group) => ({ group, tags: data.tags.filter((t) => t.groupId === group.id && !onLine.has(t.id)) }))
    .filter((entry) => entry.tags.length > 0)
  const takeOff = (e) => {
    setDropping(false)
    const id = e.dataTransfer.getData(DRAG_TYPE)
    if (!id || !onLine.has(id)) return
    e.preventDefault()
    const fields = {}
    for (const line of TAG_LINES) fields[line.field] = (essence[line.field] ?? []).filter((t) => t !== id)
    change(essence.id, fields)
  }
  return (
    <aside className={dropping ? 'tag-shelf dropping' : 'tag-shelf'}
      onDragOver={(e) => { if (e.dataTransfer.types.includes(DRAG_TYPE)) { e.preventDefault(); setDropping(true) } }}
      onDragLeave={(e) => { if (!e.currentTarget.contains(e.relatedTarget)) setDropping(false) }}
      onDrop={takeOff}>
      <p className="note">Tags not on a line yet. Drag one onto a line; drag one back here to take it off.</p>
      {groups.length === 0
        ? <p className="note">Every tag is on a line.</p>
        : (
          <div className="shelf-columns">
            {groups.map(({ group, tags }) => (
              <div key={group.id} className="shelf-group">
                <div className="shelf-title">{group.name}</div>
                {tags.map((tag) => (
                  <span key={tag.id} draggable
                    className={sweptGroups.includes(group.id) ? 'shelf-tag swept' : 'shelf-tag'}
                    title={sweptGroups.includes(group.id) ? 'Barred by "All other ' + plural(group.name) + '". Drag onto a line to bring it back.' : 'Drag onto a line'}
                    onDragStart={(e) => { e.dataTransfer.setData(DRAG_TYPE, tag.id); e.dataTransfer.effectAllowed = 'move' }}>
                    {tag.name}
                  </span>
                ))}
              </div>
            ))}
          </div>
        )}
    </aside>
  )
}

function Editor({ essence, data, nameTaken, change, addTag, removeTag, onDelete }) {
  const damageTypes = data.damageTypes ?? []
  const otherGroups = essence.neverOtherGroups ?? []
  // The Never line can also hold "All other <group>" entries. They are offered in the lookup like
  // tags, with ids that start "others:" so the two kinds can be told apart.
  const OTHERS = 'others:'
  const otherOptions = data.tagGroups.map((g) => ({ id: OTHERS + g.id, name: 'All other ' + plural(g.name), groupId: g.id }))
  const wanted = [...essence.tagsAll, ...essence.tagsMost, ...essence.tagsSome, ...(essence.tagsFinite ?? [])]
  // Every tag that ends up barred: the ones typed on the Never line plus the ones a group entry sweeps in.
  const swept = data.tags.filter((t) => otherGroups.includes(t.groupId) && !wanted.includes(t.id) && !essence.tagsNever.includes(t.id))
  const listed = TAG_LINES.reduce((n, line) => n + (essence[line.field] ?? []).length, 0) + swept.length
  const setOtherGroup = (groupId, on) =>
    change(essence.id, { neverOtherGroups: on ? [...otherGroups.filter((g) => g !== groupId), groupId] : otherGroups.filter((g) => g !== groupId) })
  const summary = essenceSummary(essence, data)
  const percent = (tagId) => Math.round(summary.share(tagId) * 100) + '%'
  return (
    <section className="editor">
      <div className="field-row">
        <label className="field name">
          Name
          <NameInput value={essence.name} check={(name) => (nameTaken(name, essence.id) ? 'taken' : '')}
            onCommit={(name) => change(essence.id, { name })} />
        </label>
        <label className="field">
          Primary
          {/* The stat this essence is bound to when a character takes it, if no other essence holds it. */}
          <select value={essence.primaryStat ?? ''} onChange={(e) => change(essence.id, { primaryStat: e.target.value || null })}>
            <option value="">Not set</option>
            {PRIMARY_STATS.map((stat) => <option key={stat} value={stat}>{stat}</option>)}
          </select>
        </label>
        <label className="field">
          Damage type
          <select value={essence.damageTypeId ?? ''} onChange={(e) => change(essence.id, { damageTypeId: e.target.value || null })}>
            <option value="">None</option>
            {damageTypes.map((t) => <option key={t.id} value={t.id}>{t.name}</option>)}
          </select>
        </label>
        <label className="field">
          Rarity
          <select value={rarityOf(essence.rarity).id} onChange={(e) => change(essence.id, { rarity: e.target.value })}>
            {RARITIES.map((r) => <option key={r.id} value={r.id}>{r.name}</option>)}
          </select>
        </label>
        <label className="field narrow">
          Total Powers
          {/* Counted from the powers that belong to this essence; it is not typed in. */}
          <input value={summary.powers.length} readOnly tabIndex={-1} className="count" title="How many powers are designed for this essence so far, drafts included" />
        </label>
      </div>
      <p className="note">
        Damage type gives the essence its colour. Primary is the stat the essence is bound to when a character takes
        it; each level its powers gain adds a point to that stat. Total Powers counts the powers designed for it so far.
      </p>
      <label className="field">
        Description
        <TextArea value={essence.description} onCommit={(description) => change(essence.id, { description })} />
      </label>

      <h3>Tags its powers lean towards</h3>
      <div className="tag-lines">
        {TAG_LINES.map((line) => (
          <div key={line.field} className={'tag-line ' + line.field}>
            <div className="tag-line-label">
              <strong>{line.label}</strong>
              <span className="note">{line.help}</span>
            </div>
            {line.field === 'tagsNever'
              ? (
                <div className="never-box">
                  <TagInput allTags={[...otherOptions, ...data.tags]} groups={data.tagGroups}
                    value={[...otherGroups.map((g) => OTHERS + g), ...essence.tagsNever]}
                    placeholder='Type part of a tag name, or "all other", then Tab'
                    onAdd={(id) => (id.startsWith(OTHERS) ? setOtherGroup(id.slice(OTHERS.length), true) : addTag(essence, line.field, id))}
                    onRemove={(id) => (id.startsWith(OTHERS) ? setOtherGroup(id.slice(OTHERS.length), false) : removeTag(essence, line.field, id))} />
                  {otherGroups.length > 0 && (
                    <span className="note">
                      Currently sweeps in: {swept.length > 0 ? swept.map((t) => t.name).join(', ') : 'nothing'}
                    </span>
                  )}
                </div>
              )
              : (
                <TagInput allTags={data.tags} groups={data.tagGroups} value={essence[line.field] ?? []}
                  placeholder="Type part of a tag name, then Tab"
                  onAdd={(tagId) => addTag(essence, line.field, tagId)}
                  onRemove={(tagId) => removeTag(essence, line.field, tagId)} />
              )}
          </div>
        ))}
      </div>
      <p className="note">
        The other {data.tags.length - listed} tags are not listed, so they count as "a few": allowed, but rare.
      </p>
      <h3>Its powers so far</h3>
      {summary.powers.length === 0
        ? <p className="note">No powers belong to this essence yet.</p>
        : (
          <>
            <p className="note">
              {summary.powers.length} powers ({summary.drafts} still drafts).{' '}
              <span className={summary.auras === 1 ? '' : 'problem-text'}>
                {summary.auras === 1 ? 'One aura, as intended.' : summary.auras === 0 ? 'No aura yet; every essence needs exactly one.' : summary.auras + ' auras; every essence gets exactly one.'}
              </span>{' '}
              <span className={summary.ultimates >= 1 && summary.ultimates <= 3 ? '' : 'problem-text'}>
                {summary.ultimates === 0 ? 'No Ultimate yet; every essence needs at least one.' : summary.ultimates > 3 ? summary.ultimates + ' Ultimates; three is the most, and even that should be rare.' : summary.ultimates === 1 ? 'One Ultimate.' : summary.ultimates + ' Ultimates.'}
              </span>
            </p>
            {(essence.tagsFinite ?? []).length > 0 && (
              <div className="shares">
                {essence.tagsFinite.map((tagId) => {
                  const count = summary.powers.filter((p) => (p.tags ?? []).includes(tagId)).length
                  return (
                    <span key={tagId} className={count >= 1 && count <= 2 ? 'share' : 'share problem-text'}>
                      {data.tags.find((t) => t.id === tagId)?.name} <strong>{count}</strong> <span className="note">(Finite: 1 or 2)</span>
                    </span>
                  )
                })}
              </div>
            )}
            <div className="shares">
              {[['tagsAll', 'All', '100%'], ['tagsMost', 'Most', 'about 60%'], ['tagsSome', 'Some', 'about 30%']].map(([field, label, target]) =>
                essence[field].map((tagId) => (
                  <span key={tagId} className="share">
                    {data.tags.find((t) => t.id === tagId)?.name} <strong>{percent(tagId)}</strong> <span className="note">({label}: {target})</span>
                  </span>
                )))}
            </div>
            <ul className="power-summary">
              {[...summary.powers].sort((x, y) => x.name.localeCompare(y.name)).map((power) => (
                <li key={power.id}>
                  <strong>{power.name}</strong>
                  {power.status !== 'accepted' && <span className="badge">Draft</span>}
                  <span className="tag-text">{(power.tags ?? []).map((id) => data.tags.find((t) => t.id === id)?.name).filter(Boolean).join(' · ')}</span>
                  <span className="power-summary-text">{power.description}</span>
                  <span className="power-summary-text note">
                    {[power.potency != null ? power.potency + ' ' + (power.potencyNote ?? '') : power.potencyNote, power.manaCost != null && power.manaCost + ' mana', power.cooldown ? power.cooldown + 's cooldown' : null, power.radius != null && 'radius ' + power.radius].filter(Boolean).join(' · ')}
                  </span>
                </li>
              ))}
            </ul>
          </>
        )}
      <DeleteButton name={essence.name} label="Delete essence" onDelete={onDelete} />
    </section>
  )
}
