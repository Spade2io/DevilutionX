#include "panels/spell_icons.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "cooldowns.h"
#include "engine/load_cel.hpp"
#include "engine/load_clx.hpp"
#include "engine/palette.h"
#include "engine/render/clx_render.hpp"
#include "engine/render/primitive_render.hpp"
#include "game_mode.hpp"

namespace devilution {

namespace {

#ifdef UNPACKED_MPQS
OptionalOwnedClxSpriteList LargeSpellIconsBackground;
OptionalOwnedClxSpriteList SmallSpellIconsBackground;
#endif

OptionalOwnedClxSpriteList SmallSpellIcons;
OptionalOwnedClxSpriteList LargeSpellIcons;

uint8_t SplTransTbl[256];

/** Maps from SpellID to spelicon.cel frame number. */
const SpellIcon SpellITbl[] = {
	// clang-format off
/* SpellID::Null             */ SpellIcon::Empty,
/* SpellID::Firebolt         */ SpellIcon::Firebolt,
/* SpellID::Healing          */ SpellIcon::Healing,
/* SpellID::Lightning        */ SpellIcon::Lightning,
/* SpellID::Flash            */ SpellIcon::Flash,
/* SpellID::Identify         */ SpellIcon::Identify,
/* SpellID::FireWall         */ SpellIcon::FireWall,
/* SpellID::TownPortal       */ SpellIcon::TownPortal,
/* SpellID::StoneCurse       */ SpellIcon::StoneCurse,
/* SpellID::Infravision      */ SpellIcon::Infravision,
/* SpellID::Phasing          */ SpellIcon::Phasing,
/* SpellID::ManaShield       */ SpellIcon::ManaShield,
/* SpellID::Fireball         */ SpellIcon::Fireball,
/* SpellID::Guardian         */ SpellIcon::DoomSerpents,
/* SpellID::ChainLightning   */ SpellIcon::ChainLightning,
/* SpellID::FlameWave        */ SpellIcon::FlameWave,
/* SpellID::DoomSerpents     */ SpellIcon::DoomSerpents,
/* SpellID::BloodRitual      */ SpellIcon::BloodRitual,
/* SpellID::Nova             */ SpellIcon::Nova,
/* SpellID::Invisibility     */ SpellIcon::Invisibility,
/* SpellID::Inferno          */ SpellIcon::Inferno,
/* SpellID::Golem            */ SpellIcon::Golem,
/* SpellID::Rage             */ SpellIcon::BloodBoil,
/* SpellID::Teleport         */ SpellIcon::Teleport,
/* SpellID::Apocalypse       */ SpellIcon::Apocalypse,
/* SpellID::Etherealize      */ SpellIcon::Etherealize,
/* SpellID::ItemRepair       */ SpellIcon::ItemRepair,
/* SpellID::StaffRecharge    */ SpellIcon::StaffRecharge,
/* SpellID::TrapDisarm       */ SpellIcon::TrapDisarm,
/* SpellID::Elemental        */ SpellIcon::Elemental,
/* SpellID::ChargedBolt      */ SpellIcon::ChargedBolt,
/* SpellID::HolyBolt         */ SpellIcon::HolyBolt,
/* SpellID::Resurrect        */ SpellIcon::Resurrect,
/* SpellID::Telekinesis      */ SpellIcon::Telekinesis,
/* SpellID::HealOther        */ SpellIcon::HealOther,
/* SpellID::BloodStar        */ SpellIcon::BloodStar,
/* SpellID::BoneSpirit       */ SpellIcon::BoneSpirit,
/* SpellID::Mana             */ SpellIcon::Mana,
/* SpellID::Magi             */ SpellIcon::Mana,
/* SpellID::Jester           */ SpellIcon::Jester,
/* SpellID::LightningWall    */ SpellIcon::LightningWall,
/* SpellID::Immolation       */ SpellIcon::Immolation,
/* SpellID::Warp             */ SpellIcon::Warp,
/* SpellID::Reflect          */ SpellIcon::Reflect,
/* SpellID::Berserk          */ SpellIcon::Berserk,
/* SpellID::RingOfFire       */ SpellIcon::RingOfFire,
/* SpellID::Search           */ SpellIcon::Search,
/* SpellID::RuneOfFire       */ SpellIcon::PentaStar,
/* SpellID::RuneOfLight      */ SpellIcon::PentaStar,
/* SpellID::RuneOfNova       */ SpellIcon::PentaStar,
/* SpellID::RuneOfImmolation */ SpellIcon::PentaStar,
/* SpellID::RuneOfStone      */ SpellIcon::PentaStar,
/* SpellID::Frostbolt        */ SpellIcon::Firebolt,
/* SpellID::Strength         */ SpellIcon::Berserk, // icon 47
/* SpellID::Corruption       */ SpellIcon::RedSkull, // icon 30, unused by vanilla
/* SpellID::FireAura         */ SpellIcon::FireCloud, // icon 32, unused by vanilla
/* SpellID::FlamingWeapon    */ SpellIcon::Pentagram, // icon 31, unused by vanilla
/* SpellID::FlameStrike      */ SpellIcon::LongHorn,  // icon 33, unused by vanilla
/* SpellID::InfernoStrike    */ SpellIcon::PentaStar, // icon 34, used only by runes
	// clang-format on
};

} // namespace

std::expected<void, std::string> LoadLargeSpellIcons()
{
#ifdef UNPACKED_MPQS
	LargeSpellIcons = LoadOptionalClx("data\\spelicon_fg.clx");
	LargeSpellIconsBackground = LoadOptionalClx("data\\spelicon_bg.clx");
	if (!LargeSpellIcons.has_value() || !LargeSpellIconsBackground.has_value()) {
		ASSIGN_OR_RETURN(LargeSpellIcons, LoadClxWithStatus("ctrlpan\\spelicon_fg.clx"));
		ASSIGN_OR_RETURN(LargeSpellIconsBackground, LoadClxWithStatus("ctrlpan\\spelicon_bg.clx"));
	}
#else
	LargeSpellIcons = LoadOptionalCel("data\\spelicon", SPLICONLENGTH);
	if (!LargeSpellIcons.has_value()) {
		ASSIGN_OR_RETURN(LargeSpellIcons, LoadCelWithStatus("ctrlpan\\spelicon", SPLICONLENGTH));
	}
#endif
	SetSpellTrans(SpellType::Skill);
	return {};
}

void FreeLargeSpellIcons()
{
#ifdef UNPACKED_MPQS
	LargeSpellIconsBackground = std::nullopt;
#endif
	LargeSpellIcons = std::nullopt;
}

std::expected<void, std::string> LoadSmallSpellIcons()
{
#ifdef UNPACKED_MPQS
	ASSIGN_OR_RETURN(SmallSpellIcons, LoadClxWithStatus("data\\spelli2_fg.clx"));
	ASSIGN_OR_RETURN(SmallSpellIconsBackground, LoadClxWithStatus("data\\spelli2_bg.clx"));
#else
	ASSIGN_OR_RETURN(SmallSpellIcons, LoadCelWithStatus("data\\spelli2", 37));
#endif
	return {};
}

void FreeSmallSpellIcons()
{
#ifdef UNPACKED_MPQS
	SmallSpellIconsBackground = std::nullopt;
#endif
	SmallSpellIcons = std::nullopt;
}

uint8_t GetSpellIconFrame(SpellID spell)
{
	// Essence Mod: a power read from essence_powers.tsv names its picture in its own row.
	if (IsExtendedSpell(spell))
		return GetSpellData(spell).iconFrame;
	return static_cast<uint8_t>(SpellITbl[static_cast<size_t>(spell)]);
}

namespace {

/** Essence Mod: the colours an icon is drawn in while its ability is on cooldown: a washed-out red. */
const uint8_t *GetCooldownTrn()
{
	static const std::array<uint8_t, 256> Table = [] {
		std::array<uint8_t, 256> table;
		for (size_t i = 0; i < table.size(); i++)
			table[i] = static_cast<uint8_t>(i);
		table[255] = 0;
		table[PAL8_YELLOW] = PAL16_RED + 1;
		table[PAL8_YELLOW + 1] = PAL16_RED + 3;
		table[PAL8_YELLOW + 2] = PAL16_RED + 5;
		for (int shade = 0; shade < 16; shade++) {
			table[PAL16_BEIGE + shade] = PAL16_RED + shade;
			table[PAL16_YELLOW + shade] = PAL16_RED + shade;
			table[PAL16_ORANGE + shade] = PAL16_RED + shade;
			table[PAL16_BLUE + shade] = PAL16_RED + shade;
		}
		return table;
	}();
	return Table.data();
}

/** A scratch picture the size of one icon, reused between frames. */
Surface &ScratchSurface(int width, int height)
{
	static std::optional<OwnedSurface> scratch;
	if (!scratch || scratch->w() != width || scratch->h() != height)
		scratch.emplace(width, height);
	return *scratch;
}

/**
 * @brief Essence Mod: draws the cooldown "clock" over an icon that has just been drawn.
 *
 * The part of the icon the clock hand has not yet swept is redrawn in the cooldown colours. The
 * hand starts at 12 o'clock and turns clockwise, so the icon returns to its usable colour as the
 * cooldown runs out.
 */
void OverlayCooldownClock(const Surface &out, Point position, ClxSprite sprite, SpellID spell)
{
	const float progress = GetSpellCooldownProgress(spell);
	if (progress >= 1.0F)
		return;

	const int width = sprite.width();
	const int height = sprite.height();
	Surface &scratch = ScratchSurface(width, height);
	ClxDrawTRN(scratch, { 0, height - 1 }, sprite, GetCooldownTrn());

	constexpr float FullTurn = 6.2831853F;
	const float sweptAngle = progress * FullTurn;
	const Point topLeft { position.x, position.y - height + 1 };
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			const float dx = static_cast<float>(x) + 0.5F - static_cast<float>(width) / 2;
			const float dy = static_cast<float>(y) + 0.5F - static_cast<float>(height) / 2;
			// Angle measured clockwise from straight up.
			float angle = std::atan2(dx, -dy);
			if (angle < 0)
				angle += FullTurn;
			if (angle <= sweptAngle)
				continue;
			const Point target { topLeft.x + x, topLeft.y + y };
			if (out.InBounds(target))
				*out.at(target.x, target.y) = *scratch.at(x, y);
		}
	}
}

} // namespace

void DrawLargeSpellIcon(const Surface &out, Point position, SpellID spell)
{
#ifdef UNPACKED_MPQS
	ClxDrawTRN(out, position, (*LargeSpellIconsBackground)[0], SplTransTbl);
#endif
	const ClxSprite sprite = (*LargeSpellIcons)[GetSpellIconFrame(spell)];
	ClxDrawTRN(out, position, sprite, SplTransTbl);
	OverlayCooldownClock(out, position, sprite, spell);
}

void DrawSmallSpellIcon(const Surface &out, Point position, SpellID spell)
{
#ifdef UNPACKED_MPQS
	ClxDrawTRN(out, position, (*SmallSpellIconsBackground)[0], SplTransTbl);
#endif
	const ClxSprite sprite = (*SmallSpellIcons)[GetSpellIconFrame(spell)];
	ClxDrawTRN(out, position, sprite, SplTransTbl);
	OverlayCooldownClock(out, position, sprite, spell);
}

void DrawSmallSpellIconScaled(const Surface &out, Point position, SpellID spell, int percent)
{
	const ClxSprite sprite = (*SmallSpellIcons)[GetSpellIconFrame(spell)];
	const int width = sprite.width();
	const int height = sprite.height();
	Surface &scratch = ScratchSurface(width, height);
	ClxDrawTRN(scratch, { 0, height - 1 }, sprite, SplTransTbl);

	// Grow from the icon's centre, so it swells in place.
	const int scaledWidth = width * percent / 100;
	const int scaledHeight = height * percent / 100;
	const Point topLeft { position.x - (scaledWidth - width) / 2, position.y - height + 1 - (scaledHeight - height) / 2 };
	for (int y = 0; y < scaledHeight; y++) {
		for (int x = 0; x < scaledWidth; x++) {
			const Point target { topLeft.x + x, topLeft.y + y };
			if (out.InBounds(target))
				*out.at(target.x, target.y) = *scratch.at(x * width / scaledWidth, y * height / scaledHeight);
		}
	}
}

void DrawLargeSpellIconBorder(const Surface &out, Point position, uint8_t color)
{
	const int width = (*LargeSpellIcons)[0].width();
	const int height = (*LargeSpellIcons)[0].height();
	UnsafeDrawBorder2px(out, Rectangle { Point { position.x, position.y - height + 1 }, Size { width, height } }, color);
}

void DrawSmallSpellIconBorder(const Surface &out, Point position)
{
	const int width = (*SmallSpellIcons)[0].width();
	const int height = (*SmallSpellIcons)[0].height();
	UnsafeDrawBorder2px(out, Rectangle { Point { position.x, position.y - height + 1 }, Size { width, height } }, SplTransTbl[PAL8_YELLOW + 2]);
}

void SetSpellTrans(SpellType t)
{
	if (t == SpellType::Skill) {
		for (int i = 0; i < 128; i++)
			SplTransTbl[i] = i;
	}
	for (int i = 128; i < 256; i++)
		SplTransTbl[i] = i;
	SplTransTbl[255] = 0;

	switch (t) {
	case SpellType::Spell:
		SplTransTbl[PAL8_YELLOW] = PAL16_BLUE + 1;
		SplTransTbl[PAL8_YELLOW + 1] = PAL16_BLUE + 3;
		SplTransTbl[PAL8_YELLOW + 2] = PAL16_BLUE + 5;
		for (int i = PAL16_BLUE; i < PAL16_BLUE + 16; i++) {
			SplTransTbl[PAL16_BEIGE - PAL16_BLUE + i] = i;
			SplTransTbl[PAL16_YELLOW - PAL16_BLUE + i] = i;
			SplTransTbl[PAL16_ORANGE - PAL16_BLUE + i] = i;
		}
		break;
	case SpellType::Scroll:
		SplTransTbl[PAL8_YELLOW] = PAL16_BEIGE + 1;
		SplTransTbl[PAL8_YELLOW + 1] = PAL16_BEIGE + 3;
		SplTransTbl[PAL8_YELLOW + 2] = PAL16_BEIGE + 5;
		for (int i = PAL16_BEIGE; i < PAL16_BEIGE + 16; i++) {
			SplTransTbl[PAL16_YELLOW - PAL16_BEIGE + i] = i;
			SplTransTbl[PAL16_ORANGE - PAL16_BEIGE + i] = i;
		}
		break;
	case SpellType::Charges:
		SplTransTbl[PAL8_YELLOW] = PAL16_ORANGE + 1;
		SplTransTbl[PAL8_YELLOW + 1] = PAL16_ORANGE + 3;
		SplTransTbl[PAL8_YELLOW + 2] = PAL16_ORANGE + 5;
		for (int i = PAL16_ORANGE; i < PAL16_ORANGE + 16; i++) {
			SplTransTbl[PAL16_BEIGE - PAL16_ORANGE + i] = i;
			SplTransTbl[PAL16_YELLOW - PAL16_ORANGE + i] = i;
		}
		break;
	case SpellType::Invalid:
		SplTransTbl[PAL8_YELLOW] = PAL16_GRAY + 1;
		SplTransTbl[PAL8_YELLOW + 1] = PAL16_GRAY + 3;
		SplTransTbl[PAL8_YELLOW + 2] = PAL16_GRAY + 5;
		for (int i = PAL16_GRAY; i < PAL16_GRAY + 15; i++) {
			SplTransTbl[PAL16_BEIGE - PAL16_GRAY + i] = i;
			SplTransTbl[PAL16_YELLOW - PAL16_GRAY + i] = i;
			SplTransTbl[PAL16_ORANGE - PAL16_GRAY + i] = i;
		}
		SplTransTbl[PAL16_BEIGE + 15] = 0;
		SplTransTbl[PAL16_YELLOW + 15] = 0;
		SplTransTbl[PAL16_ORANGE + 15] = 0;
		break;
	case SpellType::Skill:
		break;
	}
}

} // namespace devilution
