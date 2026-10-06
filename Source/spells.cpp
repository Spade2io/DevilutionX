/**
 * @file spells.cpp
 *
 * Implementation of functionality for casting player spells.
 */
#include "spells.h"

#include "control/control.hpp"
#include "cursor.h"
#ifdef _DEBUG
#include "debug.h"
#endif
#include "engine/backbuffer_state.hpp"
#include "engine/point.hpp"
#include "engine/random.hpp"
#include "engine/world_tile.hpp"
#include "game_mode.hpp"
#include "gamemenu.h"
#include "inv.h"
#include "cooldowns.h"
#include "missiles.h"
#include "spell_xp.h"
#include "buffs.h"
#include "msg.h"
#include "utils/is_of.hpp"
#include "essence_tint.h"
#include "qol/floatingnumbers.h"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

/**
 * @brief Gets a value indicating whether the player's current readied spell is a valid spell. Readied spells can be
 * invalidaded in a few scenarios where the spell comes from items, for example (like dropping the only scroll that
 * provided the spell).
 * @param player The player whose readied spell is to be checked.
 * @return 'true' when the readied spell is currently valid, and 'false' otherwise.
 */
bool IsReadiedSpellValid(const Player &player)
{
	switch (player._pRSplType) {
	case SpellType::Skill:
	case SpellType::Spell:
	case SpellType::Invalid:
		return true;

	case SpellType::Charges:
		return (player._pISpells & GetSpellBitmask(player._pRSpell)) != 0;

	case SpellType::Scroll:
		return (player._pScrlSpells & GetSpellBitmask(player._pRSpell)) != 0;

	default:
		return false;
	}
}

/**
 * @brief Clears the current player's readied spell selection.
 * @note Will force a UI redraw in case the values actually change, so that the new spell reflects on the bottom panel.
 * @param player The player whose readied spell is to be cleared.
 */
void ClearReadiedSpell(Player &player)
{
	if (player._pRSpell != SpellID::Invalid) {
		player._pRSpell = SpellID::Invalid;
		RedrawEverything();
	}

	if (player._pRSplType != SpellType::Invalid) {
		player._pRSplType = SpellType::Invalid;
		RedrawEverything();
	}
}

} // namespace

bool IsValidSpell(SpellID spl)
{
	// Essence Mod: the numbers between the original spells and the extended powers are blank rows.
	return spl > SpellID::Null && static_cast<size_t>(spl) < SpellsData.size() && !SpellsData[static_cast<size_t>(spl)].sNameText.empty();
}

bool IsValidSpellFrom(int spellFrom)
{
	if (spellFrom == 0)
		return true;
	if (spellFrom >= INVITEM_INV_FIRST && spellFrom <= INVITEM_INV_LAST)
		return true;
	if (spellFrom >= INVITEM_BELT_FIRST && spellFrom <= INVITEM_BELT_LAST)
		return true;
	return false;
}

bool IsWallSpell(SpellID spl)
{
	return spl == SpellID::FireWall || spl == SpellID::LightningWall;
}

bool TargetsMonster(SpellID id)
{
	return id == SpellID::Fireball
	    || id == SpellID::FireWall
	    || id == SpellID::Inferno
	    || id == SpellID::Lightning
	    || id == SpellID::StoneCurse
	    || id == SpellID::FlameWave
	    || id == SpellID::Corruption
	    || id == SpellID::FlameStrike
	    || id == SpellID::InfernoStrike;
}

int GetManaAmount(const Player &player, SpellID sn)
{
	int ma; // mana amount

	// mana adjust
	int adj = 0;

	// spell level
	const int sl = std::max(player.GetSpellLevel(sn) - 1, 0);

	if (sl > 0) {
		adj = sl * GetSpellData(sn).sManaAdj;
	}
	if (sn == SpellID::Firebolt) {
		adj /= 2;
	}
	if (sn == SpellID::Resurrect && sl > 0) {
		adj = sl * (GetSpellData(SpellID::Resurrect).sManaCost / 8);
	}

	if (sn == SpellID::Healing || sn == SpellID::HealOther) {
		ma = (GetSpellData(SpellID::Healing).sManaCost + 2 * player.getCharacterLevel() - adj);
	} else if (GetSpellData(sn).sManaCost == 255) {
		ma = (player._pMaxManaBase >> 6) - adj;
	} else {
		ma = (GetSpellData(sn).sManaCost - adj);
	}

	ma = std::max(ma, 0);
	ma <<= 6;

	const ClassAttributes &classAttributes = GetClassAttributes(player._pClass);
	ma = ma * classAttributes.manaCost >> 6;

	if (GetSpellData(sn).sMinMana > ma >> 6) {
		ma = GetSpellData(sn).sMinMana << 6;
	}

	return ma;
}

void ConsumeSpell(Player &player, SpellID sn)
{
	switch (player.executedSpell.spellType) {
	case SpellType::Skill:
	case SpellType::Invalid:
		break;
	case SpellType::Scroll:
		ConsumeScroll(player);
		break;
	case SpellType::Charges:
		ConsumeStaffCharge(player);
		break;
	case SpellType::Spell:
#ifdef _DEBUG
		if (DebugGodMode)
			break;
#endif
		int ma = GetManaAmount(player, sn);
		player._pMana -= ma;
		player._pManaBase -= ma;
		RedrawComponent(PanelDrawComponent::Mana);
		break;
	}
	if (sn == SpellID::BloodStar) {
		ApplyPlrDamage(DamageType::Physical, player, 5);
	}
	if (sn == SpellID::BoneSpirit) {
		ApplyPlrDamage(DamageType::Physical, player, 6);
	}
}

void EnsureValidReadiedSpell(Player &player)
{
	if (!IsReadiedSpellValid(player)) {
		ClearReadiedSpell(player);
	}
}

SpellCheckResult CheckSpell(const Player &player, SpellID sn, SpellType st, bool manaonly)
{
#ifdef _DEBUG
	if (DebugGodMode)
		return SpellCheckResult::Success;
#endif

	if (!manaonly && pcurs != CURSOR_HAND) {
		return SpellCheckResult::Fail_Busy;
	}

	if (st == SpellType::Skill) {
		return SpellCheckResult::Success;
	}

	if (player.GetSpellLevel(sn) <= 0) {
		return SpellCheckResult::Fail_Level0;
	}

	if (player._pMana < GetManaAmount(player, sn) || HasAnyOf(player._pIFlags, ItemSpecialEffect::NoMana)) {
		return SpellCheckResult::Fail_NoMana;
	}

	return SpellCheckResult::Success;
}

void CastSpell(Player &player, SpellID spl, WorldTilePosition src, WorldTilePosition dst, int spllvl)
{
	Direction dir = player._pdir;
	if (IsWallSpell(spl)) {
		dir = player.tempDirection;
	}

	bool fizzled = false;
	const SpellData &spellData = GetSpellData(spl);
	SpellBeingCast = spl;
	for (size_t i = 0; i < sizeof(spellData.sMissiles) / sizeof(spellData.sMissiles[0]) && spellData.sMissiles[i] != MissileID::Null; i++) {
		Missile *missile = AddMissile(src, dst, dir, spellData.sMissiles[i], TARGET_MONSTERS, player, 0, spllvl);
		fizzled |= (missile == nullptr);
	}
	SpellBeingCast = SpellID::Invalid;
	if (spl == SpellID::ChargedBolt) {
		for (int i = (spllvl / 2) + 3; i > 0; i--) {
			Missile *missile = AddMissile(src, dst, dir, MissileID::ChargedBolt, TARGET_MONSTERS, player, 0, spllvl);
			fizzled |= (missile == nullptr);
		}
	}
	if (!fizzled) {
		ConsumeSpell(player, spl);
		// Essence Mod: a power read from essence_powers.tsv goes on cooldown when it is cast.
		// (A special attack's cooldown starts when its swing lands instead; see player.cpp.)
		if (&player == MyPlayer && IsExtendedSpell(spl))
			StartSpellCooldown(spl);
	}
}

/** Essence Mod: the share of their life the local player returns with, set by the power raising them. */
int PendingRevivePercent = 0;

void SpawnResurrectBeam(Player &caster, Player &target)
{
	AddMissile(
	    target.position.tile,
	    target.position.tile,
	    Direction::South,
	    MissileID::ResurrectBeam,
	    TARGET_MONSTERS,
	    caster.getId(),
	    0,
	    0);
}

void ApplyResurrect(Player &target)
{
	if (&target == MyPlayer) {
		MyPlayerIsDead = false;
		gamemenu_off();
		RedrawComponent(PanelDrawComponent::Health);
		RedrawComponent(PanelDrawComponent::Mana);
	}

	ClrPlrPath(target);
	target.destAction = ACTION_NONE;
	target._pInvincible = false;
	SyncInitPlrPos(target);

	int hp = 10 << 6;
	if (target._pMaxHPBase < (10 << 6)) {
		hp = target._pMaxHPBase;
	}
	// Essence Mod: a power that raises the fallen brings them back with a share of their life.
	if (&target == MyPlayer && PendingRevivePercent > 0) {
		hp = std::max(hp, static_cast<int>(static_cast<int64_t>(target._pMaxHP) * PendingRevivePercent / 100));
		PendingRevivePercent = 0;
	}
	SetPlayerHitPoints(target, hp);

	target._pHPBase = target._pHitPoints + (target._pMaxHPBase - target._pMaxHP); // CODEFIX: does the same stuff as SetPlayerHitPoints above, can be removed
	target._pMana = 0;
	target._pManaBase = target._pMana + (target._pMaxManaBase - target._pMaxMana);

	target._pmode = PM_STAND;

	CalcPlrInv(target, true);

	if (target.isOnActiveLevel()) {
		StartStand(target, target._pdir);
	}
}

void DoHealOther(const Player &caster, Player &target)
{
	if (target.hasNoLife()) {
		return;
	}

	int hp = (GenerateRnd(10) + 1) << 6;
	for (unsigned i = 0; i < caster.getCharacterLevel(); i++) {
		hp += (GenerateRnd(4) + 1) << 6;
	}
	for (int i = 0; i < caster.GetSpellLevel(SpellID::HealOther); i++) {
		hp += (GenerateRnd(6) + 1) << 6;
	}
	const ClassAttributes &classAttributes = GetClassAttributes(caster._pClass);
	hp = hp * classAttributes.healOtherRestoreLife >> 6;

	AddSpellExperienceForHealing(caster, target, SpellID::HealOther, std::min(hp, target._pMaxHP - target._pHitPoints));
	target._pHitPoints = std::min(target._pHitPoints + hp, target._pMaxHP);
	target._pHPBase = std::min(target._pHPBase + hp, target._pMaxHPBase);

	if (&target == MyPlayer) {
		RedrawComponent(PanelDrawComponent::Health);
	}
}

bool IsAllyTargetedPower(SpellID spell)
{
	return IsExtendedSpell(spell) && IsValidSpell(spell) && GetSpellData(spell).targetsAlly;
}

/**
 * Removes up to a number of harmful effects from a player and says how many went.
 *
 * Monsters put no harmful effects on players yet, so there is never anything to remove. This is
 * the one place cleansing happens, ready for when they do.
 */
int CleansePlayer(Player & /*target*/, int /*count*/)
{
	return 0;
}

void ApplyPowerToPlayer(const Player &caster, Player &target, SpellID spell, int amount)
{
	if (!IsExtendedSpell(spell) || !IsValidSpell(spell) || amount <= 0)
		return;
	const SpellData &spellData = GetSpellData(spell);
	const std::string &effect = spellData.effect;
	const std::string &rider = spellData.rider;
	const UiFlags textStyle = GetSpellTextColor(spell) | UiFlags::FontSize12;
	// A separate id from the damage numbers, so the game does not merge the two.
	const int textId = 2000 + target.getId();

	// "Resurrect": a fallen player is raised with the amount as a percentage of their life. The
	// player's own PC does the rising, as with the game's Resurrect, and tells the others.
	if (effect == "Resurrect" && target.hasNoLife()) {
		if (target.isOnActiveLevel())
			AddMissile(target.position.tile, target.position.tile, Direction::South, MissileID::ResurrectBeam, TARGET_MONSTERS, caster.getId(), 0, 0);
		if (&target == MyPlayer) {
			PendingRevivePercent = std::clamp(amount, 1, 100);
			NetSendCmd(true, CMD_PLRALIVE);
		}
		return;
	}
	if (target.hasNoLife())
		return;

	// A rider that names "Cleanse" washes harmful effects off as well as whatever else happens.
	if (rider.find("Cleanse") != std::string::npos)
		CleansePlayer(target, 99);

	if (IsAnyOf(effect, "Buff", "Aura")) {
		const bool applied = ActivatePowerBuff(target, spell, amount, caster);
		if (target.isOnActiveLevel())
			AddFloatingNumber(target.position.tile, { 0, 0 }, applied ? std::string(spellData.sNameText) : std::string("Already has a stronger one"), textStyle, textId);
		return;
	}

	if (effect == "Mana") {
		if (HasAnyOf(target._pIFlags, ItemSpecialEffect::NoMana))
			return;
		const int given = std::clamp(target._pMaxMana - target._pMana, 0, amount);
		target._pMana += given;
		target._pManaBase += given;
		if (&target == MyPlayer)
			RedrawComponent(PanelDrawComponent::Mana);
		if (target.isOnActiveLevel())
			AddFloatingNumber(target.position.tile, { 0, 0 }, given > 0 ? StrCat("+", (given + 32) >> 6, " mana") : std::string("Full mana"), textStyle, textId);
		return;
	}

	// "Cleanse" by itself: removes harmful effects and nothing else.
	if (effect == "Cleanse") {
		const int removed = CleansePlayer(target, amount);
		if (target.isOnActiveLevel())
			AddFloatingNumber(target.position.tile, { 0, 0 }, removed > 0 ? std::string("Cleansed") : std::string("Nothing to cleanse"), textStyle, textId);
		return;
	}

	// Everything else that reaches a player is healing: a heal of any kind, the healing half of
	// a burst or a strike, or what the living receive from a power that raises the fallen.
	// "HealPercent" gives the amount as a percentage of the player's maximum life.
	if (effect == "HealPercent")
		amount = static_cast<int>(static_cast<int64_t>(target._pMaxHP) * std::min(amount, 100) / 100);
	if (IsAnyOf(effect, "Heal", "HealPercent", "ChainHeal", "Resurrect") || rider.starts_with("Heal")) {
		const int healed = std::clamp(target._pMaxHP - target._pHitPoints, 0, amount);
		AddSpellExperienceForHealing(caster, target, spell, healed);
		target._pHitPoints = std::min(target._pHitPoints + amount, target._pMaxHP);
		target._pHPBase = std::min(target._pHPBase + amount, target._pMaxHPBase);

		if (&target == MyPlayer)
			RedrawComponent(PanelDrawComponent::Health);
		if (target.isOnActiveLevel())
			AddFloatingNumber(target.position.tile, { 0, 0 }, healed > 0 ? StrCat("+", (healed + 32) >> 6) : std::string("Full health"), textStyle, textId);
	}
}

int GetSpellBookLevel(SpellID s)
{
	if (gbIsSpawn) {
		switch (s) {
		case SpellID::StoneCurse:
		case SpellID::Guardian:
		case SpellID::Golem:
		case SpellID::Elemental:
		case SpellID::BloodStar:
		case SpellID::BoneSpirit:
			return -1;
		default:
			break;
		}
	}

	if (s < SpellID::Null || static_cast<size_t>(s) >= SpellsData.size()) {
		return -1;
	}

	return GetSpellData(s).sBookLvl;
}

int GetSpellStaffLevel(SpellID s)
{
	if (gbIsSpawn) {
		switch (s) {
		case SpellID::StoneCurse:
		case SpellID::Guardian:
		case SpellID::Golem:
		case SpellID::Apocalypse:
		case SpellID::Elemental:
		case SpellID::BloodStar:
		case SpellID::BoneSpirit:
			return -1;
		default:
			break;
		}
	}

	if (s < SpellID::Null || static_cast<size_t>(s) >= SpellsData.size()) {
		return -1;
	}

	return GetSpellData(s).sStaffLvl;
}

} // namespace devilution
