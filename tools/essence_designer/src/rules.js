// Checks a power against the tag rules written down in devilutionX/docs/MOD_DESIGN.md
// ("Rules for drafting powers"). Each problem comes back as one plain sentence. These are
// warnings only: nothing is blocked.
//
// The rules name specific tags and groups (Target, Aura, Damage Style...), so they are looked up
// by name. If a tag or group is renamed or missing, its rule is simply skipped.

const same = (a, b) => a.trim().toLowerCase() === b.toLowerCase()

// Every tag an essence bars: the ones on its Never line, plus the ones an "All other <group>" entry sweeps in.
export function neverTagIds(essence, data) {
  const wanted = [...essence.tagsAll, ...essence.tagsMost, ...essence.tagsSome, ...(essence.tagsFinite ?? [])]
  const groups = essence.neverOtherGroups ?? []
  const swept = data.tags.filter((t) => groups.includes(t.groupId) && !wanted.includes(t.id)).map((t) => t.id)
  return [...new Set([...essence.tagsNever, ...swept])]
}

// The Ultimate cooldown tag, however it is spelled or annotated ("Ultimate (120+)").
const isUltimate = (tags) => tags.some((t) => /^u[lt]{2}imate/i.test(t.name.trim()))

export function checkPower(power, data) {
  const warnings = []
  const tags = (power.tags ?? []).map((id) => data.tags.find((t) => t.id === id)).filter(Boolean)
  const has = (name) => tags.some((t) => same(t.name, name))
  const group = (name) => data.tagGroups.find((g) => same(g.name, name))
  const inGroup = (name) => {
    const g = group(name)
    return g ? tags.filter((t) => t.groupId === g.id) : null
  }

  // A group marked mutually exclusive allows one tag.
  for (const g of data.tagGroups) {
    if (!g.exclusive) continue
    const mine = tags.filter((t) => t.groupId === g.id)
    if (mine.length > 1) warnings.push(`Has ${mine.length} ${g.name} tags (${mine.map((t) => t.name).join(', ')}); only one is allowed.`)
  }

  // Every power needs one of each of these.
  for (const [name, label] of [['Element', 'an Element'], ['Target', 'a Target'], ['Area of Effect', 'an Area of Effect'], ['Duration', 'a Duration']]) {
    const got = inGroup(name)
    if (got && got.length === 0) warnings.push(`Needs ${label}.`)
  }

  // Every power that is cast has a Cooldown tag ("No Cooldown" counts). An aura is never cast, so it is exempt.
  const cooldowns = inGroup('Cooldown')
  if (cooldowns && cooldowns.length === 0 && !has('Aura')) warnings.push('Needs a Cooldown tag (No CD counts).')

  const aura = has('Aura')
  // The purpose tag for dealing damage is "Attack". ("Damage" is a Stat Effect: a boon to damage dealt.)
  const damage = has('Attack')
  const stats = inGroup('Stat Effect')
  const buff = has('Boon')
  const debuff = has('Affliction')
  const styles = inGroup('Damage Style')

  if (aura) {
    // Aura is an Area of Effect: it has a radius, always targets Self and is always Permanent.
    if (group('Target') && !has('Self')) warnings.push('An aura always targets Self.')
    if (group('Duration') && !has('Permanent')) warnings.push('An aura is always Permanent.')
    if (styles && styles.length > 0) warnings.push('An aura carries no Damage Style.')
    if (has('Stealth') || has('Threat')) warnings.push('Stealth and Threat never go on an aura.')
  } else {
    if (damage && styles && styles.length === 0) warnings.push('Deals damage, so it needs Special Attack or Spell Attack.')
    if (!damage && styles && styles.length > 0) warnings.push('Has a Damage Style but no Attack tag.')
    if (damage && (has('Self') || has('Ally'))) warnings.push('An attack targets Enemy, Ground or Direction.')
  }

  if (has('Timed') && !buff && !debuff && !has('Shielding')) warnings.push('Timed is for boons, afflictions and shields.')
  if (has('Cleanse') && has('Permanent')) warnings.push('A cleanse is never Permanent.')
  if (has('Resurrect') && has('Self') && !isUltimate(tags)) warnings.push('A self-resurrect must be an Ultimate.')
  if (has('Resurrect') && !has('Healing')) warnings.push('A resurrect gives life back, so it is also tagged Healing.')
  // A Weapon tag means "only works with this weapon equipped", and only Special Attacks carry one.
  const weapons = inGroup('Weapon')
  if (weapons && weapons.length > 0 && !has('Special Attack')) warnings.push('Weapon tags (' + weapons.map((t) => t.name).join(', ') + ') only go on Special Attacks.')
  if (has('Cone') && !has('Direction')) warnings.push('A Cone needs Target: Direction.')
  if (has('Permanent') && !buff && !debuff && !aura) warnings.push('Permanent is only for boons, afflictions and auras.')
  if ((has('Stealth') || has('Threat')) && !buff) warnings.push('Stealth and Threat only go on boons.')
  if ((has('Stealth') || has('Threat')) && (!has('Permanent') || has('Timed'))) warnings.push('Stealth and Threat boons are always Permanent, never Timed.')
  // Stat Effect tags say what a buff or debuff changes, so the two go together.
  if (stats && stats.length > 0 && !buff && !debuff) warnings.push('Stat Effect tags (' + stats.map((t) => t.name).join(', ') + ') go on a Boon or an Affliction.')
  if (stats && stats.length === 0 && (buff || debuff) && !has('Stealth') && !has('Threat')) warnings.push('A boon or affliction should carry a Stat Effect tag saying what it changes.')
  if (stats && stats.length > 1 && (has('Defensive') || has('Offensive'))) warnings.push('Defensive and Offensive are the general fallbacks; drop them when a specific Stat Effect is tagged.')
  if (has('Everyone (allies only)') && (damage || debuff || has('Enemy'))) warnings.push('"Everyone" only reaches allies; anything that affects enemies needs a target and a radius.')
  if (buff && !aura && (has('Enemy') || has('Ground') || has('Direction')) && !debuff && !damage) warnings.push('A boon targets Self or Ally.')
  if (debuff && !aura && (has('Self') || has('Ally')) && !buff) warnings.push('An affliction targets Enemy or Ground.')

  // What its essences ask for.
  for (const essenceId of power.essenceIds ?? []) {
    const essence = (data.essences ?? []).find((e) => e.id === essenceId)
    if (!essence) continue
    const never = neverTagIds(essence, data)
    for (const t of tags) if (never.includes(t.id)) warnings.push(`${essence.name} never allows ${t.name}.`)
    for (const id of essence.tagsAll) {
      const t = data.tags.find((x) => x.id === id)
      if (!t || tags.includes(t)) continue
      const weaponGroup = group('Weapon')
      if (weaponGroup && t.groupId === weaponGroup.id) {
        if (has('Special Attack')) warnings.push(`${essence.name} wants every Special Attack to require ${t.name}.`)
      } else {
        warnings.push(`${essence.name} wants ${t.name} on every power.`)
      }
    }
  }
  return warnings
}

// How an essence's powers measure up: how many it has, how many are auras, and each listed tag's share.
export function essenceSummary(essence, data) {
  const powers = (data.powers ?? []).filter((p) => (p.essenceIds ?? []).includes(essence.id))
  const auraTag = data.tags.find((t) => same(t.name, 'Aura'))
  const auras = auraTag ? powers.filter((p) => (p.tags ?? []).includes(auraTag.id)).length : 0
  const share = (tagId) => (powers.length ? powers.filter((p) => (p.tags ?? []).includes(tagId)).length / powers.length : 0)
  const ultimates = powers.filter((p) => isUltimate((p.tags ?? []).map((id) => data.tags.find((t) => t.id === id)).filter(Boolean))).length
  return { powers, auras, ultimates, share, drafts: powers.filter((p) => p.status !== 'accepted').length }
}
