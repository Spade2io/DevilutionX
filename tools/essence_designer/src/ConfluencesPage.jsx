import { useState } from 'react'
import { NameInput, sameName } from './parts.jsx'
import { TINTS, orbStyle } from './palette.js'

// Data shape used on this page:
//   confluence: { essences: [essenceId, essenceId, essenceId], name }
// A confluence is what three different essences make together. The order of the three does not
// matter, so the ids are kept sorted and that sorted list is what identifies the confluence.
// Every set of three essences is shown, whether or not it has been given a name yet.

export const confluenceKey = (essenceIds) => [...essenceIds].sort().join('+')

// Every way of choosing three different essences, in the order the essences are listed.
export const essenceTrios = (essences) => {
  const trios = []
  for (let a = 0; a < essences.length; a++)
    for (let b = a + 1; b < essences.length; b++)
      for (let c = b + 1; c < essences.length; c++) trios.push([essences[a], essences[b], essences[c]])
  return trios
}

export default function ConfluencesPage({ data, update }) {
  const essences = data.essences ?? []
  const confluences = data.confluences ?? []
  const [showing, setShowing] = useState([])

  const names = new Map(confluences.map((c) => [confluenceKey(c.essences), c.name]))
  const trios = essenceTrios(essences)
  const shown = trios.filter((trio) => showing.every((id) => trio.some((e) => e.id === id)))
  const unnamed = trios.filter((trio) => !names.get(confluenceKey(trio.map((e) => e.id)))).length

  const tintOf = (essence) => {
    const type = (data.damageTypes ?? []).find((t) => t.id === essence.damageTypeId)
    return TINTS.find((t) => t.id === type?.itemColor)
  }

  const nameTaken = (name, key) => confluences.some((c) => confluenceKey(c.essences) !== key && sameName(c.name, name))

  const rename = (trio, name) =>
    update((d) => {
      const ids = trio.map((e) => e.id).sort()
      const key = confluenceKey(ids)
      const others = (d.confluences ?? []).filter((c) => confluenceKey(c.essences) !== key)
      return { ...d, confluences: [...others, { essences: ids, name }] }
    })

  const toggle = (id) => setShowing((now) => (now.includes(id) ? now.filter((x) => x !== id) : now.length < 3 ? [...now, id] : now))

  const orb = (essence) => {
    const tint = tintOf(essence)
    return <span className={tint ? 'orb' : 'orb empty-orb'} style={tint ? orbStyle(tint) : undefined} />
  }

  return (
    <>
      <header className="page-head">
        <h2>Confluences</h2>
      </header>
      <p className="hint">
        Three different essences make a confluence, which fills the fourth slot. The order of the three
        does not matter. With {essences.length} essences there are {trios.length} confluences
        {unnamed > 0 ? <>; <strong>{unnamed} still need a name</strong></> : null}. Click a name to change it.
      </p>
      <div className="trio-filter">
        <span>Show only confluences with:</span>
        {essences.map((essence) => (
          <button key={essence.id} type="button" className={showing.includes(essence.id) ? 'trio-pick on' : 'trio-pick'}
            onClick={() => toggle(essence.id)}>
            {orb(essence)}
            {essence.name}
          </button>
        ))}
        {showing.length > 0 && <button type="button" className="quiet" onClick={() => setShowing([])}>Show all</button>}
      </div>
      {trios.length === 0
        ? <p className="empty">A confluence needs three essences. Add more on the Essences page.</p>
        : (
          <table className="grid confluences">
            <thead>
              <tr>
                <th>Essence 1</th>
                <th>Essence 2</th>
                <th>Essence 3</th>
                <th>Confluence</th>
              </tr>
            </thead>
            <tbody>
              {shown.map((trio) => {
                const key = confluenceKey(trio.map((e) => e.id))
                return (
                  <tr key={key}>
                    {trio.map((essence) => (
                      <td key={essence.id}><span className="trio-essence">{orb(essence)}{essence.name}</span></td>
                    ))}
                    <td>
                      <NameInput className="confluence-name" value={names.get(key) ?? ''}
                        check={(name) => (nameTaken(name, key) ? 'taken' : '')}
                        onCommit={(name) => rename(trio, name)} />
                    </td>
                  </tr>
                )
              })}
            </tbody>
          </table>
        )}
      <p className="hint">Showing {shown.length} of {trios.length}. Two confluences cannot share a name.</p>
    </>
  )
}
