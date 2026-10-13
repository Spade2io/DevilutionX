/**
 * @file essence_tint.h
 *
 * Essence Mod: colour tints for existing art.
 *
 * A tint is a 256-entry lookup table applied while drawing. It moves the fixed colour ramps of the
 * palette (the ones shared by every dungeon level) onto another ramp, so the original art is reused
 * without being edited.
 */
#pragma once

#include <cstdint>
#include <optional>

#include "DiabloUI/ui_flags.hpp"
#include "engine/clx_sprite.hpp"
#include "engine/point.hpp"
#include "engine/surface.hpp"
#include "essences.h"
#include "tables/misdat.h"
#include "tables/spelldat.h"

namespace devilution {

enum class EssenceTint : uint8_t {
	/** Bright red. Fire. */
	VividRed,
	/** Bright yellow. Lightning. */
	VividYellow,
	/** Bright blue. Frost. */
	VividBlue,
	/** The darkest, most purple the shared palette offers: deep slate-indigo. Shadow. */
	Shadow,
	// The "dull" colours are the palette's soft 16-shade ramps, named as in the Essence Designer.
	DullRed,
	DullOrange,
	DullYellow,
	DullBlue,
	DullBeige,
	DullGray,
	/** Bright orange. */
	VividOrange,
	/** The light half of the greys: white, for a Common awakening stone. */
	White,
};

/** @brief The lookup table for a tint, for use with ClxDrawTRN. */
const uint8_t *GetEssenceTintTrn(EssenceTint tint);

/**
 * @brief The colour that belongs to a spell, if it has one.
 *
 * One colour is used for everything about the spell: its projectile, its awakening stone, and the
 * flash on a monster when its damage over time ticks.
 */
std::optional<EssenceTint> GetSpellTint(SpellID spell);

/** @brief The colour that belongs to an essence, used for its item and its page of the spellbook. */
std::optional<EssenceTint> GetEssenceTint(EssenceID essence);

struct Item;

/**
 * @brief Draws an essence or an awakening stone in its colour: an essence in its own, a stone by
 * its rarity (Common white, Rare blue, Epic purple, Legendary orange). Purple is two colours laid
 * in a checkerboard, since the palette has none.
 * @return false if the item has no colour of its own; the caller then draws it as usual.
 */
bool DrawItemTinted(const Surface &out, Point position, ClxSprite sprite, const Item &item);

/**
 * @brief The colour of an awakening stone's name, by its rarity: Common the ordinary white, Rare
 * blue, Epic beige (the palette has no purple for lettering), Legendary orange.
 */
UiFlags GetAwakeningStoneTextColor(const Item &item);

/**
 * @brief The damage type a power belongs to. Its damage is dealt as this type, and every word or
 * number the power puts on screen (damage, healing, debuffs, damage over time) is drawn in this
 * type's colour.
 */
DamageType GetSpellDamageType(SpellID spell);

/**
 * @brief The text colour of a damage type. The damage numbers themselves are coloured by the
 * "Floating Numbers - Damage" script, which holds the same list; keep the two in step.
 */
UiFlags GetDamageTypeTextColor(DamageType damageType);

/** @brief The text colour of a power: that of its damage type. */
UiFlags GetSpellTextColor(SpellID spell);

struct Item;

/** @brief The colour an item is drawn in: an awakening stone takes its spell's colour. */
std::optional<EssenceTint> GetItemTint(const Item &item);

} // namespace devilution
