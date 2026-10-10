/**
 * @file essences.h
 *
 * Essence Mod: essences and the abilities slotted under them.
 *
 * A character absorbs up to three essences, one per spellbook tab (tabs 2, 3 and 4), in the order
 * they are used. Each essence holds five abilities: the first five awakening stones of that essence
 * the character uses. Both choices are permanent. This is the local player's data; it is kept in
 * the sidecar file next to the save.
 *
 * Three essences form a confluence, which is offered on the fifth spellbook tab. Its name depends
 * on the three (txtdata/spells/confluences.tsv, written by the Essence Designer). Once accepted it
 * is a fourth row of five abilities, filled by powers of any of the three essences once that
 * essence's own row is full. A player may turn it down by absorbing a fourth essence instead,
 * which then has the fourth row to itself. Either choice is permanent.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

#include "tables/spelldat.h"

namespace devilution {

struct Player;

enum class EssenceID : uint8_t {
	None,
	Fire,
	Lightning,
	Water,
	Holy,
	Nature,
	Shield,
	Dark,
	Earth,
	LAST = Earth,
};

/**
 * The four stats, by the mod's names for them (Strength, Magic, Dexterity, Vitality in the game's own).
 * Each essence, and the confluence, is bound to one when it is taken: the essence's primary stat
 * (txtdata/spells/essence_stats.tsv, from the designer) if no other holds it, otherwise one of
 * those still free. Every level one of its abilities gains adds a point to that stat.
 */
enum class EssenceStat : uint8_t {
	None,
	Power,
	Spirit,
	Speed,
	Recovery,
};

/** Essence tabs in the spellbook: tabs 2, 3 and 4. */
constexpr size_t EssenceSlotCount = 3;

/** The fourth row of abilities: the confluence's, or those of a fourth essence taken in its place. */
constexpr size_t ConfluenceSlot = 3;

/** Rows of abilities in all: the three essences and the fourth row. */
constexpr size_t AbilitySlotCount = 4;

/** Levels the character's abilities must gain between them for the character to gain one. */
constexpr int PowerLevelsPerCharacterLevel = 8;

/** Abilities each essence holds. */
constexpr size_t AbilitiesPerEssence = 5;

std::string_view GetEssenceName(EssenceID essence);

std::string_view GetEssenceStatName(EssenceStat stat);

/** @brief The stat a row of abilities (0 to 3) is bound to, or None for a row not yet in use. */
EssenceStat GetSlotStat(size_t slot);

/** @brief The stat an essence the player holds is bound to, or None if they do not hold it. */
EssenceStat GetEssenceStat(EssenceID essence);

/**
 * @brief Adds to the player's own stats whatever their abilities' levels have earned and not yet
 * been given: one point for each level above the first, to the stat the ability's row is bound to.
 * Also sets the character's level, which is 1 plus one for every eight of those levels.
 * @param inGame true while playing, when the other PCs are told and a message is shown; false
 * while a character is being loaded.
 */
void GrantEarnedStatPoints(Player &player, bool inGame);

/** @brief The essence an ability belongs to, or None for abilities that have no essence yet. */
EssenceID GetSpellEssence(SpellID spell);

/** @brief The essence in a slot (0 to 3), or None if the slot is empty. Slot 3 only ever holds a fourth essence taken in place of the confluence. */
EssenceID GetEssenceInSlot(size_t slot);

/** @brief The ability at a position (0 to 4) in a row (0 to 3), or SpellID::Invalid. */
SpellID GetEssenceAbility(size_t slot, size_t position);

bool HasEssence(EssenceID essence);

/** @brief Whether the player can absorb this essence: not already held, and a slot is free. */
bool CanAbsorbEssence(EssenceID essence);

/** @brief Puts an essence in the first free slot. Does nothing if it cannot be absorbed. */
void AbsorbEssence(EssenceID essence);

/** @brief Whether the player holds three essences and has not yet chosen what fills the fourth row. */
bool IsConfluenceOffered();

/** @brief Whether the player has accepted their confluence. */
bool HasConfluence();

/** @brief Accepts the confluence on offer. Does nothing if none is. */
void AcceptConfluence();

/** @brief The name of the confluence the player's three essences form, or empty with fewer than three. */
std::string GetConfluenceName();

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

/**
 * @brief Whether an item name is that of an awakening stone that picks a power by tags, as listed
 * in txtdata/spells/stone_tags.tsv (the designer's Awakening Stones page).
 */
bool IsTagStoneKnown(std::string_view itemName);

/**
 * @brief The power an awakening stone with tags gives the player now. Of the powers the player
 * could learn at this moment (an essence held, room for it, not known, and not a second aura),
 * it takes those matching the most of the stone's tags and picks one at random. So a stone whose
 * tags match nothing learnable gives any learnable power at all.
 * @return SpellID::Invalid if there is no power the player can learn.
 */
SpellID PickPowerForStone(std::string_view itemName);

/**
 * @brief The separate roll every dying monster makes for an awakening stone or an essence, beside
 * the game's own loot (txtdata/spells/drops.tsv). Uses the game's shared random numbers, so the
 * caller must have seeded them the same on every PC.
 * @param isBoss A boss always drops one or the other, and never a Common one while anything rarer exists.
 * @param always Drop one or the other for certain, of any rarity (a bookcase that holds one).
 * @return The row of the item table to drop, or -1 for nothing.
 */
int RollEssenceLoot(bool isBoss, bool always = false);

/**
 * @brief Whether a player is Filthy. A character becomes Filthy on filling their fourth row (their
 * confluence, or a fourth essence), when the filth of their old self pours out, and stays so until
 * they use a Crystal Wash. It does nothing but show: it is there for the joke.
 */
bool IsFilthy(const Player &player);

/** @brief Another player's PC says whether they are Filthy: 0 clean, 1 filthy, 2 has just become so. */
void SetOtherPlayerFilthy(Player &player, int state);

/** @brief The local player uses a Crystal Wash. */
void WashOffFilth(Player &player);

/** @brief Called every game tick: now and then tells the other PCs again that the local player is Filthy. */
void ProcessFilth();

/** @brief Records a newly learned ability in the next free position under its essence. */
void SlotLearnedAbility(SpellID spell);

/** @brief The abilities the player holds under their essences, as "known spell" bits. */
uint64_t GetSlottedAbilityMask();

/** Whether a spell sits in one of the player's essence ability slots. Works for every spell number. */
bool IsAbilitySlotted(SpellID spell);

/** @brief Forgets all essences and abilities, e.g. before loading a different character. */
void ResetEssences();

/** @brief Called when a sidecar file is about to be read: it says whether its character has been rebased. */
void BeginEssenceSidecarRead();

/**
 * @brief Every race now starts with 10 in each stat. A character made before that keeps what they
 * have earned but loses the old head start of their class, once; the sidecar line "B 1" records it.
 */
void RebaseStartingStats(Player &player);

/** @brief Writes the essences and their abilities as lines of the sidecar file. */
void WriteEssenceSidecarLines(FILE *file);

/** @brief Reads one sidecar line if it is an essence line. Returns false if it is something else. */
bool ReadEssenceSidecarLine(const char *line);

} // namespace devilution
