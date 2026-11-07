#pragma once
// SiN 2006 game module factories versus *actual* SourceVR 0.5.5 ARM64
// game module registrations, cross-checked across all seven native profiles.
// Read-only research; DO NOT alias strings or reinterpret x86 vtable pointers.
// Source evidence: audit_sourcevr_native_game_factories.py, requiring original
// SiN PE32 and native Android ELF hashes plus actual registrar/lookup code.

#include "sin_compat/sourcevr_factory_preflight.hpp"
#include <array>

namespace sin_compat::sourcevr_game_factory {

using sin_compat::phase96::FactoryGate;
using sin_compat::phase96::InterfaceEvidence;
using sin_compat::phase96::gate_factory;
using sin_compat::phase96::safe_to_bind;

inline constexpr std::array<InterfaceEvidence, 2> sourcevr_055_native_game_modules{{
    {"ServerGameDLL003", "ServerGameDLL010", true, false},
    {"VClient011", "VClient017", true, false},
}};

// Name aliases alone would not establish C++ interface ABI even if accepted.
// The exact observed names differ, therefore no direct original game module
// attachment can be considered safe using this evidence.
static_assert(!safe_to_bind(sourcevr_055_native_game_modules),
              "SiN original game module factories are NOT SourceVR 0.5.5 compatible");
static_assert(gate_factory(sourcevr_055_native_game_modules[0]) ==
                  FactoryGate::VersionMismatch,
              "ServerGameDLL ABI mismatch must remain explicit");
static_assert(gate_factory(sourcevr_055_native_game_modules[1]) ==
                  FactoryGate::VersionMismatch,
              "VClient ABI mismatch must remain explicit");

} // namespace sin_compat::sourcevr_game_factory
