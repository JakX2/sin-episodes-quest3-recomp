#pragma once
// Phase168: original 2006 server.dll ScatterGun flak spawn + origin setter.
// Branch and observable callback order follow original x86 functions
// 0x2231B9F0 and 0x22141A00. Host objects are NOT original Source/MSVC ABI.
#include <cstdint>
#include <cstring>

namespace sin_compat::original_server_flak_origin {

inline constexpr const char* kNativeProjectileClassname = "flak";
inline constexpr std::uint32_t kNativeClassnameVA = 0x2247A1D8u;
inline constexpr std::uint32_t kFlakBindingUnassigned = 0xffffffffu;

struct RawVector3 {
    float xyz[3]{};
};
inline bool native_float_vector_equal(const RawVector3& a,
                                      const RawVector3& b) noexcept {
    // Native x87 FUCOMPP: signed zero is equal; NaN is unordered/changed.
    return a.xyz[0] == b.xyz[0] && a.xyz[1] == b.xyz[1] &&
           a.xyz[2] == b.xyz[2];
}
inline void copy_native_vector(RawVector3& out,
                               const RawVector3& in) noexcept {
    // Original copies all three raw DWORD payloads, preserving -0 and NaNs.
    std::memcpy(&out, &in, sizeof(RawVector3));
}

// Flak helper 0x2231B9F0. Exact original external operations:
// CreateEntityByName-equivalent 0x22266B00("flak", -1), 0x22285AB0,
// optional source virtual +0xFC (called twice), 0x222AA640 and
// optional returned record virtual +0x8; created-object virtual +0x50,
// 0x221439C0 and created-object virtual +0x64.
// Native code assumes entity creation succeeds; this model also requires
// a non-null create_flak return (no fabricated original null branch).
struct FlakSpawnRecord {
    std::uint32_t binding_794 = kFlakBindingUnassigned;
};

template<class Host>
FlakSpawnRecord* original_spawn_flak(
    const RawVector3& first_arg, const RawVector3& second_arg,
    std::uintptr_t optional_source, Host& host) {
    FlakSpawnRecord* flak = host.create_flak(kNativeProjectileClassname,
                                           0xffffffffu); // 0x22266B00
    host.initialize_flak(flak, first_arg, 0u); // 0x22285AB0
    if (optional_source != 0u) {
        const std::uintptr_t first =
            host.source_virtual_fc(optional_source);
        if (first != 0u) {
            const std::uintptr_t second =
                host.source_virtual_fc(optional_source);
            const std::uintptr_t binding =
                host.lookup_binding_222AA640(second);
            if (binding != 0u) {
                flak->binding_794 = host.read_binding_word_after_v8(binding);
            } else {
                flak->binding_794 = kFlakBindingUnassigned;
            }
        }
    }
    host.created_virtual_50(flak, optional_source);
    host.initialize_flak_vector_221439C0(flak, second_arg);
    host.created_virtual_64(flak);
    return flak;
}

struct ParentSnapshot {
    RawVector3 world_origin_250{};
    std::uint32_t flags_b8 = 0;
};

struct OriginState {
    RawVector3 world_origin_250{};
    RawVector3 local_origin_2c8{}; // actual field assignment through host
    std::uint8_t flags_b9 = 0;
    std::uint32_t parent_handle_f0 = kFlakBindingUnassigned;
};

enum class OriginOutcome : std::uint8_t {
    Unchanged,
    ChangedNoValidParent,
    ChangedWithParent,
};

// Original server 0x22141A00: three x87 compare pairs are independently
// evaluated until a component differs/unorders. A valid parent handle is
// index bits 0..11 + serial bits 12..31, resolved by the host using the
// native entity-handle registry; no owner-pointer inference is made.
template<class Host>
OriginOutcome original_set_world_origin(
    OriginState& self, const RawVector3& proposed, Host& host) {
    if (native_float_vector_equal(self.world_origin_250, proposed))
        return OriginOutcome::Unchanged;
    host.notify_position_change(self, 4u); // 0x2214B620
    self.flags_b9 = static_cast<std::uint8_t>(
        self.flags_b9 & static_cast<std::uint8_t>(~0x10u));
    copy_native_vector(self.world_origin_250, proposed);

    ParentSnapshot* parent = self.parent_handle_f0 == kFlakBindingUnassigned\n        ? nullptr : host.resolve_parent(self.parent_handle_f0);
    if (parent == nullptr) {
        host.set_local_origin(self, proposed); // 0x2213E1A0
        return OriginOutcome::ChangedNoValidParent;
    }

    // Parent flag bit12 first refresh; then subtract parent's *updated*
    // world coordinates before checking flag bit11 and matrix transform.
    if ((parent->flags_b8 & 0x1000u) != 0u)
        host.parent_refresh_bit12(*parent); // 0x2213D750
    RawVector3 delta{};
    for (unsigned i=0; i<3; ++i)
        delta.xyz[i] = proposed.xyz[i] - parent->world_origin_250.xyz[i];
    if ((parent->flags_b8 & 0x800u) != 0u)
        host.parent_refresh_bit11(*parent); // 0x2213FA60
    RawVector3 local =
        host.transform_with_parent_matrix(*parent, delta); // 0x22268E90
    host.set_local_origin(self, local); // 0x2213E1A0
    return OriginOutcome::ChangedWithParent;
}

} // namespace sin_compat::original_server_flak_origin
