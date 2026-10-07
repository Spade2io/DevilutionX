#pragma once

#include <expected>
#include <string>

#include "engine/clx_sprite.hpp"
#include "engine/surface.hpp"
#include "tables/spelldat.h"

namespace devilution {

std::expected<void, std::string> InitSpellBook();
void FreeSpellBook();
void CheckSBook();
/** @brief Essence Mod: the known power whose icon the cursor is over in the open spellbook, or SpellID::Invalid. */
SpellID GetSpellBookAbilityUnderCursor();
void DrawSpellBook(const Surface &out);

} // namespace devilution
