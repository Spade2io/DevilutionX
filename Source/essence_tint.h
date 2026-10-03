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

namespace devilution {

enum class EssenceTint : uint8_t {
	VividBlue,
};

/** @brief The lookup table for a tint, for use with ClxDrawTRN. */
const uint8_t *GetEssenceTintTrn(EssenceTint tint);

} // namespace devilution
