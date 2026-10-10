/**
 * @file essence_version.h
 *
 * Essence Mod: which release of the mod this build is.
 *
 * A multiplayer game says what it is running in three small numbers, which ordinary DevilutionX
 * fills with its own version (1.6.0). The mod fills them with these instead, so that a game can
 * only be joined by someone on the same release of the mod: plain DevilutionX never matches, and
 * neither does an older or newer release.
 *
 * Raise EssenceModRelease by one for every build that is handed out to other people.
 */
#pragma once

#include <cstdint>

namespace devilution {

/** Stands where DevilutionX puts its major version. No DevilutionX release will ever be 77. */
constexpr uint8_t EssenceModFamily = 77;
/** The release handed out. Everyone in a game must be on the same one. */
constexpr uint8_t EssenceModRelease = 1;

} // namespace devilution
