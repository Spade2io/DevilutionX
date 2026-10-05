/**
 * @file spelldat.cpp
 *
 * Implementation of all spell data.
 */
#include "tables/spelldat.h"

#include <algorithm>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "appfat.h"
#include "data/file.hpp"
#include "data/iterators.hpp"
#include "data/record_reader.hpp"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

void AddNullSpell()
{
	SpellData &null = SpellsData.emplace_back();
	null.sSFX = SfxID::None;
	null.bookCost10 = null.staffCost10 = null.sManaCost = 0;
	null.flags = SpellDataFlags::Fire;
	null.sBookLvl = null.sStaffLvl = 0;
	null.minInt = 0;
	null.sMissiles[0] = null.sMissiles[1] = MissileID::Null;
	null.sManaAdj = null.sMinMana = 0;
	null.sStaffMin = 40;
	null.sStaffMax = 80;
}

// A temporary solution for parsing soundID until we have a more general one.
std::expected<SfxID, std::string> ParseSpellSoundId(std::string_view value)
{
	if (value == "CastFire") return SfxID::CastFire;
	if (value == "CastHealing") return SfxID::CastHealing;
	if (value == "CastLightning") return SfxID::CastLightning;
	if (value == "CastSkill") return SfxID::CastSkill;
	return std::unexpected("Unknown enum value (only a few are supported for now)");
}

std::expected<SpellDataFlags, std::string> ParseSpellDataFlag(std::string_view value)
{
	if (value == "Fire") return SpellDataFlags::Fire;
	if (value == "Lightning") return SpellDataFlags::Lightning;
	if (value == "Magic") return SpellDataFlags::Magic;
	if (value == "Targeted") return SpellDataFlags::Targeted;
	if (value == "AllowedInTown") return SpellDataFlags::AllowedInTown;
	return std::unexpected("Unknown enum value");
}

std::expected<MissileID, std::string> ParseMissileId(std::string_view value)
{
	if (value == "Arrow") return MissileID::Arrow;
	if (value == "Firebolt") return MissileID::Firebolt;
	if (value == "Guardian") return MissileID::Guardian;
	if (value == "Phasing") return MissileID::Phasing;
	if (value == "NovaBall") return MissileID::NovaBall;
	if (value == "FireWall") return MissileID::FireWall;
	if (value == "Fireball") return MissileID::Fireball;
	if (value == "LightningControl") return MissileID::LightningControl;
	if (value == "Lightning") return MissileID::Lightning;
	if (value == "MagmaBallExplosion") return MissileID::MagmaBallExplosion;
	if (value == "TownPortal") return MissileID::TownPortal;
	if (value == "FlashBottom") return MissileID::FlashBottom;
	if (value == "FlashTop") return MissileID::FlashTop;
	if (value == "ManaShield") return MissileID::ManaShield;
	if (value == "FlameWave") return MissileID::FlameWave;
	if (value == "ChainLightning") return MissileID::ChainLightning;
	if (value == "ChainBall") return MissileID::ChainBall;
	if (value == "BloodHit") return MissileID::BloodHit;
	if (value == "BoneHit") return MissileID::BoneHit;
	if (value == "MetalHit") return MissileID::MetalHit;
	if (value == "Rhino") return MissileID::Rhino;
	if (value == "MagmaBall") return MissileID::MagmaBall;
	if (value == "ThinLightningControl") return MissileID::ThinLightningControl;
	if (value == "ThinLightning") return MissileID::ThinLightning;
	if (value == "BloodStar") return MissileID::BloodStar;
	if (value == "BloodStarExplosion") return MissileID::BloodStarExplosion;
	if (value == "Teleport") return MissileID::Teleport;
	if (value == "FireArrow") return MissileID::FireArrow;
	if (value == "DoomSerpents") return MissileID::DoomSerpents;
	if (value == "FireOnly") return MissileID::FireOnly;
	if (value == "StoneCurse") return MissileID::StoneCurse;
	if (value == "BloodRitual") return MissileID::BloodRitual;
	if (value == "Invisibility") return MissileID::Invisibility;
	if (value == "Golem") return MissileID::Golem;
	if (value == "Etherealize") return MissileID::Etherealize;
	if (value == "Spurt") return MissileID::Spurt;
	if (value == "ApocalypseBoom") return MissileID::ApocalypseBoom;
	if (value == "Healing") return MissileID::Healing;
	if (value == "FireWallControl") return MissileID::FireWallControl;
	if (value == "Infravision") return MissileID::Infravision;
	if (value == "Identify") return MissileID::Identify;
	if (value == "FlameWaveControl") return MissileID::FlameWaveControl;
	if (value == "Nova") return MissileID::Nova;
	if (value == "Rage") return MissileID::Rage;
	if (value == "Apocalypse") return MissileID::Apocalypse;
	if (value == "ItemRepair") return MissileID::ItemRepair;
	if (value == "StaffRecharge") return MissileID::StaffRecharge;
	if (value == "TrapDisarm") return MissileID::TrapDisarm;
	if (value == "Inferno") return MissileID::Inferno;
	if (value == "InfernoControl") return MissileID::InfernoControl;
	if (value == "FireMan") return MissileID::FireMan;
	if (value == "Krull") return MissileID::Krull;
	if (value == "ChargedBolt") return MissileID::ChargedBolt;
	if (value == "HolyBolt") return MissileID::HolyBolt;
	if (value == "Resurrect") return MissileID::Resurrect;
	if (value == "Telekinesis") return MissileID::Telekinesis;
	if (value == "LightningArrow") return MissileID::LightningArrow;
	if (value == "Acid") return MissileID::Acid;
	if (value == "AcidSplat") return MissileID::AcidSplat;
	if (value == "AcidPuddle") return MissileID::AcidPuddle;
	if (value == "HealOther") return MissileID::HealOther;
	if (value == "Elemental") return MissileID::Elemental;
	if (value == "ResurrectBeam") return MissileID::ResurrectBeam;
	if (value == "BoneSpirit") return MissileID::BoneSpirit;
	if (value == "WeaponExplosion") return MissileID::WeaponExplosion;
	if (value == "RedPortal") return MissileID::RedPortal;
	if (value == "DiabloApocalypseBoom") return MissileID::DiabloApocalypseBoom;
	if (value == "DiabloApocalypse") return MissileID::DiabloApocalypse;
	if (value == "Mana") return MissileID::Mana;
	if (value == "Magi") return MissileID::Magi;
	if (value == "LightningWall") return MissileID::LightningWall;
	if (value == "LightningWallControl") return MissileID::LightningWallControl;
	if (value == "Immolation") return MissileID::Immolation;
	if (value == "SpectralArrow") return MissileID::SpectralArrow;
	if (value == "FireballBow") return MissileID::FireballBow;
	if (value == "LightningBow") return MissileID::LightningBow;
	if (value == "ChargedBoltBow") return MissileID::ChargedBoltBow;
	if (value == "HolyBoltBow") return MissileID::HolyBoltBow;
	if (value == "Warp") return MissileID::Warp;
	if (value == "Reflect") return MissileID::Reflect;
	if (value == "Berserk") return MissileID::Berserk;
	if (value == "RingOfFire") return MissileID::RingOfFire;
	if (value == "StealPotions") return MissileID::StealPotions;
	if (value == "StealMana") return MissileID::StealMana;
	if (value == "RingOfLightning") return MissileID::RingOfLightning;
	if (value == "Search") return MissileID::Search;
	if (value == "Aura") return MissileID::Aura;
	if (value == "Aura2") return MissileID::Aura2;
	if (value == "SpiralFireball") return MissileID::SpiralFireball;
	if (value == "RuneOfFire") return MissileID::RuneOfFire;
	if (value == "RuneOfLight") return MissileID::RuneOfLight;
	if (value == "RuneOfNova") return MissileID::RuneOfNova;
	if (value == "RuneOfImmolation") return MissileID::RuneOfImmolation;
	if (value == "RuneOfStone") return MissileID::RuneOfStone;
	if (value == "BigExplosion") return MissileID::BigExplosion;
	if (value == "HorkSpawn") return MissileID::HorkSpawn;
	if (value == "Jester") return MissileID::Jester;
	if (value == "OpenNest") return MissileID::OpenNest;
	if (value == "OrangeFlare") return MissileID::OrangeFlare;
	if (value == "BlueFlare") return MissileID::BlueFlare;
	if (value == "RedFlare") return MissileID::RedFlare;
	if (value == "YellowFlare") return MissileID::YellowFlare;
	if (value == "BlueFlare2") return MissileID::BlueFlare2;
	if (value == "YellowExplosion") return MissileID::YellowExplosion;
	if (value == "RedExplosion") return MissileID::RedExplosion;
	if (value == "BlueExplosion") return MissileID::BlueExplosion;
	if (value == "BlueExplosion2") return MissileID::BlueExplosion2;
	if (value == "OrangeExplosion") return MissileID::OrangeExplosion;
	if (value == "Frostbolt") return MissileID::Frostbolt;
	if (value == "FrostboltExplosion") return MissileID::FrostboltExplosion;
	if (value == "StrengthBuff") return MissileID::StrengthBuff;
	if (value == "Corruption") return MissileID::Corruption;
	if (value == "CorruptionExplosion") return MissileID::CorruptionExplosion;
	if (value == "FireAuraBuff") return MissileID::FireAuraBuff;
	if (value == "FireAuraPulse") return MissileID::FireAuraPulse;
	if (value == "FireAuraPulseBack") return MissileID::FireAuraPulseBack;
	if (value == "FlamingWeaponBuff") return MissileID::FlamingWeaponBuff;
	return std::unexpected("Unknown enum value");
}

/** Essence Mod: the keys of powers read from essence_powers.tsv, so other tables can name them. */
std::vector<std::pair<std::string, SpellID>> ExtendedSpellKeys;

/** Fills the unused numbers between the original spells and the extended powers with blank rows. */
void AddGapSpell()
{
	AddNullSpell();
	SpellData &gap = SpellsData.back();
	gap.sBookLvl = gap.sStaffLvl = -1;
}

/**
 * Essence Mod: reads the powers that live beyond the original 64. Each row carries its own number,
 * so numbers stay fixed however the file is sorted. The file is optional.
 */
void LoadExtendedSpellData()
{
	ExtendedSpellKeys.clear();
	const std::string_view filename = "txtdata\\spells\\essence_powers.tsv";
	std::expected<DataFile, DataFile::Error> dataFileResult = DataFile::load(filename);
	if (!dataFileResult.has_value())
		return;
	DataFile &dataFile = dataFileResult.value();
	dataFile.skipHeaderOrDie(filename);
	for (DataFileRecord record : dataFile) {
		RecordReader reader { record, filename };
		std::string key;
		int number = 0;
		reader.readString("id", key);
		reader.readInt("number", number);
		if (number < FirstExtendedSpell || number > 32000)
			app_fatal(StrCat("essence_powers.tsv: ", key, " has number ", number, "; it must be ", FirstExtendedSpell, " or more"));
		while (SpellsData.size() <= static_cast<size_t>(number))
			AddGapSpell();
		SpellData &item = SpellsData[number];
		if (!item.sNameText.empty())
			app_fatal(StrCat("essence_powers.tsv: number ", number, " is used twice"));
		reader.readString("name", item.sNameText);
		reader.readString("essence", item.essence);
		reader.read("soundId", item.sSFX, ParseSpellSoundId);
		reader.readInt("manaCost", item.sManaCost);
		reader.readEnumList("flags", item.flags, ParseSpellDataFlag);
		reader.readEnumArray("missiles", /*fillMissing=*/std::make_optional(MissileID::Null), item.sMissiles, ParseMissileId);
		reader.readInt("manaMultiplier", item.sManaAdj);
		reader.readInt("minMana", item.sMinMana);
		reader.readInt("icon", item.iconFrame);
		reader.readInt("cooldown", item.cooldownSeconds);
		reader.readString("description", item.description);
		reader.readString("effect", item.effect);
		reader.readInt("amount", item.effectAmount);
		reader.readInt("radius", item.effectRadius);
		reader.readString("rider", item.rider);
		reader.readInt("riderAmount", item.riderAmount);
		item.sBookLvl = 1;
		ExtendedSpellKeys.emplace_back(std::move(key), static_cast<SpellID>(number));
	}
}

} // namespace

/** Data related to each spell ID. */
std::vector<SpellData> SpellsData;

size_t LegacySpellCount()
{
	return std::min<size_t>(SpellsData.size(), LegacySpellLimit);
}

std::expected<SpellID, std::string> ParseSpellId(std::string_view value)
{
	if (value == "Null") return SpellID::Null;
	if (value == "Firebolt") return SpellID::Firebolt;
	if (value == "Healing") return SpellID::Healing;
	if (value == "Lightning") return SpellID::Lightning;
	if (value == "Flash") return SpellID::Flash;
	if (value == "Identify") return SpellID::Identify;
	if (value == "FireWall") return SpellID::FireWall;
	if (value == "TownPortal") return SpellID::TownPortal;
	if (value == "StoneCurse") return SpellID::StoneCurse;
	if (value == "Infravision") return SpellID::Infravision;
	if (value == "Phasing") return SpellID::Phasing;
	if (value == "ManaShield") return SpellID::ManaShield;
	if (value == "Fireball") return SpellID::Fireball;
	if (value == "Guardian") return SpellID::Guardian;
	if (value == "ChainLightning") return SpellID::ChainLightning;
	if (value == "FlameWave") return SpellID::FlameWave;
	if (value == "DoomSerpents") return SpellID::DoomSerpents;
	if (value == "BloodRitual") return SpellID::BloodRitual;
	if (value == "Nova") return SpellID::Nova;
	if (value == "Invisibility") return SpellID::Invisibility;
	if (value == "Inferno") return SpellID::Inferno;
	if (value == "Golem") return SpellID::Golem;
	if (value == "Rage") return SpellID::Rage;
	if (value == "Teleport") return SpellID::Teleport;
	if (value == "Apocalypse") return SpellID::Apocalypse;
	if (value == "Etherealize") return SpellID::Etherealize;
	if (value == "ItemRepair") return SpellID::ItemRepair;
	if (value == "StaffRecharge") return SpellID::StaffRecharge;
	if (value == "TrapDisarm") return SpellID::TrapDisarm;
	if (value == "Elemental") return SpellID::Elemental;
	if (value == "ChargedBolt") return SpellID::ChargedBolt;
	if (value == "HolyBolt") return SpellID::HolyBolt;
	if (value == "Resurrect") return SpellID::Resurrect;
	if (value == "Telekinesis") return SpellID::Telekinesis;
	if (value == "HealOther") return SpellID::HealOther;
	if (value == "BloodStar") return SpellID::BloodStar;
	if (value == "BoneSpirit") return SpellID::BoneSpirit;
	if (value == "Mana") return SpellID::Mana;
	if (value == "Magi") return SpellID::Magi;
	if (value == "Jester") return SpellID::Jester;
	if (value == "LightningWall") return SpellID::LightningWall;
	if (value == "Immolation") return SpellID::Immolation;
	if (value == "Warp") return SpellID::Warp;
	if (value == "Reflect") return SpellID::Reflect;
	if (value == "Berserk") return SpellID::Berserk;
	if (value == "RingOfFire") return SpellID::RingOfFire;
	if (value == "Search") return SpellID::Search;
	if (value == "RuneOfFire") return SpellID::RuneOfFire;
	if (value == "RuneOfLight") return SpellID::RuneOfLight;
	if (value == "RuneOfNova") return SpellID::RuneOfNova;
	if (value == "RuneOfImmolation") return SpellID::RuneOfImmolation;
	if (value == "RuneOfStone") return SpellID::RuneOfStone;
	if (value == "Frostbolt") return SpellID::Frostbolt;
	if (value == "Strength") return SpellID::Strength;
	if (value == "Corruption") return SpellID::Corruption;
	if (value == "FireAura") return SpellID::FireAura;
	if (value == "FlamingWeapon") return SpellID::FlamingWeapon;
	if (value == "FlameStrike") return SpellID::FlameStrike;
	if (value == "InfernoStrike") return SpellID::InfernoStrike;
	for (const auto &[key, spell] : ExtendedSpellKeys) {
		if (value == key)
			return spell;
	}
	return std::unexpected("Unknown enum value");
}

void LoadSpellData()
{
	SpellsData.clear();
	const std::string_view filename = "txtdata\\spells\\spelldat.tsv";
	DataFile dataFile = DataFile::loadOrDie(filename);
	SpellsData.reserve(dataFile.numRecords() + 1);
	AddNullSpell();
	dataFile.skipHeaderOrDie(filename);
	for (DataFileRecord record : dataFile) {
		RecordReader reader { record, filename };
		SpellData &item = SpellsData.emplace_back();
		reader.advance(); // skip id
		reader.readString("name", item.sNameText);
		reader.read("soundId", item.sSFX, ParseSpellSoundId);
		reader.readInt("bookCost10", item.bookCost10);
		reader.readInt("staffCost10", item.staffCost10);
		reader.readInt("manaCost", item.sManaCost);
		reader.readEnumList("flags", item.flags, ParseSpellDataFlag);
		reader.readInt("bookLevel", item.sBookLvl);
		reader.readInt("staffLevel", item.sStaffLvl);
		reader.readInt("minIntelligence", item.minInt);
		reader.readEnumArray("missiles", /*fillMissing=*/std::make_optional(MissileID::Null), item.sMissiles, ParseMissileId);
		reader.readInt("manaMultiplier", item.sManaAdj);
		reader.readInt("minMana", item.sMinMana);
		reader.readInt("staffMin", item.sStaffMin);
		reader.readInt("staffMax", item.sStaffMax);
	}
	LoadExtendedSpellData();
	SpellsData.shrink_to_fit();
}

} // namespace devilution
