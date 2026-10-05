import { useCallback, useEffect, useRef, useState } from 'react'
import TagsPage from './TagsPage.jsx'
import EssencesPage from './EssencesPage.jsx'
import PowersPage from './PowersPage.jsx'
import DamageTypesPage from './DamageTypesPage.jsx'

// Every page in the spec. Only the ones marked "ready" are built so far.
const PAGES = [
  { id: 'tags', label: 'Tags', ready: true },
  { id: 'essences', label: 'Essences', ready: true },
  { id: 'powers', label: 'Powers', ready: true },
  { id: 'confluences', label: 'Confluences' },
  { id: 'confluence-powers', label: 'Confluence Powers' },
  { id: 'damage-types', label: 'Damage Types', ready: true },
  { id: 'stones', label: 'Awakening Stones' },
]

const STATUS_TEXT = {
  loading: 'Loading…',
  saved: 'All changes saved',
  saving: 'Saving…',
  reloaded: 'The data file was changed by something else, so it was reloaded. YOUR LAST EDIT WAS NOT SAVED. Please make it again.',
  error: "NOT SAVING. The designer's helper has stopped, so nothing you change is being kept. Double-click designer.bat to start it, then make your last edit again.",
}

export default function App() {
  const [page, setPage] = useState('tags')
  const [data, setData] = useState(null)
  const [status, setStatus] = useState('loading')
  const stamp = useRef('0')
  const saveTimer = useRef(null)
  const latest = useRef(null)

  const load = useCallback(async () => {
    const res = await fetch('/api/data')
    const body = await res.json()
    stamp.current = body.stamp
    latest.current = body.data
    setData(body.data)
  }, [])

  useEffect(() => {
    load().then(() => setStatus('saved'), () => setStatus('error'))
  }, [load])

  const save = useCallback(async () => {
    try {
      const res = await fetch('/api/data', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ stamp: stamp.current, data: latest.current }),
      })
      if (res.status === 409) {
        await load()
        setStatus('reloaded')
        return
      }
      if (!res.ok) throw new Error('save failed')
      stamp.current = (await res.json()).stamp
      setStatus('saved')
    } catch {
      setStatus('error')
    }
  }, [load])

  // Pages call update() with a function that turns the old data into the new
  // data. The change shows at once and is saved a moment later.
  const update = useCallback(
    (change) => {
      const next = change(latest.current)
      if (next === latest.current) return
      latest.current = next
      setData(next)
      setStatus('saving')
      clearTimeout(saveTimer.current)
      saveTimer.current = setTimeout(save, 300)
    },
    [save],
  )

  return (
    <div className="shell">
      <nav className="sidebar">
        <h1>Essence Designer</h1>
        {PAGES.map((p) => (
          <button
            key={p.id}
            className={p.id === page ? 'nav active' : 'nav'}
            disabled={!p.ready}
            title={p.ready ? '' : 'Not built yet'}
            onClick={() => setPage(p.id)}
          >
            {p.label}
          </button>
        ))}
        <p className={'status ' + status}>{STATUS_TEXT[status]}</p>
      </nav>
      <main className="page">
        {(status === 'error' || status === 'reloaded') && <div className="alarm" role="alert">{STATUS_TEXT[status]}</div>}
        {data && page === 'tags' && <TagsPage data={data} update={update} />}
        {data && page === 'essences' && <EssencesPage data={data} update={update} />}
        {data && page === 'powers' && <PowersPage data={data} update={update} />}
        {data && page === 'damage-types' && <DamageTypesPage data={data} update={update} />}
      </main>
    </div>
  )
}
