# CLAUDE.md — Essence Mod (DevilutionX)

## What this project is
The **Essence Mod**: a personal mod of **DevilutionX** (the open-source source port of Diablo 1 / Hellfire, written in C++).
The goal is to turn Diablo 1 into a classless, build-your-own-character game: essence-based spell
loadouts, a real curse system, learn-by-doing ability progression, stat-driven spells, and mana
regeneration. Full design notes live in `docs/MOD_DESIGN.md` — read it before starting any feature.

Inspired by the *He Who Fights With Monsters* books (essences, awakening stones, Astral). **Keep it
quick and simple:** a few major changes and many small ones. Prefer reusing what Diablo already does
over building new systems, and offer the smaller version of an idea first.

**Primary target: Hellfire** (decided 2026-10-02). The mod relies on Hellfire's extra dungeons,
monsters, spells, and its 5-tab spellbook. Write shared-engine features so they also work in plain
Diablo where that costs nothing, but test on Hellfire characters and build Hellfire-specific UI.

## How I want to work
- I'm newer to C++ and want to **learn as we go**. Explain what each change does and why, not just the code.
- Go **one step at a time** and wait for my confirmation before moving to the next step.
- Before changing code for a feature, first **show me where the relevant existing code lives** and
  walk me through how it works today (e.g., trace how an existing spell goes from cast to effect).
- Keep changes small and testable. One feature, one build, one test in-game.
- Don't ask whether I want to stop or wrap up — I'll end the session myself.

## Environment
- **OS:** Windows
- **Compiler/IDE:** Visual Studio Community with "Desktop development with C++",
  "C++ CMake tools for Windows", and the Windows SDK matching my Windows version
- **Dependencies:** vcpkg (`vcpkg integrate install`)
- **Build:** Open `CMakeLists.txt` in Visual Studio (File → Open → CMake), use the `x64-Debug`
  configuration while developing, `x64-Release` for play builds
- **Source control:** my own GitHub fork of `diasurgical/DevilutionX`; work on feature branches,
  not `master`
- **Official build docs:** `docs/building.md` in this repo

## Hard rules
1. **Do not change the save file format.** Diablo saves are fixed-layout binary; adding fields to
   saved player/monster data breaks old saves and compatibility with regular DevilutionX.
   - Short-lived state (curses, temporary buffs) lives in memory only and is **not saved** —
     it starts empty on load.
   - Permanent new data (essence loadouts, ability XP, essence levels) goes in a **separate
     sidecar file** next to the save, never inside the original save format.
2. **Never commit or redistribute Blizzard assets** (`DIABDAT.MPQ`, `hellfire.mpq`, original art or
   sound, or edited versions of them). Players supply their own copy of the game.
3. **Reuse existing art** for new spells/effects/icons until the mechanics are proven.
4. **Keep multiplayer in mind.** New effects must behave deterministically so all players running
   the same build stay in sync. Note any change that would affect multiplayer.
5. **Confirm uncertain facts in the code** instead of assuming (see "To verify" in the design doc).

## Testing setup
- Run the modded build from a **separate portable test folder** (DevilutionX runs self-contained
  when `diablo.ini` sits next to the executable) so test saves never touch real ones.
- `DIABDAT.MPQ` (and Hellfire MPQs) are copied into that test folder, not into the repo.
- Use throwaway characters. Back up any save I care about before running a new build.
- Test with a controller as well as mouse/keyboard when UI changes are involved.

## Setup progress
- [x] Step 1 — Install Visual Studio Community with the C++ workload + CMake tools + Windows SDK
- [x] Step 2 — Install Git for Windows
- [x] Step 3 — Install and integrate vcpkg
- [x] Step 4 — Fork DevilutionX on GitHub and clone my fork
- [x] Step 5 — Build unmodified DevilutionX and confirm it launches with my game data
- [x] Step 6 — Set up the portable test folder
- [x] Step 7 — First code change: mana regeneration (see design doc build order)
