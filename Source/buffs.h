/**
 * @file buffs.h
 *
 * Essence Mod: persistent buffs and auras.
 *
 * A buff is switched on by casting its spell and stays on until the player dies. Buffs are held in
 * memory only and are never saved, so a character always loads with none active.
 */
#pragma once

#include <cstdint>

#include "engine/surface.hpp"
#include "tables/spelldat.h"

namespace devilution {

struct Player;

enum class BuffID : uint8_t {
	Strength,
	FireAura,
	LAST = FireAura,
};

/** @brief The spell that switches a buff on, and that earns its experience. */
SpellID GetBuffSpell(BuffID buff);

/**
 * @brief Whether a buff's spell earns a tenth of the player's other experience gains.
 * A damage aura earns experience from its own damage instead.
 */
bool BuffSharesExperience(BuffID buff);

/** @brief Switches a buff on for a player and recalculates their stats. */
void ActivateBuff(Player &player, BuffID buff);

bool IsBuffActive(const Player &player, BuffID buff);

/** @brief Switches every buff off, e.g. on death or when a character is loaded. Does not recalculate stats. */
void ClearBuffs(Player &player);

/** @brief Recalculates a player's stats if they have any buff active, e.g. after a buff spell levels up. */
void RefreshBuffStats(Player &player);

/** @brief Strength added by the player's active buffs: 10 at spell level 1, plus 5 per level after that. */
int GetBuffStrengthBonus(const Player &player);

/** @brief Runs auras for one game tick: every 2 seconds a damage aura pulses around its owner. */
void ProcessBuffs();

/** @brief Draws the local player's active buffs as a row of icons across the top of the screen. */
void DrawBuffBar(const Surface &out);

} // namespace devilution
