/**
 * @file spell_xp.h
 *
 * Essence Mod: learn-by-doing experience for spells.
 */
#pragma once

#include <cstdint>
#include <string>

#include "tables/spelldat.h"

namespace devilution {

struct Monster;
struct Player;

/**
 * @brief The spell a damaging projectile belongs to, or SpellID::Invalid for arrows, traps and effects.
 *
 * Spells that work by firing another spell's projectile are credited to that spell:
 * Guardian (Firebolt), Chain Lightning (Lightning) and Ring of Fire (Fire Wall).
 */
SpellID GetSpellForMissile(MissileID missile);

/**
 * @brief Credits a spell with experience for damage it dealt to a monster.
 *
 * The spell earns the same share of the monster's kill experience as the share of the monster's
 * total health it removed, adjusted by the level difference between the monster and the player.
 * Enough experience raises the spell's level, using the character level thresholds.
 * Spells the player has not learned earn nothing.
 *
 * @param damage Damage dealt, in 64ths of a hit point.
 * @param hitPointsBefore The monster's hit points before the damage, in 64ths of a hit point.
 */
void AddSpellExperienceForDamage(const Player &player, const Monster &monster, SpellID spell, int damage, int hitPointsBefore);

/**
 * @brief Works out what a weapon hit is worth, the same way as spell damage, and gives each active
 * buff its tenth. No ability earns the attack's own experience yet.
 *
 * @param damage Damage dealt, in 64ths of a hit point.
 * @param hitPointsBefore The monster's hit points before the damage, in 64ths of a hit point.
 */
void AddAttackExperienceForDamage(const Player &player, const Monster &monster, int damage, int hitPointsBefore);

/**
 * @brief Kill experience of a typical monster of the given level.
 *
 * A smooth curve (5L² + 10L + 40) fitted to the average of Hellfire's monster table, which runs
 * from about 54 at level 1 to about 4,800 at level 30. Used where a spell's target is not a monster.
 */
unsigned AverageMonsterExperienceForLevel(unsigned level);

/**
 * @brief Credits a healing spell with experience for hit points it restored to a player.
 *
 * The healed player counts as an average monster of their own level: restoring a share of their
 * health earns that share of such a monster's kill experience. Healing a player who is already at
 * full health earns nothing.
 *
 * @param healed Hit points actually restored, in 64ths of a hit point.
 */
void AddSpellExperienceForHealing(const Player &caster, const Player &target, SpellID spell, int healed);

/** @brief Total experience a spell has earned, in 64ths of a point. */
uint32_t GetSpellExperience(SpellID spell);

/**
 * @brief How far a spell is towards its next level, from 0 to 100.
 * Returns 100 for a spell at the maximum level and 0 for a spell that is not learned.
 */
int GetSpellLevelProgressPercent(const Player &player, SpellID spell);

/** @brief Clears all spell experience, e.g. before loading a different character. */
void ResetSpellExperience();

/**
 * @brief Loads spell experience from the sidecar file kept next to the save.
 *
 * Also restores the levels of spells the original save has no room for (spell numbers 47 and up),
 * marking them as known on the given player. A missing or unreadable file leaves every spell at zero.
 */
void LoadSpellExperience(const std::string &path, Player &player);

/**
 * @brief Writes spell experience, and the levels of spells the original save has no room for,
 * to the sidecar file kept next to the save.
 */
void SaveSpellExperience(const std::string &path, const Player &player);

} // namespace devilution
