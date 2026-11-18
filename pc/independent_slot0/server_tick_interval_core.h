#pragma once
// Bounded owner-original ServerGameDLL003::GetTickInterval behavior.
// Original RVA 0x1E7A90, SHA-pinned Windows PE32 server.dll.
// The existing engine vstdlib.dll owns the command-line singleton.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Native Windows x86 command-line interface only
#endif

namespace sin_independent {

using CommandLineProvider = void* (__cdecl *)();
using CheckParmFn = const char* (__thiscall *)(const void*, const char*, const char**);
using IntParmValueFn = int (__thiscall *)(const void*, const char*, int);
constexpr float kDefaultTickInterval = 0.015f;

inline float TickIntervalFromRate(int rate) noexcept {
    return rate > 10 ? 1.0f / static_cast<float>(rate) : kDefaultTickInterval;
}

// The two vtable entries below were checked against the original import
// of vstdlib!CommandLine and SDK 2004 ICommandLine. The caller supplies a
// trusted engine-owned singleton or a deliberately owned synthetic fixture.
// No game-state mutation, original game DLL forwarding, or new DLL load.
inline bool TryGetTickInterval(CommandLineProvider provider, float& result) noexcept {
    if (!provider) return false;
    const void* cmd = provider();
    if (!cmd) return false;
    const auto* table = *reinterpret_cast<void* const* const*>(cmd);
    if (!table || !table[3]) return false;
    auto check = reinterpret_cast<CheckParmFn>(table[3]);
    if (!check(cmd, "-tickrate", nullptr)) {
        result = kDefaultTickInterval;
        return true;
    }
    // Original rereads the command-line singleton for the integer query.
    cmd = provider();
    if (!cmd) return false;
    table = *reinterpret_cast<void* const* const*>(cmd);
    if (!table || !table[7]) return false;
    auto value = reinterpret_cast<IntParmValueFn>(table[7]);
    result = TickIntervalFromRate(value(cmd, "-tickrate", 0));
    return true;
}

} // namespace sin_independent
