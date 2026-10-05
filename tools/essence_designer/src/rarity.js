// The rarity steps, least rare first. Drop rates for each step are set elsewhere.
// Awakening stones will use the same steps.
export const RARITIES = [
  { id: 'common', name: 'Common' },
  { id: 'rare', name: 'Rare' },
  { id: 'epic', name: 'Epic' },
  { id: 'legendary', name: 'Legendary' },
]

// Anything that is not one of the steps (such as an old number) counts as Common.
export const rarityOf = (value) => RARITIES.find((r) => r.id === value) ?? RARITIES[0]
