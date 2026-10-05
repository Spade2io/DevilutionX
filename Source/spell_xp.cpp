/**
 * @file spell_xp.cpp
 *
 * Essence Mod: learn-by-doing experience for spells.
 */
#include "spell_xp.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "buffs.h"
#include "essences.h"
#include "monster.h"
#include "msg.h"
#include "multi.h"
#include "player.h"
#include "plrmsg.h"
#include "spells.h"
#include "tables/playerdat.hpp"
#include "utils/file_util.h"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

/** First line of the sidecar file. The number is the file layout version. */
constexpr char SidecarHeader[] = "essence-spell-xp 1";

/**
 * The original save only records spell levels for spell numbers 0 to 46.
 * Levels of higher-numbered spells (new ones the mod adds) are kept in the sidecar file instead.
 */
constexpr size_t FirstSpellNotInSave = 47;

/** Experience per spell, in 64ths of a point so that small hits still add up. */
std::vector<uint32_t> SpellExperience;

/** A spell and how it is cast (learned, scroll, staff), as held on a hotkey or readied. */
struct SpellSelection {
	SpellID spell = SpellID::Invalid;
	SpellType type = SpellType::Invalid;
};

/** What the side file last loaded says is readied and on each hotkey. Invalid where it says nothing. */
SpellSelection SavedReadiedSpell;
std::array<SpellSelection, NumHotkeys> SavedHotkeys;

/** The experience counter for one spell. The list grows to cover every row of the spell table. */
uint32_t &ExperienceOf(SpellID spell)
{
	if (SpellExperience.size() < SpellsData.size())
		SpellExperience.resize(SpellsData.size(), 0);
	return SpellExperience[static_cast<size_t>(spell)];
}

/**
 * Formats a value held in 64ths for display: a whole number from 10 up (640 -> "10"),
 * and one decimal place below that so small gains still show (96 -> "1.5").
 */
std::string FormatSixtyFourths(uint32_t value)
{
	if (value >= 10 * 64)
		return StrCat((value + 32) / 64);
	const uint32_t tenths = static_cast<uint32_t>(static_cast<uint64_t>(value) * 10 / 64);
	return StrCat(tenths / 10, ".", tenths % 10);
}

/**
 * @brief Total experience (in 64ths) a spell needs to advance past the given level.
 * Reuses the character table: spell level 1 -> 2 costs what character level 1 -> 2 costs, and so on.
 */
uint64_t ExperienceToAdvancePast(uint8_t spellLevel)
{
	return static_cast<uint64_t>(GetNextExperienceThresholdForLevel(spellLevel)) * 64;
}

void RaiseSpellLevelIfEarned(Player &player, SpellID spell)
{
	uint8_t level = player.GetBaseSpellLevel(spell);
	const uint32_t total = ExperienceOf(spell);

	while (level < MaxSpellLevel && total >= ExperienceToAdvancePast(level)) {
		level++;
		player.SetBaseSpellLevel(spell, level);
		NetSendCmdParam2(true, CMD_CHANGE_SPELL_LEVEL, static_cast<uint16_t>(spell), level);
		EventPlrMsg(StrCat(GetSpellData(spell).sNameText, " reached level ", static_cast<int>(level)), UiFlags::ColorWhitegold);
		// A buff's strength depends on its spell's level, so a level-up takes effect straight away.
		RefreshBuffStats(player);
	}
}

/** One part of the experience message, e.g. "Firebolt 50 XP". */
std::string DescribeGain(SpellID spell, uint32_t gained)
{
	return StrCat(GetSpellData(spell).sNameText, " ", FormatSixtyFourths(gained), " XP");
}

/**
 * @brief Adds experience to a learned spell and raises the spell's level if earned.
 * @param gained Experience before the level adjustment, in 64ths of a point.
 * @param levelDelta Target's level minus the player's level.
 * @return The experience actually added, in 64ths of a point. Zero if none.
 */
uint32_t AwardSpellExperience(Player &player, SpellID spell, int64_t gained, int levelDelta)
{
	// Same rule the game uses for character experience: 10% more or less per level of difference.
	gained = gained * (10 + levelDelta) / 10;
	if (gained <= 0)
		return 0;

	uint32_t &total = ExperienceOf(spell);

	// A spell can hold a level its experience never earned (a class's starting spell, a shrine).
	// Count from the start of that level, so progress shows straight away instead of after catching up.
	const uint8_t level = player.GetBaseSpellLevel(spell);
	if (level > 1 && level <= MaxSpellLevel)
		total = std::max(total, static_cast<uint32_t>(ExperienceToAdvancePast(level - 1)));

	const auto added = static_cast<uint32_t>(std::min<int64_t>(gained, UINT32_MAX - total));
	total += added;

	RaiseSpellLevelIfEarned(player, spell);
	return added;
}

/** Share of any experience gain that each active buff's spell also earns. */
constexpr int BuffExperienceDivisor = 10;

/**
 * @brief Gives every active buff a tenth of an experience gain (damage, healing, or a weapon hit),
 * adding each to the message, e.g. "Firebolt 50 XP, Strength 5 XP".
 */
void ShareExperienceWithActiveBuffs(Player &player, uint32_t gained, std::string &message)
{
	for (size_t i = 0; i <= static_cast<size_t>(BuffID::LAST); i++) {
		const auto buff = static_cast<BuffID>(i);
		if (!IsBuffActive(player, buff) || !BuffSharesExperience(buff))
			continue;
		const SpellID buffSpell = GetBuffSpell(buff);
		if (buffSpell == SpellID::Invalid || player.GetBaseSpellLevel(buffSpell) == 0)
			continue;
		const uint32_t buffGain = AwardSpellExperience(player, buffSpell, gained / BuffExperienceDivisor, 0);
		if (buffGain != 0)
			StrAppend(message, message.empty() ? "" : ", ", DescribeGain(buffSpell, buffGain));
	}
}

/**
 * @brief The experience a hit is worth, in 64ths of a point: the monster's kill experience times the
 * share of its total health the hit removed, adjusted for the level difference. Zero if none.
 *
 * Damage beyond the monster's remaining health counts for nothing, so one kill never pays more than 100%.
 */
uint32_t ExperienceForDamage(const Player &player, const Monster &monster, int damage, int hitPointsBefore)
{
	if (monster.maxHitPoints <= 0)
		return 0;
	const int effectiveDamage = std::clamp(damage, 0, std::max(hitPointsBefore, 0));
	if (effectiveDamage == 0)
		return 0;

	const _difficulty difficulty = sgGameInitInfo.nDifficulty;
	int64_t gained = static_cast<int64_t>(monster.exp(difficulty)) * 64 * effectiveDamage / monster.maxHitPoints;

	// Same rule the game uses for character experience: 10% more or less per level of difference.
	const int levelDelta = static_cast<int>(monster.level(difficulty)) - static_cast<int>(player.getCharacterLevel());
	gained = gained * (10 + levelDelta) / 10;
	return static_cast<uint32_t>(std::clamp<int64_t>(gained, 0, UINT32_MAX));
}

} // namespace

SpellID GetSpellForMissile(MissileID missile)
{
	switch (missile) {
	case MissileID::Firebolt:
		return SpellID::Firebolt;
	case MissileID::Frostbolt:
		return SpellID::Frostbolt;
	case MissileID::ChargedBolt:
		return SpellID::ChargedBolt;
	case MissileID::HolyBolt:
		return SpellID::HolyBolt;
	case MissileID::Lightning:
		return SpellID::Lightning;
	case MissileID::FlashBottom:
	case MissileID::FlashTop:
		return SpellID::Flash;
	case MissileID::FireWall:
		return SpellID::FireWall;
	case MissileID::Fireball:
		return SpellID::Fireball;
	case MissileID::FlameWave:
		return SpellID::FlameWave;
	case MissileID::NovaBall:
		return SpellID::Nova;
	case MissileID::Inferno:
		return SpellID::Inferno;
	case MissileID::Elemental:
		return SpellID::Elemental;
	case MissileID::BloodStar:
		return SpellID::BloodStar;
	case MissileID::BoneSpirit:
		return SpellID::BoneSpirit;
	case MissileID::ApocalypseBoom:
		return SpellID::Apocalypse;
	case MissileID::LightningWall:
		return SpellID::LightningWall;
	case MissileID::Immolation:
		return SpellID::Immolation;
	default:
		return SpellID::Invalid;
	}
}

void AddSpellExperienceForDamage(const Player &player, const Monster &monster, SpellID spell, int damage, int hitPointsBefore)
{
	if (&player != MyPlayer)
		return;

	// Only learned spells grow. Arrows, and spells cast from a staff or scroll, count as plain attacks.
	// A passive buff earns only its tenth share, so any damage credited to one counts as a plain
	// attack too. Buffs that deal damage, such as Flaming Weapon, earn the full value like any spell.
	if (spell == SpellID::Invalid || player.GetBaseSpellLevel(spell) == 0 || IsExperienceSharingBuffSpell(spell)) {
		AddAttackExperienceForDamage(player, monster, damage, hitPointsBefore);
		return;
	}

	const uint32_t worth = ExperienceForDamage(player, monster, damage, hitPointsBefore);
	const uint32_t added = AwardSpellExperience(*MyPlayer, spell, worth, 0);
	if (added == 0)
		return;

	// One line for the whole gain, however many buffs are sharing in it.
	std::string message = DescribeGain(spell, added);
	ShareExperienceWithActiveBuffs(*MyPlayer, added, message);
	EventPlrMsg(message, UiFlags::ColorWhite);
}

void AddAttackExperienceForDamage(const Player &player, const Monster &monster, int damage, int hitPointsBefore)
{
	if (&player != MyPlayer)
		return;

	// No ability earns the attack's own experience yet; it only feeds the active buffs.
	const uint32_t worth = ExperienceForDamage(player, monster, damage, hitPointsBefore);
	if (worth == 0)
		return;

	std::string message;
	ShareExperienceWithActiveBuffs(*MyPlayer, worth, message);
	if (!message.empty())
		EventPlrMsg(message, UiFlags::ColorWhite);
}

unsigned AverageMonsterExperienceForLevel(unsigned level)
{
	return 5 * level * level + 10 * level + 40;
}

void AddSpellExperienceForHealing(const Player &caster, const Player &target, SpellID spell, int healed)
{
	if (&caster != MyPlayer || spell == SpellID::Invalid || target._pMaxHP <= 0 || healed <= 0)
		return;

	// Only learned spells grow. Casting from a staff or scroll does not teach the spell.
	if (caster.GetBaseSpellLevel(spell) == 0)
		return;

	// The healed player stands in for an average monster of their own level:
	// restoring a share of their health earns that share of such a monster's kill experience.
	const unsigned targetLevel = target.getCharacterLevel();
	const int64_t gained = static_cast<int64_t>(AverageMonsterExperienceForLevel(targetLevel)) * 64 * std::min(healed, target._pMaxHP) / target._pMaxHP;

	const int levelDelta = static_cast<int>(targetLevel) - static_cast<int>(caster.getCharacterLevel());
	const uint32_t added = AwardSpellExperience(*MyPlayer, spell, gained, levelDelta);
	if (added == 0)
		return;

	// Every source of spell experience feeds the active buffs, not only damage.
	std::string message = DescribeGain(spell, added);
	ShareExperienceWithActiveBuffs(*MyPlayer, added, message);
	EventPlrMsg(message, UiFlags::ColorWhite);
}

uint32_t GetSpellExperience(SpellID spell)
{
	if (spell == SpellID::Invalid)
		return 0;
	return ExperienceOf(spell);
}

uint32_t GetSpellExperienceForNextLevel(const Player &player, SpellID spell)
{
	if (spell == SpellID::Invalid)
		return 0;
	const uint8_t level = player.GetBaseSpellLevel(spell);
	if (level == 0 || level >= MaxSpellLevel)
		return 0;
	return GetNextExperienceThresholdForLevel(level);
}

int GetSpellLevelProgressPercent(const Player &player, SpellID spell)
{
	if (spell == SpellID::Invalid)
		return 0;
	const uint8_t level = player.GetBaseSpellLevel(spell);
	if (level == 0)
		return 0;
	if (level >= MaxSpellLevel)
		return 100;

	// A spell raised by other means (shrines, older saves) may sit above what its experience earned.
	// It then shows an empty bar until its experience catches up with the level it already has.
	const uint64_t levelStart = level > 1 ? ExperienceToAdvancePast(level - 1) : 0;
	const uint64_t levelEnd = ExperienceToAdvancePast(level);
	const uint64_t total = ExperienceOf(spell);
	if (total <= levelStart || levelEnd <= levelStart)
		return 0;
	return static_cast<int>(std::min<uint64_t>((total - levelStart) * 100 / (levelEnd - levelStart), 100));
}

void ResetSpellExperience()
{
	SpellExperience.assign(SpellsData.size(), 0);
}

void LoadSpellExperience(const std::string &path, Player &player)
{
	ResetSpellExperience();
	ResetEssences();
	player.extendedSpellLevels.clear();
	SavedReadiedSpell = {};
	SavedHotkeys.fill({});

	FILE *file = OpenFile(path.c_str(), "rb");
	if (file == nullptr)
		return;

	char line[64] = {};
	if (std::fgets(line, sizeof(line), file) != nullptr && std::string_view(line).starts_with(SidecarHeader)) {
		while (std::fgets(line, sizeof(line), file) != nullptr) {
			unsigned spell = 0;
			unsigned value = 0;
			unsigned hotkey = 0;
			if (ReadEssenceSidecarLine(line))
				continue;
			if (std::sscanf(line, "L %u %u", &spell, &value) == 2) {
				// The level of a spell the original save has no room for.
				if (spell >= FirstSpellNotInSave && IsValidSpell(static_cast<SpellID>(spell)) && value > 0) {
					player.SetBaseSpellLevel(static_cast<SpellID>(spell), static_cast<uint8_t>(std::min<unsigned>(value, MaxSpellLevel)));
					player._pMemSpells |= GetSpellBitmask(static_cast<SpellID>(spell));
				}
			} else if (std::sscanf(line, "R %u %u", &spell, &value) == 2) {
				// The readied spell, when it is one the save file's own field cannot name.
				if (IsValidSpell(static_cast<SpellID>(spell)) && value <= static_cast<unsigned>(SpellType::Invalid))
					SavedReadiedSpell = { static_cast<SpellID>(spell), static_cast<SpellType>(value) };
			} else if (std::sscanf(line, "H %u %u %u", &hotkey, &spell, &value) == 3) {
				// A hotkey holding such a spell.
				if (hotkey < NumHotkeys && IsValidSpell(static_cast<SpellID>(spell)) && value <= static_cast<unsigned>(SpellType::Invalid))
					SavedHotkeys[hotkey] = { static_cast<SpellID>(spell), static_cast<SpellType>(value) };
			} else if (std::sscanf(line, "%u %u", &spell, &value) == 2) {
				if (IsValidSpell(static_cast<SpellID>(spell)))
					ExperienceOf(static_cast<SpellID>(spell)) = value;
			}
		}
	}
	std::fclose(file);
	ApplyExtendedSpellSelections(player);
}

void ApplyExtendedSpellSelections(Player &player)
{
	// Only a power the character still knows is put back.
	if (SavedReadiedSpell.spell != SpellID::Invalid && player.GetBaseSpellLevel(SavedReadiedSpell.spell) != 0) {
		player._pRSpell = SavedReadiedSpell.spell;
		player._pRSplType = SavedReadiedSpell.type;
	}
	for (size_t i = 0; i < NumHotkeys; i++) {
		if (SavedHotkeys[i].spell != SpellID::Invalid && player.GetBaseSpellLevel(SavedHotkeys[i].spell) != 0) {
			player._pSplHotKey[i] = SavedHotkeys[i].spell;
			player._pSplTHotKey[i] = SavedHotkeys[i].type;
		}
	}
}

void SaveSpellExperience(const std::string &path, const Player &player)
{
	FILE *file = OpenFile(path.c_str(), "wb");
	if (file == nullptr)
		return;

	std::fprintf(file, "%s\n", SidecarHeader);
	for (size_t spell = 0; spell < SpellExperience.size(); spell++) {
		if (SpellExperience[spell] != 0)
			std::fprintf(file, "%u %u\n", static_cast<unsigned>(spell), static_cast<unsigned>(SpellExperience[spell]));
	}
	for (size_t spell = FirstSpellNotInSave; spell < LegacySpellCount(); spell++) {
		if (player._pSplLvl[spell] != 0)
			std::fprintf(file, "L %u %u\n", static_cast<unsigned>(spell), static_cast<unsigned>(player._pSplLvl[spell]));
	}
	// Powers beyond the original 64: their levels, and where one is readied or on a hotkey.
	// The save file's own fields for those hold "none" instead (see loadsave.cpp).
	for (const auto &[spell, level] : player.extendedSpellLevels)
		std::fprintf(file, "L %u %u\n", static_cast<unsigned>(spell), static_cast<unsigned>(level));
	if (IsExtendedSpell(player._pRSpell))
		std::fprintf(file, "R %u %u\n", static_cast<unsigned>(player._pRSpell), static_cast<unsigned>(player._pRSplType));
	for (size_t i = 0; i < NumHotkeys; i++) {
		if (IsExtendedSpell(player._pSplHotKey[i]))
			std::fprintf(file, "H %u %u %u\n", static_cast<unsigned>(i), static_cast<unsigned>(player._pSplHotKey[i]), static_cast<unsigned>(player._pSplTHotKey[i]));
	}
	WriteEssenceSidecarLines(file);
	std::fclose(file);
}

} // namespace devilution
