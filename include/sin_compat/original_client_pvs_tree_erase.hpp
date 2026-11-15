#pragma once
// Phase241 original-client PVS map tree-erase host reference (C++17).
// Authenticated original retail native entry 0x241083C0 (693 code bytes).
// The original 0x24107750 post-removal rebalancer remains opaque and is
// injected as a callback, NOT fabricated or linked from HL2/other engines.
// This class is not an engine-owned object or runnable gameplay DLL.
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "sin_compat/original_client_pvs_rb_fixup.hpp"

namespace sin_compat::original_client_pvs_tree_erase {
namespace map=sin_compat::original_client_pvs_map_frontends;
namespace rb=sin_compat::original_client_pvs_rb_fixup;

inline constexpr std::uint32_t kNativeEraseVA=0x241083C0u;
inline constexpr std::uint32_t kOpaqueNativeDeleteFixupVA=0x24107750u;
inline constexpr std::uint32_t kOriginalNodeResolverVA=0x24105F40u;
inline constexpr std::size_t kOriginalEntireMethodBytes=693u;
inline constexpr std::size_t kNodeStride=16u;
inline constexpr std::size_t kLeftWORD=0u;
inline constexpr std::size_t kRightWORD=2u;
inline constexpr std::size_t kParentWORD=4u;
inline constexpr std::size_t kColorWORD=6u;
inline constexpr std::size_t kRootWORD=0x10u;

struct DeleteBalancingBackend {
    virtual ~DeleteBalancingBackend()=default;
    // Native caller invokes 0x24107750 with the replacement index only if
    // replacement != 0xFFFF and the physically removed node was black.
    virtual void balance_after_erase(map::Map& manager,
                                     std::uint16_t replacement)=0;
};

struct NativeTreeErase final : map::DeletionBackend {
    DeleteBalancingBackend& balance;
    std::size_t erase_calls{};
    std::size_t rebalance_calls{};
    explicit NativeTreeErase(DeleteBalancingBackend& hook):balance(hook){}

    // Native 0x241083C0 removes a physical node from a red-black tree.
    // For a two-child logical target, it chooses the leftmost successor of
    // its right subtree, transplants successor into target's position,
    // and leaves the original target index free for Phase236's free-list.
    // No live_count or free_head edits here: Phase236 handles those.
    void erase_tree_node(map::Map& tree,std::uint16_t target) override {
        if(target==rb::kNil)return;
        (void)map::checked(tree,target);
        if(tree.root==rb::kNil)
            throw std::logic_error("cannot erase from empty PVS tree");
        ++erase_calls;

        const auto& target_node=map::checked(tree,target);
        std::uint16_t selected=target;
        if(target_node.left!=rb::kNil &&
           target_node.right!=rb::kNil) {
            selected=target_node.right;
            std::size_t traversals=0u;
            while(map::checked(tree,selected).left!=rb::kNil) {
                if(++traversals>tree.nodes.size())
                    throw std::runtime_error("cycle in PVS successor chain");
                selected=map::checked(tree,selected).left;
            }
        }

        const auto selected_node=map::checked(tree,selected);
        const auto chosen_color=rb::color(tree,selected);
        const auto replacement=(
            selected_node.left!=rb::kNil ?
            selected_node.left : selected_node.right
        );
        const auto selected_parent=rb::parent(tree,selected);
        if(replacement!=rb::kNil) {
            (void)map::checked(tree,replacement);
            if(replacement==selected)
                throw std::logic_error("self-child in PVS deletion");
        }
        if(selected_parent!=rb::kNil) {
            const auto& ancestor=map::checked(tree,selected_parent);
            if(ancestor.left!=selected&&ancestor.right!=selected)
                throw std::logic_error("selected PVS node not linked to parent");
        } else if(tree.root!=selected) {
            throw std::logic_error("selected PVS node claims root incorrectly");
        }

        // Transplant selected out, replacing it with its only non-nil child.
        // The native nil sentinel's parent is not rewritten.
        if(replacement!=rb::kNil)
            rb::set_parent(tree,replacement,selected_parent);
        if(selected_parent==rb::kNil)
            tree.root=replacement;
        else if(map::checked(tree,selected_parent).left==selected)
            map::checked(tree,selected_parent).left=replacement;
        else
            map::checked(tree,selected_parent).right=replacement;

        if(selected!=target) {
            // Snapshot the target AFTER the selected successor transplant.
            // This matters when selected was target.right: the right link
            // is now replacement, not the old successor itself.
            const auto target_parent=rb::parent(tree,target);
            const auto target_left=map::checked(tree,target).left;
            const auto target_right=map::checked(tree,target).right;
            const auto target_color=rb::color(tree,target);
            if(target_parent!=rb::kNil) {
                const auto& ancestor=map::checked(tree,target_parent);
                if(ancestor.left!=target&&ancestor.right!=target)
                    throw std::logic_error("target PVS node not linked to parent");
            } else if(tree.root!=target) {
                throw std::logic_error("target PVS node not at claimed root");
            }
            rb::set_parent(tree,selected,target_parent);
            auto& replacement_node=map::checked(tree,selected);
            replacement_node.left=target_left;
            replacement_node.right=target_right;
            rb::set_color(tree,selected,target_color);
            if(target_parent==rb::kNil)
                tree.root=selected;
            else if(map::checked(tree,target_parent).left==target)
                map::checked(tree,target_parent).left=selected;
            else
                map::checked(tree,target_parent).right=selected;

            if(target_left!=rb::kNil)
                rb::set_parent(tree,target_left,selected);
            if(target_right!=rb::kNil)
                rb::set_parent(tree,target_right,selected);
        }

        // Exact native 0x24108657..0x2410866D gate: replacement must be
        // non-nil and the *selected* (physically removed) node's saved
        // color WORD must equal 1 (black). Do not invent nil fixup here.
        if(replacement!=rb::kNil && chosen_color==rb::kBlack) {
            ++rebalance_calls;
            balance.balance_after_erase(tree,replacement);
        }
    }
};

} // namespace sin_compat::original_client_pvs_tree_erase
