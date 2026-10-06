/**
 * @file spells.h
 *
 * Interface of functionality for casting player spells.
 */
#pragma once

#include <cstdint>

#include "engine/world_tile.hpp"
#include "player.h"

namespace devilution {

enum class SpellCheckResult : uint8_t {
	Success,
	Fail_NoMana,
	Fail_Level0,
	Fail_Busy,
};

bool IsValidSpell(SpellID spl);
bool IsValidSpellFrom(int spellFrom);
bool IsWallSpell(SpellID spl);
bool TargetsMonster(SpellID id);
int GetManaAmount(const Player &player, SpellID sn);
void ConsumeSpell(Player &player, SpellID sn);
SpellCheckResult CheckSpell(const Player &player, SpellID sn, SpellType st, bool manaonly);

/**
 * @brief Ensures the player's current readied spell is a valid selection for the character. If the current selection is
 * incompatible with the player's items and spell (for example, if the player does not currently have access to the spell),
 * the selection is cleared.
 * @note Will force a UI redraw in case the values actually change, so that the new spell reflects on the bottom panel.
 * @param player The player whose readied spell is to be checked.
 */
void EnsureValidReadiedSpell(Player &player);
void CastSpell(Player &player, SpellID spl, WorldTilePosition src, WorldTilePosition dst, int spllvl);
void SpawnResurrectBeam(Player &caster, Player &target);
void ApplyResurrect(Player &target);
void DoHealOther(const Player &caster, Player &target);
/**
 * @brief Essence Mod: whether a power is aimed at a friendly player rather than a monster or a
 * spot on the ground. Such a power can be cast on a player whether or not "player friendly" is on.
 */
bool IsAllyTargetedPower(SpellID spell);
/**
 * @brief Essence Mod: a power read from essence_powers.tsv lands on a player. Runs on every PC,
 * with the amount the caster's PC worked out.
 */
void ApplyPowerToPlayer(const Player &caster, Player &target, SpellID spell, int amount);
struct Monster;
/**
 * @brief Essence Mod: a power lands on a monster in a way every PC has to carry out the same.
 * So far that is a hold: the rider "Freeze" keeps the monster still for the amount in seconds.
 */
void ApplyPowerToMonster(const Player &caster, Monster &monster, SpellID spell, int amount);
int GetSpellBookLevel(SpellID s);
int GetSpellStaffLevel(SpellID s);

/**
 * @brief Gets a value that represents the specified spellID in 64bit bitmask format.
 * For example:
 *  - spell ID  1: 0000.0000.0000.0000.0000.0000.0000.0000.0000.0000.0000.0000.0000.0000.0000.0001
 *  - spell ID 43: 0000.0000.0000.0000.0000.0100.0000.0000.0000.0000.0000.0000.0000.0000.0000.0000
 * @param spellId The id of the spell to get a bitmask for.
 * @return A 64bit bitmask representation for the specified spell.
 */
constexpr uint64_t GetSpellBitmask(SpellID spellId)
{
	// Essence Mod: only spells 1 to 64 have a switch. Anything else gets none, which also keeps the
	// shift below from going out of range.
	const int number = static_cast<int>(spellId);
	if (number < 1 || number > LegacySpellLimit)
		return 0;
	return 1ULL << (number - 1);
}

} // namespace devilution
