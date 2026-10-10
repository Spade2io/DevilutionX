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
#include <expected>
#include <random>
#include <string>
#include <vector>

#include "data/file.hpp"
#include "data/iterators.hpp"
#include "data/record_reader.hpp"
#include "engine/random.hpp"
#include "items.h"
#include "missiles.h"
#include "msg.h"
#include "multi.h"
#include "player.h"
#include "races.h"
#include "plrmsg.h"
#include "spells.h"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

/** The three essences, then a fourth if the player took one in place of their confluence. */
std::array<EssenceID, AbilitySlotCount> Slots {};
std::array<std::array<SpellID, AbilitiesPerEssence>, AbilitySlotCount> Abilities {};
/** Whether the player accepted their confluence, which gives the fourth row to it. */
bool ConfluenceTaken = false;
/** Whether this character's starting stats are the 10s every race now has. Kept in the sidecar file (line "B"). */
bool StatsRebased = true;

/** 0 Common, 1 Rare, 2 Epic, 3 Legendary; defined further down. */
size_t ParseRarity(std::string_view text);

/** The stat each row of abilities is bound to. */
std::array<EssenceStat, AbilitySlotCount> SlotStats {};
/** Points already added to the character's own stats for ability levels, by stat. Kept in the sidecar file. */
std::array<int, 5> PointsGranted {};

constexpr std::array<EssenceStat, 4> AllStats { EssenceStat::Power, EssenceStat::Spirit, EssenceStat::Speed, EssenceStat::Recovery };

/** One row of essence_stats.tsv: an essence and the stat it would rather be bound to. */
struct EssencePrimary {
	std::string essence;
	EssenceStat stat;
	size_t rarity;
};
std::vector<EssencePrimary> EssencePrimaries;
bool EssencePrimariesLoaded = false;

EssenceStat ParseEssenceStat(std::string_view name)
{
	for (const EssenceStat stat : AllStats) {
		if (name == GetEssenceStatName(stat))
			return stat;
	}
	return EssenceStat::None;
}

/** The stat an essence would rather be bound to, read from the designer's table the first time it is wanted. */
EssenceStat PrimaryStatOf(EssenceID essence)
{
	if (!EssencePrimariesLoaded) {
		EssencePrimariesLoaded = true;
		const std::string_view filename = "txtdata\\spells\\essence_stats.tsv";
		std::expected<DataFile, DataFile::Error> dataFileResult = DataFile::load(filename);
		if (dataFileResult.has_value() && dataFileResult.value().skipHeader().has_value()) {
			for (DataFileRecord record : dataFileResult.value()) {
				RecordReader reader { record, filename };
				std::string name;
				std::string primary;
				std::string rarity;
				reader.readString("essence", name);
				reader.readString("primary", primary);
				reader.readString("rarity", rarity);
				EssencePrimaries.push_back({ name, ParseEssenceStat(primary), ParseRarity(rarity) });
			}
		}
	}
	for (const EssencePrimary &entry : EssencePrimaries) {
		if (entry.essence == GetEssenceName(essence))
			return entry.stat;
	}
	return EssenceStat::None;
}

bool IsStatClaimed(EssenceStat stat)
{
	return std::find(SlotStats.begin(), SlotStats.end(), stat) != SlotStats.end();
}

/** Whether the local player is Filthy. Kept in the sidecar file. */
bool LocalFilthy = false;
/** Whether the filth has poured out of the local player yet, washed off since or not. Kept in the sidecar file. */
bool FilthHasHappened = false;
/** Whether each other player is, as their PCs have said. */
std::array<bool, MAX_PLRS> OthersFilthy {};

/** The burst of sludge, seen by everyone near, and the player's death cry. */
void ShowFilthEruption(Player &player)
{
	player.Say(HeroSpeech::AuughUh);
	if (!player.isOnActiveLevel())
		return;
	const Point centre = player.position.tile;
	// A brown ring at the feet (the colour of an Earth power, if the game has one to borrow it from)...
	const std::expected<SpellID, std::string> earth = ParseSpellId("StoneSpike");
	Missile *ringAtFeet = nullptr;
	for (const MissileID ring : { MissileID::FireAuraPulse, MissileID::FireAuraPulseBack }) {
		Missile *missile = AddMissile(centre, centre, player._pdir, ring, TARGET_MONSTERS, player.getId(), 0, 0);
		if (missile == nullptr)
			continue;
		if (earth.has_value())
			missile->sourceSpell = *earth;
		if (ringAtFeet == nullptr)
			ringAtFeet = missile;
	}
	// ...and dark bursts on the player's tile and every tile around it. The game's explosion takes
	// its place from the missile that caused it, so each is made from the ring and then moved.
	if (ringAtFeet == nullptr)
		return;
	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			const Point tile = centre + Displacement { dx, dy };
			if (!InDungeonBounds(tile))
				continue;
			Missile *burst = AddMissile(tile, { 0, 0 }, Direction::South, MissileID::CorruptionExplosion, TARGET_MONSTERS, player.getId(), 0, 0, ringAtFeet);
			if (burst == nullptr)
				continue;
			burst->position.tile = tile;
			burst->position.start = tile;
			burst->position.offset = { 0, 0 };
			// The ring's colour came along with it; these are the dark ones.
			burst->sourceSpell = SpellID::Invalid;
		}
	}
}

/** The local player reaches Iron Rank: the filth pours out and stays until washed off. */
void BecomeFilthy()
{
	if (MyPlayer == nullptr || FilthHasHappened)
		return;
	Player &player = *MyPlayer;
	FilthHasHappened = true;
	LocalFilthy = true;
	ShowFilthEruption(player);
	CalcPlrInv(player, true);
	EventPlrMsg("You have reached Iron Rank.", UiFlags::ColorWhitegold);
	EventPlrMsg("The filth of your old self pours out of you. You are Filthy.", UiFlags::ColorWhitegold);
	NetSendCmdParam1(true, CMD_FILTHY, 2);
}

/** One row of confluences.tsv: three essence names in alphabetical order, and what they form. */
struct ConfluenceName {
	std::array<std::string, EssenceSlotCount> essences;
	std::string name;
};
std::vector<ConfluenceName> ConfluenceNames;
bool ConfluenceNamesLoaded = false;

/** Reads the confluence names the first time one is wanted. The file is optional. */
void LoadConfluenceNames()
{
	if (ConfluenceNamesLoaded)
		return;
	ConfluenceNamesLoaded = true;
	const std::string_view filename = "txtdata\\spells\\confluences.tsv";
	std::expected<DataFile, DataFile::Error> dataFileResult = DataFile::load(filename);
	if (!dataFileResult.has_value())
		return;
	DataFile &dataFile = dataFileResult.value();
	if (!dataFile.skipHeader().has_value())
		return;
	for (DataFileRecord record : dataFile) {
		RecordReader reader { record, filename };
		ConfluenceName &entry = ConfluenceNames.emplace_back();
		reader.readString("essence1", entry.essences[0]);
		reader.readString("essence2", entry.essences[1]);
		reader.readString("essence3", entry.essences[2]);
		reader.readString("name", entry.name);
		std::sort(entry.essences.begin(), entry.essences.end());
	}
}

void ClearAbilities()
{
	for (auto &row : Abilities)
		row.fill(SpellID::Invalid);
}

/** An empty position is SpellID::Invalid, not zero, so the table must be cleared before first use. */
const bool AbilitiesCleared = (ClearAbilities(), true);

/** A row of power_tags.tsv: a power and the designer's tags on it. */
struct TaggedPower {
	SpellID spell;
	std::vector<std::string> tags;
};
/** A row of stone_tags.tsv: an awakening stone's full item name and its tags. */
struct TagStone {
	std::string itemName;
	std::vector<std::string> tags;
	/** 0 Common, 1 Rare, 2 Epic, 3 Legendary. */
	size_t rarity;
};

/** Rarity steps, least rare first: Common, Rare, Epic, Legendary. */
constexpr size_t RarityCount = 4;

size_t ParseRarity(std::string_view text)
{
	switch (text.empty() ? 'c' : text.front()) {
	case 'R':
	case 'r':
		return 1;
	case 'E':
	case 'e':
		return 2;
	case 'L':
	case 'l':
		return 3;
	default:
		return 0;
	}
}
std::vector<TaggedPower> TaggedPowers;
std::vector<TagStone> TagStones;
bool StoneTablesLoaded = false;

/** Splits "Fire|Enemy|Attack" into its tags. */
std::vector<std::string> SplitTags(std::string_view text)
{
	std::vector<std::string> tags;
	while (!text.empty()) {
		const size_t bar = text.find('|');
		if (bar != 0)
			tags.emplace_back(text.substr(0, bar));
		if (bar == std::string_view::npos)
			break;
		text.remove_prefix(bar + 1);
	}
	return tags;
}

/** Reads the tags of powers and stones the first time they are wanted. Both files are optional. */
void LoadStoneTables()
{
	if (StoneTablesLoaded)
		return;
	StoneTablesLoaded = true;
	{
		const std::string_view filename = "txtdata\\spells\\power_tags.tsv";
		std::expected<DataFile, DataFile::Error> dataFileResult = DataFile::load(filename);
		if (dataFileResult.has_value() && dataFileResult.value().skipHeader().has_value()) {
			for (DataFileRecord record : dataFileResult.value()) {
				RecordReader reader { record, filename };
				std::string key;
				std::string tags;
				reader.readString("id", key);
				reader.readString("tags", tags);
				if (const std::expected<SpellID, std::string> spell = ParseSpellId(key); spell.has_value())
					TaggedPowers.push_back({ *spell, SplitTags(tags) });
			}
		}
	}
	{
		const std::string_view filename = "txtdata\\spells\\stone_tags.tsv";
		std::expected<DataFile, DataFile::Error> dataFileResult = DataFile::load(filename);
		if (dataFileResult.has_value() && dataFileResult.value().skipHeader().has_value()) {
			for (DataFileRecord record : dataFileResult.value()) {
				RecordReader reader { record, filename };
				std::string name;
				std::string rarity;
				std::string tags;
				reader.readString("name", name);
				reader.readString("rarity", rarity);
				reader.readString("tags", tags);
				TagStones.push_back({ StrCat("Awakening Stone of ", name), SplitTags(tags), ParseRarity(rarity) });
			}
		}
	}
}

const TagStone *FindTagStone(std::string_view itemName)
{
	LoadStoneTables();
	for (const TagStone &stone : TagStones) {
		if (stone.itemName == itemName)
			return &stone;
	}
	return nullptr;
}

/** The slot holding an essence, or AbilitySlotCount if the player does not have it. */
size_t SlotOf(EssenceID essence)
{
	for (size_t slot = 0; slot < AbilitySlotCount; slot++) {
		if (Slots[slot] == essence)
			return slot;
	}
	return AbilitySlotCount;
}

bool RowHasRoom(size_t slot)
{
	const auto &row = Abilities[slot];
	return std::any_of(row.begin(), row.end(), [](SpellID held) { return held == SpellID::Invalid; });
}

/**
 * The row a new ability of an essence would go in: the essence's own, or once that is full the
 * confluence's, which takes powers from any of the three. AbilitySlotCount if neither has room.
 */
size_t RowWithRoomFor(EssenceID essence)
{
	const size_t slot = SlotOf(essence);
	if (slot >= AbilitySlotCount)
		return AbilitySlotCount;
	if (RowHasRoom(slot))
		return slot;
	if (ConfluenceTaken && slot < EssenceSlotCount && RowHasRoom(ConfluenceSlot))
		return ConfluenceSlot;
	return AbilitySlotCount;
}

/**
 * Binds a row to a stat, if it has none: its essence's primary stat when no other row holds it,
 * otherwise one of the stats still free, chosen at random. The confluence has no primary stat.
 * Four rows and four stats, so no two rows ever share one.
 */
void AssignSlotStat(size_t slot)
{
	if (slot >= AbilitySlotCount || SlotStats[slot] != EssenceStat::None)
		return;
	EssenceStat wanted = Slots[slot] != EssenceID::None ? PrimaryStatOf(Slots[slot]) : EssenceStat::None;
	if (wanted == EssenceStat::None || IsStatClaimed(wanted)) {
		std::vector<EssenceStat> free;
		for (const EssenceStat stat : AllStats) {
			if (!IsStatClaimed(stat))
				free.push_back(stat);
		}
		if (free.empty())
			return;
		static std::mt19937 generator { std::random_device {}() };
		wanted = free[std::uniform_int_distribution<size_t>(0, free.size() - 1)(generator)];
	}
	SlotStats[slot] = wanted;
}

bool HasThreeEssences()
{
	return std::none_of(Slots.begin(), Slots.begin() + EssenceSlotCount, [](EssenceID held) { return held == EssenceID::None; });
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
	case EssenceID::Holy:
		return "Holy";
	case EssenceID::Nature:
		return "Nature";
	case EssenceID::Shield:
		return "Shield";
	case EssenceID::Dark:
		return "Dark";
	case EssenceID::Earth:
		return "Earth";
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
	case SpellID::Teleport: // a Lightning power in this mod; Flash and Nova are not in the essence
		return EssenceID::Lightning;
	case SpellID::Frostbolt:
		return EssenceID::Water;
	case SpellID::HolyBolt:
		return EssenceID::Holy;
	case SpellID::Corruption:
		return EssenceID::Dark;
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
	return slot < AbilitySlotCount ? Slots[slot] : EssenceID::None;
}

SpellID GetEssenceAbility(size_t slot, size_t position)
{
	if (slot >= AbilitySlotCount || position >= AbilitiesPerEssence)
		return SpellID::Invalid;
	return Abilities[slot][position];
}

bool HasEssence(EssenceID essence)
{
	return essence != EssenceID::None && SlotOf(essence) < AbilitySlotCount;
}

bool CanAbsorbEssence(EssenceID essence)
{
	if (essence == EssenceID::None || HasEssence(essence))
		return false;
	// One of the three, or a fourth while the confluence has not been accepted in its place.
	const size_t free = SlotOf(EssenceID::None);
	return free < EssenceSlotCount || (free == ConfluenceSlot && !ConfluenceTaken);
}

void AbsorbEssence(EssenceID essence)
{
	if (!CanAbsorbEssence(essence))
		return;
	const size_t slot = SlotOf(EssenceID::None);
	Slots[slot] = essence;
	AssignSlotStat(slot);
	// A fourth essence fills the fourth row, as the confluence would have.
	if (slot == ConfluenceSlot)
		BecomeFilthy();
}

bool IsFilthy(const Player &player)
{
	return &player == MyPlayer ? LocalFilthy : OthersFilthy[player.getId()];
}

void SetOtherPlayerFilthy(Player &player, int state)
{
	if (&player == MyPlayer)
		return;
	const bool filthy = state != 0;
	if (OthersFilthy[player.getId()] == filthy)
		return;
	OthersFilthy[player.getId()] = filthy;
	if (state == 2)
		ShowFilthEruption(player);
	CalcPlrInv(player, true);
}

void WashOffFilth(Player &player)
{
	if (&player != MyPlayer || !LocalFilthy)
		return;
	LocalFilthy = false;
	CalcPlrInv(player, true);
	EventPlrMsg("You are clean again.", UiFlags::ColorWhitegold);
	NetSendCmdParam1(true, CMD_FILTHY, 0);
}

void ProcessFilth()
{
	if (MyPlayer == nullptr)
		return;
	// A character whose fourth row was filled before any of this existed has it happen now.
	if (!FilthHasHappened && (ConfluenceTaken || Slots[ConfluenceSlot] != EssenceID::None) && !MyPlayer->hasNoLife())
		BecomeFilthy();

	// Every ten seconds, for the sake of anyone who has joined since it began.
	static int countdown = 0;
	if (!LocalFilthy || !gbIsMultiplayer)
		return;
	if (--countdown > 0)
		return;
	countdown = 200;
	NetSendCmdParam1(false, CMD_FILTHY, 1);
}

std::string_view GetEssenceStatName(EssenceStat stat)
{
	switch (stat) {
	case EssenceStat::Power:
		return "Power";
	case EssenceStat::Spirit:
		return "Spirit";
	case EssenceStat::Speed:
		return "Speed";
	case EssenceStat::Recovery:
		return "Recovery";
	case EssenceStat::None:
		break;
	}
	return "No stat";
}

EssenceStat GetSlotStat(size_t slot)
{
	return slot < AbilitySlotCount ? SlotStats[slot] : EssenceStat::None;
}

EssenceStat GetEssenceStat(EssenceID essence)
{
	return essence != EssenceID::None ? GetSlotStat(SlotOf(essence)) : EssenceStat::None;
}

void GrantEarnedStatPoints(Player &player, bool inGame)
{
	// A row taken before stats grew this way is bound now, in the order the rows were taken.
	for (size_t slot = 0; slot < AbilitySlotCount; slot++) {
		if (Slots[slot] != EssenceID::None || (slot == ConfluenceSlot && ConfluenceTaken))
			AssignSlotStat(slot);
	}

	// One point for each level an ability has gained since it was learned.
	std::array<int, 5> earned {};
	for (size_t slot = 0; slot < AbilitySlotCount; slot++) {
		const EssenceStat stat = SlotStats[slot];
		if (stat == EssenceStat::None)
			continue;
		for (const SpellID spell : Abilities[slot]) {
			if (spell != SpellID::Invalid)
				earned[static_cast<size_t>(stat)] += ShownPowerLevel(player.GetBaseSpellLevel(spell));
		}
	}

	bool any = false;
	for (const EssenceStat stat : AllStats) {
		const auto index = static_cast<size_t>(stat);
		const int points = earned[index] - PointsGranted[index];
		if (points <= 0)
			continue;
		PointsGranted[index] = earned[index];
		any = true;
		if (inGame) {
			// These tell the other PCs, and bring life and mana along with the stat.
			switch (stat) {
			case EssenceStat::Power:
				ModifyPlrStr(player, points);
				break;
			case EssenceStat::Spirit:
				ModifyPlrMag(player, points);
				break;
			case EssenceStat::Speed:
				ModifyPlrDex(player, points);
				break;
			default:
				ModifyPlrVit(player, points);
				break;
			}
			EventPlrMsg(StrCat(GetEssenceStatName(stat), " +", points), UiFlags::ColorWhitegold);
		} else {
			// While loading there is nobody to tell yet; the stats are set and worked out afresh below.
			int &base = stat == EssenceStat::Power ? player._pBaseStr : stat == EssenceStat::Spirit ? player._pBaseMag
			    : stat == EssenceStat::Speed                                                       ? player._pBaseDex
			                                                                                       : player._pBaseVit;
			base = std::min(base + points, 255);
		}
	}
	if (any && !inGame) {
		RecalculateBaseLifeAndMana(player);
		CalcPlrInv(player, false);
	}

	// The character's level follows: 1 to start, and one more for every eight ability levels.
	int abilityLevels = 0;
	for (const int points : earned)
		abilityLevels += points;
	const int level = std::clamp(1 + abilityLevels / PowerLevelsPerCharacterLevel, 1, static_cast<int>(player.getMaxCharacterLevel()));
	if (inGame) {
		bool rose = false;
		while (player.getCharacterLevel() < level) {
			NextPlrLevel(player);
			rose = true;
		}
		if (rose) {
			NetSendCmdParam1(false, CMD_PLRLEVEL, player.getCharacterLevel());
			EventPlrMsg(StrCat("You reached level ", level), UiFlags::ColorWhitegold);
		}
	} else if (player.getCharacterLevel() != level) {
		// A character from before this rule is brought to it, up or down.
		player.setCharacterLevel(static_cast<uint8_t>(level));
		CalcPlrInv(player, false);
	}
}

bool IsConfluenceOffered()
{
	return HasThreeEssences() && !ConfluenceTaken && Slots[ConfluenceSlot] == EssenceID::None;
}

bool HasConfluence()
{
	return ConfluenceTaken;
}

void AcceptConfluence()
{
	if (!IsConfluenceOffered())
		return;
	ConfluenceTaken = true;
	AssignSlotStat(ConfluenceSlot);
	BecomeFilthy();
}

std::string GetConfluenceName()
{
	if (!HasThreeEssences())
		return {};
	LoadConfluenceNames();
	std::array<std::string, EssenceSlotCount> held;
	for (size_t slot = 0; slot < EssenceSlotCount; slot++)
		held[slot] = GetEssenceName(Slots[slot]);
	std::sort(held.begin(), held.end());
	for (const ConfluenceName &entry : ConfluenceNames) {
		if (entry.essences == held)
			return entry.name;
	}
	// Three essences the designer has not named a confluence for.
	return "Unnamed";
}

bool IsAuraPower(SpellID spell)
{
	if (spell == SpellID::FireAura)
		return true;
	return IsExtendedSpell(spell) && IsValidSpell(spell) && GetSpellData(spell).effect == "Aura";
}

LearnResult CheckCanLearn(SpellID spell)
{
	if (MyPlayer != nullptr && MyPlayer->GetBaseSpellLevel(spell) != 0)
		return LearnResult::AlreadyKnown;

	// One aura for each player. Abilities are only ever learned into essence slots, so the
	// slots hold every aura the player could know.
	if (IsAuraPower(spell)) {
		for (const auto &row : Abilities) {
			if (std::any_of(row.begin(), row.end(), [](SpellID held) { return held != SpellID::Invalid && IsAuraPower(held); }))
				return LearnResult::AuraAlreadyKnown;
		}
	}

	const EssenceID essence = GetSpellEssence(spell);
	if (essence == EssenceID::None)
		return LearnResult::NoEssenceAssigned;

	if (SlotOf(essence) >= AbilitySlotCount)
		return LearnResult::EssenceMissing;

	if (RowWithRoomFor(essence) >= AbilitySlotCount)
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
		return IsConfluenceOffered() ? "That essence is full. Accept your confluence for more room" : "You have no room left for this ability";
	case LearnResult::AuraAlreadyKnown:
		return "You already have an aura; you can only ever have one";
	}
	return "";
}

namespace {

/** How often stones and essences drop, as read from drops.tsv. These are what it holds if the file is missing. */
struct DropSettings {
	int stoneOneIn = 50;
	int essenceOneIn = 100;
	std::array<int, RarityCount> weights { 65, 25, 8, 2 };
};
DropSettings Drops;
/** The rows of the item table that can drop, by rarity. */
std::array<std::vector<int>, RarityCount> StoneDrops;
std::array<std::vector<int>, RarityCount> EssenceDrops;
bool DropTablesLoaded = false;

void LoadDropTables()
{
	if (DropTablesLoaded)
		return;
	DropTablesLoaded = true;
	LoadStoneTables();
	PrimaryStatOf(EssenceID::None); // reads the essences' table

	const std::string_view filename = "txtdata\\spells\\drops.tsv";
	std::expected<DataFile, DataFile::Error> dataFileResult = DataFile::load(filename);
	if (dataFileResult.has_value() && dataFileResult.value().skipHeader().has_value()) {
		for (DataFileRecord record : dataFileResult.value()) {
			RecordReader reader { record, filename };
			std::string setting;
			int value = 0;
			reader.readString("setting", setting);
			reader.readInt("value", value);
			if (setting == "stoneOneIn")
				Drops.stoneOneIn = value;
			else if (setting == "essenceOneIn")
				Drops.essenceOneIn = value;
			else if (setting == "common")
				Drops.weights[0] = value;
			else if (setting == "rare")
				Drops.weights[1] = value;
			else if (setting == "epic")
				Drops.weights[2] = value;
			else if (setting == "legendary")
				Drops.weights[3] = value;
		}
	}

	for (size_t row = 0; row < AllItemsList.size(); row++) {
		const ItemData &base = AllItemsList[row];
		if (base.iMiscId == IMISC_ESSENCE) {
			for (const EssencePrimary &entry : EssencePrimaries) {
				if (StrCat(entry.essence, " Essence") == base.iName)
					EssenceDrops[entry.rarity].push_back(static_cast<int>(row));
			}
		} else if (base.iMiscId == IMISC_AWAKENINGSTONE && base.iSpell == SpellID::Null) {
			if (const TagStone *stone = FindTagStone(base.iName); stone != nullptr)
				StoneDrops[stone->rarity].push_back(static_cast<int>(row));
		}
	}
}

} // namespace

int RollEssenceLoot(bool isBoss, bool always)
{
	LoadDropTables();

	// A stone, an essence, or nothing. A boss always drops one of the two, in proportion to how
	// often each drops from anything else (a stone twice as often as an essence, as the file stands).
	bool stone = true;
	if (isBoss || always) {
		stone = GenerateRnd(std::max(Drops.stoneOneIn + Drops.essenceOneIn, 1)) < Drops.essenceOneIn;
	} else if (Drops.stoneOneIn > 0 && GenerateRnd(Drops.stoneOneIn) == 0) {
		stone = true;
	} else if (Drops.essenceOneIn > 0 && GenerateRnd(Drops.essenceOneIn) == 0) {
		stone = false;
	} else {
		return -1;
	}

	// How rare. A boss rolls as if there were no Common ones.
	std::array<int, RarityCount> weights = Drops.weights;
	if (isBoss)
		weights[0] = 0;
	int total = 0;
	for (const int weight : weights)
		total += std::max(weight, 0);
	size_t rarity = 0;
	if (total > 0) {
		int roll = GenerateRnd(total);
		for (size_t r = 0; r < RarityCount; r++) {
			if (roll < weights[r]) {
				rarity = r;
				break;
			}
			roll -= std::max(weights[r], 0);
		}
	}

	// Which one: any of that rarity. If none has been made at that rarity, the next one down.
	const auto pick = [rarity](const std::array<std::vector<int>, RarityCount> &table, size_t lowest) {
		for (size_t r = rarity + 1; r-- > lowest;) {
			if (!table[r].empty())
				return table[r][GenerateRnd(static_cast<int>(table[r].size()))];
		}
		return -1;
	};
	const auto &wanted = stone ? StoneDrops : EssenceDrops;
	const auto &other = stone ? EssenceDrops : StoneDrops;
	if (!isBoss)
		return pick(wanted, 0);
	// A boss would sooner drop the other kind than a Common one, and a Common one than nothing.
	int row = pick(wanted, 1);
	if (row == -1)
		row = pick(other, 1);
	if (row == -1)
		row = pick(wanted, 0);
	return row;
}

bool IsTagStoneKnown(std::string_view itemName)
{
	return FindTagStone(itemName) != nullptr;
}

SpellID PickPowerForStone(std::string_view itemName)
{
	// A stone the designer no longer lists has no tags, so anything learnable matches it.
	const TagStone *stone = FindTagStone(itemName);
	std::vector<std::string> wanted;
	if (stone != nullptr)
		wanted = stone->tags;
	// A racial aptitude: a stone that says Attack without saying which kind leans the race's way.
	const auto wants = [&wanted](std::string_view tag) { return std::find(wanted.begin(), wanted.end(), tag) != wanted.end(); };
	if (MyPlayer != nullptr && wants("Attack") && !wants("Special Attack") && !wants("Spell Attack")) {
		if (GetRacialPercent(MyPlayer->_pClass, "SpecialAptitude") != 0)
			wanted.emplace_back("Special Attack");
		else if (GetRacialPercent(MyPlayer->_pClass, "SpellAptitude") != 0)
			wanted.emplace_back("Spell Attack");
	}
	std::vector<SpellID> pool;
	size_t best = 0;
	for (const TaggedPower &power : TaggedPowers) {
		// Everything that stops a power being learned is checked here, the one-aura rule included.
		if (!IsValidSpell(power.spell) || CheckCanLearn(power.spell) != LearnResult::Ok)
			continue;
		size_t matched = 0;
		for (const std::string &tag : wanted) {
			if (std::find(power.tags.begin(), power.tags.end(), tag) != power.tags.end())
				matched++;
		}
		// Only the powers matching the most tags stay in the draw.
		if (matched > best) {
			best = matched;
			pool.clear();
		}
		if (matched == best)
			pool.push_back(power.spell);
	}
	if (pool.empty())
		return SpellID::Invalid;
	// The draw is this player's alone; the other PCs are only told which power was learned.
	static std::mt19937 generator { std::random_device {}() };
	return pool[std::uniform_int_distribution<size_t>(0, pool.size() - 1)(generator)];
}

void SlotLearnedAbility(SpellID spell)
{
	if (IsAbilitySlotted(spell))
		return;
	const size_t slot = RowWithRoomFor(GetSpellEssence(spell));
	if (slot >= AbilitySlotCount)
		return;
	auto &row = Abilities[slot];
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
	SlotStats.fill(EssenceStat::None);
	PointsGranted.fill(0);
	LocalFilthy = false;
	FilthHasHappened = false;
	OthersFilthy.fill(false);
	ConfluenceTaken = false;
	StatsRebased = true; // a new character already starts at 10s
	ClearAbilities();
}

void BeginEssenceSidecarRead()
{
	StatsRebased = false; // until the file says otherwise
}

void RebaseStartingStats(Player &player)
{
	if (StatsRebased)
		return;
	StatsRebased = true;
	// What each old class started with above or below 10: Power, Spirit, Speed, Recovery.
	std::array<int, 4> head {};
	switch (player._pClass) {
	case HeroClass::Warrior:
		head = { 5, 0, 0, 0 };
		break;
	case HeroClass::Rogue:
		head = { -5, 0, 5, 5 };
		break;
	case HeroClass::Sorcerer:
		head = { 0, 10, -5, 0 };
		break;
	case HeroClass::Monk:
		head = { 5, -5, 5, 0 };
		break;
	case HeroClass::Bard:
		head = { 10, 10, 15, 10 };
		break;
	default:
		return;
	}
	player._pBaseStr = std::clamp(player._pBaseStr - head[0], 10, 255);
	player._pBaseMag = std::clamp(player._pBaseMag - head[1], 10, 255);
	player._pBaseDex = std::clamp(player._pBaseDex - head[2], 10, 255);
	player._pBaseVit = std::clamp(player._pBaseVit - head[3], 10, 255);
	RecalculateBaseLifeAndMana(player);
	CalcPlrInv(player, false);
}

void WriteEssenceSidecarLines(FILE *file)
{
	if (StatsRebased)
		std::fprintf(file, "B 1\n");
	if (ConfluenceTaken)
		std::fprintf(file, "C 1\n");
	if (FilthHasHappened)
		std::fprintf(file, "F %u\n", LocalFilthy ? 1U : 0U);
	// The stat each row is bound to, and the points its abilities' levels have already added.
	for (size_t slot = 0; slot < AbilitySlotCount; slot++) {
		if (SlotStats[slot] != EssenceStat::None)
			std::fprintf(file, "S %u %u\n", static_cast<unsigned>(slot), static_cast<unsigned>(SlotStats[slot]));
	}
	for (size_t stat = 1; stat < PointsGranted.size(); stat++) {
		if (PointsGranted[stat] != 0)
			std::fprintf(file, "G %u %u\n", static_cast<unsigned>(stat), static_cast<unsigned>(PointsGranted[stat]));
	}
	for (size_t slot = 0; slot < AbilitySlotCount; slot++) {
		// The confluence's row has no essence of its own.
		if (Slots[slot] == EssenceID::None && !(slot == ConfluenceSlot && ConfluenceTaken))
			continue;
		if (Slots[slot] != EssenceID::None)
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
	if (std::sscanf(line, "F %u", &slot) == 1) {
		FilthHasHappened = true;
		LocalFilthy = slot != 0;
		return true;
	}
	if (std::sscanf(line, "B %u", &slot) == 1) {
		StatsRebased = slot != 0;
		return true;
	}
	if (std::sscanf(line, "C %u", &slot) == 1) {
		ConfluenceTaken = slot != 0;
		return true;
	}
	if (std::sscanf(line, "S %u %u", &slot, &second) == 2) {
		if (slot < AbilitySlotCount && second <= static_cast<unsigned>(EssenceStat::Recovery))
			SlotStats[slot] = static_cast<EssenceStat>(second);
		return true;
	}
	if (std::sscanf(line, "G %u %u", &slot, &second) == 2) {
		if (slot < PointsGranted.size())
			PointsGranted[slot] = static_cast<int>(std::min<unsigned>(second, 255));
		return true;
	}
	if (std::sscanf(line, "E %u %u", &slot, &second) == 2) {
		if (slot < AbilitySlotCount && second <= static_cast<unsigned>(EssenceID::LAST))
			Slots[slot] = static_cast<EssenceID>(second);
		return true;
	}
	if (std::sscanf(line, "A %u %u %u", &slot, &second, &third) == 3) {
		if (slot < AbilitySlotCount && second < AbilitiesPerEssence && IsValidSpell(static_cast<SpellID>(third)))
			Abilities[slot][second] = static_cast<SpellID>(third);
		return true;
	}
	return false;
}

} // namespace devilution
