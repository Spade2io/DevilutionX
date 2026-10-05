import { useState } from 'react'
import { AddBox, DeleteButton, NameInput, NumberInput, TextArea, newId, sameName } from './parts.jsx'
import TagInput from './TagInput.jsx'
import { checkPower } from './rules.js'

// Data shape used on this page:
//   power: { id, name, description, status, essenceIds, tags,
//            potency, potencyNote, manaCost, cooldown, radius, growth }
// status is 'draft' (written by Claude, or just added) or 'accepted' (Bryan has approved it).
// essenceIds lists every essence the power belongs to; a power can sit in several.
// tags is the full tag list. Target and area come from the tags, not from separate fields.
// The numbers may be blank and are for power level 1. potencyNote says what the potency number
// measures ("damage to each target", "% maximum life, lasting"). growth is the percent the
// potency rises per level.

const NUMBERS = [
  { field: 'potency', label: 'Potency', help: 'damage or healing amount' },
  { field: 'manaCost', label: 'Mana cost' },
  { field: 'cooldown', label: 'Cooldown (s)', decimals: true },
  { field: 'radius', label: 'Radius', help: 'tiles; for areas and auras' },
  { field: 'growth', label: 'Growth % / level', decimals: true },
]

export default function PowersPage({ data, update }) {
  const powers = data.powers ?? []
  const essences = data.essences ?? []
  const [selectedId, setSelectedId] = useState(null)
  const [search, setSearch] = useState('')
  const [essenceFilter, setEssenceFilter] = useState('')
  const [statusFilter, setStatusFilter] = useState('')
  const [tagFilter, setTagFilter] = useState([])
  const [sort, setSort] = useState({ key: 'name', down: false })

  const nameTaken = (name, exceptId) => powers.some((p) => p.id !== exceptId && sameName(p.name, name))
  const essenceNames = (power) => (power.essenceIds ?? []).map((id) => essences.find((e) => e.id === id)?.name).filter(Boolean)
  const tagNames = (power) => (power.tags ?? []).map((id) => data.tags.find((t) => t.id === id)?.name).filter(Boolean)

  const add = (name) => {
    const id = newId('p')
    update((d) => ({
      ...d,
      powers: [
        ...(d.powers ?? []),
        { id, name, description: '', status: 'draft', essenceIds: essenceFilter ? [essenceFilter] : [], tags: [], potency: null, manaCost: null, cooldown: null, radius: null, growth: 12.5 },
      ],
    }))
    setSelectedId(id)
  }

  const change = (id, fields) =>
    update((d) => ({ ...d, powers: d.powers.map((p) => (p.id === id ? { ...p, ...fields } : p)) }))

  const remove = (id) => update((d) => ({ ...d, powers: d.powers.filter((p) => p.id !== id) }))

  const words = search.trim().toLowerCase()
  const shown = powers
    .filter((p) => !words || p.name.toLowerCase().includes(words) || (p.description ?? '').toLowerCase().includes(words))
    .filter((p) => !essenceFilter || (essenceFilter === 'none' ? (p.essenceIds ?? []).length === 0 : (p.essenceIds ?? []).includes(essenceFilter)))
    .filter((p) => !statusFilter || (p.status ?? 'draft') === statusFilter)
    .filter((p) => tagFilter.every((id) => (p.tags ?? []).includes(id)))
  const sortValue = {
    name: (p) => p.name.toLowerCase(),
    essence: (p) => essenceNames(p).join(', ').toLowerCase(),
    status: (p) => p.status ?? 'draft',
    checks: (p) => String(999 - checkPower(p, data).length),
  }[sort.key]
  shown.sort((a, b) => sortValue(a).localeCompare(sortValue(b)) * (sort.down ? -1 : 1) || a.name.localeCompare(b.name))

  const selected = powers.find((p) => p.id === selectedId) ?? null
  const header = (key, label) => (
    <th>
      <button className="sort" onClick={() => setSort({ key, down: sort.key === key ? !sort.down : false })}>
        {label}{sort.key === key ? (sort.down ? ' ▾' : ' ▴') : ''}
      </button>
    </th>
  )

  return (
    <>
      <header className="page-head">
        <h2>Powers</h2>
        <AddBox placeholder="New power name" button="Add power" onAdd={add}
          check={(name) => (nameTaken(name) ? 'A power with that name already exists.' : '')} />
      </header>
      <div className="filters">
        <input placeholder="Search names and descriptions" value={search} onChange={(e) => setSearch(e.target.value)} />
        <select value={essenceFilter} onChange={(e) => setEssenceFilter(e.target.value)}>
          <option value="">Every essence</option>
          {essences.map((e) => <option key={e.id} value={e.id}>{e.name}</option>)}
          <option value="none">In no essence</option>
        </select>
        <select value={statusFilter} onChange={(e) => setStatusFilter(e.target.value)}>
          <option value="">Drafts and accepted</option>
          <option value="draft">Drafts only</option>
          <option value="accepted">Accepted only</option>
        </select>
        <div className="filter-tags">
          <TagInput allTags={data.tags} groups={data.tagGroups} value={tagFilter} placeholder="Has every one of these tags"
            onAdd={(id) => setTagFilter([...tagFilter, id])} onRemove={(id) => setTagFilter(tagFilter.filter((t) => t !== id))} />
        </div>
        <span className="note">{shown.length} of {powers.length} shown</span>
      </div>
      {powers.length === 0
        ? <p className="empty">No powers yet. Add one above, or ask Claude to draft some for an essence.</p>
        : (
          <div className="split">
            <div className="power-list">
              <table className="grid powers">
                <thead>
                  <tr>{header('name', 'Name')}{header('essence', 'Essences')}<th>Numbers</th><th>Tags</th>{header('status', 'Status')}{header('checks', 'Checks')}</tr>
                </thead>
                <tbody>
                  {shown.map((power) => {
                    const problems = checkPower(power, data).length
                    return (
                      <tr key={power.id} className={power.id === selectedId ? 'row active' : 'row'} onClick={() => setSelectedId(power.id)}>
                        <td className="strong">{power.name}</td>
                        <td>{essenceNames(power).join(', ') || <span className="note">none</span>}</td>
                        <td className="numbers-text">
                          {power.potency != null && <><strong>{power.potency}</strong> {power.potencyNote}<br /></>}
                          {power.potency == null && power.potencyNote && <>{power.potencyNote}<br /></>}
                          <span className="note">
                            {[power.manaCost != null && power.manaCost + ' mana', power.cooldown ? power.cooldown + 's cooldown' : null, power.radius != null && 'radius ' + power.radius].filter(Boolean).join(' · ')}
                          </span>
                        </td>
                        <td className="tag-text">{tagNames(power).join(' · ')}</td>
                        <td>{power.status === 'accepted' ? 'Accepted' : <span className="badge">Draft</span>}</td>
                        <td>{problems > 0 ? <span className="problem-count">{problems}</span> : <span className="note">ok</span>}</td>
                      </tr>
                    )
                  })}
                </tbody>
              </table>
            </div>
            {selected && (
              <Editor key={selected.id} power={selected} data={data} nameTaken={nameTaken} change={change}
                onDelete={() => { remove(selected.id); setSelectedId(null) }} />
            )}
          </div>
        )}
    </>
  )
}

function Editor({ power, data, nameTaken, change, onDelete }) {
  // The essence picker reuses the tag lookup box, with essences offered in place of tags.
  const essenceOptions = (data.essences ?? []).map((e) => ({ id: e.id, name: e.name, groupId: null }))
  const essenceIds = power.essenceIds ?? []
  const tags = power.tags ?? []
  const accepted = power.status === 'accepted'
  const warnings = checkPower(power, data)
  return (
    <section className="editor power-editor">
      <div className="field-row">
        <label className="field grow">
          Name
          <NameInput value={power.name} check={(name) => (nameTaken(name, power.id) ? 'taken' : '')}
            onCommit={(name) => change(power.id, { name })} />
        </label>
        <div className="field">
          Status
          <div className="status-row">
            {accepted ? <span>Accepted</span> : <span className="badge">Draft</span>}
            <button className={accepted ? 'quiet' : ''} onClick={() => change(power.id, { status: accepted ? 'draft' : 'accepted' })}>
              {accepted ? 'Back to draft' : 'Accept'}
            </button>
          </div>
        </div>
      </div>
      <div className="field">
        Essences it belongs to
        <TagInput allTags={essenceOptions} groups={[]} value={essenceIds} placeholder="Type part of an essence name, then Tab"
          onAdd={(id) => change(power.id, { essenceIds: [...essenceIds, id] })}
          onRemove={(id) => change(power.id, { essenceIds: essenceIds.filter((e) => e !== id) })} />
      </div>
      <div className="field">
        Tags
        <TagInput allTags={data.tags} groups={data.tagGroups} value={tags} placeholder="Type part of a tag name, then Tab"
          onAdd={(id) => change(power.id, { tags: [...tags, id] })}
          onRemove={(id) => change(power.id, { tags: tags.filter((t) => t !== id) })} />
      </div>
      <label className="field">
        What it does
        <TextArea rows={4} value={power.description} onCommit={(description) => change(power.id, { description })} />
      </label>
      <div className="field-row">
        {NUMBERS.map((n) => (
          <label key={n.field} className="field narrow" title={n.help}>
            {n.label}
            <NumberInput optional decimals={n.decimals} value={power[n.field]} onCommit={(value) => change(power.id, { [n.field]: value })} />
          </label>
        ))}
      </div>
      <label className="field">
        What the potency number means
        <TextArea rows={2} value={power.potencyNote ?? ''} onCommit={(potencyNote) => change(power.id, { potencyNote })} />
      </label>
      {warnings.length > 0 && (
        <div className="warnings">
          <strong>Check against the rules</strong>
          <ul>{warnings.map((w) => <li key={w}>{w}</li>)}</ul>
        </div>
      )}
      <DeleteButton name={power.name} label="Delete power" onDelete={onDelete} />
    </section>
  )
}
