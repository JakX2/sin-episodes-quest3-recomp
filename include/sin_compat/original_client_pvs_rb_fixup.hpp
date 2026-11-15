#pragma once
// Phase240: source-grounded independent C++17 host reference for the
// original SiN Episodes (2006) client PVS red-black tree rotation and
// insertion fixup methods. No original copyrighted x86 bodies are included.
// This is NOT a replacement game DLL or a byte-for-byte compiler recreation.
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "sin_compat/original_client_pvs_tree_attach.hpp"

namespace sin_compat::original_client_pvs_rb_fixup {

namespace map = sin_compat::original_client_pvs_map_frontends;
namespace attach = sin_compat::original_client_pvs_tree_attach;
inline constexpr std::uint32_t kOriginalRotateLeftVA=0x241067C0u;
inline constexpr std::uint32_t kOriginalRotateRightVA=0x24106D40u;
inline constexpr std::uint32_t kOriginalInsertFixupVA=0x24106E30u;
inline constexpr std::size_t kOriginalReviewedMethods=3u;
inline constexpr std::size_t kOriginalNativeBytes=1947u;
inline constexpr std::uint16_t kNil=0xFFFFu;
inline constexpr std::uint16_t kRed=0u;
inline constexpr std::uint16_t kBlack=1u;
inline constexpr std::size_t kRootOffset=0x10u;
inline constexpr std::size_t kNodeStride=16u;
inline constexpr std::size_t kParentOffset=4u;
inline constexpr std::size_t kColorOffset=6u;

// Native 16-byte node header:
// left WORD +0, right WORD +2, parent WORD +4, color WORD +6.
// The Phase236 host packs parent+color into Record::opaque_header.
inline std::uint16_t parent(const map::Map& tree,std::uint16_t id) {
    return id==kNil?kNil:static_cast<std::uint16_t>(
        map::checked(tree,id).opaque_header&0xFFFFu);
}
inline std::uint16_t color(const map::Map& tree,std::uint16_t id) {
    // Original global sentinel's color WORD+6 is 1.
    return id==kNil?kBlack:static_cast<std::uint16_t>(
        map::checked(tree,id).opaque_header>>16);
}
inline void set_parent(map::Map& tree,std::uint16_t id,std::uint16_t next) {
    if(id==kNil)throw std::out_of_range("PVS red-black nil cannot receive parent");
    if(next!=kNil)(void)map::checked(tree,next);
    auto& rec=map::checked(tree,id);
    rec.opaque_header=(rec.opaque_header&0xFFFF0000u)|next;
}
inline void set_color(map::Map& tree,std::uint16_t id,std::uint16_t next) {
    if(id==kNil) {
        if(next!=kBlack)
            throw std::out_of_range("PVS global nil must remain black");
        return;
    }
    if(next!=kRed && next!=kBlack)
        throw std::out_of_range("invalid PVS red-black node color");
    auto& rec=map::checked(tree,id);
    rec.opaque_header=(rec.opaque_header&0x0000FFFFu)|
                      (static_cast<std::uint32_t>(next)<<16);
}
inline bool red(const map::Map& tree,std::uint16_t id) {
    return color(tree,id)==kRed;
}

// Whole-native-SHA-reviewed original 0x241067C0:
// rotate left at node x. Its right child y moves to x's position;
// y's left subtree becomes x's right subtree; y.left becomes x.
// Parent links, child links and map root are all repaired.
inline void rotate_left(map::Map& tree,std::uint16_t x) {
    if(x==kNil)throw std::out_of_range("rotate_left called on nil");
    const auto y=map::checked(tree,x).right;
    if(y==kNil||y==x)
        throw std::out_of_range("rotate_left missing valid right pivot");
    const auto beta=map::checked(tree,y).left;
    const auto up=parent(tree,x);
    if(beta!=kNil)(void)map::checked(tree,beta);
    if(up!=kNil) {
        const auto& p=map::checked(tree,up);
        if(p.left!=x&&p.right!=x)
            throw std::logic_error("rotate_left broken parent link");
    }
    map::checked(tree,x).right=beta;
    if(beta!=kNil)set_parent(tree,beta,x);
    set_parent(tree,y,up);
    if(up==kNil)tree.root=y;
    else if(map::checked(tree,up).left==x)
        map::checked(tree,up).left=y;
    else map::checked(tree,up).right=y;
    map::checked(tree,y).left=x;
    set_parent(tree,x,y);
}

// Whole-native-SHA-reviewed original 0x24106D40:
// mirror rotation around node y, using its left child x as the pivot.
inline void rotate_right(map::Map& tree,std::uint16_t y) {
    if(y==kNil)throw std::out_of_range("rotate_right called on nil");
    const auto x=map::checked(tree,y).left;
    if(x==kNil||x==y)
        throw std::out_of_range("rotate_right missing valid left pivot");
    const auto beta=map::checked(tree,x).right;
    const auto up=parent(tree,y);
    if(beta!=kNil)(void)map::checked(tree,beta);
    if(up!=kNil) {
        const auto& p=map::checked(tree,up);
        if(p.left!=y&&p.right!=y)
            throw std::logic_error("rotate_right broken parent link");
    }
    map::checked(tree,y).left=beta;
    if(beta!=kNil)set_parent(tree,beta,y);
    set_parent(tree,x,up);
    if(up==kNil)tree.root=x;
    else if(map::checked(tree,up).right==y)
        map::checked(tree,up).right=x;
    else map::checked(tree,up).left=x;
    map::checked(tree,x).right=y;
    set_parent(tree,y,x);
}

// The original full method 0x24106E30 executes a red-black insertion
// repair: red parent + red uncle -> recolor; inside child -> first rotate
// parent; outside child -> recolor parent/grandparent and rotate grandparent.
// The original native code inlines many lazy-global-sentinel checks; host
// explicitly represents the sentinel as nil/black, with safety guards.
struct NativeRedBlackFixup final:attach::BalancingBackend {
    std::size_t left_rotations{};
    std::size_t right_rotations{};
    std::size_t recolor_steps{};
    std::size_t callbacks{};

    void balance_after_attach(map::Map& tree,std::uint16_t inserted) override {
        ++callbacks;
        if(inserted==kNil)
            throw std::out_of_range("PVS red-black insert nil node");
        (void)map::checked(tree,inserted);
        if(tree.root==kNil)
            throw std::logic_error("PVS red-black fixup missing root");
        std::uint16_t z=inserted;
        std::size_t steps=0;
        while(z!=tree.root && red(tree,parent(tree,z))) {
            // Guard against cycles/malformed parent structures. Not present
            // in the original 2006 retail native implementation.
            if(++steps>tree.nodes.size()*2u+1u)
                throw std::runtime_error("cyclic PVS red-black ancestry");
            const auto p=parent(tree,z);
            const auto g=parent(tree,p);
            if(g==kNil)
                throw std::logic_error("red PVS tree parent has no grandparent");
            if(map::checked(tree,g).left==p) {
                const auto uncle=map::checked(tree,g).right;
                if(red(tree,uncle)) {
                    set_color(tree,p,kBlack);
                    set_color(tree,uncle,kBlack);
                    set_color(tree,g,kRed);
                    ++recolor_steps;
                    z=g;
                } else {
                    if(z==map::checked(tree,p).right) {
                        z=p;
                        rotate_left(tree,z);
                        ++left_rotations;
                    }
                    const auto new_parent=parent(tree,z);
                    const auto new_grandparent=parent(tree,new_parent);
                    set_color(tree,new_parent,kBlack);
                    set_color(tree,new_grandparent,kRed);
                    rotate_right(tree,new_grandparent);
                    ++right_rotations;
                }
            } else if(map::checked(tree,g).right==p) {
                const auto uncle=map::checked(tree,g).left;
                if(red(tree,uncle)) {
                    set_color(tree,p,kBlack);
                    set_color(tree,uncle,kBlack);
                    set_color(tree,g,kRed);
                    ++recolor_steps;
                    z=g;
                } else {
                    if(z==map::checked(tree,p).left) {
                        z=p;
                        rotate_right(tree,z);
                        ++right_rotations;
                    }
                    const auto new_parent=parent(tree,z);
                    const auto new_grandparent=parent(tree,new_parent);
                    set_color(tree,new_parent,kBlack);
                    set_color(tree,new_grandparent,kRed);
                    rotate_left(tree,new_grandparent);
                    ++left_rotations;
                }
            } else {
                throw std::logic_error("PVS red-black grandparent link corrupted");
            }
        }
        set_color(tree,tree.root,kBlack);
    }
};

} // namespace sin_compat::original_client_pvs_rb_fixup
