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

#include "tables/spelldat.h"

namespace devilution {

enum class EssenceTint : uint8_t {
	/** Bright red. Fire. */
	VividRed,
	/** Bright blue. Frost. */
	VividBlue,
	/** The darkest, most purple the shared palette offers: deep slate-indigo. Shadow. */
	Shadow,
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

struct Item;

/** @brief The colour an item is drawn in: an awakening stone takes its spell's colour. */
std::optional<EssenceTint> GetItemTint(const Item &item);

} // namespace devilution
