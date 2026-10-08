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
#include "engine/backbuffer_state.hpp"
#include "engine/render/text_render.hpp"
#include "dots.h"
#include "essence_tint.h"
#include "qol/floatingnumbers.h"
#include "items.h"
#include "missiles.h"
#include "monster.h"
#include "msg.h"
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
	/** Its strength, as given by the caster's PC. The buffed player need not know the power. */
	int amount;
	/** Who cast it. A buff from someone else is drawn in a different colour. */
	uint8_t caster;
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

bool ActivatePowerBuff(Player &player, SpellID spell, int amount, const Player &caster)
{
	if (!IsValidSpell(spell))
		return false;
	const int seconds = GetSpellData(spell).durationSeconds;
	const int ticks = seconds > 0 ? seconds * TicksPerSecond : -1;
	std::vector<PowerBuff> &buffs = PowerBuffs[player.getId()];
	for (PowerBuff &buff : buffs) {
		if (buff.spell != spell)
			continue;
		// A lasting buff keeps the stronger cast. A timed one takes the newest, weaker or not.
		if (ticks < 0 && amount < buff.amount)
			return false;
		buff.ticksLeft = ticks;
		buff.caster = caster.getId();
		if (buff.amount != amount) {
			buff.amount = amount;
			CalcPlrInv(player, true);
		}
		return true;
	}
	buffs.push_back(PowerBuff { spell, ticks, amount, caster.getId() });
	CalcPlrInv(player, true);
	return true;
}

std::vector<SpellID> GetActivePowerBuffs(const Player &player)
{
	std::vector<SpellID> spells;
	for (const PowerBuff &buff : PowerBuffs[player.getId()])
		spells.push_back(buff.spell);
	return spells;
}

/** Whether another player is somewhere their auras could reach this player: alive and on the same level. */
bool AuraOwnerReaches(const Player &owner, const Player &player)
{
	return &owner != &player && owner.plractive && !owner.hasNoLife()
	    && owner.plrlevel == player.plrlevel && owner.plrIsOnSetLevel == player.plrIsOnSetLevel;
}

/** A buff or aura that is acting on a player, wherever it comes from. */
struct ReachingBuff {
	SpellID spell;
	int amount;
	/** Game ticks left, or -1 for one that does not run out. */
	int ticksLeft;
	/** Whether another player is the source: they cast it on this player, or it is their aura. */
	bool fromOther;
};

/**
 * Everything acting on a player: their own buffs, then the auras of other players in reach.
 * Each buff appears once. Where the same aura arrives from several players the strongest is kept.
 */
std::vector<ReachingBuff> GetReachingBuffs(const Player &player)
{
	std::vector<ReachingBuff> reaching;
	for (const PowerBuff &buff : PowerBuffs[player.getId()])
		reaching.push_back({ buff.spell, buff.amount, buff.ticksLeft, buff.caster != player.getId() });
	for (const Player &other : Players) {
		if (!AuraOwnerReaches(other, player))
			continue;
		for (const PowerBuff &buff : PowerBuffs[other.getId()]) {
			const SpellData &spellData = GetSpellData(buff.spell);
			if (spellData.effect != "Aura" || other.position.tile.WalkingDistance(player.position.tile) > spellData.effectRadius)
				continue;
			const auto held = std::find_if(reaching.begin(), reaching.end(), [&](const ReachingBuff &entry) { return entry.spell == buff.spell; });
			if (held == reaching.end()) {
				reaching.push_back({ buff.spell, buff.amount, -1, true });
			} else if (buff.amount > held->amount) {
				held->amount = buff.amount;
				held->fromOther = true;
			}
		}
	}
	return reaching;
}

int GetPowerBuffPercent(const Player &player, std::string_view stat)
{
	int percent = 0;
	for (const ReachingBuff &buff : GetReachingBuffs(player)) {
		const SpellData &spellData = GetSpellData(buff.spell);
		if (spellData.buffStat == stat)
			percent += buff.amount;
		// A buff's rider can name a second stat it changes, by the rider's own amount.
		else if (!spellData.buffStat.empty() && spellData.rider == stat && stat != "Distance")
			percent += spellData.riderAmount;
	}
	return percent;
}

/** How near a player with the Fire Aura seems to monsters: half as far away as they are. */
constexpr int FireAuraDistanceRatio = 50;

int GetPlayerDistanceRatio(const Player &player)
{
	int ratio = 100;
	// The Fire Aura draws monsters to its owner. Only the owner: it gives allies nothing.
	if (IsBuffActive(player, BuffID::FireAura))
		ratio = FireAuraDistanceRatio;
	for (const PowerBuff &buff : PowerBuffs[player.getId()]) {
		// The one furthest from normal counts.
		const SpellData &spellData = GetSpellData(buff.spell);
		if (spellData.buffStat == "Distance" && std::abs(buff.amount - 100) > std::abs(ratio - 100))
			ratio = buff.amount;
		// A buff that is mainly about something else can carry threat or stealth as its second gift.
		if (!spellData.buffStat.empty() && spellData.rider == "Distance" && std::abs(spellData.riderAmount - 100) > std::abs(ratio - 100))
			ratio = spellData.riderAmount;
	}
	return ratio;
}

int GetPlayerNoticeRange(const Player &player)
{
	const int ratio = GetPlayerDistanceRatio(player);
	if (ratio <= 100)
		return 255; // as far as they can be seen, as in the original game
	// Normal sight reaches about 10 tiles. A stealthy player is noticed from proportionally
	// closer, but never from less than 3 tiles.
	return std::max(10 * 100 / ratio, 3);
}

bool HasHealOverTimeFrom(const Player &target, const Player &caster)
{
	for (const PowerBuff &buff : PowerBuffs[target.getId()]) {
		if (buff.ticksLeft > 0 && buff.caster == caster.getId() && GetSpellData(buff.spell).buffStat == "HealPulse")
			return true;
	}
	return false;
}

/** What the local player has left of the shield an aura gave them. It is renewed every pulse. */
int AuraShieldLeft = 0;

int AbsorbDamageWithShields(Player &player, int damage)
{
	if (&player != MyPlayer || damage <= 0)
		return damage;
	const int before = damage;

	const int fromAura = std::min(AuraShieldLeft, damage);
	AuraShieldLeft -= fromAura;
	damage -= fromAura;

	std::vector<PowerBuff> &buffs = PowerBuffs[player.getId()];
	bool emptied = false;
	for (PowerBuff &buff : buffs) {
		if (damage <= 0)
			break;
		if (GetSpellData(buff.spell).buffStat != "Shield")
			continue;
		const int taken = std::min(buff.amount, damage);
		buff.amount -= taken;
		damage -= taken;
		emptied |= buff.amount <= 0;
	}
	if (emptied) {
		std::erase_if(buffs, [](const PowerBuff &buff) { return buff.amount <= 0 && GetSpellData(buff.spell).buffStat == "Shield"; });
		CalcPlrInv(player, true);
	}

	if (before > damage)
		AddFloatingNumber(player.position.tile, { 0, 0 }, StrCat("Shield -", std::max((before - damage + 32) >> 6, 1)), UiFlags::ColorGold | UiFlags::FontSize12, 3000 + player.getId());
	return damage;
}

int ReduceDamageTaken(const Player &player, int damage)
{
	const int percent = std::clamp(GetPowerBuffPercent(player, "DamageTaken"), 0, 90);
	return damage - static_cast<int>(static_cast<int64_t>(damage) * percent / 100);
}

const Player *GetOathGuardian(const Player &player, int &percent)
{
	for (const PowerBuff &buff : PowerBuffs[player.getId()]) {
		if (GetSpellData(buff.spell).buffStat != "Oath" || buff.caster == player.getId() || buff.caster >= Players.size())
			continue;
		const Player &guardian = Players[buff.caster];
		if (!guardian.plractive || guardian.hasNoLife() || guardian.plrlevel != player.plrlevel || guardian.plrIsOnSetLevel != player.plrIsOnSetLevel)
			continue;
		percent = std::clamp(buff.amount, 0, 90);
		return &guardian;
	}
	return nullptr;
}

void RemovePowerBuffFromOthers(SpellID spell, const Player &caster, const Player &keep)
{
	for (Player &other : Players) {
		if (&other == &keep)
			continue;
		std::vector<PowerBuff> &buffs = PowerBuffs[other.getId()];
		if (std::erase_if(buffs, [&](const PowerBuff &buff) { return buff.spell == spell && buff.caster == caster.getId(); }) > 0)
			CalcPlrInv(other, true);
	}
}

/** Game ticks between the pulses of shield auras: every 10 seconds. */
constexpr int ShieldPulseInterval = 10 * TicksPerSecond;
int ShieldPulseCountdown = ShieldPulseInterval;

/**
 * Auras with the stat "ShieldPulse" give everyone they reach a small shield every 10 seconds.
 * It is renewed, not added to: what is left of the last one is replaced. Out of reach at a
 * pulse, it is gone. Each PC looks after its own player's.
 */
void PulseShieldAuras()
{
	if (--ShieldPulseCountdown > 0)
		return;
	ShieldPulseCountdown = ShieldPulseInterval;
	if (MyPlayer == nullptr || MyPlayer->hasNoLife()) {
		AuraShieldLeft = 0;
		return;
	}
	AuraShieldLeft = std::max(GetPowerBuffPercent(*MyPlayer, "ShieldPulse"), 0);
	if (AuraShieldLeft > 0)
		AddFloatingNumber(MyPlayer->position.tile, { 0, 0 }, StrCat("Shield ", std::max((AuraShieldLeft + 32) >> 6, 1)), UiFlags::ColorGold | UiFlags::FontSize12, 3000 + MyPlayer->getId());
}

int GetAuraResistanceCut(const Monster &monster, uint8_t categoryBit)
{
	if (categoryBit == 0)
		return 0;
	// The strongest copy of each aura in reach, then all of them added together.
	std::vector<std::pair<SpellID, int>> auras;
	for (const Player &owner : Players) {
		if (!owner.plractive || !owner.isOnActiveLevel() || owner.hasNoLife())
			continue;
		for (const PowerBuff &buff : PowerBuffs[owner.getId()]) {
			const SpellData &spellData = GetSpellData(buff.spell);
			if (spellData.effect != "Aura" || (GetResistCutCategories(spellData.buffStat) & categoryBit) == 0)
				continue;
			if (owner.position.tile.WalkingDistance(monster.position.tile) > spellData.effectRadius)
				continue;
			const auto known = std::find_if(auras.begin(), auras.end(), [&](const auto &entry) { return entry.first == buff.spell; });
			if (known == auras.end())
				auras.emplace_back(buff.spell, buff.amount);
			else
				known->second = std::max(known->second, buff.amount);
		}
	}
	int points = 0;
	for (const auto &[spell, amount] : auras)
		points += amount;
	return points;
}

bool GetPowerBuffWithStat(const Player &player, std::string_view stat, SpellID &spell, int &amount)
{
	for (const PowerBuff &buff : PowerBuffs[player.getId()]) {
		if (GetSpellData(buff.spell).buffStat != stat)
			continue;
		spell = buff.spell;
		amount = buff.amount;
		return true;
	}
	return false;
}

int ApplyDamageBuffs(const Player &player, int damage)
{
	const int percent = GetPowerBuffPercent(player, "Damage");
	if (percent == 0)
		return damage;
	return damage + static_cast<int>(static_cast<int64_t>(damage) * percent / 100);
}

int GetBuffSkippedFrames(const Player &player)
{
	return std::clamp(GetPowerBuffPercent(player, "Speed") / 10, 0, 4);
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
	// A buff with the stat "Rebirth" (an ally's Guardian Angel) does the same once, and is used up.
	std::vector<PowerBuff> &buffs = PowerBuffs[player.getId()];
	for (auto it = buffs.begin(); it != buffs.end(); ++it) {
		if (GetSpellData(it->spell).buffStat != "Rebirth")
			continue;
		const SpellID spell = it->spell;
		const int percent = std::clamp(it->amount, 1, 100);
		buffs.erase(it);
		SetPlayerHitPoints(player, std::max<int>(static_cast<int>(static_cast<int64_t>(player._pMaxHP) * percent / 100), 64));
		EventPlrMsg(StrCat(GetSpellData(spell).sNameText, ": you rise again"), UiFlags::ColorWhitegold);
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

/** Game ticks between the pulses of healing auras: every 2 seconds. */
constexpr int HealPulseInterval = 2 * TicksPerSecond;
int HealPulseCountdown = HealPulseInterval;

/**
 * Auras with the stat "HealPulse" restore life to everyone they reach every 2 seconds. Each PC
 * heals only its own player, who knows best how much life they have; the others see it arrive.
 */
void PulseHealingAuras()
{
	if (--HealPulseCountdown > 0)
		return;
	HealPulseCountdown = HealPulseInterval;
	if (MyPlayer == nullptr)
		return;
	for (Player &player : Players) {
		if (!player.plractive || !player.isOnActiveLevel() || player.hasNoLife() || player._pHitPoints >= player._pMaxHP)
			continue;
		int amount = 0;
		SpellID source = SpellID::Invalid;
		for (const ReachingBuff &buff : GetReachingBuffs(player)) {
			if (GetSpellData(buff.spell).buffStat != "HealPulse")
				continue;
			amount += buff.amount;
			source = buff.spell;
		}
		if (amount <= 0)
			continue;
		const int healed = std::min(amount, player._pMaxHP - player._pHitPoints);
		// Only a player's own PC changes their life. The others show the number, and see the
		// life itself arrive a moment later.
		if (&player == MyPlayer) {
			player._pHitPoints += healed;
			player._pHPBase = std::min(player._pHPBase + healed, player._pMaxHPBase);
			RedrawComponent(PanelDrawComponent::Health);
		}
		AddFloatingNumber(player.position.tile, { 0, 0 }, StrCat("+", std::max((healed + 32) >> 6, 1)), GetSpellTextColor(source) | UiFlags::FontSize12, 2000 + player.getId());
	}
}

/** What each player's item-calculated stats last received from buffs and auras. */
std::array<int, MAX_PLRS> LastStatBuffs {};
int StatCheckCountdown = TicksPerSecond;

/**
 * Resistances, armor and the life and mana pools are worked out with the player's items, not
 * every tick. An aura's share of them comes and goes as players walk, so once a second each
 * player whose share has changed has those stats worked out again.
 */
void RefreshAuraStats()
{
	if (--StatCheckCountdown > 0)
		return;
	StatCheckCountdown = TicksPerSecond;
	for (Player &player : Players) {
		if (!player.plractive || player.hasNoLife())
			continue;
		const int now = GetPowerBuffPercent(player, "Resist") + 1000 * GetPowerBuffPercent(player, "Armor")
		    + 1000000 * (GetPowerBuffPercent(player, "MaxLife") + GetPowerBuffPercent(player, "MaxMana"));
		if (now == LastStatBuffs[player.getId()])
			continue;
		LastStatBuffs[player.getId()] = now;
		CalcPlrInv(player, true);
	}
}

void ProcessBuffTimers()
{
	PulseHealingAuras();
	PulseShieldAuras();
	RefreshAuraStats();

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

void CancelBuff(Player &player, SpellID spell)
{
	for (size_t i = 0; i < BuffCount; i++) {
		if (GetBuffSpell(static_cast<BuffID>(i)) == spell)
			BuffsOf(player)[i] = false;
	}
	std::erase_if(PowerBuffs[player.getId()], [spell](const PowerBuff &buff) { return buff.spell == spell; });
	CalcPlrInv(player, true);
}

bool CancelBuffUnderCursor()
{
	if (MyPlayer == nullptr)
		return false;
	const Player &player = *MyPlayer;

	// The same places DrawBuffBar puts the icons.
	constexpr int IconWidth = 37;
	constexpr int IconHeight = 38;
	constexpr int Margin = 8;
	constexpr int Gap = 4;
	constexpr int Left = 68;

	Rectangle iconArea { Point { Left, Margin }, Size { IconWidth, IconHeight } };
	const auto cancel = [](SpellID spell) {
		NetSendCmdParam1(true, CMD_CANCELBUFF, static_cast<uint16_t>(spell));
	};
	for (size_t i = 0; i < BuffCount; i++) {
		const auto buff = static_cast<BuffID>(i);
		if (!IsBuffActive(player, buff))
			continue;
		if (iconArea.contains(MousePosition)) {
			cancel(GetBuffSpell(buff));
			return true;
		}
		iconArea.position.x += IconWidth + Gap;
	}
	for (const ReachingBuff &reaching : GetReachingBuffs(player)) {
		if (iconArea.contains(MousePosition)) {
			// Only what the player holds can be put down: an ally's aura is the ally's to switch off.
			const std::vector<PowerBuff> &held = PowerBuffs[player.getId()];
			if (std::any_of(held.begin(), held.end(), [&](const PowerBuff &buff) { return buff.spell == reaching.spell; }))
				cancel(reaching.spell);
			return true;
		}
		iconArea.position.x += IconWidth + Gap;
	}
	return false;
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
	// The party portraits run down the left edge of the screen in multiplayer; the icons start
	// just to the right of them, in every game, so they are always found in the same place.
	constexpr int Left = 68;
	// The description under the icons runs onto further lines when it is too long for one.
	constexpr int DescriptionWidth = 420;
	constexpr int DescriptionLineHeight = 14;

	// Icons are drawn from their bottom-left corner.
	Point position { Left, Margin + IconHeight - 1 };
	for (size_t i = 0; i < BuffCount; i++) {
		const auto buff = static_cast<BuffID>(i);
		if (!IsBuffActive(player, buff))
			continue;

		SetSpellTrans(SpellType::Spell);
		DrawSmallSpellIcon(out, position, GetBuffSpell(buff));

		const Rectangle iconArea { Point { position.x, position.y - IconHeight + 1 }, Size { IconWidth, IconHeight } };
		if (iconArea.contains(MousePosition)) {
			DrawString(out, WordWrapString(DescribeBuff(player, buff), DescriptionWidth),
			    Rectangle { Point { Left, Margin + IconHeight + 2 }, Size { DescriptionWidth, 3 * DescriptionLineHeight } },
			    { .flags = UiFlags::ColorWhitegold, .lineHeight = DescriptionLineHeight });
		}

		position.x += IconWidth + Gap;
	}

	// Buffs read from essence_powers.tsv follow, with the seconds left on those that run out, and
	// the auras of other players that are reaching this player now. One that comes from another
	// player is drawn in yellow rather than blue.
	for (const ReachingBuff &buff : GetReachingBuffs(player)) {
		SetSpellTrans(buff.fromOther ? SpellType::Skill : SpellType::Spell);
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
			if (spellData.buffStat == "Shield")
				StrAppend(text, " (", std::max((buff.amount + 32) >> 6, 1), " shield left)");
			if (buff.ticksLeft > 0)
				StrAppend(text, " (", secondsLeft, "s left)");
			DrawString(out, WordWrapString(text, DescriptionWidth),
			    Rectangle { Point { Left, Margin + IconHeight + 2 }, Size { DescriptionWidth, 3 * DescriptionLineHeight } },
			    { .flags = UiFlags::ColorWhitegold, .lineHeight = DescriptionLineHeight });
		}

		position.x += IconWidth + Gap;
	}
	SetSpellTrans(SpellType::Spell);
}

} // namespace devilution
