/**
 * @file xpbar.cpp
 *
 * Adds XP bar QoL feature
 */
#include "xpbar.h"

#include <array>
#include <cstdint>

#include "control/control.hpp"
#include "essences.h"
#include "engine/clx_sprite.hpp"
#include "engine/load_clx.hpp"
#include "engine/point.hpp"
#include "engine/render/clx_render.hpp"
#include "engine/render/primitive_render.hpp"
#include "game_mode.hpp"
#include "options.h"
#include "tables/playerdat.hpp"
#include "utils/format.hpp"
#include "utils/format_int.hpp"
#include "utils/language.h"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

constexpr int BarWidth = 307;

using ColorGradient = std::array<Uint8, 12>;
constexpr ColorGradient GoldGradient = { 0xCF, 0xCE, 0xCD, 0xCC, 0xCB, 0xCA, 0xC9, 0xC8, 0xC7, 0xC6, 0xC5, 0xC4 };
constexpr ColorGradient SilverGradient = { 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8, 0xF7, 0xF6, 0xF5, 0xF4, 0xF3 };

constexpr int BackWidth = 313;
constexpr int BackHeight = 9;

OptionalOwnedClxSpriteList xpbarArt;

void DrawBar(const Surface &out, Point screenPosition, int width, const ColorGradient &gradient)
{
	UnsafeDrawHorizontalLine(out, screenPosition + Displacement { 0, 1 }, width, gradient[(gradient.size() * 3 / 4) - 1]);
	UnsafeDrawHorizontalLine(out, screenPosition + Displacement { 0, 2 }, width, gradient[gradient.size() - 1]);
	UnsafeDrawHorizontalLine(out, screenPosition + Displacement { 0, 3 }, width, gradient[(gradient.size() / 2) - 1]);
}

void DrawEndCap(const Surface &out, Point point, int idx, const ColorGradient &gradient)
{
	out.SetPixel({ point.x, point.y + 1 }, gradient[idx * 3 / 4]);
	out.SetPixel({ point.x, point.y + 2 }, gradient[idx]);
	out.SetPixel({ point.x, point.y + 3 }, gradient[idx / 2]);
}

void OptionExperienceBarChanged()
{
	if (!gbRunGame)
		return;
	if (*GetOptions().Gameplay.experienceBar)
		InitXPBar();
	else
		FreeXPBar();
}

const auto OptionChangeHandler = (GetOptions().Gameplay.experienceBar.SetValueChangedCallback(OptionExperienceBarChanged), true);

} // namespace

void InitXPBar()
{
	if (*GetOptions().Gameplay.experienceBar) {
		xpbarArt = LoadClx("data\\xpbar.clx");
	}
}

void FreeXPBar()
{
	xpbarArt = std::nullopt;
}

void DrawXPBar(const Surface &out)
{
	if (!*GetOptions().Gameplay.experienceBar || ChatFlag)
		return;

	const Player &player = *MyPlayer;
	const Rectangle &mainPanel = GetMainPanel();

	const Point back = { mainPanel.position.x + (mainPanel.size.width / 2) - 155, mainPanel.position.y + mainPanel.size.height - 11 };
	const Point position = back + Displacement { 3, 2 };

	RenderClxSprite(out, (*xpbarArt)[0], back);

	if (player.isMaxCharacterLevel()) {
		// Draw a nice golden bar for max level characters.
		DrawBar(out, position, BarWidth, GoldGradient);

		return;
	}

	// Essence Mod: the bar fills an eighth for each level one of the player's powers gains, and is
	// full when the eighth makes the character's next level.
	const int inThisLevel = GetTotalPowerLevels(player) % PowerLevelsPerCharacterLevel;
	DrawBar(out, position, BarWidth * inThisLevel / PowerLevelsPerCharacterLevel, SilverGradient);
}

bool CheckXPBarInfo()
{
	if (!*GetOptions().Gameplay.experienceBar)
		return false;
	const Rectangle &mainPanel = GetMainPanel();

	const int backX = mainPanel.position.x + (mainPanel.size.width / 2) - 155;
	const int backY = mainPanel.position.y + mainPanel.size.height - 11;

	if (MousePosition.x < backX || MousePosition.x >= backX + BackWidth || MousePosition.y < backY || MousePosition.y >= backY + BackHeight)
		return false;

	const Player &player = *MyPlayer;

	const uint8_t charLevel = player.getCharacterLevel();

	AddInfoBoxString(FormatRuntime(_("Level {:d}"), charLevel));

	if (player.isMaxCharacterLevel()) {
		// Show a maximum level indicator for max level players.
		InfoColor = UiFlags::ColorWhitegold;

		AddInfoBoxString(FormatRuntime(_("Experience: {:s}"), FormatInteger(player._pExperience)));
		AddInfoBoxString(_("Maximum Level"));

		return true;
	}

	InfoColor = UiFlags::ColorWhite;

	// Essence Mod: a character's level comes from the levels of their powers, eight to a level.
	const int powerLevels = GetTotalPowerLevels(player);
	const int nextAt = (powerLevels / PowerLevelsPerCharacterLevel + 1) * PowerLevelsPerCharacterLevel;
	AddInfoBoxString(StrCat("Power levels: ", powerLevels, " of ", nextAt));
	AddInfoBoxString(StrCat(nextAt - powerLevels, " more to Level ", charLevel + 1));

	return true;
}

} // namespace devilution
