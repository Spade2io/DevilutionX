// The colours the game can tint things with. These are the ten fixed colour ramps in Diablo's
// palette (the same ones shown in graphics_tool/extracted/tints_26.png), read from the game's
// own palette file. "ramp" is where the ramp starts in the palette; "shades" runs bright to dark.
// "Dull" ramps have 16 soft shades; "Bright" ramps have 8 vivid ones.
// Damage text uses the same ten: the game's ordinary gold lettering is the Dull Yellow ramp.
// This list is fixed by the engine, so it lives here and not in the data file.

export const TINTS = [
  { id: 'dull-red', name: 'Dull Red', ramp: 224, shades: ['#ffbdbd', '#f49696', '#e87d7d', '#e06c6c', '#d85b5b', '#cf4949', '#c73838', '#bf2727', '#a92222', '#931e1e', '#7c1919', '#661515', '#4f1111', '#390d0d', '#230909', '#0c0505'] },
  { id: 'dull-orange', name: 'Dull Orange', ramp: 208, shades: ['#ffe2b3', '#f4c996', '#e7b37e', '#dc9f70', '#d08c62', '#c77b52', '#cc6133', '#c74b1f', '#b1431b', '#9b3b18', '#853213', '#6f2910', '#5a220c', '#3f1708', '#250e03', '#0f0500'] },
  { id: 'dull-yellow', name: 'Dull Yellow', ramp: 192, shades: ['#ffe3a4', '#eed18c', '#ddc47e', '#ccb775', '#bca86c', '#ab9a63', '#988b5d', '#877e54', '#786f49', '#69603f', '#5b5134', '#484027', '#39311d', '#312816', '#1a1408', '#140b00'] },
  { id: 'dull-blue', name: 'Dull Blue', ramp: 176, shades: ['#c8cdea', '#b2b7d7', '#9fa5c6', '#9399b9', '#878dac', '#797fa0', '#667099', '#58638d', '#4e587d', '#434c6f', '#39415f', '#2f3650', '#252b41', '#191e2d', '#0d111b', '#05070c'] },
  { id: 'dull-beige', name: 'Dull Beige', ramp: 160, shades: ['#e8caca', '#d7b2b2', '#ca9e9e', '#bd8f8f', '#b38080', '#a87171', '#a55a5a', '#9c4949', '#8b4141', '#793939', '#683131', '#562929', '#442121', '#331919', '#1b0e0e', '#0c0707'] },
  { id: 'dull-gray', name: 'Dull Gray', ramp: 240, shades: ['#f3f3f3', '#dedede', '#cccccc', '#b8b8b8', '#a3a3a3', '#949494', '#858585', '#737373', '#666666', '#595959', '#4c4c4c', '#3d3d3d', '#2e2e2e', '#1e1e1e', '#111111'] },
  { id: 'bright-red', name: 'Bright Red', ramp: 136, shades: ['#ff9f9f', '#ff5757', '#fe2424', '#f00000', '#bd0000', '#910000', '#5a0000', '#230000'] },
  { id: 'bright-orange', name: 'Bright Orange', ramp: 152, shades: ['#febea0', '#ff8c57', '#fe6924', '#f04c00', '#c74100', '#8f2e00', '#571a00', '#1e0700'] },
  { id: 'bright-yellow', name: 'Bright Yellow', ramp: 144, shades: ['#fffd9f', '#fffc57', '#fefb24', '#f0ec00', '#c3c300', '#868600', '#575500', '#191900'] },
  { id: 'bright-blue', name: 'Bright Blue', ramp: 128, shades: ['#9f9fff', '#5757ff', '#2424fe', '#0101ef', '#0000bd', '#00008a', '#000057', '#000019'] },
]

// Magic effects have one extra choice: leave the art exactly as the original game drew it.
// Fire and lightning mix several colours in their original art, which a single ramp would flatten.
export const ORIGINAL_ART = { id: 'original', name: 'Original art (no tint)' }

export const EFFECT_COLORS = [ORIGINAL_ART, ...TINTS]

// Damage text has one extra choice: Gold, the game's ordinary lettering with no recolouring.
// It is drawn from the Dull Yellow ramp, but reads lighter and warmer than the Dull Yellow text.
export const GOLD_TEXT = { id: 'gold', name: 'Gold', shades: ['#ffe3a4', '#eed18c', '#ddc47e', '#bca86c', '#877e54'] }

export const TEXT_COLORS = [GOLD_TEXT, ...TINTS]

// The three resistance categories, plus "none" for damage nothing resists (physical).
export const CATEGORIES = [
  { id: 'elemental', name: 'Elemental' },
  { id: 'natural', name: 'Natural' },
  { id: 'astral', name: 'Astral' },
  { id: 'none', name: 'None (physical)' },
]

// A CSS background that paints a ramp as a small shaded ball, like the gem in the tint sheet.
export const orbStyle = (color) => {
  // "Original art" has no ramp, so it gets a striped sample instead of a colour.
  if (!color.shades) return { background: 'repeating-linear-gradient(45deg, #6b5d4c 0 3px, #2b231b 3px 6px)' }
  const s = color.shades
  const pick = (fraction) => s[Math.min(s.length - 1, Math.round(fraction * (s.length - 1)))]
  return { background: `radial-gradient(circle at 35% 30%, ${pick(0)} 0%, ${pick(0.3)} 30%, ${pick(0.55)} 60%, ${pick(0.85)} 100%)` }
}

// The single colour used when lettering is drawn in a ramp.
export const textShade = (color) => (color.id === 'gold' ? color.shades[0] : color.shades[Math.min(2, color.shades.length - 1)])
