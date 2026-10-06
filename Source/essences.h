/**
 * @file essences.h
 *
 * Essence Mod: essences and the abilities slotted under them.
 *
 * A character absorbs up to three essences, one per spellbook tab (tabs 2, 3 and 4), in the order
 * they are used. Each essence holds five abilities: the first five awakening stones of that essence
 * the character uses. Both choices are permanent. This is the local player's data; it is kept in
 * the sidecar file next to the save.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

#include "tables/spelldat.h"

namespace devilution {

enum class EssenceID : uint8_t {
	None,
	Fire,
	Lightning,
	Water,
	Holy,
	Nature,
	Shield,
	LAST = Shield,
};

/** Essence tabs in the spellbook: tabs 2, 3 and 4. */
constexpr size_t EssenceSlotCount = 3;

/** Abilities each essence holds. */
constexpr size_t AbilitiesPerEssence = 5;

std::string_view GetEssenceName(EssenceID essence);

/** @brief The essence an ability belongs to, or None for abilities that have no essence yet. */
EssenceID GetSpellEssence(SpellID spell);

/** @brief The essence in a slot (0 to 2), or None if the slot is empty. */
EssenceID GetEssenceInSlot(size_t slot);

/** @brief The ability at a position (0 to 4) under the essence in a slot, or SpellID::Invalid. */
SpellID GetEssenceAbility(size_t slot, size_t position);

bool HasEssence(EssenceID essence);

/** @brief Whether the player can absorb this essence: not already held, and a slot is free. */
bool CanAbsorbEssence(EssenceID essence);

/** @brief Puts an essence in the first free slot. Does nothing if it cannot be absorbed. */
void AbsorbEssence(EssenceID essence);

enum class LearnResult : uint8_t {
	Ok,
	AlreadyKnown,
	/** The ability has not been given an essence yet, so nobody can learn it. */
	NoEssenceAssigned,
	/** The player does not hold the ability's essence. */
	EssenceMissing,
	/** The essence already holds five abilities. */
	EssenceFull,
	/** The ability is an aura and the player already knows one. A player has one aura at most. */
	AuraAlreadyKnown,
};

/** @brief Whether an ability is an aura. A player can learn only one, whatever essences they hold. */
bool IsAuraPower(SpellID spell);

/** @brief Whether the player may learn an ability now. */
LearnResult CheckCanLearn(SpellID spell);

/** @brief A short sentence explaining why an ability cannot be learned. */
std::string_view DescribeLearnResult(LearnResult result);

/** @brief Records a newly learned ability in the next free position under its essence. */
void SlotLearnedAbility(SpellID spell);

/** @brief The abilities the player holds under their essences, as "known spell" bits. */
uint64_t GetSlottedAbilityMask();

/** Whether a spell sits in one of the player's essence ability slots. Works for every spell number. */
bool IsAbilitySlotted(SpellID spell);

/** @brief Forgets all essences and abilities, e.g. before loading a different character. */
void ResetEssences();

/** @brief Writes the essences and their abilities as lines of the sidecar file. */
void WriteEssenceSidecarLines(FILE *file);

/** @brief Reads one sidecar line if it is an essence line. Returns false if it is something else. */
bool ReadEssenceSidecarLine(const char *line);

} // namespace devilution
