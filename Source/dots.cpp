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
#include <vector>

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

struct DotDefinition {
	/** The spell that applies the effect and earns experience from its damage. */
	SpellID spell;
	/** The projectile that applies it, used when checking a monster's immunities. */
	MissileID missile;
	DamageType damageType;
	/** Damage per stack each time the effect ticks, in 64ths of a hit point. */
	int damagePerStack;
	/** Game ticks between damage ticks. */
	int interval;
	/** Game ticks the effect lasts. Every new stack starts this again. */
	int duration;
};

constexpr DotDefinition Definitions[] = {
	// Corruption: 1 shadow damage per stack every 2 seconds, for 20 seconds.
	{ SpellID::Corruption, MissileID::Corruption, DamageType::Shadow, 1 * 64, 2 * TicksPerSecond, 20 * TicksPerSecond },
};

const DotDefinition &DefinitionOf(DotID dot)
{
	return Definitions[static_cast<size_t>(dot)];
}

struct ActiveDot {
	DotID id;
	int stacks;
	int ticksUntilDamage;
	int ticksLeft;
};

struct Pulse {
	int ticksLeft = 0;
	EssenceTint tint = EssenceTint::Shadow;
};

std::array<std::vector<ActiveDot>, MaxMonsters> MonsterDots;
std::array<Pulse, MaxMonsters> MonsterPulses;

void DealDotDamage(Monster &monster, const ActiveDot &dot)
{
	const DotDefinition &definition = DefinitionOf(dot.id);
	const int spellLevel = MyPlayer->_pSplLvl[static_cast<size_t>(definition.spell)];
	const int damage = ScaleDamageForSpellLevel(definition.damagePerStack, spellLevel) * dot.stacks;
	DealSpellTickDamage(monster, definition.spell, definition.missile, definition.damageType, damage);
}

} // namespace

int ScaleDamageForSpellLevel(int baseDamage, int spellLevel)
{
	for (int level = 1; level < spellLevel; level++)
		baseDamage += baseDamage / 8;
	return baseDamage;
}

void DealSpellTickDamage(Monster &monster, SpellID spell, MissileID missile, DamageType damageType, int damage)
{
	// Resistances work as they do for a spell hit: immune takes nothing, resistant takes a quarter.
	if (monster.isImmune(missile, damageType))
		return;
	if (monster.isResistant(missile, damageType))
		damage /= 4;
	if (damage <= 0)
		return;

	const Player &player = *MyPlayer;
	AddSpellExperienceForDamage(player, monster, spell, damage, monster.hitPoints);
	ApplyMonsterDamage(damageType, monster, damage);
	if (monster.hasNoLife()) {
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

int AddMonsterDot(Monster &monster, DotID dot)
{
	std::vector<ActiveDot> &dots = MonsterDots[monster.getId()];
	const DotDefinition &definition = DefinitionOf(dot);

	Pulse &pulse = MonsterPulses[monster.getId()];
	pulse.ticksLeft = PulseTicks;
	pulse.tint = GetSpellTint(definition.spell).value_or(EssenceTint::Shadow);

	for (ActiveDot &active : dots) {
		if (active.id != dot)
			continue;
		// Another cast adds a stack and starts the duration again. The damage rhythm carries on.
		active.stacks++;
		active.ticksLeft = definition.duration;
		return active.stacks;
	}

	if (dots.size() >= MaxEffectsPerMonster)
		return 0;
	dots.push_back(ActiveDot { dot, 1, definition.interval, definition.duration });
	return 1;
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
			dot.ticksUntilDamage = DefinitionOf(dot.id).interval;
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
