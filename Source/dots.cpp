/**
 * @file dots.cpp
 *
 * Essence Mod: damage-over-time effects on monsters.
 */
#include "dots.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "buffs.h"
#include "essence_tint.h"
#include "monster.h"
#include "player.h"
#include "spell_xp.h"
#include "tables/misdat.h"
#include "tables/spelldat.h"

namespace devilution {

namespace {

/** Game ticks per second at normal speed. */
constexpr int TicksPerSecond = 20;

/** Safety net only; an effect list should never get near this. */
constexpr size_t MaxEffectsPerMonster = 32;

/** How long a monster stays tinted after a damage tick. */
constexpr int PulseTicks = 6;

/** Every effect ticks every 2 seconds and lasts 20 seconds from its latest application. */
constexpr int TickInterval = 2 * TicksPerSecond;
constexpr int EffectDuration = 20 * TicksPerSecond;

struct DotDefinition {
	std::string_view name;
	/** A projectile of the same kind, used when checking a monster's immunities. */
	MissileID missile;
	DamageType damageType;
	/** The colour a monster flashes when the effect lands or ticks. */
	EssenceTint tint;
};

constexpr DotDefinition Definitions[] = {
	{ "Corruption", MissileID::Corruption, DamageType::Shadow, EssenceTint::Shadow },
	{ "Burn", MissileID::Firebolt, DamageType::Fire, EssenceTint::VividRed },
};

const DotDefinition &DefinitionOf(DotID dot)
{
	return Definitions[static_cast<size_t>(dot)];
}

/** What one power has poured into an effect on one monster. */
struct DotShare {
	SpellID spell;
	/** Damage each tick, in 64ths of a hit point. */
	int damagePerTick;
};

struct ActiveDot {
	DotID id;
	/** How many times it has been applied since it last ran out. */
	int stacks;
	int ticksUntilDamage;
	int ticksLeft;
	/** One entry per power that has added to it, so each earns experience for its own part. */
	std::vector<DotShare> shares;
};

struct Pulse {
	int ticksLeft = 0;
	EssenceTint tint = EssenceTint::Shadow;
};

std::array<std::vector<ActiveDot>, MaxMonsters> MonsterDots;
std::array<Pulse, MaxMonsters> MonsterPulses;

void FlashMonster(const Monster &monster, EssenceTint tint)
{
	Pulse &pulse = MonsterPulses[monster.getId()];
	pulse.ticksLeft = PulseTicks;
	pulse.tint = tint;
}

void DealDotDamage(Monster &monster, const ActiveDot &dot)
{
	const DotDefinition &definition = DefinitionOf(dot.id);
	// Each power deals the part it added, so each is credited with its own experience.
	for (const DotShare &share : dot.shares) {
		DealSpellTickDamage(monster, share.spell, definition.missile, definition.damageType, share.damagePerTick);
		if (monster.hasNoLife())
			break;
	}
	if (!monster.hasNoLife())
		FlashMonster(monster, definition.tint);
}

} // namespace

int ScaleDamageForSpellLevel(int baseDamage, int spellLevel)
{
	for (int level = 1; level < spellLevel; level++)
		baseDamage += baseDamage / 8;
	return baseDamage;
}

void DealSpellTickDamage(Monster &monster, SpellID spell, MissileID missile, DamageType damageType, int damage, bool finishKill)
{
	// Resistances work as they do for a spell hit: immune takes nothing, resistant takes a quarter.
	if (monster.isImmune(missile, damageType))
		return;
	if (monster.isResistant(missile, damageType))
		damage /= 4;
	if (damage <= 0)
		return;

	const Player &player = *MyPlayer;
	damage = ApplyDamageBuffs(player, damage);
	AddSpellExperienceForDamage(player, monster, spell, damage, monster.hitPoints);
	ApplyMonsterDamage(damageType, monster, damage);
	if (monster.hasNoLife()) {
		if (finishKill)
			M_StartKill(monster, player);
	} else {
		monster.tag(player);
		// Being hurt gets a monster's attention.
		if (monster.activeForTicks == 0) {
			monster.activeForTicks = UINT8_MAX;
			monster.position.last = player.position.tile;
		}
	}

	// Flash the monster in the spell's colour so each effect's beat can be seen.
	Pulse &pulse = MonsterPulses[monster.getId()];
	pulse.ticksLeft = PulseTicks;
	pulse.tint = GetSpellTint(spell).value_or(EssenceTint::Shadow);
}

std::optional<DotID> ParseDotName(std::string_view name)
{
	for (size_t i = 0; i <= static_cast<size_t>(DotID::LAST); i++) {
		if (Definitions[i].name == name)
			return static_cast<DotID>(i);
	}
	return std::nullopt;
}

std::string_view GetDotName(DotID dot)
{
	return DefinitionOf(dot).name;
}

bool IsImmuneToDot(const Monster &monster, DotID dot)
{
	const DotDefinition &definition = DefinitionOf(dot);
	return monster.isImmune(definition.missile, definition.damageType);
}

int AddMonsterDot(Monster &monster, DotID dot, SpellID spell, int damagePerTick)
{
	std::vector<ActiveDot> &dots = MonsterDots[monster.getId()];
	FlashMonster(monster, DefinitionOf(dot).tint);

	ActiveDot *active = nullptr;
	for (ActiveDot &existing : dots) {
		if (existing.id == dot)
			active = &existing;
	}
	if (active == nullptr) {
		if (dots.size() >= MaxEffectsPerMonster)
			return 0;
		active = &dots.emplace_back(ActiveDot { dot, 0, TickInterval, EffectDuration, {} });
	}

	// Pour this application in and restart the timer for the whole effect. The rhythm of the
	// damage ticks carries on undisturbed.
	active->stacks++;
	active->ticksLeft = EffectDuration;
	for (DotShare &share : active->shares) {
		if (share.spell == spell) {
			share.damagePerTick += damagePerTick;
			return active->stacks;
		}
	}
	active->shares.push_back(DotShare { spell, damagePerTick });
	return active->stacks;
}

void ProcessMonsterDots()
{
	if (MyPlayer == nullptr)
		return;

	for (size_t i = 0; i < MaxMonsters; i++) {
		Pulse &pulse = MonsterPulses[i];
		if (pulse.ticksLeft > 0)
			pulse.ticksLeft--;

		std::vector<ActiveDot> &dots = MonsterDots[i];
		if (dots.empty())
			continue;

		Monster &monster = Monsters[i];
		if (monster.hasNoLife()) {
			dots.clear();
			continue;
		}

		for (ActiveDot &dot : dots) {
			dot.ticksLeft--;
			dot.ticksUntilDamage--;
			if (dot.ticksUntilDamage > 0)
				continue;
			dot.ticksUntilDamage = TickInterval;
			DealDotDamage(monster, dot);
			if (monster.hasNoLife())
				break;
		}

		if (monster.hasNoLife()) {
			dots.clear();
			continue;
		}
		std::erase_if(dots, [](const ActiveDot &dot) { return dot.ticksLeft <= 0; });
	}
}

void ClearAllMonsterDots()
{
	for (std::vector<ActiveDot> &dots : MonsterDots)
		dots.clear();
	MonsterPulses.fill(Pulse {});
}

const uint8_t *GetMonsterDotPulseTrn(const Monster &monster)
{
	const Pulse &pulse = MonsterPulses[monster.getId()];
	if (pulse.ticksLeft <= 0)
		return nullptr;
	return GetEssenceTintTrn(pulse.tint);
}

} // namespace devilution
