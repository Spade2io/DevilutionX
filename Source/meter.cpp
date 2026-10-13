/**
 * @file meter.cpp
 *
 * Essence Mod: the meter. See meter.h.
 */
#include "meter.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "DiabloUI/ui_flags.hpp"
#include "control/control.hpp"
#include "engine/palette.h"
#include "engine/point.hpp"
#include "engine/rectangle.hpp"
#include "engine/render/clx_render.hpp"
#include "engine/render/primitive_render.hpp"
#include "engine/render/text_render.hpp"
#include "inv.h"
#include "panels/quest_log.hpp"
#include "panels/ui_panels.hpp"
#include "qol/stash.h"
#include "qol/visual_store.h"
#include "stores.h"
#include "quests.h"
#include "spells.h"
#include "utils/format_int.hpp"
#include "utils/str_cat.hpp"

namespace devilution {

bool MeterOpen = false;

namespace {

/** What a line of the meter counts. A power is told apart by its spell; the others have no spell. */
enum class LineKind : uint8_t {
	Power,
	WeaponHits,
	Regeneration,
	Leech,
};

struct Line {
	LineKind kind;
	SpellID spell;
	/** In 64ths of a hit point. */
	int64_t total;
};

std::vector<Line> DamageLines;
std::vector<Line> HealingLines;

/** How many times each power has been used: cast, or swung as a special attack. */
std::vector<std::pair<SpellID, int>> Uses;
/** How many plain weapon hits have landed. */
int WeaponHitCount = 0;

/** Which of the two pages is showing: 0 damage, 1 healing and shields. */
int Page = 0;

void Add(std::vector<Line> &lines, LineKind kind, SpellID spell, int amount)
{
	if (amount <= 0)
		return;
	for (Line &line : lines) {
		if (line.kind == kind && line.spell == spell) {
			line.total += amount;
			return;
		}
	}
	lines.push_back({ kind, spell, amount });
}

std::string_view NameOf(const Line &line)
{
	switch (line.kind) {
	case LineKind::WeaponHits:
		return "Weapon hits";
	case LineKind::Regeneration:
		return "Health regen";
	case LineKind::Leech:
		return "Life leech";
	case LineKind::Power:
		break;
	}
	return IsValidSpell(line.spell) ? std::string_view { GetSpellData(line.spell).sNameText } : std::string_view { "Unknown" };
}

/** How many uses a line's total is spread over, or 0 where "a use" means nothing (regeneration, leech). */
int UsesOf(const Line &line)
{
	if (line.kind == LineKind::WeaponHits)
		return WeaponHitCount;
	if (line.kind != LineKind::Power)
		return 0;
	for (const auto &[spell, count] : Uses) {
		if (spell == line.spell)
			return count;
	}
	return 0;
}

std::string Amount(int64_t sixtyFourths)
{
	return FormatInteger(static_cast<int>(std::min<int64_t>((sixtyFourths + 32) >> 6, INT32_MAX)));
}

// The panel is the quest log's: 320 wide and 352 high. These are places on it.
constexpr int TextLeft = 34;
constexpr int TextWidth = 252;
constexpr int TitleTop = 26;
constexpr int HeaderTop = 46;
constexpr int ListTop = 62;
// Columns, each number set against its right-hand edge: the name, uses, the average of one use, the total.
constexpr int NameWidth = 126;
constexpr int UsesRight = 160;
constexpr int EachRight = 204;
constexpr int RowHeight = 22;
constexpr int MaxRows = 10;
constexpr int BarHeight = 4;
constexpr int ButtonTop = 300;
constexpr int ButtonHeight = 16;

/** The three words along the bottom, as places on the panel. */
constexpr Rectangle DamageButton { { TextLeft, ButtonTop }, { 76, ButtonHeight } };
constexpr Rectangle HealingButton { { TextLeft + 84, ButtonTop }, { 76, ButtonHeight } };
constexpr Rectangle ResetButton { { TextLeft + TextWidth - 56, ButtonTop }, { 56, ButtonHeight } };

Rectangle OnScreen(Rectangle onPanel)
{
	return { GetPanelPosition(UiPanels::Quest, onPanel.position), onPanel.size };
}

/** Whether the meter is the left-hand panel being shown: the others are drawn in its place. */
bool IsShowing()
{
	return MeterOpen && !CharFlag && !QuestLogIsOpen && !IsStashOpen && !IsVisualStoreOpen;
}

} // namespace

void ResetMeter()
{
	DamageLines.clear();
	HealingLines.clear();
	Uses.clear();
	WeaponHitCount = 0;
}

void MeterAddDamage(SpellID spell, int amount)
{
	if (spell == SpellID::Invalid || spell == SpellID::Null)
		Add(DamageLines, LineKind::WeaponHits, SpellID::Invalid, amount);
	else
		Add(DamageLines, LineKind::Power, spell, amount);
}

void MeterAddHealing(SpellID spell, int amount)
{
	Add(HealingLines, LineKind::Power, spell, amount);
}

void MeterAddRegeneration(int amount)
{
	Add(HealingLines, LineKind::Regeneration, SpellID::Invalid, amount);
}

void MeterAddLeech(int amount)
{
	Add(HealingLines, LineKind::Leech, SpellID::Invalid, amount);
}

void MeterAddUse(SpellID spell)
{
	if (spell == SpellID::Invalid || spell == SpellID::Null) {
		WeaponHitCount++;
		return;
	}
	for (auto &[known, count] : Uses) {
		if (known == spell) {
			count++;
			return;
		}
	}
	Uses.emplace_back(spell, 1);
}

void ToggleMeter()
{
	if (IsPlayerInStore())
		return;
	if (IsShowing()) {
		MeterOpen = false;
		return;
	}
	// It takes the place of whatever was open on the left.
	CloseCharPanel();
	QuestLogIsOpen = false;
	CloseGoldWithdraw();
	CloseStash();
	CloseVisualStore();
	MeterOpen = true;
}

void DrawMeter(const Surface &out)
{
	if (!IsShowing())
		return;
	ClxDraw(out, GetPanelPosition(UiPanels::Quest, { 0, 351 }), (*pQLogCel)[0]);

	// The page's lines, biggest first.
	std::vector<Line> lines = Page == 0 ? DamageLines : HealingLines;
	std::sort(lines.begin(), lines.end(), [](const Line &a, const Line &b) { return a.total > b.total; });
	int64_t total = 0;
	for (const Line &line : lines)
		total += line.total;

	const auto text = [&](std::string_view words, int left, int top, int width, UiFlags flags) {
		DrawString(out, words, OnScreen({ { left, top }, { width, 14 } }), { .flags = flags });
	};
	const UiFlags small = UiFlags::FontSize12;

	text(Page == 0 ? "Damage done" : "Healing and shields", TextLeft, TitleTop, TextWidth, UiFlags::ColorWhitegold);
	text(Amount(total), TextLeft, TitleTop, TextWidth, UiFlags::ColorWhitegold | UiFlags::AlignRight);

	if (lines.empty()) {
		text(Page == 0 ? "No damage dealt yet this game." : "No healing or shielding yet.", TextLeft, ListTop, TextWidth, UiFlags::ColorWhite | small);
	} else {
		// What the three columns of numbers are.
		text("Uses", TextLeft, HeaderTop, UsesRight, UiFlags::ColorGold | small | UiFlags::AlignRight);
		text("Each", TextLeft, HeaderTop, EachRight, UiFlags::ColorGold | small | UiFlags::AlignRight);
		text("Total", TextLeft, HeaderTop, TextWidth, UiFlags::ColorGold | small | UiFlags::AlignRight);
	}

	// When there are more lines than fit, the last row gathers the rest.
	const int64_t biggest = lines.empty() ? 1 : std::max<int64_t>(lines.front().total, 1);
	const uint8_t barColor = Page == 0 ? PAL8_RED + 3 : PAL8_YELLOW + 3;
	for (int row = 0; row < MaxRows && row < static_cast<int>(lines.size()); row++) {
		const bool gathers = row == MaxRows - 1 && static_cast<int>(lines.size()) > MaxRows;
		int64_t amount = lines[row].total;
		int uses = UsesOf(lines[row]);
		std::string name { NameOf(lines[row]) };
		if (gathers) {
			amount = 0;
			uses = 0;
			for (size_t i = row; i < lines.size(); i++)
				amount += lines[i].total;
			name = StrCat("The rest (", lines.size() - row, ")");
		}
		const int top = ListTop + row * RowHeight;
		text(name, TextLeft, top, NameWidth, UiFlags::ColorWhite | small);
		// The average of one use: what the power did in all, over the times it was used.
		if (uses > 0) {
			text(StrCat(uses), TextLeft, top, UsesRight, UiFlags::ColorWhite | small | UiFlags::AlignRight);
			text(Amount(amount / uses), TextLeft, top, EachRight, UiFlags::ColorWhite | small | UiFlags::AlignRight);
		}
		text(Amount(amount), TextLeft, top, TextWidth, UiFlags::ColorWhite | small | UiFlags::AlignRight);
		// The bar is drawn against the biggest line, so the first is always full width.
		const int width = std::clamp(static_cast<int>(amount * TextWidth / biggest), 1, TextWidth);
		const Point barAt = GetPanelPosition(UiPanels::Quest, { TextLeft, top + 14 });
		FillRect(out, barAt.x, barAt.y, TextWidth, BarHeight, PAL16_GRAY + 13);
		FillRect(out, barAt.x, barAt.y, width, BarHeight, barColor);
	}

	// The two pages and Reset, as words to click. The page showing is in gold.
	const auto button = [&](Rectangle place, std::string_view text, bool chosen, UiFlags align) {
		const Rectangle at = OnScreen(place);
		const bool hovered = at.contains(MousePosition);
		DrawString(out, text, at, { .flags = (chosen ? UiFlags::ColorWhitegold : hovered ? UiFlags::ColorBlue : UiFlags::ColorWhite) | UiFlags::FontSize12 | align });
	};
	button(DamageButton, "Damage", Page == 0, UiFlags::None);
	button(HealingButton, "Healing", Page == 1, UiFlags::None);
	button(ResetButton, "Reset", false, UiFlags::AlignRight);
}

bool CheckMeterClick()
{
	if (!IsShowing() || !GetLeftPanel().contains(MousePosition))
		return false;
	if (OnScreen(DamageButton).contains(MousePosition))
		Page = 0;
	else if (OnScreen(HealingButton).contains(MousePosition))
		Page = 1;
	else if (OnScreen(ResetButton).contains(MousePosition))
		ResetMeter();
	return true;
}

} // namespace devilution
