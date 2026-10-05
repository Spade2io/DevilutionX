import { useRef, useState } from 'react'

// A line of tags you fill by typing. Used wherever tags are picked (essences now; powers and
// awakening stones later).
//
//   - Type any part of a tag's name and matching tags are listed.
//   - Tab or Enter takes the highlighted match and leaves the cursor ready for the next tag.
//   - Up and Down move the highlight. Escape closes the list.
//   - Backspace in an empty box removes the last tag. Clicking a tag's x removes that tag.
//   - A tag can be dragged from one line and dropped on another.
//
// Props: allTags and groups come straight from the data file; value is the list of tag ids on
// this line; onAdd(id) and onRemove(id) report changes.
export default function TagInput({ allTags, groups, value, onAdd, onRemove, placeholder }) {
  const [text, setText] = useState('')
  const [highlight, setHighlight] = useState(0)
  const [focused, setFocused] = useState(false)
  const [dropping, setDropping] = useState(false)
  const input = useRef(null)

  const byId = (id) => allTags.find((t) => t.id === id)
  const groupName = (tag) => groups.find((g) => g.id === tag.groupId)?.name ?? ''

  const search = text.trim().toLowerCase()
  const matches = search
    ? allTags
        .filter((t) => !value.includes(t.id) && t.name.toLowerCase().includes(search))
        // Names that start with what was typed come first.
        .sort((a, b) => a.name.toLowerCase().indexOf(search) - b.name.toLowerCase().indexOf(search) || a.name.localeCompare(b.name))
    : []
  const current = Math.min(highlight, Math.max(0, matches.length - 1))

  const take = (tag) => {
    onAdd(tag.id)
    setText('')
    setHighlight(0)
    input.current?.focus()
  }

  // Dragging: a tag carries its id under a label only this app uses, so stray drops are ignored.
  // A line only accepts a tag it could have offered in its own lookup.
  const DRAG_TYPE = 'application/x-essence-tag'
  const canDrop = (e) => e.dataTransfer.types.includes(DRAG_TYPE)
  const onDrop = (e) => {
    setDropping(false)
    const id = e.dataTransfer.getData(DRAG_TYPE)
    if (!id) return
    e.preventDefault()
    if (byId(id) && !value.includes(id)) onAdd(id)
  }

  const onKeyDown = (e) => {
    if ((e.key === 'Tab' || e.key === 'Enter') && matches.length > 0) {
      e.preventDefault()
      take(matches[current])
    } else if (e.key === 'ArrowDown' && matches.length > 0) {
      e.preventDefault()
      setHighlight((current + 1) % matches.length)
    } else if (e.key === 'ArrowUp' && matches.length > 0) {
      e.preventDefault()
      setHighlight((current - 1 + matches.length) % matches.length)
    } else if (e.key === 'Escape') {
      setText('')
    } else if (e.key === 'Backspace' && text === '' && value.length > 0) {
      onRemove(value[value.length - 1])
    }
  }

  return (
    <div className={dropping ? 'tag-input dropping' : 'tag-input'} onClick={() => input.current?.focus()}
      onDragOver={(e) => { if (canDrop(e)) { e.preventDefault(); setDropping(true) } }}
      onDragLeave={(e) => { if (!e.currentTarget.contains(e.relatedTarget)) setDropping(false) }}
      onDrop={onDrop}>
      {value.map((id) => {
        const tag = byId(id)
        if (!tag) return null
        return (
          <span key={id} className="chip" draggable title="Drag to another line"
            onDragStart={(e) => { e.dataTransfer.setData(DRAG_TYPE, id); e.dataTransfer.effectAllowed = 'move' }}>
            {tag.name}
            <button type="button" tabIndex={-1} title={'Remove ' + tag.name} onClick={(e) => { e.stopPropagation(); onRemove(id) }}>×</button>
          </span>
        )
      })}
      <div className="tag-input-box">
        <input ref={input} value={text} placeholder={value.length === 0 ? placeholder : ''}
          onChange={(e) => { setText(e.target.value); setHighlight(0) }}
          onKeyDown={onKeyDown} onFocus={() => setFocused(true)} onBlur={() => setFocused(false)} />
        {focused && search && (
          <ul className="tag-matches">
            {matches.length === 0 && <li className="no-match">No tag contains "{text.trim()}"</li>}
            {matches.slice(0, 12).map((tag, index) => (
              <li key={tag.id}>
                {/* onMouseDown, not onClick, so the box does not lose focus before the pick registers */}
                <button type="button" tabIndex={-1} className={index === current ? 'match current' : 'match'}
                  onMouseDown={(e) => { e.preventDefault(); take(tag) }} onMouseEnter={() => setHighlight(index)}>
                  <span>{tag.name}</span>
                  <span className="note">{groupName(tag)}</span>
                </button>
              </li>
            ))}
          </ul>
        )}
      </div>
    </div>
  )
}
