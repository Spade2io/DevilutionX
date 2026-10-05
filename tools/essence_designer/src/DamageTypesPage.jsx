import { useState } from 'react'
import { AddBox, DeleteButton, NameInput, newId, sameName } from './parts.jsx'
import { CATEGORIES, EFFECT_COLORS, TEXT_COLORS, TINTS, orbStyle, textShade } from './palette.js'

// Data shape used on this page:
//   damageType: { id, name, category, itemColor, textColor, effectColor }
// category is one of the ids in CATEGORIES; the three colours are ids from palette.js.

export default function DamageTypesPage({ data, update }) {
  const damageTypes = data.damageTypes ?? []

  const nameTaken = (name, exceptId) => damageTypes.some((t) => t.id !== exceptId && sameName(t.name, name))

  const add = (name) =>
    update((d) => ({
      ...d,
      damageTypes: [
        ...(d.damageTypes ?? []),
        { id: newId('d'), name, category: 'elemental', itemColor: 'dull-gray', textColor: 'gold', effectColor: 'original' },
      ],
    }))

  const change = (id, fields) =>
    update((d) => ({ ...d, damageTypes: d.damageTypes.map((t) => (t.id === id ? { ...t, ...fields } : t)) }))

  const remove = (id) => update((d) => ({ ...d, damageTypes: d.damageTypes.filter((t) => t.id !== id) }))

  return (
    <>
      <header className="page-head">
        <h2>Damage Types</h2>
        <AddBox placeholder="New damage type" button="Add damage type" onAdd={add}
          check={(name) => (nameTaken(name) ? 'A damage type with that name already exists.' : '')} />
      </header>
      <p className="hint">
        Each damage type belongs to one resistance category and has three colours, all taken from the
        ten colour ramps the game can draw with.
      </p>
      <div className="category-counts">
        {CATEGORIES.map((c) => (
          <span key={c.id}>{c.name}: <strong>{damageTypes.filter((t) => t.category === c.id).length}</strong></span>
        ))}
      </div>
      {damageTypes.length === 0
        ? <p className="empty">No damage types yet. Add one above to get started.</p>
        : (
          <div className="table-scroll">
            <table className="grid">
              <thead>
                <tr>
                  <th>Name</th>
                  <th>Resistance category</th>
                  <th>Stones and essences</th>
                  <th>Damage text</th>
                  <th>Magic effects</th>
                  <th></th>
                </tr>
              </thead>
              <tbody>
                {damageTypes.map((type) => (
                  <tr key={type.id}>
                    <td>
                      <NameInput value={type.name} check={(name) => (nameTaken(name, type.id) ? 'taken' : '')}
                        onCommit={(name) => change(type.id, { name })} />
                    </td>
                    <td>
                      <select value={type.category} onChange={(e) => change(type.id, { category: e.target.value })}>
                        {CATEGORIES.map((c) => <option key={c.id} value={c.id}>{c.name}</option>)}
                      </select>
                    </td>
                    <td><ColorPicker colors={TINTS} value={type.itemColor} onChange={(itemColor) => change(type.id, { itemColor })} /></td>
                    <td><ColorPicker colors={TEXT_COLORS} asText value={type.textColor} onChange={(textColor) => change(type.id, { textColor })} /></td>
                    <td><ColorPicker colors={EFFECT_COLORS} value={type.effectColor} onChange={(effectColor) => change(type.id, { effectColor })} /></td>
                    <td><DeleteButton name={type.name} onDelete={() => remove(type.id)} /></td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}
    </>
  )
}

// A dropdown that shows each colour instead of only naming it. With "asText" the sample is a
// number drawn in that colour, the way damage text looks; otherwise it is a small shaded ball.
function ColorPicker({ colors, value, onChange, asText }) {
  const [open, setOpen] = useState(false)
  const current = colors.find((c) => c.id === value) ?? colors[0]
  const sample = (color) =>
    asText
      ? <span className="text-sample" style={{ color: textShade(color) }}>128</span>
      : <span className="orb" style={orbStyle(color)} />
  return (
    <div className="color-picker" onBlur={(e) => { if (!e.currentTarget.contains(e.relatedTarget)) setOpen(false) }}>
      <button type="button" className="color-current" onClick={() => setOpen(!open)}>
        {sample(current)}
        <span>{current.name}</span>
        <span className="caret">▾</span>
      </button>
      {open && (
        <ul className="color-menu">
          {colors.map((color) => (
            <li key={color.id}>
              <button type="button" className={color.id === current.id ? 'color-option chosen' : 'color-option'}
                onClick={() => { onChange(color.id); setOpen(false) }}>
                {sample(color)}
                <span>{color.name}</span>
              </button>
            </li>
          ))}
        </ul>
      )}
    </div>
  )
}
