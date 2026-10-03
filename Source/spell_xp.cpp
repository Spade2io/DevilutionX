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

#include "monster.h"
#include "msg.h"
#include "multi.h"
#include "player.h"
#include "plrmsg.h"
#include "tables/playerdat.hpp"
#include "utils/file_util.h"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

/** First line of the sidecar file. The number is the file layout version. */
constexpr char SidecarHeader[] = "essence-spell-xp 1";

/** Experience per spell, in 64ths of a point so that small hits still add up. */
std::array<uint32_t, static_cast<size_t>(SpellID::LAST) + 1> SpellExperience {};

/** Formats a value held in 64ths as a number with one decimal place, e.g. 96 -> "1.5". */
std::string FormatSixtyFourths(uint32_t value)
{
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
	uint8_t &level = player._pSplLvl[static_cast<size_t>(spell)];
	const uint32_t total = SpellExperience[static_cast<size_t>(spell)];

	while (level < MaxSpellLevel && total >= ExperienceToAdvancePast(level)) {
		level++;
		NetSendCmdParam2(true, CMD_CHANGE_SPELL_LEVEL, static_cast<uint16_t>(spell), level);
		EventPlrMsg(StrCat(GetSpellData(spell).sNameText, " reached level ", static_cast<int>(level)), UiFlags::ColorWhitegold);
	}
}

/**
 * @brief Adds experience to a learned spell, announces it, and raises the spell's level if earned.
 * @param gained Experience before the level adjustment, in 64ths of a point.
 * @param levelDelta Target's level minus the player's level.
 */
void AwardSpellExperience(Player &player, SpellID spell, int64_t gained, int levelDelta)
{
	// Same rule the game uses for character experience: 10% more or less per level of difference.
	gained = gained * (10 + levelDelta) / 10;
	if (gained <= 0)
		return;

	uint32_t &total = SpellExperience[static_cast<size_t>(spell)];

	// A spell can hold a level its experience never earned (a class's starting spell, a shrine).
	// Count from the start of that level, so progress shows straight away instead of after catching up.
	const uint8_t level = player._pSplLvl[static_cast<size_t>(spell)];
	if (level > 1 && level <= MaxSpellLevel)
		total = std::max(total, static_cast<uint32_t>(ExperienceToAdvancePast(level - 1)));

	total += static_cast<uint32_t>(std::min<int64_t>(gained, UINT32_MAX - total));

	EventPlrMsg(StrCat(GetSpellData(spell).sNameText, " XP +", FormatSixtyFourths(static_cast<uint32_t>(gained)), " (total ", FormatSixtyFourths(total), ")"), UiFlags::ColorWhite);

	RaiseSpellLevelIfEarned(player, spell);
}

} // namespace

SpellID GetSpellForMissile(MissileID missile)
{
	switch (missile) {
	case MissileID::Firebolt:
		return SpellID::Firebolt;
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
	if (&player != MyPlayer || spell == SpellID::Invalid || monster.maxHitPoints <= 0)
		return;

	// Only learned spells grow. Casting from a staff or scroll does not teach the spell.
	if (player._pSplLvl[static_cast<size_t>(spell)] == 0)
		return;

	// Damage beyond the monster's remaining health earns nothing, so one kill never pays more than 100%.
	const int effectiveDamage = std::clamp(damage, 0, std::max(hitPointsBefore, 0));
	if (effectiveDamage == 0)
		return;

	const _difficulty difficulty = sgGameInitInfo.nDifficulty;

	// Share of the monster's kill experience, in 64ths of a point.
	int64_t gained = static_cast<int64_t>(monster.exp(difficulty)) * 64 * effectiveDamage / monster.maxHitPoints;

	const int levelDelta = static_cast<int>(monster.level(difficulty)) - static_cast<int>(player.getCharacterLevel());
	AwardSpellExperience(*MyPlayer, spell, gained, levelDelta);
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
	if (caster._pSplLvl[static_cast<size_t>(spell)] == 0)
		return;

	// The healed player stands in for an average monster of their own level:
	// restoring a share of their health earns that share of such a monster's kill experience.
	const unsigned targetLevel = target.getCharacterLevel();
	const int64_t gained = static_cast<int64_t>(AverageMonsterExperienceForLevel(targetLevel)) * 64 * std::min(healed, target._pMaxHP) / target._pMaxHP;

	const int levelDelta = static_cast<int>(targetLevel) - static_cast<int>(caster.getCharacterLevel());
	AwardSpellExperience(*MyPlayer, spell, gained, levelDelta);
}

uint32_t GetSpellExperience(SpellID spell)
{
	if (spell == SpellID::Invalid)
		return 0;
	return SpellExperience[static_cast<size_t>(spell)];
}

int GetSpellLevelProgressPercent(const Player &player, SpellID spell)
{
	if (spell == SpellID::Invalid)
		return 0;
	const uint8_t level = player._pSplLvl[static_cast<size_t>(spell)];
	if (level == 0)
		return 0;
	if (level >= MaxSpellLevel)
		return 100;

	// A spell raised by other means (shrines, older saves) may sit above what its experience earned.
	// It then shows an empty bar until its experience catches up with the level it already has.
	const uint64_t levelStart = level > 1 ? ExperienceToAdvancePast(level - 1) : 0;
	const uint64_t levelEnd = ExperienceToAdvancePast(level);
	const uint64_t total = SpellExperience[static_cast<size_t>(spell)];
	if (total <= levelStart || levelEnd <= levelStart)
		return 0;
	return static_cast<int>(std::min<uint64_t>((total - levelStart) * 100 / (levelEnd - levelStart), 100));
}

void ResetSpellExperience()
{
	SpellExperience.fill(0);
}

void LoadSpellExperience(const std::string &path)
{
	ResetSpellExperience();

	FILE *file = OpenFile(path.c_str(), "rb");
	if (file == nullptr)
		return;

	char header[32] = {};
	if (std::fgets(header, sizeof(header), file) != nullptr && std::string_view(header).starts_with(SidecarHeader)) {
		unsigned spell = 0;
		unsigned experience = 0;
		while (std::fscanf(file, "%u %u", &spell, &experience) == 2) {
			if (spell < SpellExperience.size())
				SpellExperience[spell] = experience;
		}
	}
	std::fclose(file);
}

void SaveSpellExperience(const std::string &path)
{
	FILE *file = OpenFile(path.c_str(), "wb");
	if (file == nullptr)
		return;

	std::fprintf(file, "%s\n", SidecarHeader);
	for (size_t spell = 0; spell < SpellExperience.size(); spell++) {
		if (SpellExperience[spell] != 0)
			std::fprintf(file, "%u %u\n", static_cast<unsigned>(spell), static_cast<unsigned>(SpellExperience[spell]));
	}
	std::fclose(file);
}

} // namespace devilution
