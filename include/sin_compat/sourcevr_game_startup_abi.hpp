#pragma once
// SHA-pinned actual SourceVR 0.5.5 ARM64 engine->game virtual callsites.
// Evidence: audit_sourcevr_game_startup_dispatch.py, seven game profiles.
// No native SourceVR engine is linked. C++ signatures are NOT yet proven;
// these are instruction-level register arguments and observed return usage.
#include "sin_compat/sourcevr_game_vtable_layout.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::sourcevr_startup_abi {

enum class ResultUsage : std::uint8_t {
    BranchOnNonzeroW0,    // engine examines w0 after call
    Ignored,              // engine does not consume a result at callsite
    PointerAsLogString,   // returned x0 passed as %s
    Unverified,
};

struct NativeStartupCallsite {
    std::string_view hint;    // Source SDK candidate, NOT a recovered symbol
    native_game_vtables::NativeGameObjectRole role;
    std::uint32_t vtable_method_index;
    std::uint8_t pointer_register_count; // x1..xN, excluding x0=this
    ResultUsage observed_result;
    bool engine_dispatch_verified;
    bool callee_prologue_verified;
    bool complete_cpp_signature_verified;
};

// Native ARM64 client dispatch in seven ELF engines:
// x0=this, x1=pointer, x2=copy of x1, x3=pointer; CBNZ w0.
// Both native and original versions remain independently typed objects.
inline constexpr NativeStartupCallsite client_init{
    "IBaseClientDLL::Init (candidate)", native_game_vtables::NativeGameObjectRole::Client,
    0, 3, ResultUsage::BranchOnNonzeroW0, true, true, false
};

// Engine invokes method index 2 on the client interface during cleanup
// after another factory request. This is consistent with SDK Shutdown.
inline constexpr NativeStartupCallsite client_cleanup{
    "IBaseClientDLL::Shutdown (candidate)",
    native_game_vtables::NativeGameObjectRole::Client,
    2, 0, ResultUsage::Ignored, true, true, false
};

// Server vtable slot12 returns x0; engine logs it through second %s.
// Consistent with SDK GetGameDescription, but complete ABI is not proven.
inline constexpr NativeStartupCallsite server_description{
    "IServerGameDLL::GetGameDescription (candidate)",
    native_game_vtables::NativeGameObjectRole::Server,
    12, 0, ResultUsage::PointerAsLogString, true, true, false
};

// The native server's vtable method0 prologue demonstrably consumes
// x1,x2,x3,x4, as expected of SDK DLLInit. ENGINE callsite NOT verified.
inline constexpr NativeStartupCallsite server_startup_candidate{
    "IServerGameDLL::DLLInit (candidate)",
    native_game_vtables::NativeGameObjectRole::Server,
    0, 4, ResultUsage::Unverified, false, true, false
};

inline constexpr std::array<NativeStartupCallsite,4> calls{{
    client_init,client_cleanup,server_description,server_startup_candidate
}};

// Always closed until every actual C++ method signature and object
// lifetime guarantee has been separately verified; native table
// length and register shape alone do not make virtual dispatch safe.
constexpr bool safe_to_dispatch(const NativeStartupCallsite& c) noexcept {
    return c.engine_dispatch_verified && c.callee_prologue_verified &&
           c.complete_cpp_signature_verified;
}
static_assert(!safe_to_dispatch(client_init));
static_assert(!safe_to_dispatch(client_cleanup));
static_assert(!safe_to_dispatch(server_description));
static_assert(!safe_to_dispatch(server_startup_candidate));

} // namespace sin_compat::sourcevr_startup_abi
