import { AddBox, DeleteButton, NameInput, newId, sameName } from './parts.jsx'
import TagInput from './TagInput.jsx'

// Data shape used on this page:
//   stone: { id, name, tags: [tagId, ...] }
// The full name in the game is "Awakening Stone of " + name. A stone carries one to five tags.
// When it is used, the game looks at the powers the player could learn right now and gives one
// at random from those matching the most of the stone's tags. Rarity is not chosen: it follows
// from how many tags the stone has.

export const MAX_STONE_TAGS = 5

// One or two tags is Common, three Rare, four Epic, five Legendary.
export const stoneRarity = (tagCount) =>
  tagCount >= 5 ? 'Legendary' : tagCount === 4 ? 'Epic' : tagCount === 3 ? 'Rare' : tagCount >= 1 ? 'Common' : ''

export default function StonesPage({ data, update }) {
  const stones = data.stones ?? []
  const powers = data.powers ?? []
  const essences = data.essences ?? []

  // Shown by rarity (Common first, a stone with no tags last), then by name. The data keeps its own order.
  const RARITY_ORDER = ['Common', 'Rare', 'Epic', 'Legendary', '']
  const sorted = [...stones].sort((a, b) =>
    RARITY_ORDER.indexOf(stoneRarity(a.tags.length)) - RARITY_ORDER.indexOf(stoneRarity(b.tags.length))
    || a.name.localeCompare(b.name, undefined, { sensitivity: 'base' }))

  const nameProblem = (name, exceptId) =>
    stones.some((s) => s.id !== exceptId && sameName(s.name, name)) ? 'A stone with that name already exists.'
      : powers.some((p) => sameName(p.name, name)) ? 'A power has that name, and every power already has a stone called that.'
      : ''

  const add = (name) => update((d) => ({ ...d, stones: [...(d.stones ?? []), { id: newId('s'), name, tags: [] }] }))
  const change = (id, fields) => update((d) => ({ ...d, stones: d.stones.map((s) => (s.id === id ? { ...s, ...fields } : s)) }))
  const remove = (id) => update((d) => ({ ...d, stones: d.stones.filter((s) => s.id !== id) }))

  // The powers a stone draws from, as the game works it out: those carrying the most of its tags.
  // That is every tag when any power has them all. Counted for each essence; a power in two
  // essences counts under both.
  const matchesOf = (stone) => {
    const inGame = powers.filter((p) => p.essenceIds?.length)
    const hits = (p) => stone.tags.filter((t) => (p.tags ?? []).includes(t)).length
    const best = Math.max(0, ...inGame.map(hits))
    const matching = best === 0 ? [] : inGame.filter((p) => hits(p) === best)
    const perEssence = essences
      .map((e) => ({ name: e.name, count: matching.filter((p) => p.essenceIds.includes(e.id)).length }))
      .filter((e) => e.count > 0)
    return { total: matching.length, best, perEssence, missing: essences.length - perEssence.length }
  }

  return (
    <>
      <header className="page-head">
        <h2>Awakening Stones</h2>
        <AddBox placeholder="Name, e.g. Flame" button="Add stone" onAdd={add} check={(name) => nameProblem(name)} />
      </header>
      <p className="hint">
        A stone is named "Awakening Stone of ..." and carries one to five tags. Used, it gives the player one
        power at random from those they can learn that match every tag. If none match every tag, it settles
        for the powers matching the most. One or two tags is Common, three Rare, four Epic, five Legendary.
      </p>
      {stones.length === 0
        ? <p className="empty">No awakening stones yet. Add one above to get started.</p>
        : (
          <div className="table-scroll">
            <table className="grid stones">
              <thead>
                <tr>
                  <th>Awakening Stone of</th>
                  <th>Tags</th>
                  <th>Rarity</th>
                  <th>Powers it draws from</th>
                  <th></th>
                </tr>
              </thead>
              <tbody>
                {sorted.map((stone) => {
                  const match = matchesOf(stone)
                  const rarity = stoneRarity(stone.tags.length)
                  return (
                    <tr key={stone.id}>
                      <td>
                        <NameInput value={stone.name} check={(name) => nameProblem(name, stone.id)}
                          onCommit={(name) => change(stone.id, { name })} />
                      </td>
                      <td className="stone-tags">
                        <TagInput allTags={data.tags} full={stone.tags.length >= MAX_STONE_TAGS} groups={data.tagGroups} value={stone.tags}
                          placeholder="Type part of a tag name, then Tab"
                          onAdd={(tag) => { if (stone.tags.length < MAX_STONE_TAGS) change(stone.id, { tags: [...stone.tags, tag] }) }}
                          onRemove={(tag) => change(stone.id, { tags: stone.tags.filter((t) => t !== tag) })} />
                      </td>
                      <td>{rarity ? <span className={'rarity-word ' + rarity.toLowerCase()}>{rarity}</span> : <span className="problem">Needs a tag</span>}</td>
                      <td className="stone-matches">
                        {stone.tags.length === 0 ? null : match.total === 0
                          ? <span className="problem">No power has any of these tags, so it gives any power at all.</span>
                          : (
                            <>
                              <strong>{match.total}</strong>{' '}
                              {match.best < stone.tags.length && <span className="note">(no power has all {stone.tags.length} tags; these have {match.best}) </span>}
                              <span className="note">
                                {match.perEssence.map((e) => `${e.name} ${e.count}`).join(', ')}
                                {match.missing > 0 ? `. Nothing in ${match.missing} essence${match.missing === 1 ? '' : 's'}.` : ''}
                              </span>
                            </>
                          )}
                      </td>
                      <td><DeleteButton name={stone.name} onDelete={() => remove(stone.id)} /></td>
                    </tr>
                  )
                })}
              </tbody>
            </table>
          </div>
        )}
    </>
  )
}
