#pragma once

#include <cstddef>
#include <vector>

#include "engine/point.hpp"
#include "engine/surface.hpp"
#include "tables/spelldat.h"

namespace devilution {

struct SpellListItem {
	Point location;
	SpellType type;
	SpellID id;
	bool isSelected;
};

/**
 * @brief draws the current right mouse button spell.
 * @param out screen buffer representing the main UI panel
 */
void DrawSpell(const Surface &out);
void DrawSpellList(const Surface &out);
std::vector<SpellListItem> GetSpellListItems();
void SetSpell();
void SetSpeedSpell(size_t slot);

/** Essence Mod: how many of the hotkey slots make up the spell bar (Q W E R / A S D F). */
constexpr size_t SpellBarSlots = 8;

/**
 * @brief Essence Mod: puts a power on a hotkey slot. Pressing the key of the slot it is already
 * on takes it off again; a power sits on one slot at most.
 */
void AssignSpeedSpell(size_t slot, SpellID spell, SpellType type);

/**
 * @brief Essence Mod: draws the spell bar, the eight quick-cast powers as two rows of four at the
 * bottom right of the screen, each with its key and its cooldown. It is hidden while the spell
 * list, the inventory or the spellbook is open.
 */
void DrawSpellBar(const Surface &out);
bool IsValidSpeedSpell(size_t slot);
void ToggleSpell(size_t slot);

/**
 * Draws the "Speed Book": the rows of known spells for quick-setting a spell that
 * show up when you click the spell slot at the control panel.
 */
void DoSpeedBook();

} // namespace devilution
