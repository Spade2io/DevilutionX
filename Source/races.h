/**
 * @file races.h
 *
 * Essence Mod: the racial powers every character of a race is born with.
 *
 * They are read from txtdata/classes/racial_powers.tsv, written by the designer's Races page. Each
 * row is a race (the class's name: "Human", "Elf"), the power's name, what it does (its effect),
 * an amount and, for an affinity, an element. A racial power is never cast and never grows; it
 * depends only on the character's race, so every PC works out the same thing for every player.
 *
 * Effects:
 *   XpGain            percent more experience from everything
 *   SpecialAptitude   a stone tagged Attack, naming neither kind of attack, leans to special attacks
 *   SpellAptitude     the same, leaning to spell attacks
 *   EssenceGift       percent more damage, healing and shields from the powers of each essence held
 *   Affinity          percent more damage, healing and shields from the powers of the named element
 *   Power, Spirit, Speed, Recovery   percent more of that stat
 *   MaxLife, MaxMana  percent larger pool
 *   ManaRegen, LifeRegen   percent faster regeneration
 *   SpellDamage       percent more damage from spell attacks
 *   Resist            points on all three resistances
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "tables/playerdat.hpp"
#include "tables/spelldat.h"

namespace devilution {

struct Player;

/** A racial power as the spellbook shows it: its name and a short line saying what it does. */
struct RacialPowerLine {
	std::string name;
	std::string text;
};

/**
 * @brief A player's racial powers for the spellbook's first page. For the local player a Human's
 * gifts name the essences held.
 */
std::vector<RacialPowerLine> DescribeRacialPowers(const Player &player);

/** @brief The names of a race's racial powers, in the designer's order. */
std::vector<std::string> GetRacialPowerNames(HeroClass heroClass);

/** @brief The total a race's powers give for an effect ("MaxMana"): 0 when it has none. */
int GetRacialPercent(HeroClass heroClass, std::string_view effect);

/**
 * @brief The percentage a player's race adds to the damage, healing or shield of a power because
 * of its element: an affinity for that element, and (for the local player, whose essences are
 * known here) a gift for each essence held.
 */
int GetRacialElementPercent(const Player &player, SpellID spell);

} // namespace devilution
