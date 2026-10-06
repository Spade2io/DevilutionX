/**
 * @file spelldat.h
 *
 * Interface of all spell data.
 */
#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "sound_effect_enums.h"
#include "utils/enum_traits.h"

namespace devilution {

enum class SpellType : uint8_t {
	Skill,
	FIRST = Skill,
	Spell,
	Scroll,
	Charges,
	LAST = Charges,
	Invalid,
};

/**
 * Essence Mod: spell numbers are 16 bits wide (the original game used 8), so there is room for
 * far more than 127. Numbers below LegacySpellLimit are the original spells and the first few the
 * mod added; they use the game's 64-entry tables, which are part of the save file. Numbers from
 * FirstExtendedSpell up are powers read from essence_powers.tsv; what a character knows of those
 * is kept in the sidecar file instead.
 */
enum class SpellID : int16_t {
	Null,
	FIRST = Null,
	Firebolt,
	Healing,
	Lightning,
	Flash,
	Identify,
	FireWall,
	TownPortal,
	StoneCurse,
	Infravision,
	Phasing,
	ManaShield,
	Fireball,
	Guardian,
	ChainLightning,
	FlameWave,
	DoomSerpents,
	BloodRitual,
	Nova,
	Invisibility,
	Inferno,
	Golem,
	Rage,
	Teleport,
	Apocalypse,
	Etherealize,
	ItemRepair,
	StaffRecharge,
	TrapDisarm,
	Elemental,
	ChargedBolt,
	HolyBolt,
	Resurrect,
	Telekinesis,
	HealOther,
	BloodStar,
	BoneSpirit,
	LastDiablo = BoneSpirit,
	Mana,
	Magi,
	Jester,
	LightningWall,
	Immolation,
	Warp,
	Reflect,
	Berserk,
	RingOfFire,
	Search,
	RuneOfFire,
	RuneOfLight,
	RuneOfNova,
	RuneOfImmolation,
	RuneOfStone,
	// Essence Mod: new spells take the unused numbers 52 to 63. Never renumber the spells above.
	Frostbolt,
	Strength,
	Corruption,
	FireAura,
	FlamingWeapon,
	FlameStrike,
	InfernoStrike,

	LAST = InfernoStrike,
	Invalid = -1,
};

/** Spells numbered below this fit the original 64-entry "known spells" switches and level list. */
constexpr int LegacySpellLimit = 64;
/** The first number given to a power read from essence_powers.tsv. */
constexpr int FirstExtendedSpell = 100;

/** Whether a spell is numbered beyond what the original 64-entry tables can hold. */
constexpr bool IsExtendedSpell(SpellID spell)
{
	return static_cast<int>(spell) >= LegacySpellLimit;
}

std::expected<SpellID, std::string> ParseSpellId(std::string_view value);

enum class MagicType : uint8_t {
	Fire,
	Lightning,
	Magic,
};

enum class MissileID : int8_t {
	// clang-format off
	Arrow,
	Firebolt,
	Guardian,
	Phasing,
	NovaBall,
	FireWall,
	Fireball,
	LightningControl,
	Lightning,
	MagmaBallExplosion,
	TownPortal,
	FlashBottom,
	FlashTop,
	ManaShield,
	FlameWave,
	ChainLightning,
	ChainBall, // unused
	BloodHit, // unused
	BoneHit, // unused
	MetalHit, // unused
	Rhino,
	MagmaBall,
	ThinLightningControl,
	ThinLightning,
	BloodStar,
	BloodStarExplosion,
	Teleport,
	FireArrow,
	DoomSerpents, // unused
	FireOnly, // unused
	StoneCurse,
	BloodRitual, // unused
	Invisibility, // unused
	Golem,
	Etherealize,
	Spurt, // unused
	ApocalypseBoom,
	Healing,
	FireWallControl,
	Infravision,
	Identify,
	FlameWaveControl,
	Nova,
	Rage, // BloodBoil in Diablo
	Apocalypse,
	ItemRepair,
	StaffRecharge,
	TrapDisarm,
	Inferno,
	InfernoControl,
	FireMan, // unused
	Krull, // unused
	ChargedBolt,
	HolyBolt,
	Resurrect,
	Telekinesis,
	LightningArrow,
	Acid,
	AcidSplat,
	AcidPuddle,
	HealOther,
	Elemental,
	ResurrectBeam,
	BoneSpirit,
	WeaponExplosion,
	RedPortal,
	DiabloApocalypseBoom,
	DiabloApocalypse,
	LastDiablo = DiabloApocalypse,
	Mana,
	Magi,
	LightningWall,
	LightningWallControl,
	Immolation,
	SpectralArrow,
	FireballBow,
	LightningBow,
	ChargedBoltBow,
	HolyBoltBow,
	Warp,
	Reflect,
	Berserk,
	RingOfFire,
	StealPotions,
	StealMana,
	RingOfLightning, // unused
	Search,
	Aura, // unused
	Aura2, // unused
	SpiralFireball, // unused
	RuneOfFire,
	RuneOfLight,
	RuneOfNova,
	RuneOfImmolation,
	RuneOfStone,
	BigExplosion,
	HorkSpawn,
	Jester,
	OpenNest,
	OrangeFlare,
	BlueFlare,
	RedFlare,
	YellowFlare,
	BlueFlare2,
	YellowExplosion,
	RedExplosion,
	BlueExplosion,
	BlueExplosion2,
	OrangeExplosion,
	// Essence Mod: projectiles for new spells go after the original ones.
	Frostbolt,
	FrostboltExplosion,
	StrengthBuff,
	Corruption,
	CorruptionExplosion,
	FireAuraBuff,
	FireAuraPulse,
	FireAuraPulseBack,
	FlamingWeaponBuff,
	HealingZone,

	LAST = HealingZone,
	Null = -1,
	// clang-format on
};

enum class SpellDataFlags : uint8_t {
	// The lower 2 bytes are used to store MagicType.
	Fire = static_cast<uint8_t>(MagicType::Fire),
	Lightning = static_cast<uint8_t>(MagicType::Lightning),
	Magic = static_cast<uint8_t>(MagicType::Magic),
	Targeted = 1U << 2,
	AllowedInTown = 1U << 3,
};
use_enum_as_flags(SpellDataFlags);

struct SpellData {
	std::string sNameText;
	SfxID sSFX;
	uint16_t bookCost10;
	uint8_t staffCost10;
	uint8_t sManaCost;
	SpellDataFlags flags;
	int8_t sBookLvl;
	int8_t sStaffLvl;
	uint8_t minInt;
	MissileID sMissiles[2];
	uint8_t sManaAdj;
	uint8_t sMinMana;
	uint8_t sStaffMin;
	uint8_t sStaffMax;
	/** Essence Mod: for a power from essence_powers.tsv, the name of the essence it belongs to. */
	std::string essence;
	/** Essence Mod: for a power from essence_powers.tsv, its picture in the spell icon sheet. */
	uint8_t iconFrame = 0;
	/** Essence Mod: for a power from essence_powers.tsv, its cooldown in seconds. Zero means none. */
	uint16_t cooldownSeconds = 0;
	/** Essence Mod: for a power from essence_powers.tsv, the short line the spellbook shows under it. */
	std::string description;
	/**
	 * Essence Mod: for a power from essence_powers.tsv, the effect it applies ("Burn"), how much,
	 * and how far around the target it reaches. The amount's meaning depends on the effect; for a
	 * damage-over-time effect it is damage per tick in 64ths of a hit point. Radius 0 is the target alone.
	 */
	std::string effect;
	int effectAmount = 0;
	uint8_t effectRadius = 0;
	/**
	 * Essence Mod: a second effect that rides along with the first and lands on the main target,
	 * e.g. a weapon strike that also adds Burn. The amount is per tick, in 64ths of a hit point.
	 */
	std::string rider;
	int riderAmount = 0;
	/**
	 * Essence Mod: for a power whose effect is "Buff", the stat it changes ("Damage") and how long
	 * it lasts in seconds. Zero seconds is a lasting buff: it stays until death or the end of the
	 * session. The size of the change is effectAmount, as a plain number (30 is +30%).
	 */
	std::string buffStat;
	uint16_t durationSeconds = 0;
	/** Whether the power can be aimed at another player. Without one under the cursor it lands on the caster. */
	bool targetsAlly = false;

	[[nodiscard]] MagicType type() const
	{
		return static_cast<MagicType>(static_cast<std::underlying_type<SpellDataFlags>::type>(flags) & 0b11U);
	}

	[[nodiscard]] uint32_t bookCost() const
	{
		return bookCost10 * 10;
	}

	[[nodiscard]] uint16_t staffCost() const
	{
		return staffCost10 * 10;
	}

	[[nodiscard]] bool isTargeted() const
	{
		return HasAnyOf(flags, SpellDataFlags::Targeted);
	}

	[[nodiscard]] bool isAllowedInTown() const
	{
		return HasAnyOf(flags, SpellDataFlags::AllowedInTown);
	}
};

extern std::vector<SpellData> SpellsData;

/** How many rows of the spell table belong to the original 64-entry range. */
size_t LegacySpellCount();

inline const SpellData &GetSpellData(SpellID spellId)
{
	return SpellsData[static_cast<std::underlying_type<SpellID>::type>(spellId)];
}

void LoadSpellData();

} // namespace devilution
