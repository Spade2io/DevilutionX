/**
 * @file special_attacks.h
 *
 * Essence Mod: special attacks.
 *
 * A special attack is learned, levelled and paid for with mana like a spell, but using it makes a
 * weapon attack with an added effect instead of casting. The player right-clicks a monster; the
 * character walks over and swings as for a normal attack, and that one swing carries the effect.
 */
#pragma once

#include "tables/spelldat.h"

namespace devilution {

struct Monster;
struct Player;

/** @brief Whether a spell number is a special attack rather than a spell that is cast. */
bool IsSpecialAttack(SpellID spell);

/**
 * @brief Remembers that the local player's next swing at this monster is a special attack.
 * The order lapses after a few seconds if the swing never happens.
 */
void QueueSpecialAttack(SpellID spell, int monsterId);

/**
 * @brief Called when the local player's weapon swing at a monster resolves, hit or miss.
 *
 * If a special attack is queued for that monster and the player can pay for it, spends the mana
 * and returns the attack; otherwise returns SpellID::Invalid.
 */
SpellID TakeQueuedSpecialAttack(Player &player, const Monster &monster);

/** @brief Counts down the queued order so that it lapses. Called once per game tick. */
void ProcessSpecialAttacks();

/** @brief Fire damage Flame Strike adds to the swing, in 64ths of a hit point. */
int GetFlameStrikeBonusDamage(const Player &player);

/**
 * @brief The damage of a swing once a special attack's bonus is added.
 *
 * Flame Strike adds a flat amount. Inferno Strike adds 200% of the weapon's damage, tripling the hit.
 * Both bonuses grow 12.5% per spell level.
 *
 * @param weaponDamage The swing's normal damage, in 64ths of a hit point.
 */
int ApplySpecialAttackDamage(const Player &player, SpellID attack, int weaponDamage);

} // namespace devilution
