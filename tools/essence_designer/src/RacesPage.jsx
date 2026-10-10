import { AddBox, DeleteButton, NameInput, NumberInput, newId, sameName } from './parts.jsx'

// Data shape used on this page:
//   race: { id, name, gameClass, powers: [{ id, name, effect, amount, element }] }
// "gameClass" is the old Diablo class the race is played as and is fixed: the game finds a race by
// its name, and the export keeps the class's name in the game the same as the race's name here.
// A racial power is something every character of the race is born with. It is never cast and never
// grows. What it does is its effect, chosen from the list the game understands.

// Every effect the game knows. "amount" says what the number means; "element" marks the one effect
// that also needs an element.
export const RACIAL_EFFECTS = [
  { id: 'XpGain', label: 'More experience', unit: '%', says: (p) => `All experience gains are ${p.amount}% larger.` },
  { id: 'SpecialAptitude', label: 'Aptitude: special attacks', says: () => 'An awakening stone tagged Attack that names neither kind of attack is treated as Special Attack. A stone tagged Spell Attack still gives a spell.' },
  { id: 'SpellAptitude', label: 'Aptitude: spell attacks', says: () => 'An awakening stone tagged Attack that names neither kind of attack is treated as Spell Attack. A stone tagged Special Attack still gives a special attack.' },
  { id: 'EssenceGift', label: 'A gift for each essence', unit: '%', says: (p) => `For each essence awakened: ${p.amount}% more damage, healing and shields from that essence's powers. In the game each is named for its essence: Fire Gift, Water Gift.` },
  { id: 'Affinity', label: 'Affinity for an element', unit: '%', element: true, says: (p) => `${p.amount}% more damage, healing and shields from ${p.element || '(choose an element)'} powers.` },
  { id: 'Power', label: 'More Power', unit: '%', says: (p) => `Power is ${p.amount}% higher (10 becomes ${Math.floor(10 * (100 + p.amount) / 100)}).` },
  { id: 'Speed', label: 'More Speed', unit: '%', says: (p) => `Speed is ${p.amount}% higher (10 becomes ${Math.floor(10 * (100 + p.amount) / 100)}).` },
  { id: 'Spirit', label: 'More Spirit', unit: '%', says: (p) => `Spirit is ${p.amount}% higher (10 becomes ${Math.floor(10 * (100 + p.amount) / 100)}).` },
  { id: 'Recovery', label: 'More Recovery', unit: '%', says: (p) => `Recovery is ${p.amount}% higher (10 becomes ${Math.floor(10 * (100 + p.amount) / 100)}).` },
  { id: 'MaxLife', label: 'Larger health pool', unit: '%', says: (p) => `The health pool is ${p.amount}% larger.` },
  { id: 'MaxMana', label: 'Larger mana pool', unit: '%', says: (p) => `The mana pool is ${p.amount}% larger.` },
  { id: 'ManaRegen', label: 'Faster mana regeneration', unit: '%', says: (p) => `Mana comes back ${p.amount}% faster.` },
  { id: 'LifeRegen', label: 'Faster health regeneration', unit: '%', says: (p) => `Health comes back ${p.amount}% faster.` },
  { id: 'SpellDamage', label: 'More spell damage', unit: '%', says: (p) => `Spell attacks deal ${p.amount}% more damage.` },
  { id: 'Resist', label: 'Resistance to everything', unit: 'points', says: (p) => `${p.amount} points on all three resistances, before any gear.` },
]
const effectOf = (id) => RACIAL_EFFECTS.find((e) => e.id === id)

export default function RacesPage({ data, update }) {
  const races = data.races ?? []
  const essences = data.essences ?? []
  // An element is an essence's name. "Life" is listed although its essence has not been made yet.
  const elements = [...new Set([...essences.map((e) => e.name), 'Life'])]

  const changeRace = (raceId, change) =>
    update((d) => ({ ...d, races: d.races.map((r) => (r.id === raceId ? change(r) : r)) }))
  const addPower = (raceId, name) =>
    changeRace(raceId, (r) => ({ ...r, powers: [...r.powers, { id: newId('rp'), name, effect: '', amount: 10, element: '' }] }))
  const changePower = (raceId, powerId, fields) =>
    changeRace(raceId, (r) => ({ ...r, powers: r.powers.map((p) => (p.id === powerId ? { ...p, ...fields } : p)) }))
  const removePower = (raceId, powerId) =>
    changeRace(raceId, (r) => ({ ...r, powers: r.powers.filter((p) => p.id !== powerId) }))

  return (
    <>
      <header className="page-head">
        <h2>Races</h2>
      </header>
      <p className="hint">
        Every race starts with 10 in each of the four stats. What sets them apart is their racial powers:
        things a character is born with, never cast and never growing. The character screen in the game
        lists their names. The game shows five at most.
      </p>
      {races.map((race) => (
        <section key={race.id} className="race">
          <h3>{race.name} <span className="note">played as the {race.gameClass}</span></h3>
          <div className="table-scroll">
            <table className="grid races">
              <thead>
                <tr>
                  <th>Racial power</th>
                  <th>What it does</th>
                  <th>Amount</th>
                  <th>Element</th>
                  <th>In plain words</th>
                  <th></th>
                </tr>
              </thead>
              <tbody>
                {race.powers.map((power) => {
                  const effect = effectOf(power.effect)
                  return (
                    <tr key={power.id}>
                      <td>
                        <NameInput value={power.name}
                          check={(name) => (race.powers.some((p) => p.id !== power.id && sameName(p.name, name)) ? 'This race already has a power of that name.' : '')}
                          onCommit={(name) => changePower(race.id, power.id, { name })} />
                      </td>
                      <td>
                        <select value={power.effect} onChange={(e) => changePower(race.id, power.id, { effect: e.target.value })}>
                          <option value="">(choose)</option>
                          {RACIAL_EFFECTS.map((e) => <option key={e.id} value={e.id}>{e.label}</option>)}
                        </select>
                      </td>
                      <td>
                        {effect?.unit
                          ? <><NumberInput value={power.amount} onCommit={(amount) => changePower(race.id, power.id, { amount })} /> <span className="note">{effect.unit}</span></>
                          : null}
                      </td>
                      <td>
                        {effect?.element
                          ? (
                            <select value={power.element} onChange={(e) => changePower(race.id, power.id, { element: e.target.value })}>
                              <option value="">(choose)</option>
                              {elements.map((name) => <option key={name} value={name}>{name}</option>)}
                            </select>
                          )
                          : null}
                      </td>
                      <td className="race-says">
                        {effect ? <span className="note">{effect.says(power)}</span> : <span className="problem">Does nothing until you choose what it does.</span>}
                        {effect?.element && power.element && !essences.some((e) => e.name === power.element)
                          ? <span className="note"> No such essence yet, so nothing in the game is touched by it.</span>
                          : null}
                      </td>
                      <td><DeleteButton name={power.name} onDelete={() => removePower(race.id, power.id)} /></td>
                    </tr>
                  )
                })}
              </tbody>
            </table>
          </div>
          {race.powers.length > 5 && <p className="problem">The game's character screen has room for five names; this race has {race.powers.length}.</p>}
          <AddBox small placeholder="Name, e.g. Ancestral Strength" button="Add racial power"
            onAdd={(name) => addPower(race.id, name)}
            check={(name) => (race.powers.some((p) => sameName(p.name, name)) ? 'This race already has a power of that name.' : '')} />
        </section>
      ))}
    </>
  )
}
