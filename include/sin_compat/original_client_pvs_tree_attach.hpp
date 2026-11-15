#pragma once
// Phase239 independent C++17 host reference for the original SiN Episodes
// (2006) client PVS map tree attachment method at 0x241076E0.
// This does NOT reconstruct the called original tree balancer at 0x24106E30,
// the original engine-owned C++ map, or a standalone playable game DLL.
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "sin_compat/original_client_pvs_map_search_index.hpp"

namespace sin_compat::original_client_pvs_tree_attach {

namespace map = sin_compat::original_client_pvs_map_frontends;
namespace index = sin_compat::original_client_pvs_map_search_index;

inline constexpr std::uint32_t kOriginalAttachVA = 0x241076E0u;
inline constexpr std::uint32_t kOpaqueOriginalBalancerVA = 0x24106E30u;
inline constexpr std::size_t kWholeOriginalMethodBytes = 106u;
inline constexpr std::size_t kOriginalNativeRET12Paths = 3u;
inline constexpr std::size_t kNodeStride = 16u;
inline constexpr std::size_t kNodeLeftWORD = 0u;
inline constexpr std::size_t kNodeRightWORD = 2u;
inline constexpr std::size_t kNodeParentWORD = 4u;
inline constexpr std::size_t kNodeStateWORD = 6u;
inline constexpr std::size_t kMapStorageDWORD = 4u;
inline constexpr std::size_t kMapRootWORD = 0x10u;

// Native method invokes the same balancing method once on each path:
// empty tree, new left child and new right child. The balancer may rotate or
// rewrite the tree, so its behavior belongs exclusively to an injected hook.
struct BalancingBackend {
    virtual ~BalancingBackend() = default;
    virtual void balance_after_attach(map::Map& manager,
                                      std::uint16_t index) = 0;
};

// Host representation of thiscall void map_attach(new_index,parent,side).
// Native: initialize record +4 WORD parent, +2/+0 WORD empty children,
// +6 WORD zero; write map.root or parent.left/right; call balancer.
// Caller (Phase236 insert) copies the eight-byte payload AFTER this returns.
struct NativeTreeAttach final : index::TreeAttachCallback {
    BalancingBackend& balancing;
    explicit NativeTreeAttach(BalancingBackend& backend):balancing(backend){}

    void attach(map::Map& manager, std::uint16_t new_index,
                map::Placement placement) override {
        if(new_index == map::kAbsent)
            throw std::out_of_range("invalid native PVS tree node sentinel");
        auto& record = map::checked(manager,new_index);
        if(placement.parent != map::kAbsent) {
            if(placement.parent == new_index)
                throw std::out_of_range("synthetic PVS tree self-parent");
            (void)map::checked(manager,placement.parent);
        }

        record.left = map::kAbsent;
        record.right = map::kAbsent;
        // A Record has parent and state/color packed at native +4/+6.
        // High WORD must be zero. Original x86 writes exactly these WORDs.
        record.opaque_header = static_cast<std::uint32_t>(placement.parent);

        if(placement.parent == map::kAbsent) {
            manager.root = new_index;
        } else {
            auto& parent = map::checked(manager,placement.parent);
            if(placement.as_left)
                parent.left = new_index;
            else
                parent.right = new_index;
        }
        balancing.balance_after_attach(manager,new_index);
    }
};

} // namespace sin_compat::original_client_pvs_tree_attach
