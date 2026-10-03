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
#include "items.h"

namespace devilution {

namespace {

using TintTable = std::array<uint8_t, 256>;

TintTable MakeIdentity()
{
	TintTable table;
	for (size_t i = 0; i < table.size(); i++)
		table[i] = static_cast<uint8_t>(i);
	return table;
}

/**
 * @brief Builds a table that moves every warm colour ramp onto one 8-shade ramp.
 *
 * Colours below 128 differ from level to level and greys carry no hue, so both are left alone.
 * The 16-shade ramps have twice the shading steps of an 8-shade ramp, so two of their shades share
 * each target shade.
 */
TintTable MakeWarmToEightShadeRamp(uint8_t targetRamp)
{
	TintTable table = MakeIdentity();

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

/**
 * @brief Builds a table that moves every shared ramp, greys and blues included, onto the dark end
 * of one 16-shade ramp.
 *
 * Used where a whole sprite should take the colour, such as a monster flashing when it takes
 * damage over time. Only shades from `brightestShade` down to the darkest are used, which keeps
 * the result deep. Colours below 128 differ from level to level and are left alone.
 */
TintTable MakeEverythingToDarkRamp(uint8_t targetRamp, uint8_t brightestShade)
{
	TintTable table = MakeIdentity();
	const int span = 15 - brightestShade;

	for (const uint8_t ramp : { PAL8_BLUE, PAL8_RED, PAL8_YELLOW, PAL8_ORANGE }) {
		for (uint8_t shade = 0; shade < 8; shade++)
			table[ramp + shade] = static_cast<uint8_t>(targetRamp + brightestShade + shade * span / 7);
	}
	for (const uint8_t ramp : { PAL16_BEIGE, PAL16_BLUE, PAL16_YELLOW, PAL16_ORANGE, PAL16_RED, PAL16_GRAY }) {
		for (uint8_t shade = 0; shade < 16; shade++)
			table[ramp + shade] = static_cast<uint8_t>(targetRamp + brightestShade + shade * span / 15);
	}
	return table;
}

/**
 * @brief Builds a table that moves every shared ramp, greys and blues included, onto one 8-shade ramp.
 * Used to recolour whole sprites, such as turning the blue Flash burst red.
 */
TintTable MakeEverythingToEightShadeRamp(uint8_t targetRamp)
{
	TintTable table = MakeIdentity();

	for (const uint8_t ramp : { PAL8_BLUE, PAL8_RED, PAL8_YELLOW, PAL8_ORANGE }) {
		for (uint8_t shade = 0; shade < 8; shade++)
			table[ramp + shade] = targetRamp + shade;
	}
	for (const uint8_t ramp : { PAL16_BEIGE, PAL16_BLUE, PAL16_YELLOW, PAL16_ORANGE, PAL16_RED, PAL16_GRAY }) {
		for (uint8_t shade = 0; shade < 16; shade++)
			table[ramp + shade] = targetRamp + shade / 2;
	}
	return table;
}

} // namespace

const uint8_t *GetEssenceTintTrn(EssenceTint tint)
{
	static const TintTable VividRed = MakeEverythingToEightShadeRamp(PAL8_RED);
	static const TintTable VividBlue = MakeWarmToEightShadeRamp(PAL8_BLUE);
	// The slate-blue ramp is the nearest the shared palette has to purple. Its darker two-thirds read as shadow.
	static const TintTable Shadow = MakeEverythingToDarkRamp(PAL16_BLUE, 5);

	switch (tint) {
	case EssenceTint::VividRed:
		return VividRed.data();
	case EssenceTint::VividBlue:
		return VividBlue.data();
	case EssenceTint::Shadow:
		return Shadow.data();
	}
	return VividBlue.data();
}

std::optional<EssenceTint> GetSpellTint(SpellID spell)
{
	switch (spell) {
	case SpellID::Frostbolt:
		return EssenceTint::VividBlue;
	case SpellID::Corruption:
		return EssenceTint::Shadow;
	case SpellID::FireAura:
		return EssenceTint::VividRed;
	default:
		return std::nullopt;
	}
}

std::optional<EssenceTint> GetItemTint(const Item &item)
{
	if (item._iMiscId != IMISC_AWAKENINGSTONE)
		return std::nullopt;
	return GetSpellTint(item._iSpell);
}

} // namespace devilution
