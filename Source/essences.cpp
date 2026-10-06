/**
 * @file essences.cpp
 *
 * Essence Mod: essences and the abilities slotted under them.
 */
#include "essences.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#include "player.h"
#include "spells.h"

namespace devilution {

namespace {

std::array<EssenceID, EssenceSlotCount> Slots {};
std::array<std::array<SpellID, AbilitiesPerEssence>, EssenceSlotCount> Abilities {};

void ClearAbilities()
{
	for (auto &row : Abilities)
		row.fill(SpellID::Invalid);
}

/** An empty position is SpellID::Invalid, not zero, so the table must be cleared before first use. */
const bool AbilitiesCleared = (ClearAbilities(), true);

/** The slot holding an essence, or EssenceSlotCount if the player does not have it. */
size_t SlotOf(EssenceID essence)
{
	for (size_t slot = 0; slot < EssenceSlotCount; slot++) {
		if (Slots[slot] == essence)
			return slot;
	}
	return EssenceSlotCount;
}

} // namespace

std::string_view GetEssenceName(EssenceID essence)
{
	switch (essence) {
	case EssenceID::Fire:
		return "Fire";
	case EssenceID::Lightning:
		return "Lightning";
	case EssenceID::Water:
		return "Water";
	case EssenceID::None:
		break;
	}
	return "None";
}

EssenceID GetSpellEssence(SpellID spell)
{
	switch (spell) {
	case SpellID::Firebolt:
	case SpellID::Fireball:
	case SpellID::FireWall:
	case SpellID::FlameWave:
	case SpellID::Inferno:
	case SpellID::FireAura:
	case SpellID::FlamingWeapon:
	case SpellID::FlameStrike:
	case SpellID::InfernoStrike:
		return EssenceID::Fire;
	case SpellID::Lightning:
	case SpellID::ChainLightning:
	case SpellID::ChargedBolt:
	case SpellID::Flash:
	case SpellID::Nova:
		return EssenceID::Lightning;
	case SpellID::Frostbolt:
		return EssenceID::Water;
	default:
		break;
	}
	// A power read from essence_powers.tsv names its essence in its own row.
	if (IsValidSpell(spell)) {
		const std::string &name = GetSpellData(spell).essence;
		for (unsigned i = 1; !name.empty() && i <= static_cast<unsigned>(EssenceID::LAST); i++) {
			if (name == GetEssenceName(static_cast<EssenceID>(i)))
				return static_cast<EssenceID>(i);
		}
	}
	return EssenceID::None;
}

EssenceID GetEssenceInSlot(size_t slot)
{
	return slot < EssenceSlotCount ? Slots[slot] : EssenceID::None;
}

SpellID GetEssenceAbility(size_t slot, size_t position)
{
	if (slot >= EssenceSlotCount || position >= AbilitiesPerEssence)
		return SpellID::Invalid;
	return Abilities[slot][position];
}

bool HasEssence(EssenceID essence)
{
	return essence != EssenceID::None && SlotOf(essence) < EssenceSlotCount;
}

bool CanAbsorbEssence(EssenceID essence)
{
	return essence != EssenceID::None && !HasEssence(essence) && SlotOf(EssenceID::None) < EssenceSlotCount;
}

void AbsorbEssence(EssenceID essence)
{
	if (!CanAbsorbEssence(essence))
		return;
	Slots[SlotOf(EssenceID::None)] = essence;
}

LearnResult CheckCanLearn(SpellID spell)
{
	if (MyPlayer != nullptr && MyPlayer->GetBaseSpellLevel(spell) != 0)
		return LearnResult::AlreadyKnown;

	const EssenceID essence = GetSpellEssence(spell);
	if (essence == EssenceID::None)
		return LearnResult::NoEssenceAssigned;

	const size_t slot = SlotOf(essence);
	if (slot >= EssenceSlotCount)
		return LearnResult::EssenceMissing;

	const auto &row = Abilities[slot];
	if (std::none_of(row.begin(), row.end(), [](SpellID held) { return held == SpellID::Invalid; }))
		return LearnResult::EssenceFull;

	return LearnResult::Ok;
}

std::string_view DescribeLearnResult(LearnResult result)
{
	switch (result) {
	case LearnResult::Ok:
		return "";
	case LearnResult::AlreadyKnown:
		return "You already know this ability";
	case LearnResult::NoEssenceAssigned:
		return "This ability does not belong to any essence yet";
	case LearnResult::EssenceMissing:
		return "You do not have the essence for this ability";
	case LearnResult::EssenceFull:
		return "That essence already holds five abilities";
	}
	return "";
}

void SlotLearnedAbility(SpellID spell)
{
	const size_t slot = SlotOf(GetSpellEssence(spell));
	if (slot >= EssenceSlotCount)
		return;
	auto &row = Abilities[slot];
	if (std::find(row.begin(), row.end(), spell) != row.end())
		return;
	const auto free = std::find(row.begin(), row.end(), SpellID::Invalid);
	if (free != row.end())
		*free = spell;
}

uint64_t GetSlottedAbilityMask()
{
	uint64_t mask = 0;
	for (const auto &row : Abilities) {
		for (const SpellID spell : row) {
			if (spell != SpellID::Invalid)
				mask |= GetSpellBitmask(spell);
		}
	}
	return mask;
}

bool IsAbilitySlotted(SpellID spell)
{
	if (spell == SpellID::Invalid)
		return false;
	for (const auto &row : Abilities) {
		if (std::find(row.begin(), row.end(), spell) != row.end())
			return true;
	}
	return false;
}

void ResetEssences()
{
	Slots.fill(EssenceID::None);
	ClearAbilities();
}

void WriteEssenceSidecarLines(FILE *file)
{
	for (size_t slot = 0; slot < EssenceSlotCount; slot++) {
		if (Slots[slot] == EssenceID::None)
			continue;
		std::fprintf(file, "E %u %u\n", static_cast<unsigned>(slot), static_cast<unsigned>(Slots[slot]));
		for (size_t position = 0; position < AbilitiesPerEssence; position++) {
			const SpellID spell = Abilities[slot][position];
			if (spell != SpellID::Invalid)
				std::fprintf(file, "A %u %u %u\n", static_cast<unsigned>(slot), static_cast<unsigned>(position), static_cast<unsigned>(spell));
		}
	}
}

bool ReadEssenceSidecarLine(const char *line)
{
	unsigned slot = 0;
	unsigned second = 0;
	unsigned third = 0;
	if (std::sscanf(line, "E %u %u", &slot, &second) == 2) {
		if (slot < EssenceSlotCount && second <= static_cast<unsigned>(EssenceID::LAST))
			Slots[slot] = static_cast<EssenceID>(second);
		return true;
	}
	if (std::sscanf(line, "A %u %u %u", &slot, &second, &third) == 3) {
		if (slot < EssenceSlotCount && second < AbilitiesPerEssence && IsValidSpell(static_cast<SpellID>(third)))
			Abilities[slot][second] = static_cast<SpellID>(third);
		return true;
	}
	return false;
}

} // namespace devilution
