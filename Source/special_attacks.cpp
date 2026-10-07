/**
 * @file special_attacks.cpp
 *
 * Essence Mod: special attacks.
 */
#include "special_attacks.h"

#include "essence_tint.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <vector>

#include "cooldowns.h"
#include "dots.h"
#include "levels/gendung.h"
#include "missiles.h"
#include "msg.h"
#include "qol/floatingnumbers.h"
#include "utils/str_cat.hpp"
#include "engine/backbuffer_state.hpp"
#include "monster.h"
#include "player.h"
#include "spells.h"
#include "utils/is_of.hpp"

namespace devilution {

namespace {

/** How long a queued special attack waits for its swing: 10 seconds at normal speed. */
constexpr int QueueLifetimeTicks = 200;

/** Fire damage Flame Strike adds at spell level 1, in 64ths of a hit point. */
constexpr int FlameStrikeBaseBonus = 3 * 64;

/** Extra weapon damage Inferno Strike adds at spell level 1, in percent: 200% more is three times the hit. */
constexpr int InfernoStrikeBaseBonusPercent = 200;

SpellID QueuedAttack = SpellID::Invalid;
int QueuedMonsterId = -1;
int QueuedTicksLeft = 0;

void ClearQueue()
{
	QueuedAttack = SpellID::Invalid;
	QueuedMonsterId = -1;
	QueuedTicksLeft = 0;
}

int SpellLevelOf(const Player &player, SpellID spell)
{
	return std::max<int>(player.GetBaseSpellLevel(spell), 1);
}

} // namespace

bool IsSpecialAttack(SpellID spell)
{
	if (IsAnyOf(spell, SpellID::FlameStrike, SpellID::InfernoStrike))
		return true;
	// A power read from essence_powers.tsv is a weapon attack when its effect is "Strike".
	return IsExtendedSpell(spell) && IsValidSpell(spell) && GetSpellData(spell).effect == "Strike";
}

bool IsShieldStrikeQueued()
{
	return QueuedAttack != SpellID::Invalid && IsValidSpell(QueuedAttack) && GetSpellData(QueuedAttack).requiredWeapon == "Shield";
}

void QueueSpecialAttack(SpellID spell, int monsterId)
{
	QueuedAttack = spell;
	QueuedMonsterId = monsterId;
	QueuedTicksLeft = QueueLifetimeTicks;
}

namespace {

/** Takes the queued attack off the queue and pays for it. Invalid if it cannot be used right now. */
SpellID SpendQueuedSpecialAttack(Player &player)
{
	const SpellID attack = QueuedAttack;
	ClearQueue();

	if (IsSpellOnCooldown(attack))
		return SpellID::Invalid;

	// The mana is paid when the swing happens, whether or not it lands.
	// A cooldown starts only if the swing lands; that is done where the hit is resolved.
	const int cost = GetManaAmount(player, attack);
	if (player._pMana < cost)
		return SpellID::Invalid;
	player._pMana -= cost;
	player._pManaBase -= cost;
	RedrawComponent(PanelDrawComponent::Mana);
	return attack;
}

} // namespace

SpellID TakeQueuedSpecialAttack(Player &player, const Monster &monster)
{
	if (QueuedAttack == SpellID::Invalid || QueuedMonsterId != static_cast<int>(monster.getId()))
		return SpellID::Invalid;
	return SpendQueuedSpecialAttack(player);
}

SpellID TakeQueuedSpecialAttackForShot(Player &player)
{
	if (QueuedAttack == SpellID::Invalid)
		return SpellID::Invalid;
	return SpendQueuedSpecialAttack(player);
}

void ProcessSpecialAttacks()
{
	if (QueuedAttack != SpellID::Invalid && --QueuedTicksLeft <= 0)
		ClearQueue();
}

int GetFlameStrikeBonusDamage(const Player &player)
{
	return ScaleDamageForSpellLevel(FlameStrikeBaseBonus, SpellLevelOf(player, SpellID::FlameStrike));
}

int ApplySpecialAttackDamage(const Player &player, SpellID attack, int weaponDamage)
{
	switch (attack) {
	case SpellID::FlameStrike:
		return weaponDamage + GetFlameStrikeBonusDamage(player);
	case SpellID::InfernoStrike: {
		const int bonusPercent = ScaleDamageForSpellLevel(InfernoStrikeBaseBonusPercent, SpellLevelOf(player, SpellID::InfernoStrike));
		return weaponDamage + static_cast<int>(static_cast<int64_t>(weaponDamage) * bonusPercent / 100);
	}
	default:
		break;
	}
	// A power read from essence_powers.tsv adds its amount to the swing.
	if (IsExtendedSpell(attack) && IsValidSpell(attack))
		return weaponDamage + ScaleDamageForSpellLevel(GetSpellData(attack).effectAmount, SpellLevelOf(player, attack));
	return weaponDamage;
}

DamageType GetSpecialAttackDamageType(SpellID attack)
{
	if (IsExtendedSpell(attack) && IsValidSpell(attack)) {
		return GetSpellDamageType(attack);
	}
	return DamageType::Fire;
}

void ApplySpecialAttackExtras(const Player &player, Monster &target, SpellID attack, int damage)
{
	if (!IsExtendedSpell(attack) || !IsValidSpell(attack))
		return;
	const SpellData &spellData = GetSpellData(attack);
	const DamageType damageType = GetSpecialAttackDamageType(attack);

	// How many monsters the strike landed on, for riders that count them.
	int monstersStruck = 1;

	// With a radius, every other monster near the target takes the same damage as the target did.
	if (const int radius = spellData.effectRadius; radius > 0) {
		const Point centre = target.position.tile;
		std::vector<int> others;
		for (int y = centre.y - radius; y <= centre.y + radius; y++) {
			for (int x = centre.x - radius; x <= centre.x + radius; x++) {
				if (!InDungeonBounds({ x, y }))
					continue;
				const int monsterId = std::abs(dMonster[x][y]) - 1;
				if (monsterId < 0 || monsterId == static_cast<int>(target.getId()) || std::find(others.begin(), others.end(), monsterId) != others.end())
					continue;
				const Monster &candidate = Monsters[monsterId];
				if (candidate.isPossibleToHit() && !candidate.isPlayerMinion())
					others.push_back(monsterId);
			}
		}
		monstersStruck += static_cast<int>(others.size());
		for (const int monsterId : others) {
			Monster &other = Monsters[monsterId];
			DealSpellTickDamage(other, attack, MissileID::WeaponExplosion, damageType, damage);
			AddMissile(other.position.tile, { 1, 0 }, Direction::South, MissileID::WeaponExplosion, TARGET_MONSTERS, player, 0, 0);
		}
	}

	// A rider lands on the target alone, if it survived the hit.
	if (const std::optional<DotID> dot = ParseDotName(spellData.rider); dot && !target.hasNoLife() && !IsImmuneToDot(target, *dot)) {
		const int stacks = AddMonsterDot(target, *dot, attack, ScaleDamageForSpellLevel(spellData.riderAmount, SpellLevelOf(player, attack)));
		// A separate id from the damage numbers, so the game does not merge the two.
		AddFloatingNumber(target.position.tile, { 0, 0 }, StrCat(GetDotName(*dot), " x", stacks), GetDamageTypeTextColor(*dot == DotID::Corruption ? DamageType::Shadow : DamageType::Fire) | UiFlags::FontSize12, 1000 + static_cast<int>(target.getId()));
	}

	// "Branded": every direct hit on the target deals extra fire from then on. The rider's amount
	// is that extra damage, in 64ths of a hit point. The strike that applies it sets it off as well.
	if (spellData.rider == "Branded" && !target.hasNoLife()) {
		const int extra = ScaleDamageForSpellLevel(spellData.riderAmount, SpellLevelOf(player, attack));
		BrandMonster(target, attack, extra);
		AddFloatingNumber(target.position.tile, { 0, 0 }, "Branded", GetSpellTextColor(attack) | UiFlags::FontSize12, 1000 + static_cast<int>(target.getId()));
	}

	// "Freeze": the target is held still for the rider's amount in seconds. Every PC has to do
	// this alike, so they are all told.
	if (spellData.rider == "Freeze" && !target.hasNoLife())
		NetSendCmdPowerOnMonster(static_cast<uint16_t>(target.getId()), attack, static_cast<uint32_t>(spellData.riderAmount));

	// "ArmorBonus": the hit deals extra damage from the striker's armor. The rider's amount is
	// the percentage of their armor that is added, as hit points.
	if (spellData.rider == "ArmorBonus" && !target.hasNoLife()) {
		const int extra = static_cast<int>(static_cast<int64_t>(std::max(player.GetArmor(), 0)) * 64 * spellData.riderAmount / 100);
		if (extra > 0)
			DealSpellTickDamage(target, attack, MissileID::WeaponExplosion, damageType, extra);
	}

	// "Guard": the striker gains the power's buff, with the rider's amount as its strength.
	// "ShieldPerHit": the striker gains a shield of the rider's amount for each monster struck.
	if (spellData.rider == "Guard")
		NetSendCmdPowerOnPlayer(player, attack, static_cast<uint32_t>(ScaleDamageForSpellLevel(spellData.riderAmount, SpellLevelOf(player, attack))));
	if (spellData.rider == "ShieldPerHit")
		NetSendCmdPowerOnPlayer(player, attack, static_cast<uint32_t>(monstersStruck * ScaleDamageForSpellLevel(spellData.riderAmount, SpellLevelOf(player, attack))));

	// "Arc": the shock leaps from the target to the two nearest other monsters, each taking the
	// rider's amount in the power's damage type.
	if (spellData.rider == "Arc")
		ChainSpellDamage(target, attack, damageType, ScaleDamageForSpellLevel(spellData.riderAmount, SpellLevelOf(player, attack)), 100, 2);

	// "Rupture": the hit deals extra damage for the Corruption still ticking on the target. The
	// rider's amount is the share of it, as a percentage. The Corruption is not used up.
	if (spellData.rider == "Rupture" && !target.hasNoLife()) {
		const int extra = static_cast<int>(static_cast<int64_t>(GetMonsterDotRemaining(target, DotID::Corruption)) * spellData.riderAmount / 100);
		if (extra > 0)
			DealSpellTickDamage(target, attack, MissileID::WeaponExplosion, damageType, extra);
	}

	// "Bane": the hit deals more to the undead and to demons. The rider's amount is the percentage.
	if (spellData.rider == "Bane" && !target.hasNoLife() && IsAnyOf(target.data().monsterClass, MonsterClass::Undead, MonsterClass::Demon)) {
		const int extra = static_cast<int>(static_cast<int64_t>(damage) * spellData.riderAmount / 100);
		if (extra > 0)
			DealSpellTickDamage(target, attack, MissileID::WeaponExplosion, damageType, extra);
	}

	// "HealAlly": the most badly wounded living player within 8 tiles, the striker included, is
	// healed for the rider's amount.
	if (spellData.rider == "HealAlly") {
		const Player *wounded = nullptr;
		for (const Player &other : Players) {
			if (!other.plractive || !other.isOnActiveLevel() || other.hasNoLife() || other._pMaxHP <= 0 || other._pHitPoints >= other._pMaxHP)
				continue;
			if (other.position.tile.WalkingDistance(player.position.tile) > 8)
				continue;
			if (wounded == nullptr || static_cast<int64_t>(other._pHitPoints) * wounded->_pMaxHP < static_cast<int64_t>(wounded->_pHitPoints) * other._pMaxHP)
				wounded = &other;
		}
		if (wounded != nullptr)
			NetSendCmdPowerOnPlayer(*wounded, attack, static_cast<uint32_t>(ScaleDamageForSpellLevel(spellData.riderAmount, SpellLevelOf(player, attack))));
	}

	// "Vulnerable": the target takes more damage from then on. The rider's amount is the percentage.
	if (spellData.rider == "Vulnerable" && !target.hasNoLife()) {
		const int percent = ScaleDamageForSpellLevel(spellData.riderAmount, SpellLevelOf(player, attack));
		MakeMonsterVulnerable(target, percent);
		AddFloatingNumber(target.position.tile, { 0, 0 }, StrCat("Vulnerable +", percent, "%"), GetSpellTextColor(attack) | UiFlags::FontSize12, 1000 + static_cast<int>(target.getId()));
	}
}

} // namespace devilution
