/**
 * @file buffs.cpp
 *
 * Essence Mod: persistent buffs.
 */
#include "buffs.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "diablo.h"
#include "engine/point.hpp"
#include "engine/rectangle.hpp"
#include "engine/render/text_render.hpp"
#include "items.h"
#include "multi.h"
#include "panels/spell_icons.hpp"
#include "player.h"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

constexpr size_t BuffCount = static_cast<size_t>(BuffID::LAST) + 1;

/** Which buffs each player has on. Memory only; never saved. */
std::array<std::array<bool, BuffCount>, MAX_PLRS> ActiveBuffs {};

std::array<bool, BuffCount> &BuffsOf(const Player &player)
{
	return ActiveBuffs[player.getId()];
}

/** What a buff currently does, for the label shown when the mouse is over its icon. */
std::string DescribeBuff(const Player &player, BuffID buff)
{
	switch (buff) {
	case BuffID::Strength:
		return StrCat("Strength: +", GetBuffStrengthBonus(player), " Strength");
	}
	return {};
}

} // namespace

SpellID GetBuffSpell(BuffID buff)
{
	switch (buff) {
	case BuffID::Strength:
		return SpellID::Strength;
	}
	return SpellID::Invalid;
}

void ActivateBuff(Player &player, BuffID buff)
{
	BuffsOf(player)[static_cast<size_t>(buff)] = true;
	CalcPlrInv(player, true);
}

bool IsBuffActive(const Player &player, BuffID buff)
{
	return BuffsOf(player)[static_cast<size_t>(buff)];
}

void ClearBuffs(Player &player)
{
	BuffsOf(player).fill(false);
}

void RefreshBuffStats(Player &player)
{
	const auto &buffs = BuffsOf(player);
	if (std::any_of(buffs.begin(), buffs.end(), [](bool active) { return active; }))
		CalcPlrInv(player, true);
}

int GetBuffStrengthBonus(const Player &player)
{
	if (!IsBuffActive(player, BuffID::Strength))
		return 0;
	const int level = std::max<int>(player._pSplLvl[static_cast<size_t>(SpellID::Strength)], 1);
	return 10 + 5 * (level - 1);
}

void DrawBuffBar(const Surface &out)
{
	if (MyPlayer == nullptr)
		return;
	const Player &player = *MyPlayer;

	constexpr int IconWidth = 37;
	constexpr int IconHeight = 38;
	constexpr int Margin = 8;
	constexpr int Gap = 4;

	// Icons are drawn from their bottom-left corner.
	Point position { Margin, Margin + IconHeight - 1 };
	for (size_t i = 0; i < BuffCount; i++) {
		const auto buff = static_cast<BuffID>(i);
		if (!IsBuffActive(player, buff))
			continue;

		SetSpellTrans(SpellType::Spell);
		DrawSmallSpellIcon(out, position, GetBuffSpell(buff));

		const Rectangle iconArea { Point { position.x, position.y - IconHeight + 1 }, Size { IconWidth, IconHeight } };
		if (iconArea.contains(MousePosition)) {
			DrawString(out, DescribeBuff(player, buff),
			    Rectangle { Point { Margin, Margin + IconHeight + 2 }, Size { 300, 16 } },
			    { .flags = UiFlags::ColorWhitegold });
		}

		position.x += IconWidth + Gap;
	}
}

} // namespace devilution
