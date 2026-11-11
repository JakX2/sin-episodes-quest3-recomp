#pragma once
// Phase180: original 2006 client.dll app-system initialization/cleanup and
// VPhysics030 / VPhysicsSurfaceProps001 / VPhysicsCollision007 negotiation.
// Portable observational reconstruction, NOT original MSVC32 ABI or live game.
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace sin_compat::original_client_postinit {

inline constexpr std::uint32_t kAppSystemHeadVA = 0x24369374u;
inline constexpr std::uint32_t kAppSystemCleanupVA = 0x241640F0u;
inline constexpr std::uint32_t kAppSystemInitializeVA = 0x24164120u;
inline constexpr std::uint32_t kPhysicsInitializeVA = 0x24182C60u;
inline constexpr std::uint32_t kPhysicsContextVA = 0x2433F6BCu;
inline constexpr std::uint32_t kPhysicsVA = 0x24374398u;
inline constexpr std::uint32_t kPhysicsSurfacePropsVA = 0x243743A8u;
inline constexpr std::uint32_t kPhysicsCollisionVA = 0x243743A0u;
inline constexpr std::uint32_t kPhysicsCouplingCallbackVA = 0x241927A0u;

// Original linked-list node: +0 points to an optional output pointer, +4
// creator callback, +8 release callback, +12 successor. The release
// callback is called with the current output *value* (not its address).
struct NativeAppSystemNode32 {
    std::uint32_t output_pointer_va;
    std::uint32_t creator_va;
    std::uint32_t cleanup_va;
    std::uint32_t next_va;
};
static_assert(sizeof(NativeAppSystemNode32) == 16, "native app node is four DWORDs");
static_assert(std::is_standard_layout<NativeAppSystemNode32>::value, "plain native node layout");

// Host representation of original node ordering. Native factory function
// pointers are intentionally host callbacks rather than dereferenced VAs.
struct AppNode {
    std::uintptr_t* output = nullptr;
    AppNode* next = nullptr;
};

// Original client 0x241640F0: unconditionally visit every linked node;
// if output address != null, invoke its cleanup callback only for nonzero
// *output, then write zero even if it was already zero. Null outputs skip.
// Host: void cleanup(AppNode&, uintptr_t).
template<class Host>
void original_cleanup_app_systems(AppNode* head, Host& host) {
    for (AppNode* node = head; node != nullptr; node = node->next) {
        if (!node->output) continue;
        const std::uintptr_t current = *node->output;
        if (current != 0) host.cleanup(*node, current);
        *node->output = 0;
    }
}

// Original client 0x24164120: successful factory return is assigned to
// *output; zero factory return is a FATAL error only if output is nonnull.
// On fatal error, original calls 0x241640F0 and returns false. On an
// optional entry (null output) with a null return it simply continues.
// Original does not guard nonzero success + null output: it would write
// through null. We explicitly reject this unsafe precondition on host.
// Host: uintptr_t create(AppNode&), void cleanup(AppNode&,uintptr_t).
template<class Host>
bool original_initialize_app_systems(AppNode* head, Host& host) {
    for (AppNode* node = head; node != nullptr; node = node->next) {
        const std::uintptr_t created = host.create(*node);
        if (created != 0) {
            if (!node->output)
                throw std::invalid_argument("native successful optional app entry has no output storage");
            *node->output = created;
        } else if (node->output) {
            original_cleanup_app_systems(head,host);
            return false;
        }
    }
    return true;
}

struct PhysicsLookup {
    const char* interface_version;
    std::uint32_t name_va;
    std::uint32_t result_global_va;
};
inline constexpr PhysicsLookup kPhysicsLookups[] = {
    {"VPhysics030", 0x242A2338u, kPhysicsVA},
    {"VPhysicsSurfaceProps001", 0x242A2320u, kPhysicsSurfacePropsVA},
    {"VPhysicsCollision007", 0x242A2308u, kPhysicsCollisionVA},
};
static_assert(sizeof(kPhysicsLookups) / sizeof(kPhysicsLookups[0]) == 3,
              "original client physics prefix is three queries");

// Original client 0x24182C60: three mandatory queries, each stored
// BEFORE testing null; only after all three succeed call 0x241927A0
// with raw SurfaceProps pointer followed by the existing +0x2433F6BC
// global input. This call's internal behavior is not yet reconstructed.
// Host: uintptr_t query(const char*); void publish(uint32_t,uintptr_t);
//       void after_all(const uintptr_t surface,uintptr_t existingGlobal);
template<class Host>
bool original_initialize_physics(Host& host, std::uintptr_t existing_global) {
    std::uintptr_t surface = 0;
    for (const PhysicsLookup& request : kPhysicsLookups) {
        const std::uintptr_t resolved = host.query(request.interface_version);
        host.publish(request.result_global_va,resolved);
        if (!resolved) return false;
        if (request.result_global_va == kPhysicsSurfacePropsVA) surface=resolved;
    }
    host.after_all(surface,existing_global);
    return true;
}
} // namespace sin_compat::original_client_postinit