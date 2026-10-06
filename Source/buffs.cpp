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
#include <vector>

#include "cooldowns.h"
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
#include "plrmsg.h"
#include "spells.h"
#include "tables/misdat.h"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

constexpr size_t BuffCount = static_cast<size_t>(BuffID::LAST) + 1;

/** Game ticks per second at normal speed. */
constexpr int TicksPerSecond = 20;

/** Game ticks between aura pulses: 2 seconds at normal speed. */
constexpr int AuraPulseInterval = 40;

/** Fire Aura damage per pulse at spell level 1, in 64ths of a hit point. */
constexpr int FireAuraBaseDamage = 1 * 64;

/** Fire damage Flaming Weapon adds to a weapon hit at spell level 1, in 64ths of a hit point. */
constexpr int FlamingWeaponBaseDamage = 3 * 64;

/** Which buffs each player has on. Memory only; never saved. */
std::array<std::array<bool, BuffCount>, MAX_PLRS> ActiveBuffs {};

/** A buff read from essence_powers.tsv that a player has running. */
struct PowerBuff {
	SpellID spell;
	/** Game ticks left, or -1 for a lasting buff that does not run out. */
	int ticksLeft;
};

/** The data-file buffs each player has running. Memory only; never saved. */
std::array<std::vector<PowerBuff>, MAX_PLRS> PowerBuffs;

/** A passive power that has just acted for the local player and still owes its casting motion. */
SpellID PendingCastMotion = SpellID::Invalid;
/** Game ticks left to find a moment for that motion before giving up on it. */
int PendingCastMotionTicks = 0;

/** How long the local player is protected after rising: a second and a half. */
constexpr int RebirthProtectionTicks = 30;
/** Game ticks of that protection left. */
int RebirthProtectionLeft = 0;

/** Game ticks until each player's auras next pulse. */
std::array<int, MAX_PLRS> AuraPulseCountdown {};

std::array<bool, BuffCount> &BuffsOf(const Player &player)
{
	return ActiveBuffs[player.getId()];
}

int SpellLevelOf(const Player &player, SpellID spell)
{
	return std::max<int>(player.GetBaseSpellLevel(spell), 1);
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
		return StrCat("Strength: +", GetBuffStrengthBonus(player), " Power");
	case BuffID::FireAura:
		return StrCat("Fire Aura: ", FormatDamage(FireAuraDamage(player)), " fire damage every 2 seconds within ", FireAuraRadius(player), " tiles");
	case BuffID::FlamingWeapon:
		return StrCat("Flaming Weapon: +", FormatDamage(GetFlamingWeaponDamage(player)), " fire damage on every weapon hit");
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
	case BuffID::FlamingWeapon:
		return SpellID::FlamingWeapon;
	}
	return SpellID::Invalid;
}

bool BuffSharesExperience(BuffID buff)
{
	// Only passive buffs, which deal no damage of their own, earn a tenth of the player's experience
	// gains. Buffs that deal damage (Fire Aura, Flaming Weapon) earn the full value of that damage instead.
	return buff == BuffID::Strength;
}

bool IsExperienceSharingBuffSpell(SpellID spell)
{
	for (size_t i = 0; i < BuffCount; i++) {
		const auto buff = static_cast<BuffID>(i);
		if (GetBuffSpell(buff) == spell)
			return BuffSharesExperience(buff);
	}
	// Every buff read from essence_powers.tsv is a passive one.
	return IsExtendedSpell(spell) && IsValidSpell(spell) && GetSpellData(spell).effect == "Buff";
}

int GetFlamingWeaponDamage(const Player &player)
{
	if (!IsBuffActive(player, BuffID::FlamingWeapon))
		return 0;
	return ScaleDamageForSpellLevel(FlamingWeaponBaseDamage, SpellLevelOf(player, SpellID::FlamingWeapon));
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
	PowerBuffs[player.getId()].clear();
	if (&player == MyPlayer) {
		RebirthProtectionLeft = 0;
		PendingCastMotion = SpellID::Invalid;
	}
}

void ActivatePowerBuff(Player &player, SpellID spell)
{
	if (!IsValidSpell(spell))
		return;
	const int seconds = GetSpellData(spell).durationSeconds;
	const int ticks = seconds > 0 ? seconds * TicksPerSecond : -1;
	std::vector<PowerBuff> &buffs = PowerBuffs[player.getId()];
	for (PowerBuff &buff : buffs) {
		if (buff.spell == spell) {
			buff.ticksLeft = ticks;
			return;
		}
	}
	buffs.push_back(PowerBuff { spell, ticks });
	CalcPlrInv(player, true);
}

std::vector<SpellID> GetActivePowerBuffs(const Player &player)
{
	std::vector<SpellID> spells;
	for (const PowerBuff &buff : PowerBuffs[player.getId()])
		spells.push_back(buff.spell);
	return spells;
}

int ApplyDamageBuffs(const Player &player, int damage)
{
	int percent = 0;
	for (const PowerBuff &buff : PowerBuffs[player.getId()]) {
		const SpellData &spellData = GetSpellData(buff.spell);
		// Buffs to the same stat do not add up: the strongest one counts.
		if (spellData.buffStat == "Damage")
			percent = std::max(percent, ScaleDamageForSpellLevel(spellData.effectAmount, SpellLevelOf(player, buff.spell)));
	}
	if (percent == 0)
		return damage;
	return damage + static_cast<int>(static_cast<int64_t>(damage) * percent / 100);
}

int GetBuffSkippedFrames(const Player &player)
{
	int percent = 0;
	for (const PowerBuff &buff : PowerBuffs[player.getId()]) {
		const SpellData &spellData = GetSpellData(buff.spell);
		// Buffs to the same stat do not add up: the strongest one counts.
		if (spellData.buffStat == "Speed")
			percent = std::max(percent, ScaleDamageForSpellLevel(spellData.effectAmount, SpellLevelOf(player, buff.spell)));
	}
	return std::clamp(percent / 10, 0, 4);
}

bool IsPassivePower(SpellID spell)
{
	return IsExtendedSpell(spell) && IsValidSpell(spell) && GetSpellData(spell).effect == "Rebirth";
}

std::vector<SpellID> GetPassivePowers(const Player &player)
{
	std::vector<SpellID> spells;
	for (const auto &[spell, level] : player.extendedSpellLevels) {
		if (level != 0 && IsPassivePower(spell))
			spells.push_back(spell);
	}
	return spells;
}

bool TryRebirth(Player &player)
{
	for (const SpellID spell : GetPassivePowers(player)) {
		if (IsSpellOnCooldown(spell))
			continue;
		// The power's amount is the percentage of maximum life the player rises with.
		const int percent = std::clamp(ScaleDamageForSpellLevel(GetSpellData(spell).effectAmount, SpellLevelOf(player, spell)), 1, 100);
		SetPlayerHitPoints(player, std::max<int>(static_cast<int>(static_cast<int64_t>(player._pMaxHP) * percent / 100), 64));
		StartSpellCooldown(spell);
		EventPlrMsg(StrCat(GetSpellData(spell).sNameText, ": you rise again"), UiFlags::ColorWhitegold);
		// The casting motion starts on the next tick: the blow that would have killed the player
		// is still being dealt, and it may yet throw them into a stagger that would cut it short.
		PendingCastMotion = spell;
		PendingCastMotionTicks = 2 * TicksPerSecond;
		RebirthProtectionLeft = RebirthProtectionTicks;
		return true;
	}
	return false;
}

bool IsRebirthProtected(const Player &player)
{
	return &player == MyPlayer && RebirthProtectionLeft > 0;
}

void ProcessBuffTimers()
{
	if (RebirthProtectionLeft > 0)
		RebirthProtectionLeft--;

	if (PendingCastMotion != SpellID::Invalid && MyPlayer != nullptr) {
		if (StartCastMotion(*MyPlayer, PendingCastMotion) || --PendingCastMotionTicks <= 0)
			PendingCastMotion = SpellID::Invalid;
	}

	for (Player &player : Players) {
		if (!player.plractive)
			continue;
		std::vector<PowerBuff> &buffs = PowerBuffs[player.getId()];
		bool expired = false;
		for (PowerBuff &buff : buffs) {
			if (buff.ticksLeft > 0 && --buff.ticksLeft == 0)
				expired = true;
		}
		if (!expired)
			continue;
		std::erase_if(buffs, [](const PowerBuff &buff) { return buff.ticksLeft == 0; });
		CalcPlrInv(player, true);
	}
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

	// Buffs read from essence_powers.tsv follow, with the seconds left on those that run out.
	for (const PowerBuff &buff : PowerBuffs[player.getId()]) {
		SetSpellTrans(SpellType::Spell);
		DrawSmallSpellIcon(out, position, buff.spell);

		const Rectangle iconArea { Point { position.x, position.y - IconHeight + 1 }, Size { IconWidth, IconHeight } };
		const SpellData &spellData = GetSpellData(buff.spell);
		const int secondsLeft = buff.ticksLeft > 0 ? (buff.ticksLeft + TicksPerSecond - 1) / TicksPerSecond : 0;
		if (buff.ticksLeft > 0) {
			DrawString(out, StrCat(secondsLeft), Rectangle { Point { position.x, position.y - 13 }, Size { IconWidth - 2, 12 } },
			    { .flags = UiFlags::ColorWhite | UiFlags::AlignRight | UiFlags::FontSize12 });
		}
		if (iconArea.contains(MousePosition)) {
			std::string text = StrCat(spellData.sNameText, ": ", spellData.description);
			if (buff.ticksLeft > 0)
				StrAppend(text, " (", secondsLeft, "s left)");
			DrawString(out, text,
			    Rectangle { Point { Margin, Margin + IconHeight + 2 }, Size { 420, 16 } },
			    { .flags = UiFlags::ColorWhitegold });
		}

		position.x += IconWidth + Gap;
	}
}

} // namespace devilution
