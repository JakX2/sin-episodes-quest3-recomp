#pragma once

#include <filesystem>
#include <string>

#include "sin_compat/weapon_runtime.hpp"

namespace sin_compat {

// Parse a Source KeyValues weapon script into the gameplay runtime's configuration.
// Throws std::runtime_error on missing or malformed required data.
// Script text and game assets are supplied by the owner, never embedded in the library.
WeaponConfig parse_weapon_script(const std::string& text);

// Read <game_root>/scripts/weapon_{magnum,assault_rifle,scattergun}.txt.
WeaponConfig load_weapon_script(const std::filesystem::path& game_root, WeaponKind kind);

const char* weapon_script_filename(WeaponKind kind);

} // namespace sin_compat
