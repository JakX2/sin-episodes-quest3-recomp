#pragma once
// Phase1727: two source-authenticated 85-byte one-time static initializers
// called by the original CWorld constructor (0x240CCE30).
//
// Original .text functions 0x24061540 and 0x240CB2F0 check byte bit 0 of
// their separate guard DWORDs, and if unset set bit 0, clear exactly 9
// consecutive DWORDs, and call original CRT atexit-like wrapper 0x24248788
// with respective original cleanup thunk pointers. They return original
// 2006 virtual addresses of global storage, which MUST NOT be dereferenced
// or returned as host-native pointers.
//
// This models the real one-shot initialization on distinct HOST-owned bytes,
// with a caller-provided synchronous callback to record/register an OWNED
// cleanup action. It never invokes original x86 function pointers, never
// schedules original game destructors, and cannot publish an engine object.
// Concurrency: the original 2006 initializers are not synchronized; this
// host model is likewise only valid on a serialized construction thread.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_client_world_lazy_static_phase1727 {

inline constexpr std::size_t kWordCount = 9u;
inline constexpr std::uint32_t kSourceCRTRegistrationVA = 0x24248788u;
inline constexpr std::uint32_t kSourceWorldConstructorVA = 0x240CCE30u;

enum class Family : std::uint8_t { First, Second };
struct SourceIdentity final {
    Family family;
    std::uint32_t sourceInitializerVA;
    std::uint32_t sourceGlobalFirstWordVA;
    std::uint32_t sourceGuardVA;
    std::uint32_t sourceCleanupThunkVA;
    std::uint32_t sourceCleanupNestedCallVA;
    std::uint32_t sourceCleanupTailVA;
};
inline constexpr std::array<SourceIdentity, 2> kIdentities{{
    {Family::First, 0x24061540u, 0x24324698u, 0x243246BCu,
     0x24271DA0u, 0x240608F0u, 0x240602B0u},
    {Family::Second,0x240CB2F0u, 0x24338274u, 0x24338298u,
     0x24272B10u, 0x240C7050u, 0x240C5FA0u},
}};
inline constexpr const SourceIdentity& Identity(Family family) noexcept {
    return kIdentities[family == Family::First ? 0u : 1u];
}

// Synthetic owned storage. The nine DWORDs are contiguous, as in 2006;
// the extra safety flag is HOST-only and not part of the original layout.
struct HostOwnedStatic final {
    std::array<std::uint32_t, kWordCount> words{};
    std::uint32_t nativeModeledGuard = 0u;
};
static_assert(sizeof(std::uint32_t) == 4u, "2006 DWORD size mismatch");
static_assert(offsetof(HostOwnedStatic, nativeModeledGuard) ==
              kWordCount * sizeof(std::uint32_t),
              "modeled 9-DWORD data and guard must remain contiguous");

enum class ResultCode : std::uint8_t {
    InitializedAndRegistered,
    AlreadyInitialized,
    RefusedNoHostRegistrar
};
struct Result final {
    ResultCode code;
    const SourceIdentity* sourceIdentity;
    HostOwnedStatic* hostStorage;
};
// Callback sees an ORIGINAL source VA solely as an opaque identity.
// It must only register a host-owned cleanup, not try executing that VA.
using HostCleanupRegistrar = void (*)(std::uint32_t sourceThunkLabel,
                                      void* userdata) noexcept;
inline Result InitOnce(Family family, HostOwnedStatic& storage,
                       HostCleanupRegistrar registrar,
                       void* userdata = nullptr) noexcept {
    const auto& id=Identity(family);
    if ((storage.nativeModeledGuard & 1u) != 0u)
        return {ResultCode::AlreadyInitialized, &id, &storage};
    // Safer than the 2006 native entrypoint: do not leave host globals
    // marked initialized without a way to register a host cleanup.
    if (registrar == nullptr)
        return {ResultCode::RefusedNoHostRegistrar, &id, nullptr};
    storage.nativeModeledGuard |= 1u; // native sets bit BEFORE clearing
    for (auto& word : storage.words) word=0u;
    registrar(id.sourceCleanupThunkVA, userdata);
    return {ResultCode::InitializedAndRegistered, &id, &storage};
}
inline constexpr bool CanPublishRetailWorld() noexcept { return false; }
inline constexpr bool CanExecuteOriginalCleanup() noexcept { return false; }
static_assert(kIdentities[0].sourceGlobalFirstWordVA+4u*kWordCount ==
              kIdentities[0].sourceGuardVA);
static_assert(kIdentities[1].sourceGlobalFirstWordVA+4u*kWordCount ==
              kIdentities[1].sourceGuardVA);
static_assert(kIdentities[0].sourceInitializerVA !=
              kIdentities[1].sourceInitializerVA);
static_assert(!CanPublishRetailWorld() && !CanExecuteOriginalCleanup());
} // namespace sin_compat::original_client_world_lazy_static_phase1727
