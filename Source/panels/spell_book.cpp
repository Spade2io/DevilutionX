#include "panels/spell_book.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

#include "control/control.hpp"
#include "cooldowns.h"
#include "cursor.h"
#include "engine/backbuffer_state.hpp"
#include "engine/clx_sprite.hpp"
#include "engine/load_cel.hpp"
#include "engine/load_clx.hpp"
#include "engine/palette.h"
#include "engine/rectangle.hpp"
#include "engine/render/clx_render.hpp"
#include "engine/render/primitive_render.hpp"
#include "engine/render/text_render.hpp"
#include "essence_tint.h"
#include "essences.h"
#include "game_mode.hpp"
#include "missiles.h"
#include "panels/spell_icons.hpp"
#include "panels/ui_panels.hpp"
#include "player.h"
#include "spell_xp.h"
#include "tables/itemdat.h"
#include "tables/spelldat.h"
#include "utils/format.hpp"
#include "utils/language.h"
#include "utils/status_macros.hpp"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

OptionalOwnedClxSpriteList spellBookButtons;
OptionalOwnedClxSpriteList spellBookBackground;

constexpr uint16_t SpellBookButtonWidthDiablo = 76;
constexpr uint16_t SpellBookButtonWidthHellfire = 61;

uint16_t SpellBookButtonWidth()
{
	return gbIsHellfire ? SpellBookButtonWidthHellfire : SpellBookButtonWidthDiablo;
}

constexpr Size SpellBookDescription { 250, 43 };
constexpr int SpellBookDescriptionPaddingHorizontal = 2;

// Essence Mod page layout. A page is a header row followed by five ability rows. Dropping the
// original seven rows to six gives each ability a third line of text.
constexpr int PageTop = 12;
constexpr int HeaderRowHeight = 43;
constexpr int AbilityRowHeight = 51;
constexpr int AbilityTextLineSpacing = 15;
constexpr int IconX = 11;
constexpr int IconWidth = 37;
constexpr int IconHeight = 38;
constexpr int IconPaddingTop = 4;

/** Spellbook tab 1 is racial abilities, tabs 2 to 4 are the essences, tab 5 is the confluence. */
constexpr int FirstEssenceTab = 1;
constexpr int ConfluenceTab = 4;

bool IsEssenceTab(int tab)
{
	return tab >= FirstEssenceTab && tab < FirstEssenceTab + static_cast<int>(EssenceSlotCount);
}

int AbilityRowTop(size_t position)
{
	return PageTop + HeaderRowHeight + static_cast<int>(position) * AbilityRowHeight;
}

/** The ability at a position on the open tab. Essence pages only exist for the local player. */
SpellID GetAbilityOnOpenTab(size_t position)
{
	if (IsInspectingPlayer() || !IsEssenceTab(SpellbookTab))
		return SpellID::Invalid;
	return GetEssenceAbility(static_cast<size_t>(SpellbookTab - FirstEssenceTab), position);
}

void PrintSBookStr(const Surface &out, Point position, std::string_view text, UiFlags flags = UiFlags::None)
{
	DrawString(out, text,
	    Rectangle(GetPanelPosition(UiPanels::Spell, position + Displacement { SPLICONLENGTH, 0 }),
	        SpellBookDescription)
	        .inset({ SpellBookDescriptionPaddingHorizontal, 0 }),
	    { .flags = UiFlags::ColorWhite | flags });
}

SpellType GetSBookTrans(SpellID ii, bool townok)
{
	const Player &player = *InspectPlayer;
	SpellType st = SpellType::Spell;
	if ((player._pISpells & GetSpellBitmask(ii)) != 0) {
		st = SpellType::Charges;
	}
	if ((player._pAblSpells & GetSpellBitmask(ii)) != 0) {
		st = SpellType::Skill;
	}
	if (st == SpellType::Spell) {
		if (CheckSpell(*InspectPlayer, ii, st, true) != SpellCheckResult::Success) {
			st = SpellType::Invalid;
		}
		if (player.GetSpellLevel(ii) == 0) {
			st = SpellType::Invalid;
		}
	}
	if (townok && leveltype == DTYPE_TOWN && st != SpellType::Invalid && !GetSpellData(ii).isAllowedInTown()) {
		st = SpellType::Invalid;
	}

	return st;
}

StringOrView GetSpellPowerText(SpellID spell, int spellLevel)
{
	if (spellLevel == 0) {
		return _("Unusable");
	}
	if (spell == SpellID::BoneSpirit) {
		return _(/* TRANSLATORS: UI constraints, keep short please.*/ "Dmg: 1/3 target hp");
	}
	const auto [min, max] = GetDamageAmt(spell, spellLevel);
	if (min == -1) {
		return StringOrView {};
	}
	if (spell == SpellID::Healing || spell == SpellID::HealOther) {
		return FormatRuntime(_(/* TRANSLATORS: UI constraints, keep short please.*/ "Heals: {:d} - {:d}"), min, max);
	}
	return FormatRuntime(_(/* TRANSLATORS: UI constraints, keep short please.*/ "Damage: {:d} - {:d}"), min, max);
}

/** Essence Mod: a few words on what an ability does, for the third line of its entry. */
std::string_view GetAbilityDescription(SpellID spell)
{
	switch (spell) {
	case SpellID::Firebolt:
		return "Bolt of fire";
	case SpellID::Fireball:
		return "Exploding fireball";
	case SpellID::FireWall:
		return "Wall of flame";
	case SpellID::FlameWave:
		return "Advancing wave of fire";
	case SpellID::Inferno:
		return "Stream of flame";
	case SpellID::FireAura:
		return "Burns nearby monsters";
	case SpellID::FlamingWeapon:
		return "Fire on every weapon hit";
	case SpellID::FlameStrike:
		return "Fire weapon attack, +3";
	case SpellID::InfernoStrike:
		return "Fire weapon attack, x3";
	case SpellID::Lightning:
		return "Bolt of lightning";
	case SpellID::ChainLightning:
		return "Lightning at each enemy";
	case SpellID::ChargedBolt:
		return "Wandering sparks";
	case SpellID::Flash:
		return "Burst around you";
	case SpellID::Nova:
		return "Ring of lightning";
	default:
		return "";
	}
}

/** A page that has nothing on it yet: a title and a note. */
void DrawPlaceholderPage(const Surface &out, std::string_view title, std::string_view note)
{
	PrintSBookStr(out, { 0, PageTop + 7 }, title, UiFlags::ColorWhitegold);
	PrintSBookStr(out, { 0, PageTop + 25 }, note);
}

/** The top row of an essence page: the essence itself. */
void DrawEssenceHeader(const Surface &out, size_t slot)
{
	const EssenceID essence = IsInspectingPlayer() ? EssenceID::None : GetEssenceInSlot(slot);
	if (essence == EssenceID::None) {
		PrintSBookStr(out, { 0, PageTop + 7 }, "Empty essence slot", UiFlags::ColorWhitegold);
		PrintSBookStr(out, { 0, PageTop + 25 }, "Use an essence to claim this page");
		return;
	}

	// The essence's own picture, in its colour, where an ability's icon would be.
	const ClxSprite sprite = GetInvItemSprite(static_cast<int>(CURSOR_FIRSTITEM) + ICURS_ESSENCE);
	const Point cell = GetPanelPosition(UiPanels::Spell, { IconX + (IconWidth - sprite.width()) / 2, PageTop + IconPaddingTop + (IconHeight + sprite.height()) / 2 });
	if (const std::optional<EssenceTint> tint = GetEssenceTint(essence); tint)
		ClxDrawTRN(out, cell, sprite, GetEssenceTintTrn(*tint));
	else
		ClxDraw(out, cell, sprite);

	size_t held = 0;
	for (size_t position = 0; position < AbilitiesPerEssence; position++) {
		if (GetEssenceAbility(slot, position) != SpellID::Invalid)
			held++;
	}
	PrintSBookStr(out, { 0, PageTop + 7 }, StrCat(GetEssenceName(essence), " Essence"), UiFlags::ColorWhitegold);
	PrintSBookStr(out, { 0, PageTop + 25 }, StrCat(held, " of ", AbilitiesPerEssence, " abilities"));
}

/** One ability on an essence page: icon, three lines of text and the experience bar. */
void DrawAbilityRow(const Surface &out, const Player &player, SpellID sn, size_t position)
{
	const int top = AbilityRowTop(position);

	const SpellType st = GetSBookTrans(sn, true);
	SetSpellTrans(st);
	const Point iconPosition = GetPanelPosition(UiPanels::Spell, { IconX, top + IconPaddingTop + IconHeight - 1 });
	DrawSmallSpellIcon(out, iconPosition, sn);
	if (sn == player._pRSpell && st == player._pRSplType) {
		SetSpellTrans(SpellType::Skill);
		DrawSmallSpellIconBorder(out, iconPosition);
	}

	const Point line0 { 0, top + 2 };
	const Point line1 { 0, top + 2 + AbilityTextLineSpacing };
	const Point line2 { 0, top + 2 + 2 * AbilityTextLineSpacing };
	const int level = player.GetSpellLevel(sn);

	// Line 1: name and level.
	PrintSBookStr(out, line0, pgettext("spell", GetSpellData(sn).sNameText));
	PrintSBookStr(out, line0, FormatRuntime(pgettext(/* TRANSLATORS: UI constraints, keep short please.*/ "spellbook", "Level {:d}"), level), UiFlags::AlignRight);

	// Line 2: cost and cooldown on the left, damage on the right.
	std::string cost = StrCat("Mana ", GetManaAmount(player, sn) >> 6);
	if (const int cooldownTicks = GetSpellCooldownTicks(sn); cooldownTicks > 0)
		StrAppend(cost, "  Cooldown ", cooldownTicks / 20, "s");
	PrintSBookStr(out, line1, cost);
	if (const StringOrView text = GetSpellPowerText(sn, level); !text.empty())
		PrintSBookStr(out, line1, text, UiFlags::AlignRight);

	// Line 3: what it does on the left, experience towards the next level on the right.
	PrintSBookStr(out, line2, GetAbilityDescription(sn));
	if (const uint32_t needed = GetSpellExperienceForNextLevel(player, sn); needed > 0)
		PrintSBookStr(out, line2, StrCat(GetSpellExperience(sn) / 64, "/", needed, " XP"), UiFlags::AlignRight);

	constexpr int BarHeight = 3;
	const int barWidth = SpellBookDescription.width - 2 * SpellBookDescriptionPaddingHorizontal;
	const Point barPosition = GetPanelPosition(UiPanels::Spell, { SPLICONLENGTH + SpellBookDescriptionPaddingHorizontal, top + AbilityRowHeight - BarHeight - 1 });
	FillRect(out, barPosition.x, barPosition.y, barWidth, BarHeight, PAL16_GRAY + 13);
	const int filledWidth = barWidth * GetSpellLevelProgressPercent(player, sn) / 100;
	if (filledWidth > 0)
		FillRect(out, barPosition.x, barPosition.y, filledWidth, BarHeight, PAL16_YELLOW + 4);
}

} // namespace

std::expected<void, std::string> InitSpellBook()
{
	ASSIGN_OR_RETURN(spellBookBackground, LoadCelWithStatus("data\\spellbk", static_cast<uint16_t>(SidePanelSize.width)));
	ASSIGN_OR_RETURN(spellBookButtons, LoadCelWithStatus("data\\spellbkb", SpellBookButtonWidth()));
	return LoadSmallSpellIcons();
}

void FreeSpellBook()
{
	FreeSmallSpellIcons();
	spellBookButtons = std::nullopt;
	spellBookBackground = std::nullopt;
}

void DrawSpellBook(const Surface &out)
{
	constexpr int SpellBookButtonX = 7;
	constexpr int SpellBookButtonY = 348;
	ClxDraw(out, GetPanelPosition(UiPanels::Spell, { 0, 351 }), (*spellBookBackground)[0]);
	const int buttonX = gbIsHellfire && SpellbookTab < 5
	    ? SpellBookButtonWidthHellfire * SpellbookTab
	    : (SpellBookButtonWidthDiablo * SpellbookTab)
	        // BUGFIX: rendering of page 3 and page 4 buttons are both off-by-one pixel (fixed).
	        + (SpellbookTab == 2 || SpellbookTab == 3 ? 1 : 0);

	ClxDraw(out, GetPanelPosition(UiPanels::Spell, { SpellBookButtonX + buttonX, SpellBookButtonY }), (*spellBookButtons)[SpellbookTab]);

	// Essence Mod: the panel art has seven icon frames drawn into it at the original spacing.
	// The pages use their own spacing, so the icon column is blanked and drawn afresh.
	const Point column = GetPanelPosition(UiPanels::Spell, { 8, 15 });
	FillRect(out, column.x, column.y, 44, 302, 0);

	if (SpellbookTab == 0) {
		DrawPlaceholderPage(out, "Racial Abilities", "Not yet implemented");
		return;
	}
	if (SpellbookTab == ConfluenceTab) {
		DrawPlaceholderPage(out, "Confluence", "Not yet implemented");
		return;
	}
	if (!IsEssenceTab(SpellbookTab))
		return;

	DrawEssenceHeader(out, static_cast<size_t>(SpellbookTab - FirstEssenceTab));

	const Player &player = *InspectPlayer;
	for (size_t position = 0; position < AbilitiesPerEssence; position++) {
		const SpellID sn = GetAbilityOnOpenTab(position);
		if (IsValidSpell(sn) && player.GetBaseSpellLevel(sn) != 0)
			DrawAbilityRow(out, player, sn, position);
	}
}

void CheckSBook()
{
	// Essence Mod: clicking an ability's icon readies it. Icons sit in the five ability rows.
	if (!IsInspectingPlayer()) {
		for (size_t position = 0; position < AbilitiesPerEssence; position++) {
			const Rectangle iconArea = { GetPanelPosition(UiPanels::Spell, { IconX, AbilityRowTop(position) + IconPaddingTop }), Size { IconWidth, IconHeight } };
			if (!iconArea.contains(MousePosition))
				continue;
			const SpellID sn = GetAbilityOnOpenTab(position);
			Player &player = *InspectPlayer;
			if (IsValidSpell(sn) && player.GetBaseSpellLevel(sn) != 0) {
				player._pRSpell = sn;
				player._pRSplType = SpellType::Spell;
				RedrawEverything();
			}
			return;
		}
	}

	// The width of the panel excluding the border is 305 pixels. This does not cleanly divide by 4 meaning Diablo tabs
	// end up with an extra pixel somewhere around the buttons. Vanilla Diablo had the buttons left-aligned, devilutionX
	// instead justifies the buttons and puts the gap between buttons 2/3. See DrawSpellBook
	const int buttonWidth = SpellBookButtonWidth();
	// Tabs are drawn in a row near the bottom of the panel
	const Rectangle tabArea = { GetPanelPosition(UiPanels::Spell, { 7, 320 }), Size { 305, 29 } };
	if (tabArea.contains(MousePosition)) {
		int hitColumn = MousePosition.x - tabArea.position.x;
		// Clicking on the gutter currently activates tab 3. Could make it do nothing by checking for == here and return early.
		if (!gbIsHellfire && hitColumn > buttonWidth * 2) {
			// Subtract 1 pixel to account for the gutter between buttons 2/3
			hitColumn--;
		}
		SpellbookTab = hitColumn / buttonWidth;
	}
}

} // namespace devilution
