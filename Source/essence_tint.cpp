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
#include "engine/render/clx_render.hpp"
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

/**
 * @brief Builds a table that moves every shared ramp onto one of the soft 16-shade ramps.
 * An 8-shade ramp has half the shading steps, so its shades land on every other target shade.
 */
TintTable MakeEverythingToSixteenShadeRamp(uint8_t targetRamp)
{
	TintTable table = MakeIdentity();

	for (const uint8_t ramp : { PAL8_BLUE, PAL8_RED, PAL8_YELLOW, PAL8_ORANGE }) {
		for (uint8_t shade = 0; shade < 8; shade++)
			table[ramp + shade] = targetRamp + shade * 2;
	}
	for (const uint8_t ramp : { PAL16_BEIGE, PAL16_BLUE, PAL16_YELLOW, PAL16_ORANGE, PAL16_RED, PAL16_GRAY }) {
		for (uint8_t shade = 0; shade < 16; shade++)
			table[ramp + shade] = targetRamp + shade;
	}
	return table;
}

} // namespace

const uint8_t *GetEssenceTintTrn(EssenceTint tint)
{
	static const TintTable DullRed = MakeEverythingToSixteenShadeRamp(PAL16_RED);
	static const TintTable DullOrange = MakeEverythingToSixteenShadeRamp(PAL16_ORANGE);
	static const TintTable DullYellow = MakeEverythingToSixteenShadeRamp(PAL16_YELLOW);
	static const TintTable DullBlue = MakeEverythingToSixteenShadeRamp(PAL16_BLUE);
	static const TintTable DullBeige = MakeEverythingToSixteenShadeRamp(PAL16_BEIGE);
	static const TintTable DullGray = MakeEverythingToSixteenShadeRamp(PAL16_GRAY);
	static const TintTable VividOrange = MakeEverythingToEightShadeRamp(PAL8_ORANGE);
	// Only the light half of the 16 greys is used, so the whole sprite comes out pale.
	static const TintTable White = [] {
		TintTable table = MakeEverythingToSixteenShadeRamp(PAL16_GRAY);
		for (const uint8_t ramp : { PAL8_BLUE, PAL8_RED, PAL8_YELLOW, PAL8_ORANGE }) {
			for (uint8_t shade = 0; shade < 8; shade++)
				table[ramp + shade] = PAL16_GRAY + shade;
		}
		for (const uint8_t ramp : { PAL16_BEIGE, PAL16_BLUE, PAL16_YELLOW, PAL16_ORANGE, PAL16_RED, PAL16_GRAY }) {
			for (uint8_t shade = 0; shade < 16; shade++)
				table[ramp + shade] = PAL16_GRAY + shade / 2;
		}
		return table;
	}();
	static const TintTable VividRed = MakeEverythingToEightShadeRamp(PAL8_RED);
	static const TintTable VividYellow = MakeEverythingToEightShadeRamp(PAL8_YELLOW);
	static const TintTable VividBlue = MakeWarmToEightShadeRamp(PAL8_BLUE);
	// The slate-blue ramp is the nearest the shared palette has to purple. Its darker two-thirds read as shadow.
	static const TintTable Shadow = MakeEverythingToDarkRamp(PAL16_BLUE, 5);

	switch (tint) {
	case EssenceTint::VividYellow:
		return VividYellow.data();
	case EssenceTint::VividRed:
		return VividRed.data();
	case EssenceTint::VividBlue:
		return VividBlue.data();
	case EssenceTint::Shadow:
		return Shadow.data();
	case EssenceTint::DullRed:
		return DullRed.data();
	case EssenceTint::DullOrange:
		return DullOrange.data();
	case EssenceTint::DullYellow:
		return DullYellow.data();
	case EssenceTint::DullBlue:
		return DullBlue.data();
	case EssenceTint::DullBeige:
		return DullBeige.data();
	case EssenceTint::DullGray:
		return DullGray.data();
	case EssenceTint::VividOrange:
		return VividOrange.data();
	case EssenceTint::White:
		return White.data();
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
	case SpellID::FlamingWeapon:
	case SpellID::FlameStrike:
	case SpellID::InfernoStrike:
		return EssenceTint::VividRed;
	default:
		break;
	}
	// A power read from essence_powers.tsv takes its colour from its element.
	if (IsExtendedSpell(spell) && static_cast<size_t>(spell) < SpellsData.size()) {
		switch (GetSpellData(spell).type()) {
		case MagicType::Fire:
			return EssenceTint::VividRed;
		case MagicType::Lightning:
			return EssenceTint::VividBlue;
		default:
			// Anything else takes its essence's colour.
			// Dark's effects are the deep shadow tint, darker than the dull blue of its items.
			if (GetSpellEssence(spell) == EssenceID::Dark)
				return EssenceTint::Shadow;
			return GetEssenceTint(GetSpellEssence(spell));
		}
	}
	return std::nullopt;
}

DamageType GetSpellDamageType(SpellID spell)
{
	switch (spell) {
	case SpellID::Frostbolt:
		return DamageType::Ice;
	case SpellID::Corruption:
		return DamageType::Shadow;
	default:
		break;
	}
	switch (GetSpellEssence(spell)) {
	case EssenceID::Fire:
		return DamageType::Fire;
	case EssenceID::Lightning:
		return DamageType::Lightning;
	case EssenceID::Water:
		return DamageType::Ice;
	case EssenceID::Holy:
		return DamageType::Holy;
	case EssenceID::Nature:
		return DamageType::Nature;
	case EssenceID::Shield:
		return DamageType::Physical;
	case EssenceID::Dark:
		return DamageType::Shadow;
	case EssenceID::Earth:
		return DamageType::Earth;
	case EssenceID::Life:
		return DamageType::Life;
	case EssenceID::Magic:
		return DamageType::Arcane;
	case EssenceID::Might: // its powers are Physical; only its stone is red
		return DamageType::Physical;
	case EssenceID::Blood:
		return DamageType::Bleed;
	case EssenceID::None:
		break;
	}
	if (static_cast<size_t>(spell) < SpellsData.size()) {
		switch (GetSpellData(spell).type()) {
		case MagicType::Fire:
			return DamageType::Fire;
		case MagicType::Lightning:
			return DamageType::Lightning;
		default:
			break;
		}
	}
	return DamageType::Magic;
}

UiFlags GetDamageTypeTextColor(DamageType damageType)
{
	switch (damageType) {
	case DamageType::Physical:
		return UiFlags::ColorGold;
	case DamageType::Fire:
		return UiFlags::ColorBrightRed;
	case DamageType::Lightning:
		return UiFlags::ColorBlue;
	case DamageType::Magic:
		return UiFlags::ColorOrange;
	case DamageType::Acid:
		return UiFlags::ColorYellow;
	case DamageType::Shadow:
		return UiFlags::ColorBlue;
	case DamageType::Ice:
		return UiFlags::ColorIce;
	case DamageType::Holy:
		return UiFlags::ColorYellow;
	case DamageType::Earth:
		return UiFlags::ColorDullBeige;
	case DamageType::Bleed:
		return UiFlags::ColorUiSilver; // drawn dull red in the game
	case DamageType::Poison:
		return UiFlags::ColorYellow;
	case DamageType::Death:
		return UiFlags::ColorBlue;
	case DamageType::Life:
		return UiFlags::ColorOrange;
	case DamageType::Nature:
		return UiFlags::ColorGold; // the lettering's own dull yellow
	case DamageType::Arcane:
		return UiFlags::ColorUiSilver; // drawn dull red in the game, as Bleed is
	}
	return UiFlags::ColorWhitegold;
}

UiFlags GetSpellTextColor(SpellID spell)
{
	return GetDamageTypeTextColor(GetSpellDamageType(spell));
}

std::optional<EssenceTint> GetItemTint(const Item &item)
{
	if (item._iMiscId == IMISC_ESSENCE)
		return GetEssenceTint(GetSpellEssence(item._iSpell));
	if (item._iMiscId != IMISC_AWAKENINGSTONE)
		return std::nullopt;
	// A stone is the colour of its essence, the same as the essence's own item. Only a stone
	// whose ability has no essence yet falls back on the ability's own colour.
	if (const std::optional<EssenceTint> tint = GetEssenceTint(GetSpellEssence(item._iSpell)); tint)
		return tint;
	return GetSpellTint(item._iSpell);
}

bool DrawItemTinted(const Surface &out, Point position, ClxSprite sprite, const Item &item)
{
	// An awakening stone takes the colour of its rarity; anything else, the colour it had before.
	std::optional<EssenceTint> first = GetItemTint(item);
	std::optional<EssenceTint> second;
	switch (GetAwakeningStoneRarity(item)) {
	case 0:
		first = EssenceTint::White;
		break;
	case 1:
		first = EssenceTint::VividBlue;
		break;
	case 2:
		first = EssenceTint::VividRed;
		second = EssenceTint::VividBlue;
		break;
	case 3:
		first = EssenceTint::VividOrange;
		break;
	default:
		break;
	}
	if (!first)
		return false;
	if (!second) {
		ClxDrawTRN(out, position, sprite, GetEssenceTintTrn(*first));
		return true;
	}

	// Two colours in a checkerboard. The second is drawn on a copy of what is already on the
	// screen there; the first is drawn on the screen; then every other pixel is taken from the
	// copy. Where the sprite is see-through both hold the same background, so nothing shows.
	const int width = sprite.width();
	const int height = sprite.height();
	static std::optional<OwnedSurface> scratch;
	if (!scratch || scratch->w() != width || scratch->h() != height)
		scratch.emplace(width, height);
	const Point topLeft { position.x, position.y - height + 1 };
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			const Point at { topLeft.x + x, topLeft.y + y };
			*scratch->at(x, y) = out.InBounds(at) ? *out.at(at.x, at.y) : 0;
		}
	}
	ClxDrawTRN(*scratch, { 0, height - 1 }, sprite, GetEssenceTintTrn(*second));
	ClxDrawTRN(out, position, sprite, GetEssenceTintTrn(*first));
	for (int y = 0; y < height; y++) {
		for (int x = (y & 1); x < width; x += 2) {
			const Point at { topLeft.x + x, topLeft.y + y };
			if (out.InBounds(at))
				*out.at(at.x, at.y) = *scratch->at(x, y);
		}
	}
	return true;
}

UiFlags GetAwakeningStoneTextColor(const Item &item)
{
	switch (GetAwakeningStoneRarity(item)) {
	case 0:
		return UiFlags::ColorWhite;
	case 1:
		return UiFlags::ColorBlue;
	case 2:
		return UiFlags::ColorDullBeige;
	case 3:
		return UiFlags::ColorOrange;
	default:
		return UiFlags::ColorWhite;
	}
}

std::optional<EssenceTint> GetEssenceTint(EssenceID essence)
{
	switch (essence) {
	case EssenceID::Fire:
		return EssenceTint::VividRed;
	// These are the item colours the Essence Designer gives each essence's damage type.
	case EssenceID::Lightning:
		return EssenceTint::DullBlue;
	case EssenceID::Water:
		return EssenceTint::VividBlue;
	case EssenceID::Holy:
		return EssenceTint::VividYellow;
	case EssenceID::Nature:
		return EssenceTint::DullYellow;
	case EssenceID::Shield: // Physical
		return EssenceTint::DullYellow;
	case EssenceID::Dark: // Shadow
		return EssenceTint::DullBlue;
	case EssenceID::Earth:
		return EssenceTint::DullBeige;
	case EssenceID::Life:
		return EssenceTint::VividOrange;
	case EssenceID::Magic: // Arcane
		return EssenceTint::DullBeige;
	case EssenceID::Might: // the designer gives it Fire's colour
		return EssenceTint::VividRed;
	case EssenceID::Blood: // Bleed
		return EssenceTint::DullRed;
	case EssenceID::None:
		break;
	}
	return std::nullopt;
}

} // namespace devilution
