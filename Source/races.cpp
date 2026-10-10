/**
 * @file races.cpp
 *
 * Essence Mod: the racial powers every character of a race is born with.
 */
#include "races.h"

#include <cstdlib>
#include <expected>

#include "data/file.hpp"
#include "data/iterators.hpp"
#include "data/record_reader.hpp"
#include "essences.h"
#include "player.h"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

/** One row of racial_powers.tsv. */
struct RacialPower {
	std::string race;
	std::string name;
	std::string effect;
	int amount;
	std::string element;
};
std::vector<RacialPower> RacialPowers;
bool RacialPowersLoaded = false;

/** The table, read the first time it is wanted. The file is optional: without it no race has any. */
const std::vector<RacialPower> &AllRacialPowers()
{
	if (RacialPowersLoaded)
		return RacialPowers;
	RacialPowersLoaded = true;
	const std::string_view filename = "txtdata\\classes\\racial_powers.tsv";
	std::expected<DataFile, DataFile::Error> dataFileResult = DataFile::load(filename);
	if (dataFileResult.has_value() && dataFileResult.value().skipHeader().has_value()) {
		for (DataFileRecord record : dataFileResult.value()) {
			RecordReader reader { record, filename };
			RacialPower power {};
			std::string amount;
			reader.readString("race", power.race);
			reader.readString("name", power.name);
			reader.readString("effect", power.effect);
			reader.readString("amount", amount);
			reader.readString("element", power.element);
			power.amount = std::atoi(amount.c_str());
			RacialPowers.push_back(std::move(power));
		}
	}
	return RacialPowers;
}

/** A race is the name its class goes by: "Human", "Elf". */
const std::string &RaceOf(HeroClass heroClass)
{
	return GetPlayerDataForClass(heroClass).className;
}

} // namespace

std::vector<std::string> GetRacialPowerNames(HeroClass heroClass)
{
	std::vector<std::string> names;
	if (static_cast<size_t>(heroClass) >= GetNumPlayerClasses())
		return names;
	const std::string &race = RaceOf(heroClass);
	for (const RacialPower &power : AllRacialPowers()) {
		if (power.race == race)
			names.push_back(power.name);
	}
	return names;
}

std::vector<RacialPowerLine> DescribeRacialPowers(const Player &player)
{
	std::vector<RacialPowerLine> lines;
	if (static_cast<size_t>(player._pClass) >= GetNumPlayerClasses())
		return lines;
	const std::string &race = RaceOf(player._pClass);
	for (const RacialPower &power : AllRacialPowers()) {
		if (power.race != race)
			continue;
		const std::string &effect = power.effect;
		const int amount = power.amount;
		std::string text;
		if (effect == "XpGain") {
			text = StrCat("+", amount, "% experience");
		} else if (effect == "SpecialAptitude") {
			text = "Stones lean to special attacks";
		} else if (effect == "SpellAptitude") {
			text = "Stones lean to spell attacks";
		} else if (effect == "EssenceGift") {
			// The local player's essences are known here; anyone else's are not.
			std::string held;
			if (&player == MyPlayer) {
				for (size_t slot = 0; slot < 4; slot++) {
					const EssenceID essence = GetEssenceInSlot(slot);
					if (essence != EssenceID::None)
						StrAppend(held, held.empty() ? "" : ", ", GetEssenceName(essence));
				}
			}
			text = held.empty() ? StrCat("+", amount, "% for each essence held") : StrCat("+", amount, "%: ", held);
		} else if (effect == "Affinity") {
			text = StrCat("+", amount, "% ", power.element, " damage, heals, shields");
		} else if (effect == "MaxLife") {
			text = StrCat("+", amount, "% health pool");
		} else if (effect == "MaxMana") {
			text = StrCat("+", amount, "% mana pool");
		} else if (effect == "ManaRegen") {
			text = StrCat("+", amount, "% mana regeneration");
		} else if (effect == "LifeRegen") {
			text = StrCat("+", amount, "% health regeneration");
		} else if (effect == "SpellDamage") {
			text = StrCat("+", amount, "% spell damage");
		} else if (effect == "Resist") {
			text = StrCat("+", amount, " to all resistances");
		} else {
			// A stat: Power, Spirit, Speed, Recovery.
			text = StrCat("+", amount, "% ", effect);
		}
		lines.push_back({ power.name, std::move(text) });
	}
	return lines;
}

int GetRacialPercent(HeroClass heroClass, std::string_view effect)
{
	if (static_cast<size_t>(heroClass) >= GetNumPlayerClasses())
		return 0;
	const std::string &race = RaceOf(heroClass);
	int total = 0;
	for (const RacialPower &power : AllRacialPowers()) {
		if (power.race == race && power.effect == effect)
			total += power.amount;
	}
	return total;
}

int GetRacialElementPercent(const Player &player, SpellID spell)
{
	const EssenceID essence = GetSpellEssence(spell);
	if (essence == EssenceID::None)
		return 0;
	const std::string_view element = GetEssenceName(essence);
	const std::string &race = RaceOf(player._pClass);
	int total = 0;
	for (const RacialPower &power : AllRacialPowers()) {
		if (power.race != race)
			continue;
		if (power.effect == "Affinity" && power.element == element)
			total += power.amount;
		// A gift for each essence the character holds. Only this PC knows which those are.
		if (power.effect == "EssenceGift" && &player == MyPlayer && HasEssence(essence))
			total += power.amount;
	}
	return total;
}

} // namespace devilution
