#pragma once
// Phase242 independent C++17 host reference for the original 2006 SiN
// client PVS red-black tree post-delete balancing (native 0x24107750).
// No proprietary original native code is included. This is a source-grounded
// behavioral model, not the original engine-owned C++ map or gameplay DLL.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include "sin_compat/original_client_pvs_tree_erase.hpp"

namespace sin_compat::original_client_pvs_delete_fixup {

namespace map=sin_compat::original_client_pvs_map_frontends;
namespace rb=sin_compat::original_client_pvs_rb_fixup;
namespace erase=sin_compat::original_client_pvs_tree_erase;

inline constexpr std::uint32_t kOriginalFixupVA=0x24107750u;
inline constexpr std::size_t kOriginalWholeNativeCodeBytes=2609u;
inline constexpr std::uint32_t kNativeRotateLeftVA=0x241067C0u;
inline constexpr std::uint32_t kNativeRotateRightVA=0x24106D40u;
inline constexpr std::size_t kNodeStride=16u;
inline constexpr std::uint16_t kNil=0xFFFFu;
inline constexpr std::uint16_t kRed=0u;
inline constexpr std::uint16_t kBlack=1u;
inline constexpr std::size_t kRootWORD=0x10u;
inline constexpr std::size_t kParentWORD=4u;
inline constexpr std::size_t kColorWORD=6u;

// Original global nil sentinel has absent children and black state. The
// synthetic host substitutes this typed missing-index view; it does not
// recreate the retail DLL's lazy process-global initialization code.
inline std::uint16_t left_of(const map::Map& tree,std::uint16_t id) {
    return id==kNil?kNil:map::checked(tree,id).left;
}
inline std::uint16_t right_of(const map::Map& tree,std::uint16_t id) {
    return id==kNil?kNil:map::checked(tree,id).right;
}

struct NativeDeleteFixup final:erase::DeleteBalancingBackend {
    std::size_t callbacks{};
    std::size_t left_rotations{};
    std::size_t right_rotations{};
    std::size_t red_sibling_cases{};
    std::size_t black_nephews_cases{};
    std::size_t inner_nephew_cases{};
    std::size_t outer_nephew_cases{};

    // Original caller (Phase241 native 0x241083C0) only invokes fixup when
    // the selected replacement index is NOT nil and the physically removed
    // node was black. Keep this condition in the caller, do not fabricate
    // a second nil-fixup API in the host.
    void balance_after_erase(map::Map& tree,std::uint16_t replacement) override {
        ++callbacks;
        if(replacement==kNil)
            throw std::out_of_range("native delete fixup requires real replacement");
        (void)map::checked(tree,replacement);
        if(tree.root==kNil)
            throw std::logic_error("native delete fixup missing tree root");
        std::uint16_t current=replacement;
        std::size_t iterations=0u;
        while(current!=tree.root && rb::color(tree,current)==kBlack) {
            // Native compiler-generated code does not protect bad graphs.
            if(++iterations>tree.nodes.size()*2u+1u)
                throw std::runtime_error("cyclic synthetic PVS delete ancestry");
            const auto p=rb::parent(tree,current);
            if(p==kNil)
                throw std::logic_error("nonroot PVS delete node with nil parent");
            const auto& parent=map::checked(tree,p);
            const bool is_left=parent.left==current;
            if(!is_left && parent.right!=current)
                throw std::logic_error("PVS delete fixup parent-child mismatch");
            if(is_left) {
                auto sibling=right_of(tree,p);
                if(rb::color(tree,sibling)==kRed) {
                    rb::set_color(tree,sibling,kBlack);
                    rb::set_color(tree,p,kRed);
                    rb::rotate_left(tree,p);
                    ++left_rotations;
                    ++red_sibling_cases;
                    sibling=right_of(tree,p);
                }
                if(rb::color(tree,left_of(tree,sibling))==kBlack &&
                   rb::color(tree,right_of(tree,sibling))==kBlack) {
                    if(sibling==kNil)
                        throw std::logic_error("PVS delete fixup cannot redden global nil");
                    rb::set_color(tree,sibling,kRed);
                    ++black_nephews_cases;
                    current=p;
                } else {
                    if(rb::color(tree,right_of(tree,sibling))==kBlack) {
                        const auto inside=left_of(tree,sibling);
                        rb::set_color(tree,inside,kBlack);
                        rb::set_color(tree,sibling,kRed);
                        rb::rotate_right(tree,sibling);
                        ++right_rotations;
                        ++inner_nephew_cases;
                        sibling=right_of(tree,p);
                    }
                    rb::set_color(tree,sibling,rb::color(tree,p));
                    rb::set_color(tree,p,kBlack);
                    rb::set_color(tree,right_of(tree,sibling),kBlack);
                    rb::rotate_left(tree,p);
                    ++left_rotations;
                    ++outer_nephew_cases;
                    current=tree.root;
                }
            } else {
                auto sibling=left_of(tree,p);
                if(rb::color(tree,sibling)==kRed) {
                    rb::set_color(tree,sibling,kBlack);
                    rb::set_color(tree,p,kRed);
                    rb::rotate_right(tree,p);
                    ++right_rotations;
                    ++red_sibling_cases;
                    sibling=left_of(tree,p);
                }
                if(rb::color(tree,right_of(tree,sibling))==kBlack &&
                   rb::color(tree,left_of(tree,sibling))==kBlack) {
                    if(sibling==kNil)
                        throw std::logic_error("PVS delete fixup cannot redden global nil");
                    rb::set_color(tree,sibling,kRed);
                    ++black_nephews_cases;
                    current=p;
                } else {
                    if(rb::color(tree,left_of(tree,sibling))==kBlack) {
                        const auto inside=right_of(tree,sibling);
                        rb::set_color(tree,inside,kBlack);
                        rb::set_color(tree,sibling,kRed);
                        rb::rotate_left(tree,sibling);
                        ++left_rotations;
                        ++inner_nephew_cases;
                        sibling=left_of(tree,p);
                    }
                    rb::set_color(tree,sibling,rb::color(tree,p));
                    rb::set_color(tree,p,kBlack);
                    rb::set_color(tree,left_of(tree,sibling),kBlack);
                    rb::rotate_right(tree,p);
                    ++right_rotations;
                    ++outer_nephew_cases;
                    current=tree.root;
                }
            }
        }
        // The original exit at +0xA1C..+0xA2E writes color=black to the
        // selected/current node even on the early root branch.
        rb::set_color(tree,current,kBlack);
    }
};

} // namespace sin_compat::original_client_pvs_delete_fixup
