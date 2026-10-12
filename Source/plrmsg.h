/**
 * @file plrmsg.h
 *
 * Interface of functionality for printing the ingame chat messages.
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "DiabloUI/ui_flags.hpp"
#include "engine/surface.hpp"
#include "player.h"

namespace devilution {

void DelayPlrMessages(uint32_t delayTime);
void EventPlrMsg(std::string_view text, UiFlags style = UiFlags::ColorWhitegold);
/**
 * @brief Essence Mod: shows a gain of experience. Only the newest is ever on screen: it replaces
 * the one before, and it is cut off after two lines however many boons are sharing in it.
 */
void EventXpMsg(std::string_view text);
void SendPlrMsg(Player &player, std::string_view text);
void InitPlrMsg();
void DrawPlrMsg(const Surface &out);

} // namespace devilution
