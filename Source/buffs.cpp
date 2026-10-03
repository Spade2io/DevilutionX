/**
 * @file buffs.cpp
 *
 * Essence Mod: persistent buffs and auras.
 */
#include "buffs.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "diablo.h"
#include "dots.h"
#include "engine/point.hpp"
#include "engine/rectangle.hpp"
#include "engine/render/text_render.hpp"
#include "items.h"
#include "missiles.h"
#include "monster.h"
#include "multi.h"
#include "panels/spell_icons.hpp"
#include "player.h"
#include "tables/misdat.h"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

constexpr size_t BuffCount = static_cast<size_t>(BuffID::LAST) + 1;

/** Game ticks between aura pulses: 2 seconds at normal speed. */
constexpr int AuraPulseInterval = 40;

/** Fire Aura damage per pulse at spell level 1, in 64ths of a hit point. */
constexpr int FireAuraBaseDamage = 1 * 64;

/** Which buffs each player has on. Memory only; never saved. */
std::array<std::array<bool, BuffCount>, MAX_PLRS> ActiveBuffs {};

/** Game ticks until each player's auras next pulse. */
std::array<int, MAX_PLRS> AuraPulseCountdown {};

std::array<bool, BuffCount> &BuffsOf(const Player &player)
{
	return ActiveBuffs[player.getId()];
}

int SpellLevelOf(const Player &player, SpellID spell)
{
	return std::max<int>(player._pSplLvl[static_cast<size_t>(spell)], 1);
}

/** Fire Aura reaches 2 tiles at spell level 1 and one tile further every two levels. */
int FireAuraRadius(const Player &player)
{
	return 2 + (SpellLevelOf(player, SpellID::FireAura) - 1) / 2;
}

int FireAuraDamage(const Player &player)
{
	return ScaleDamageForSpellLevel(FireAuraBaseDamage, SpellLevelOf(player, SpellID::FireAura));
}

/** Formats damage held in 64ths with up to two decimals, e.g. 72 -> "1.12". */
std::string FormatDamage(int damage)
{
	const int hundredths = damage * 100 / 64;
	if (hundredths % 100 == 0)
		return StrCat(hundredths / 100);
	return StrCat(hundredths / 100, ".", (hundredths % 100) / 10, hundredths % 10);
}

/** What a buff currently does, for the label shown when the mouse is over its icon. */
std::string DescribeBuff(const Player &player, BuffID buff)
{
	switch (buff) {
	case BuffID::Strength:
		return StrCat("Strength: +", GetBuffStrengthBonus(player), " Strength");
	case BuffID::FireAura:
		return StrCat("Fire Aura: ", FormatDamage(FireAuraDamage(player)), " fire damage every 2 seconds within ", FireAuraRadius(player), " tiles");
	}
	return {};
}

bool IsInFireAura(const Player &player, const Monster &monster, int radius)
{
	if (monster.hasNoLife() || !monster.isPossibleToHit() || monster.isPlayerMinion())
		return false;
	if (player.position.tile.WalkingDistance(monster.position.tile) > radius)
		return false;
	// The aura does not reach through walls.
	return LineClearMissile(player.position.tile, monster.position.tile);
}

void PulseFireAura(Player &player)
{
	const int radius = FireAuraRadius(player);
	const int damage = FireAuraDamage(player);
	bool anyMonsterInReach = false;

	for (size_t i = 0; i < ActiveMonsterCount; i++) {
		Monster &monster = Monsters[ActiveMonsters[i]];
		if (!IsInFireAura(player, monster, radius))
			continue;
		anyMonsterInReach = true;
		// Only the aura owner's own PC deals the damage; it is reported to the others as normal.
		if (&player == MyPlayer)
			DealSpellTickDamage(monster, SpellID::FireAura, MissileID::FireAuraPulse, DamageType::Fire, damage);
	}

	// The burst at the player's feet only plays when the aura has something to burn,
	// so walking around an empty dungeon stays quiet.
	// The Flash burst is drawn as two pictures, a front half and a back half, so both are needed.
	if (anyMonsterInReach) {
		AddMissile(player.position.tile, player.position.tile, player._pdir, MissileID::FireAuraPulse, TARGET_MONSTERS, player, 0, 0);
		AddMissile(player.position.tile, player.position.tile, player._pdir, MissileID::FireAuraPulseBack, TARGET_MONSTERS, player, 0, 0);
	}
}

} // namespace

SpellID GetBuffSpell(BuffID buff)
{
	switch (buff) {
	case BuffID::Strength:
		return SpellID::Strength;
	case BuffID::FireAura:
		return SpellID::FireAura;
	}
	return SpellID::Invalid;
}

bool BuffSharesExperience(BuffID buff)
{
	return buff != BuffID::FireAura;
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
	return 10 + 5 * (SpellLevelOf(player, SpellID::Strength) - 1);
}

void ProcessBuffs()
{
	for (Player &player : Players) {
		if (!player.plractive || !player.isOnActiveLevel() || player.hasNoLife())
			continue;
		if (!IsBuffActive(player, BuffID::FireAura))
			continue;

		int &countdown = AuraPulseCountdown[player.getId()];
		if (--countdown > 0)
			continue;
		countdown = AuraPulseInterval;
		PulseFireAura(player);
	}
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
			    Rectangle { Point { Margin, Margin + IconHeight + 2 }, Size { 420, 16 } },
			    { .flags = UiFlags::ColorWhitegold });
		}

		position.x += IconWidth + Gap;
	}
}

} // namespace devilution
