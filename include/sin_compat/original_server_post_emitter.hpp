#pragma once
// Original SiN Episodes 2006 server.dll post-emitter startup contract.
// Host-neutral C++17 reference. This is NOT an implementation of the two
// original repository parsers, Source engine services, or a game-loadable DLL.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace sin_compat::original_server_post_emitter {
inline constexpr std::uint32_t kGameSystemsInitVA = 0x221F5700u;
inline constexpr std::uint32_t kRepositoryAThunkVA = 0x221AD8A0u;
inline constexpr std::uint32_t kRepositoryBThunkVA = 0x22149B60u;
inline constexpr std::uint32_t kRepositoryAImplVA = 0x221AD3E0u;
inline constexpr std::uint32_t kRepositoryBImplVA = 0x221498C0u;
inline constexpr std::uint32_t kRepositoryAObjectVA = 0x22503EE0u;
inline constexpr std::uint32_t kRepositoryBObjectVA = 0x224F0388u;
inline constexpr std::uint32_t kDeveloperCvarInitVA = 0x221E6450u;
inline constexpr std::uint32_t kRegistryForwardVA = 0x221F56E0u;
inline constexpr std::uint32_t kDeveloperCvarDescriptorVA = 0x22510164u;
inline constexpr std::uint32_t kDeveloperCvarResultVA = 0x22571C58u;
inline constexpr std::uint32_t kSystemPendingHeadVA = 0x22572CC0u;
inline constexpr std::uint32_t kSystemInitFlagVA = 0x22572CB8u;
inline constexpr std::uint32_t kSystemArrayVA = 0x22510A58u;
inline constexpr std::uint32_t kSystemCountVA = 0x22510A64u;
inline constexpr std::uint32_t kSystemCapacityVA = 0x22510A5Cu;
inline constexpr const char* kDeveloperName = "developer";
inline constexpr const char* kGameEventsPath = "resource/gameevents.res";
inline constexpr const char* kModEventsPath = "resource/ModEvents.res";

struct SystemNode {
    // Identity is original address equality, never display-name equality.
    std::uintptr_t identity{};
    SystemNode* next{};
};
struct Registry {
    SystemNode* pending{};
    std::vector<SystemNode*> installed{};
    bool init_flag{};
};

// Native 0x221F5700 consumes the pending linked list, skips already-recorded
// POINTERS while emitting a diagnostic, then clears the original head and
// writes init_flag=1 BEFORE calling each registered system's vtable[0].
// A failed callback returns false with the cleared head/flag preserved and
// earlier callbacks already run. No native rollback is observed.
//
// Host callbacks:
// void duplicate_system(SystemNode&);
// bool init_system(SystemNode&);
// Capacity growth/memmove are intentionally managed by std::vector instead
// of pretending that the native allocator behind 0x221F51A0 is reconstructed.
template<class Host>
bool initialize_game_systems(Registry& registry, Host& host,
                             std::size_t max_pending = 65536) {
    std::size_t scanned = 0;
    for (SystemNode* p = registry.pending; p != nullptr; p = p->next) {
        if (++scanned > max_pending)
            throw std::length_error("host fixture has cyclic/excessive pending system list");
        const auto found = std::find_if(registry.installed.begin(), registry.installed.end(),
            [p](const SystemNode* existing) { return existing->identity == p->identity; });
        if (found == registry.installed.end()) registry.installed.push_back(p);
        else host.duplicate_system(*p);
    }
    registry.pending = nullptr;
    registry.init_flag = true;
    for (SystemNode* p : registry.installed)
        if (!host.init_system(*p)) return false;
    return true;
}

// Native original CServerGameDLL::DLLInit calls these three gates IN ORDER
// at 0x221E7927 / 0x221E7930 / 0x221E7939. Every false result branches
// to the SAME 0x221E79A1 failure return. Repository impls are *opaque*.
// Host: bool init_repository_a(uint32_t object_va,uint32_t impl_va);
//       bool init_repository_b(uint32_t object_va,uint32_t impl_va);
template<class Host>
bool run_three_required_gates(Registry& registry, Host& host) {
    if (!initialize_game_systems(registry,host)) return false;
    if (!host.init_repository_a(kRepositoryAObjectVA,kRepositoryAImplVA)) return false;
    if (!host.init_repository_b(kRepositoryBObjectVA,kRepositoryBImplVA)) return false;
    return true; // Only these three gates, NOT complete server DLLInit.
}

// Original 0x221E6450, called earlier in DLLInit, performs a registration
// callback then queries VEngineCvar002 virtual offset +0x0C for "developer".
// The result is published even if zero, WITHOUT a fatal guard in this helper.
// Host: register_cvar_descriptor(uint32_t), query_cvar(const char*),
//       publish_cvar(uint32_t,std::uintptr_t).
template<class Host>
std::uintptr_t register_and_query_developer(Host& host) {
    host.register_cvar_descriptor(kDeveloperCvarDescriptorVA);
    const std::uintptr_t answer = host.query_cvar(kDeveloperName);
    host.publish_cvar(kDeveloperCvarResultVA,answer);
    return answer;
}

// 0x221F56E0 forwards the original installed-array count and an incoming
// argument address to 0x221F54F0. The opaque callee is NOT reimplemented.
// Host: void forward_registry_count(uint32_t array_va,uint32_t count_va,
//                                    uint32_t callee_va,uint32_t incoming_raw).
template<class Host>
void forward_registry_count(Host& host,std::uint32_t incoming_raw) {
    host.forward_registry_count(kSystemArrayVA,kSystemCountVA,0x221F54F0u,incoming_raw);
}
} // namespace sin_compat::original_server_post_emitter