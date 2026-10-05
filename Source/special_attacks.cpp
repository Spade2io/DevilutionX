/**
 * @file special_attacks.cpp
 *
 * Essence Mod: special attacks.
 */
#include "special_attacks.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "cooldowns.h"
#include "dots.h"
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
	return IsAnyOf(spell, SpellID::FlameStrike, SpellID::InfernoStrike);
}

void QueueSpecialAttack(SpellID spell, int monsterId)
{
	QueuedAttack = spell;
	QueuedMonsterId = monsterId;
	QueuedTicksLeft = QueueLifetimeTicks;
}

SpellID TakeQueuedSpecialAttack(Player &player, const Monster &monster)
{
	if (QueuedAttack == SpellID::Invalid || QueuedMonsterId != static_cast<int>(monster.getId()))
		return SpellID::Invalid;

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
		return weaponDamage;
	}
}

} // namespace devilution
