/**
 * @file cooldowns.cpp
 *
 * Essence Mod: cooldowns on spells and special attacks.
 */
#include "cooldowns.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

#include "control/control.hpp"
#include "engine/point.hpp"
#include "panels/spell_icons.hpp"
#include "player.h"
#include "spells.h"

namespace devilution {

namespace {

constexpr int TicksPerSecond = 20;

/** How long a finished cooldown's icon stays on the tracker, swollen, before it vanishes. */
constexpr int ReadyPulseTicks = 8;

/** How much the icon grows during that moment, in percent. */
constexpr int ReadyPulsePercent = 105;

/** One entry per row of the spell table. Sized on first use, since the table is read from files. */
std::vector<int> TicksLeft;
std::vector<int> PulseTicksLeft;

void FitToSpellTable()
{
	if (TicksLeft.size() != SpellsData.size()) {
		TicksLeft.assign(SpellsData.size(), 0);
		PulseTicksLeft.assign(SpellsData.size(), 0);
	}
}

} // namespace

int GetSpellCooldownTicks(SpellID spell)
{
	// Some cooldowns shrink as the power gains levels. The level is the local player's: cooldowns
	// are only ever kept for them.
	const int level = MyPlayer != nullptr && IsValidSpell(spell) ? ShownPowerLevel(MyPlayer->GetBaseSpellLevel(spell)) : 0;
	const auto shrinking = [level](int seconds, int dropPerLevel) {
		return std::max(seconds - dropPerLevel * level, 1) * TicksPerSecond;
	};
	switch (spell) {
	case SpellID::InfernoStrike:
		return 12 * TicksPerSecond;
	case SpellID::Teleport:
		// It has nothing else to gain from a level: 30 seconds, and 2 less for each level.
		return shrinking(30, 2);
	default:
		break;
	}
	// A power read from essence_powers.tsv carries its cooldown in its own row.
	if (IsExtendedSpell(spell) && IsValidSpell(spell)) {
		const SpellData &spellData = GetSpellData(spell);
		if (spellData.cooldownSeconds == 0)
			return 0;
		return shrinking(spellData.cooldownSeconds, spellData.cooldownDropSeconds);
	}
	return 0;
}

void StartSpellCooldown(SpellID spell)
{
	const int ticks = GetSpellCooldownTicks(spell);
	if (ticks <= 0)
		return;
	FitToSpellTable();
	TicksLeft[static_cast<size_t>(spell)] = ticks;
	PulseTicksLeft[static_cast<size_t>(spell)] = 0;
}

bool IsSpellOnCooldown(SpellID spell)
{
	if (spell == SpellID::Invalid || static_cast<size_t>(spell) >= TicksLeft.size())
		return false;
	return TicksLeft[static_cast<size_t>(spell)] > 0;
}

float GetSpellCooldownProgress(SpellID spell)
{
	if (!IsSpellOnCooldown(spell))
		return 1.0F;
	const int total = GetSpellCooldownTicks(spell);
	if (total <= 0)
		return 1.0F;
	const int left = TicksLeft[static_cast<size_t>(spell)];
	return std::clamp(1.0F - static_cast<float>(left) / static_cast<float>(total), 0.0F, 1.0F);
}

void ProcessCooldowns()
{
	for (size_t i = 0; i < TicksLeft.size(); i++) {
		if (PulseTicksLeft[i] > 0)
			PulseTicksLeft[i]--;
		if (TicksLeft[i] <= 0)
			continue;
		if (--TicksLeft[i] == 0)
			PulseTicksLeft[i] = ReadyPulseTicks;
	}
}

void ClearCooldowns()
{
	TicksLeft.assign(TicksLeft.size(), 0);
	PulseTicksLeft.assign(PulseTicksLeft.size(), 0);
}

void DrawCooldownTracker(const Surface &out)
{
	if (MyPlayer == nullptr)
		return;

	constexpr int IconWidth = 37;
	constexpr int Margin = 8;
	constexpr int Gap = 4;

	// Abilities that have just become ready come first, then the rest by time remaining.
	std::vector<size_t> ready;
	std::vector<size_t> waiting;
	for (size_t i = 0; i < TicksLeft.size(); i++) {
		if (TicksLeft[i] > 0)
			waiting.push_back(i);
		else if (PulseTicksLeft[i] > 0)
			ready.push_back(i);
	}
	if (ready.empty() && waiting.empty())
		return;
	std::sort(waiting.begin(), waiting.end(), [](size_t a, size_t b) { return TicksLeft[a] < TicksLeft[b]; });

	// Bottom-right of the play area, just above the mana orb. Icons are drawn from their
	// bottom-left corner, and the row grows leftwards from the corner.
	const Rectangle &mainPanel = GetMainPanel();
	Point position { mainPanel.position.x + mainPanel.size.width - Margin - IconWidth, mainPanel.position.y - Margin };

	SetSpellTrans(SpellType::Spell);
	for (const size_t i : ready) {
		DrawSmallSpellIconScaled(out, position, static_cast<SpellID>(i), ReadyPulsePercent);
		position.x -= IconWidth + Gap;
	}
	for (const size_t i : waiting) {
		DrawSmallSpellIcon(out, position, static_cast<SpellID>(i));
		position.x -= IconWidth + Gap;
	}
}

} // namespace devilution
