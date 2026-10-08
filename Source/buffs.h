/**
 * @file buffs.h
 *
 * Essence Mod: persistent buffs and auras.
 *
 * A buff is switched on by casting its spell and stays on until the player dies. Buffs are held in
 * memory only and are never saved, so a character always loads with none active.
 */
#pragma once

#include <cstdint>

#include <vector>

#include "engine/surface.hpp"
#include "tables/misdat.h"
#include "tables/spelldat.h"

namespace devilution {

struct Player;

enum class BuffID : uint8_t {
	Strength,
	FireAura,
	FlamingWeapon,
	LAST = FlamingWeapon,
};

/** @brief The spell that switches a buff on, and that earns its experience. */
SpellID GetBuffSpell(BuffID buff);

/**
 * @brief Whether a buff's spell earns a tenth of the player's other experience gains.
 * A damage aura earns experience from its own damage instead.
 */
bool BuffSharesExperience(BuffID buff);

/**
 * @brief Whether a spell belongs to a passive buff, the kind that earns the tenth share.
 * Passive buffs deal no damage; if one ever did, it would count as a plain weapon hit.
 */
bool IsExperienceSharingBuffSpell(SpellID spell);

/** @brief Switches a buff on for a player and recalculates their stats. */
void ActivateBuff(Player &player, BuffID buff);

bool IsBuffActive(const Player &player, BuffID buff);

/** @brief Switches off a buff or aura a player has on, whoever cast it. Every PC does this when told. */
void CancelBuff(Player &player, SpellID spell);

/**
 * @brief If the mouse is over one of the local player's own buffs in the buff bar, switches it
 * off and tells the other PCs. An ally's aura that merely reaches the player cannot be cancelled.
 * @return true if the mouse was over the buff bar's icons, so the click is used up.
 */
bool CancelBuffUnderCursor();

/**
 * @brief Switches on a buff that is read from essence_powers.tsv (effect "Buff"). Casting it again
 * while it is running starts its time again. Such buffs are held in memory only, like the others.
 * The amount is the buff's strength (30 is +30%), worked out by the PC of whoever cast it.
 *
 * A player never has the same buff twice. Cast again on someone who has it, a lasting buff keeps
 * whichever cast is stronger, and a timed one takes the new cast and starts its time again.
 * @return false if the player already had a stronger lasting one, so nothing changed.
 */
bool ActivatePowerBuff(Player &player, SpellID spell, int amount, const Player &caster);

/**
 * @brief How much a player's data-file buffs raise one stat, as a percentage (30 is +30%).
 * Different buffs to the same stat add up. The very same buff or aura never counts twice: if it
 * reaches the player from several places (their own aura and an ally's, say) the strongest counts.
 *
 * This counts the player's own buffs and every aura (effect "Aura") that reaches them: their own,
 * and those of other living players on the same level standing within the aura's radius.
 */
int GetPowerBuffPercent(const Player &player, std::string_view stat);

/**
 * @brief How far away a player seems to monsters, as a percentage of the real distance.
 * 100 is normal. Above it the player is stealthy: monsters prefer other targets and notice the
 * player only from closer. A buff with the stat "Distance" sets it; such buffs do not add up.
 */
int GetPlayerDistanceRatio(const Player &player);

/** @brief How close a monster must be to a player before it can notice them, in tiles. */
int GetPlayerNoticeRange(const Player &player);

/** @brief Whether a player has a heal over time on them that another player cast. */
bool HasHealOverTimeFrom(const Player &target, const Player &caster);

/**
 * @brief Takes incoming damage off the local player's shields of temporary health, oldest first,
 * and returns what is left to hurt them. Damage is in 64ths of a hit point.
 */
int AbsorbDamageWithShields(Player &player, int damage);

/** @brief Incoming damage after the player's "DamageTaken" buffs (50 is half damage). */
int ReduceDamageTaken(const Player &player, int damage);

/**
 * @brief The player who has bound themselves to take part of this player's damage (the stat
 * "Oath"), and the share they take as a percentage. nullptr if nobody can take it now.
 */
const Player *GetOathGuardian(const Player &player, int &percent);

/** @brief A caster's bond holds one player: takes this buff, cast by them, off everyone else. */
void RemovePowerBuffFromOthers(SpellID spell, const Player &caster, const Player &keep);

struct Monster;

/**
 * @brief The points that cursing auras take off a monster's resistance: auras whose stat is one
 * of the "...ResistCut" names, held by living players standing within the aura's radius of the
 * monster. Different auras add up; the same aura from two players counts once, at its strongest.
 * Every PC knows who holds an aura, so every PC agrees.
 * @param categoryBit The resistance in question: 1 Elemental, 2 Natural, 4 Astral.
 */
int GetAuraResistanceCut(const Monster &monster, uint8_t categoryBit);

/** @brief The player's own buff with a given stat, if they have one: which power, and how strong. */
bool GetPowerBuffWithStat(const Player &player, std::string_view stat, SpellID &spell, int &amount);

/** A buff or heal with this radius or more reaches every player in the game, wherever they are. */
constexpr int EveryoneRadius = 50;

/** @brief The spells of the data-file buffs a player has running. */
std::vector<SpellID> GetActivePowerBuffs(const Player &player);

/**
 * @brief Damage after the player's "more damage dealt" buffs. Every kind of damage a player deals
 * to a monster passes through this: weapon hits, spells, and effects over time.
 */
int ApplyDamageBuffs(const Player &player, int damage);

/**
 * @brief How many frames a player's "Speed" buffs take off an attack or a cast. A buff of X% makes
 * the action X% faster: frames x X / (100 + X) come off, rounded down, so a long animation gains
 * a frame more often than a short one and nobody is faster than the buff says. The game has no
 * finer step than a frame.
 * @param animationFrames How many frames the animation has.
 * @param actionFrame The frame on which the hit or the spell happens. Frames only come off before
 * it, and one is always left.
 */
int GetBuffSkippedFrames(const Player &player, int animationFrames, int actionFrame);

/**
 * @brief Whether a power read from essence_powers.tsv is passive: it is never cast, and acts by
 * itself when its moment comes. So far that is "Rebirth".
 */
bool IsPassivePower(SpellID spell);

/** @brief The passive powers a player knows. Like passive buffs, they earn a share of other experience. */
std::vector<SpellID> GetPassivePowers(const Player &player);

/**
 * @brief Called when the local player's life has just reached zero. If they know a "Rebirth"
 * power that is off cooldown, they rise at once with part of their life and the power goes on
 * cooldown.
 * @return true if the player was saved and must not be killed.
 */
bool TryRebirth(Player &player);

/**
 * @brief Whether the player has just risen through a "Rebirth" power and is still protected.
 * For that moment they take no damage and cannot be staggered, so the rising can be seen and
 * is not undone by the very next blow.
 */
bool IsRebirthProtected(const Player &player);

/** @brief Counts down the buffs that run out. Runs every game tick, in town as well as the dungeon. */
void ProcessBuffTimers();

/** @brief Switches every buff off, e.g. on death or when a character is loaded. Does not recalculate stats. */
void ClearBuffs(Player &player);

/** @brief Recalculates a player's stats if they have any buff active, e.g. after a buff spell levels up. */
void RefreshBuffStats(Player &player);

/** @brief Strength added by the player's active buffs: 10 at spell level 1, plus 5 per level after that. */
int GetBuffStrengthBonus(const Player &player);

/** @brief Fire damage Flaming Weapon adds to each weapon hit, in 64ths of a hit point. Zero when the buff is off. */
int GetFlamingWeaponDamage(const Player &player);

/** @brief Runs auras for one game tick: every 2 seconds a damage aura pulses around its owner. */
void ProcessBuffs();

/** @brief Draws the local player's active buffs as a row of icons across the top of the screen. */
void DrawBuffBar(const Surface &out);

} // namespace devilution
