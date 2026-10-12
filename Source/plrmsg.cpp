/**
 * @file plrmsg.cpp
 *
 * Implementation of functionality for printing the ingame chat messages.
 */
#include "plrmsg.h"

#include <algorithm>
#include <array>
#include <cstdint>

#ifdef USE_SDL3
#include <SDL3/SDL_timer.h>
#else
#include <SDL.h>
#endif

#include "control/control.hpp"
#include "engine/render/primitive_render.hpp"
#include "engine/render/text_render.hpp"
#include "inv.h"
#include "qol/chatlog.h"
#include "qol/stash.h"
#include "utils/algorithm/container.hpp"
#include "utils/format.hpp"
#include "utils/language.h"
#include "utils/utf8.hpp"

namespace devilution {

namespace {

struct PlayerMessage {
	/** Time message was received */
	uint32_t time;
	/** The default text color */
	UiFlags style;
	/** The text message to display on screen */
	std::string text;
	/** Length of first portion of text that should be rendered in gold */
	size_t prefixLength;
	/** The line height of the text */
	int lineHeight;
	/** Essence Mod: a gain of experience. Only one is kept, and it is drawn two lines long at most. */
	bool isExperience = false;
};

std::array<PlayerMessage, 8> Messages;

int CountLinesOfText(std::string_view text)
{
	return static_cast<int>(1 + c_count(text, '\n'));
}

PlayerMessage &GetNextMessage()
{
	std::move_backward(Messages.begin(), Messages.end() - 1, Messages.end()); // Push back older messages

	return Messages.front();
}

} // namespace

void DelayPlrMessages(uint32_t delayTime)
{
	for (PlayerMessage &message : Messages)
		message.time += delayTime;
}

void EventPlrMsg(std::string_view text, UiFlags style)
{
	PlayerMessage &message = GetNextMessage();

	message.style = style;
	message.time = SDL_GetTicks();
	message.text = std::string(text);
	message.prefixLength = 0;
	message.lineHeight = GetLineHeight(message.text, GameFont12) + 3;
	message.isExperience = false;
	AddMessageToChatLog(text);
}

void EventXpMsg(std::string_view text)
{
	// Take the last gain off the screen: the new one stands in its place.
	const auto old = std::find_if(Messages.begin(), Messages.end(), [](const PlayerMessage &message) { return message.isExperience; });
	if (old != Messages.end()) {
		std::move(old + 1, Messages.end(), old);
		Messages.back() = {};
	}

	PlayerMessage &message = GetNextMessage();
	message.style = UiFlags::ColorWhite;
	message.time = SDL_GetTicks();
	message.text = std::string(text);
	message.prefixLength = 0;
	message.lineHeight = GetLineHeight(message.text, GameFont12) + 3;
	message.isExperience = true;
	// Kept out of the chat log, which it would bury.
}

void SendPlrMsg(Player &player, std::string_view text)
{
	PlayerMessage &message = GetNextMessage();

	const std::string from = FormatRuntime(_("{:s} (lvl {:d}): "), player._pName, player.getCharacterLevel());

	message.style = UiFlags::ColorWhite;
	message.time = SDL_GetTicks();
	message.text = from + std::string(text);
	message.prefixLength = from.size();
	message.lineHeight = GetLineHeight(message.text, GameFont12) + 3;
	message.isExperience = false;
	AddMessageToChatLog(text, &player);
}

void InitPlrMsg()
{
	Messages = {};
}

void DrawPlrMsg(const Surface &out)
{
	if (ChatLogFlag)
		return;

	int x = 10;
	int y = GetMainPanel().position.y - 13;
	int width = gnScreenWidth - 20;

	if (!ChatFlag && IsLeftPanelOpen()) {
		x += GetLeftPanel().position.x + GetLeftPanel().size.width;
		width -= GetLeftPanel().size.width;
	}
	if (!ChatFlag && IsRightPanelOpen())
		width -= gnScreenWidth - GetRightPanel().position.x;

	if (width < 300)
		return;

	width = std::min(540, width);

	for (const PlayerMessage &message : Messages) {
		if (message.text.empty())
			break;
		if (!ChatFlag && SDL_GetTicks() - message.time >= 10000)
			break;

		std::string text = WordWrapString(message.text, width);
		// A gain of experience is cut off after two lines.
		if (message.isExperience) {
			const size_t firstBreak = text.find('\n');
			const size_t secondBreak = firstBreak == std::string::npos ? firstBreak : text.find('\n', firstBreak + 1);
			if (secondBreak != std::string::npos) {
				text.resize(secondBreak);
				text += " ...";
			}
		}
		const int chatlines = CountLinesOfText(text);
		y -= message.lineHeight * chatlines;

		DrawHalfTransparentRectTo(out, x - 3, y, width + 6, message.lineHeight * chatlines);

		std::array<DrawStringFormatArg, 2> args {
			DrawStringFormatArg { std::string_view(text.data(), message.prefixLength), UiFlags::ColorWhitegold },
			DrawStringFormatArg { std::string_view(text.data() + message.prefixLength, text.size() - message.prefixLength), message.style }
		};
		DrawStringWithColors(out, "{:s}{:s}", args.data(), args.size(), { { x, y }, { width, 0 } },
		    { .flags = UiFlags::None, .lineHeight = message.lineHeight });
	}
}

} // namespace devilution
