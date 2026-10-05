import { useState } from 'react'

// Small pieces shared by several pages.

export const newId = (prefix) => prefix + '_' + crypto.randomUUID().slice(0, 8)

export const sameName = (a, b) => a.trim().toLowerCase() === b.trim().toLowerCase()

// A text box plus a button. Enter also adds. "check" returns a complaint, or '' if the name is fine.
export function AddBox({ placeholder, button, onAdd, check, small }) {
  const [text, setText] = useState('')
  const [problem, setProblem] = useState('')
  const submit = (e) => {
    e.preventDefault()
    const name = text.trim()
    if (!name) return
    const complaint = check(name)
    if (complaint) return setProblem(complaint)
    onAdd(name)
    setText('')
  }
  return (
    <form className={small ? 'add small' : 'add'} onSubmit={submit}>
      <input value={text} placeholder={placeholder}
        onChange={(e) => { setText(e.target.value); setProblem('') }} />
      <button type="submit">{button}</button>
      {problem && <span className="problem">{problem}</span>}
    </form>
  )
}

// A name that is edited in place. The change is kept when you press Enter or click away;
// an empty or already-used name snaps back to what it was.
export function NameInput({ value, onCommit, check, className }) {
  const [text, setText] = useState(value)
  const [lastValue, setLastValue] = useState(value)
  if (value !== lastValue) {
    setLastValue(value)
    setText(value)
  }
  const commit = () => {
    const name = text.trim()
    if (!name || check(name)) return setText(value)
    if (name !== value) onCommit(name)
  }
  return (
    <input className={className} value={text} onChange={(e) => setText(e.target.value)} onBlur={commit}
      onKeyDown={(e) => { if (e.key === 'Enter') e.target.blur() }} />
  )
}

// A delete button that asks once more before it deletes.
export function DeleteButton({ name, onDelete, label = 'Delete' }) {
  const [confirming, setConfirming] = useState(false)
  return confirming
    ? <button className="danger" onClick={onDelete} onBlur={() => setConfirming(false)}>Really delete "{name}"?</button>
    : <button className="quiet" onClick={() => setConfirming(true)}>{label}</button>
}

// A number box. The change is kept when you press Enter or click away. Whole numbers only, unless
// "decimals" is set.
export function NumberInput({ value, onCommit, decimals, optional, className, title }) {
  const [text, setText] = useState(String(value ?? ''))
  const [lastValue, setLastValue] = useState(value)
  if (value !== lastValue) {
    setLastValue(value)
    setText(String(value ?? ''))
  }
  const commit = () => {
    // With "optional", an emptied box means "no value".
    if (optional && text.trim() === '') { if (value != null) onCommit(null); return }
    let n = Math.max(0, Number(text))
    if (!decimals) n = Math.round(n)
    if (text.trim() === '' || Number.isNaN(n)) return setText(String(value ?? ''))
    setText(String(n))
    if (n !== value) onCommit(n)
  }
  return (
    <input type="number" min="0" step={decimals ? 'any' : 1} className={className} title={title} value={text}
      onChange={(e) => setText(e.target.value)} onBlur={commit}
      onKeyDown={(e) => { if (e.key === 'Enter') e.target.blur() }} />
  )
}

// A larger text box. The change is kept when you click away.
export function TextArea({ value, onCommit, rows = 3 }) {
  const [text, setText] = useState(value ?? '')
  const [lastValue, setLastValue] = useState(value)
  if (value !== lastValue) {
    setLastValue(value)
    setText(value ?? '')
  }
  return (
    <textarea rows={rows} value={text} onChange={(e) => setText(e.target.value)}
      onBlur={() => { if (text !== (value ?? '')) onCommit(text) }} />
  )
}
