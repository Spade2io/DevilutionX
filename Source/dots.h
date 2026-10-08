/**
 * @file dots.h
 *
 * Essence Mod: damage-over-time effects on monsters.
 *
 * An effect such as Burn is shared by every power that applies it. Think of it as a bucket of
 * "damage per tick" on the monster: each application pours in its own amount (a higher-level power
 * pours in more), the monster takes the whole bucket every 2 seconds, any new application restarts
 * the 20-second timer for the whole bucket, and when the timer runs out the bucket empties. There is
 * no limit to how full it can get. Every effect uses the same 2-second tick and 20-second duration.
 * Effects are private to this PC: they are held in memory only, never saved, and never sent to other
 * players. Only the damage they deal is reported, through the game's normal damage message.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "tables/misdat.h"
#include "tables/spelldat.h"

namespace devilution {

struct Monster;
struct Player;

/**
 * @brief The default damage scaling for the mod's spells: 12.5% more for each spell level after the
 * first, compounding. This is the rule Fireball, Flash and Nova use. Level 1 gives the base unchanged.
 */
int ScaleDamageForSpellLevel(int baseDamage, int spellLevel);

/**
 * @brief Deals one tick of a spell's damage to a monster on behalf of the local player.
 *
 * Applies immunity and resistance, credits the spell with experience, reports the damage to other
 * players through the game's normal damage message, and flashes the monster in the spell's colour.
 * Used by damage-over-time effects and damage auras.
 *
 * @param damage Damage before resistance, in 64ths of a hit point.
 * @param finishKill Start the death of a monster this kills. Pass false when the caller does that itself.
 */
void DealSpellTickDamage(Monster &monster, SpellID spell, MissileID missile, DamageType damageType, int damage, bool finishKill = true);

enum class DotID : uint8_t {
	Corruption,
	Burn,
	LAST = Burn,
};

/** @brief The effect with this name ("Burn", "Corruption"), if there is one. */
std::optional<DotID> ParseDotName(std::string_view name);

/** @brief The name shown to the player, e.g. "Burn". */
std::string_view GetDotName(DotID dot);

/** @brief Whether a monster shrugs the effect off entirely. */
bool IsImmuneToDot(const Monster &monster, DotID dot);

/**
 * @brief Adds one application of an effect to a monster, restarts the effect's timer, and flashes
 * the monster in the effect's colour so the caster can see it landed.
 * @param spell The power applying it. Each power earns the experience for the damage it added.
 * @param damagePerTick What this application adds to each tick, in 64ths of a hit point.
 * @return The number of applications the monster now has.
 */
int AddMonsterDot(Monster &monster, DotID dot, SpellID spell, int damagePerTick);

/**
 * @brief Makes a monster take more damage from the local player, for as long as it lives.
 * Applying it again does not add up: the larger percentage counts.
 * Like the effects above, it is held on this PC only.
 * @return true if this made the monster more vulnerable than it already was.
 */
bool MakeMonsterVulnerable(Monster &monster, int percent);

/**
 * @brief The old name for stripping a monster's Elemental resistance and immunity; see
 * StripMonsterResistance. Kept for the "Kindle" effect, which only the caster's PC applies.
 * @return true if the monster had any to lose.
 */
bool KindleMonster(Monster &monster);

/**
 * @brief Brands a monster: from then on every direct hit the local player lands on it (a weapon
 * hit or a spell hit, not a tick of an effect over time) also deals this much fire damage, for as
 * long as it lives. Applying it again does not add up: the larger amount counts.
 * @param spell The power that applied it, which earns the experience for the extra damage.
 * @param damage Extra damage per hit, in 64ths of a hit point.
 */
void BrandMonster(Monster &monster, SpellID spell, int damage);

/**
 * @brief Deals a branded monster its extra fire damage. Call it once for each direct hit, after
 * the hit's own damage. Does not start the monster's death; the caller's usual check does.
 */
void TriggerMonsterBrand(Monster &monster);

/**
 * @brief Makes a monster miss more often. Unlike the effects above, every PC is told and keeps
 * this, because each PC works out the hits on its own player.
 * @param percent How much is taken off its chance to hit. The larger of two counts.
 * @param ticks How long it lasts in game ticks, or -1 for as long as the monster lives.
 */
void SetMonsterAccuracyPenalty(Monster &monster, int percent, int ticks);

/** @brief What is taken off a monster's chance to hit a player, in percentage points. */
int GetMonsterAccuracyPenalty(const Monster &monster);

/** How much of the damage a resistant monster shrugs off before anything reduces it: three quarters. */
constexpr int BaseMonsterResistancePercent = 75;

/**
 * @brief Which resistances a stat name reduces: "ElementalResistCut", "NaturalResistCut",
 * "AstralResistCut" or "AllResistCut". One bit for each of the three; zero for any other stat.
 */
uint8_t GetResistCutCategories(std::string_view stat);

/**
 * @brief Puts a curse on a monster that takes points off its resistance, for as long as it
 * lives or for a set time. Every PC keeps this. Curses from different powers add up; the same
 * power cast again keeps whichever is stronger.
 * @param categories As returned by GetResistCutCategories.
 * @param ticks How long it lasts in game ticks, or -1 for as long as the monster lives.
 */
void CutMonsterResistance(Monster &monster, SpellID spell, uint8_t categories, int points, int ticks);

/**
 * @brief Damage of a kind to a monster after its resistance. A resistant monster shrugs off 75%;
 * curses on it and cursing auras near it take points off that, never below none. An immune
 * monster is not handled here: this is for damage that is known to get through.
 */
int ApplyMonsterResistance(const Monster &monster, MissileID missile, DamageType damageType, int damage);

/** @brief The bit for the resistance a kind of damage is checked against: 1 Elemental, 2 Natural, 4 Astral, or 0 if nothing resists it. */
uint8_t GetResistanceBit(DamageType damageType);

/**
 * @brief Which resistances a stat name strips outright: "ElementalStrip", "NaturalStrip",
 * "AstralStrip" or "AllStrip". One bit for each; zero for any other stat.
 */
uint8_t GetStripCategories(std::string_view stat);

/**
 * @brief Strips a monster's resistance and its immunity in the categories given, for as long as
 * it lives. This is the one thing that removes immunity. Every PC keeps it.
 * @return true if the monster had anything there to lose.
 */
bool StripMonsterResistance(Monster &monster, uint8_t categories);

/** @brief Whether a monster has had its resistance and immunity to a kind of damage stripped. */
bool IsMonsterStripped(const Monster &monster, DamageType damageType);

/**
 * @brief Makes every effect over time on a monster deal more, for as long as it lives. Held on
 * this PC only, like the effects themselves. The larger of two percentages counts.
 */
void WitherMonster(Monster &monster, int percent);

/** @brief The damage an effect over time on a monster still has to deal, in 64ths of a hit point. */
int GetMonsterDotRemaining(const Monster &monster, DotID dot);

/**
 * @brief Whether a buff, aura or curse stat is a percentage that grows by a set step each level:
 * more damage, faster regeneration, a bigger pool, less damage taken and the like. Points (armor,
 * resistance, stat points), hit points (heals, shields) and stealth or threat are not.
 */
bool IsPercentBuffStat(std::string_view stat);

/**
 * @brief A percentage buff's strength at a level. It gains a set step each level, by where it
 * starts: 1% from 5% or less, 1.5% from 10%, 2% from 20%, 2.5% from 30% or more. Half percents
 * add up across levels and only whole percents count, so a 10% buff runs 10, 11, 13, 14, 16.
 * A "Speed" buff (attack and casting speed) is its own case: 2% a level from 5%, 3% from 10%.
 * @param shownLevel The power's level as the player sees it, 0 to 10.
 */
int GetPercentBuffAmount(std::string_view stat, int base, int shownLevel);

/**
 * @brief An amount after a stat's multiplier: the stat times 10 percent. A stat of 10 leaves it
 * as it is, 15 makes it half as much again, 5 halves it.
 */
int ScaleByStat(int amount, int stat);

/** @brief The stat that multiplies a player's weapon damage: Speed (Dexterity) with a bow in hand, otherwise Power (Strength). */
int GetWeaponDamageStat(const Player &player);

/**
 * @brief Damage from one of the local player's powers after their stat and buffs. A special attack
 * is a weapon attack and takes Power, or Speed if made with a bow. Anything else is a spell
 * attack, damage over time included: it takes Spirit (Magic), then the "SpellDamage" buffs and auras.
 */
int ApplySpellDamageBuffs(SpellID spell, int damage);

/**
 * @brief What the local player's power gives another player, after Spirit. Healing and shields
 * take the multiplier, heals over time included; a buff's strength, an aura's, mana and
 * percentages are left as they are.
 */
uint32_t ScaleGivenAmountBySpirit(SpellID spell, uint32_t amount);

/**
 * @brief Lightning that leaps on from a monster already struck: to the nearest other monster in
 * reach, and again, each time for the share of the last amount that is kept. Dealt by the local
 * player; call it only on the caster's own PC.
 * @param amount What the first leap deals, in 64ths of a hit point.
 * @param keepPercent How much of the amount each further leap keeps (100 loses nothing).
 */
void ChainSpellDamage(Monster &from, SpellID spell, DamageType damageType, int amount, int keepPercent, int leaps);

/** @brief Damage to a monster after its "takes more damage" debuff, if it has one. */
int ApplyMonsterVulnerability(const Monster &monster, int damage);

/** @brief Runs every effect for one game tick: counts down, deals damage when due, and expires. */
void ProcessMonsterDots();

/** @brief Removes every effect from every monster, e.g. when a level is set up. */
void ClearAllMonsterDots();

/**
 * @brief The colour table to draw a monster with while it flashes from a damage tick,
 * or nullptr when it is not flashing.
 */
const uint8_t *GetMonsterDotPulseTrn(const Monster &monster);

} // namespace devilution
