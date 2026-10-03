/**
 * @file essence_tint.cpp
 *
 * Essence Mod: colour tints for existing art.
 */
#include "essence_tint.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include "engine/palette.h"

namespace devilution {

namespace {

using TintTable = std::array<uint8_t, 256>;

/**
 * @brief Builds a table that moves every warm colour ramp onto one 8-shade ramp.
 *
 * Colours below 128 differ from level to level and greys carry no hue, so both are left alone.
 * The 16-shade ramps have twice the shading steps of an 8-shade ramp, so two of their shades share
 * each target shade.
 */
TintTable MakeWarmToEightShadeRamp(uint8_t targetRamp)
{
	TintTable table;
	for (size_t i = 0; i < table.size(); i++)
		table[i] = static_cast<uint8_t>(i);

	for (const uint8_t ramp : { PAL8_RED, PAL8_YELLOW, PAL8_ORANGE }) {
		for (uint8_t shade = 0; shade < 8; shade++)
			table[ramp + shade] = targetRamp + shade;
	}
	for (const uint8_t ramp : { PAL16_BEIGE, PAL16_YELLOW, PAL16_ORANGE, PAL16_RED }) {
		for (uint8_t shade = 0; shade < 16; shade++)
			table[ramp + shade] = targetRamp + shade / 2;
	}
	return table;
}

} // namespace

const uint8_t *GetEssenceTintTrn(EssenceTint tint)
{
	static const TintTable VividBlue = MakeWarmToEightShadeRamp(PAL8_BLUE);

	switch (tint) {
	case EssenceTint::VividBlue:
		return VividBlue.data();
	}
	return VividBlue.data();
}

} // namespace devilution
