/**
 * @file cooldowns.h
 *
 * Essence Mod: cooldowns on spells and special attacks.
 *
 * Most abilities have none. One that does cannot be used again until its cooldown has run out.
 * Cooldowns belong to the local player only: they are held in memory, never saved, and never sent
 * to other players.
 */
#pragma once

#include "engine/surface.hpp"
#include "tables/spelldat.h"

namespace devilution {

/** @brief How long an ability's cooldown lasts, in game ticks (20 per second). Zero for no cooldown. */
int GetSpellCooldownTicks(SpellID spell);

/** @brief Starts an ability's cooldown, if it has one. */
void StartSpellCooldown(SpellID spell);

bool IsSpellOnCooldown(SpellID spell);

/**
 * @brief How far through its cooldown an ability is, from 0 (just used) to 1 (ready).
 * Returns 1 for an ability that is not on cooldown.
 */
float GetSpellCooldownProgress(SpellID spell);

/** @brief Counts every cooldown down by one game tick. */
void ProcessCooldowns();

/** @brief Clears every cooldown, e.g. when a character is loaded. */
void ClearCooldowns();

/**
 * @brief Draws the cooldown tracker: one icon per ability on cooldown, above the mana orb, the
 * soonest to be ready nearest the corner. An icon swells briefly and vanishes when its ability is ready.
 */
void DrawCooldownTracker(const Surface &out);

} // namespace devilution
