/**
 * @file meter.h
 *
 * Essence Mod: the meter, a tally of what the local player has done this session.
 *
 * It keeps two lists: damage dealt, by power, and healing and shielding given, by power. Each
 * player's PC counts only its own player's; nothing is sent to anyone. The tallies live in memory
 * only and start again from nothing with every game.
 */
#pragma once

#include "engine/surface.hpp"
#include "tables/spelldat.h"

namespace devilution {

/** Whether the meter's panel is open. */
extern bool MeterOpen;

/** @brief Empties both lists. Done at the start of every game, and by the panel's Reset. */
void ResetMeter();

/**
 * @brief Counts damage the local player has dealt to a monster.
 * @param spell The power that dealt it, or SpellID::Invalid for a plain weapon hit.
 * @param amount In 64ths of a hit point.
 */
void MeterAddDamage(SpellID spell, int amount);

/** @brief Counts healing a power of the local player's has done, or damage one of their shields has absorbed. */
void MeterAddHealing(SpellID spell, int amount);

/** @brief Counts health the local player's own regeneration has restored. */
void MeterAddRegeneration(int amount);

/** @brief Counts health the local player's life leech has restored. */
void MeterAddLeech(int amount);

/** @brief Opens or closes the panel (the M key). */
void ToggleMeter();

/** @brief Draws the panel, on the quest log's stone tablet. */
void DrawMeter(const Surface &out);

/**
 * @brief Handles a click while the panel is showing: its two page tabs and Reset.
 * @return true if the click fell on the panel and should go no further.
 */
bool CheckMeterClick();

} // namespace devilution
