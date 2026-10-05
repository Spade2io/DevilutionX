/**
 * @file dots.h
 *
 * Essence Mod: damage-over-time effects on monsters.
 *
 * Every cast adds a stack and refreshes the duration, so damage builds slowly with repeated casts.
 * Effects are private to this PC: they are held in memory only, never saved, and never sent to other
 * players. Only the damage they deal is reported, through the game's normal damage message.
 */
#pragma once

#include <cstdint>

#include "tables/misdat.h"
#include "tables/spelldat.h"

namespace devilution {

struct Monster;

/**
 * @brief The default damage scaling for the mod's spells: 12.5% more for each spell level after the
 * first, compounding. This is the rule Fireball, Flash and Nova use. Level 1 gives the base unchanged.
 */
int ScaleDamageForSpellLevel(int baseDamage, int spellLevel);

/**
 * @brief Deals one tick of a spell's damage to a monster on behalf of the local player.
 *
 * Applies immunity and resistance, credits the spell with experience, reports the damage to other
 * players through the game's normal damage message, and flashes the monster in the spell's colour.
 * Used by damage-over-time effects and damage auras.
 *
 * @param damage Damage before resistance, in 64ths of a hit point.
 * @param finishKill Start the death of a monster this kills. Pass false when the caller does that itself.
 */
void DealSpellTickDamage(Monster &monster, SpellID spell, MissileID missile, DamageType damageType, int damage, bool finishKill = true);

enum class DotID : uint8_t {
	Corruption,
	LAST = Corruption,
};

/**
 * @brief Adds one stack of an effect to a monster, refreshes its duration, and flashes the monster
 * in the spell's colour so the caster can see it landed.
 * @return The number of stacks the monster now has.
 */
int AddMonsterDot(Monster &monster, DotID dot);

/** @brief Runs every effect for one game tick: counts down, deals damage when due, and expires. */
void ProcessMonsterDots();

/** @brief Removes every effect from every monster, e.g. when a level is set up. */
void ClearAllMonsterDots();

/**
 * @brief The colour table to draw a monster with while it flashes from a damage tick,
 * or nullptr when it is not flashing.
 */
const uint8_t *GetMonsterDotPulseTrn(const Monster &monster);

} // namespace devilution
