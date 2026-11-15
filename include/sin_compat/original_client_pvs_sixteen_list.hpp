#pragma once
// Phase233 original 2006 SiN client.dll PVS 16-byte active-list methods:
//   0x241086F0 append after 0x24106610 allocator and 0x24106CB0 unlink;
//   0x24106CB0 remove a 16-byte linked-list index, native RET 4.
// Pure clean-room C++17 host state, not an original game object or loader.
// The separate 8-byte auxiliary list was reviewed in Phase232.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace sin_compat::original_client_pvs_sixteen_list {
inline constexpr std::uint32_t kOriginalAppendVA=0x241086F0u;
inline constexpr std::uint32_t kOriginalUnlinkVA=0x24106CB0u;
inline constexpr std::uint32_t kOriginalReserveVA=0x24106610u;
inline constexpr std::size_t kOriginalReviewedMethods=2u;
inline constexpr std::size_t kOriginalReviewedBytes=236u;
inline constexpr std::size_t kNativeEntryStride=16u;
inline constexpr std::uint16_t kAbsent=0xFFFFu;
inline constexpr std::size_t kNativePrevWord=0x0Cu;
inline constexpr std::size_t kNativeNextWord=0x0Eu;
inline constexpr std::size_t kManagerHeadWord=0x0Cu;
inline constexpr std::size_t kManagerTailWord=0x0Eu;
inline constexpr std::size_t kManagerCountWord=0x12u;
inline constexpr std::size_t kManagerCapacityWord=0x14u;

struct Node {
    // Original PVS notifier and map helpers own the FIRST twelve bytes.
    // These two native list functions only touch link words at +C/+E.
    std::array<std::uint8_t,12> opaque_payload{};
    std::uint16_t prev{kAbsent};
    std::uint16_t next{kAbsent};
};
struct List {
    std::vector<Node> nodes;
    std::uint16_t head{kAbsent};
    std::uint16_t tail{kAbsent};
    std::uint16_t count{};
    explicit List(std::size_t capacity):nodes(capacity) {
        if(capacity>kAbsent)
            throw std::out_of_range("16-bit PVS list capacity exceeds original index range");
        // Synthetic manager initialization; native ctor allocator
        // 0x24106610 remains outside Phase233 whole-body review.
        for(std::size_t i=0;i<capacity;i++){
            nodes[i].prev=static_cast<std::uint16_t>(i);
            nodes[i].next=static_cast<std::uint16_t>(i);
        }
    }
};
inline Node& checked_node(List& list,std::uint16_t index) {
    if(index>=list.nodes.size())
        throw std::out_of_range("host refused out-of-bounds original 16-byte list index");
    return list.nodes[index];
}
inline const Node& checked_node(const List& list,std::uint16_t index) {
    if(index>=list.nodes.size())
        throw std::out_of_range("host refused out-of-bounds original 16-byte list index");
    return list.nodes[index];
}
// Native 0x24106CB0, thiscall + RET 4. Return is host-only status.
// If index>=capacity or node.prev==index, return without modification.
// Else splice prev/next, update manager head/tail, mark both link
// words equal to removed index and decrement 16-bit count.
inline bool unlink(List& list,std::uint16_t index) {
    if(index>=list.nodes.size())return false;
    const auto old=checked_node(list,index);
    if(old.prev==index)return false;
    if(old.prev!=kAbsent)(void)checked_node(list,old.prev);
    if(old.next!=kAbsent)(void)checked_node(list,old.next);
    if(old.prev!=kAbsent)
        checked_node(list,old.prev).next=old.next;
    else
        list.head=old.next;
    if(old.next!=kAbsent)
        checked_node(list,old.next).prev=old.prev;
    else
        list.tail=old.prev;
    checked_node(list,index).prev=index;
    checked_node(list,index).next=index;
    list.count=static_cast<std::uint16_t>(list.count-1u);
    return true;
}
struct IIndexAllocator {
    virtual ~IIndexAllocator()=default;
    // Original call 0x241086F6 -> 0x24106610, with PUSH 0.
    // The original allocator's complete behavior is NOT recovered here.
    virtual std::uint16_t reserve_zero()=0;
};
// Original 0x241086F0: acquire slot, call unlink(index)
// unconditionally, append to tail, ++manager WORD count, return low
// 16 bits of index. Does not write the first 12 record bytes.
inline std::uint16_t append(List& list,IIndexAllocator& allocator) {
    const auto index=allocator.reserve_zero();
    (void)checked_node(list,index);  // host safety before mutation
    (void)unlink(list,index);
    const auto old_tail=list.tail;
    if(old_tail!=kAbsent)(void)checked_node(list,old_tail);
    auto& entry=checked_node(list,index);
    entry.next=kAbsent;
    entry.prev=old_tail;
    list.tail=index;
    if(old_tail==kAbsent)
        list.head=index;
    else
        checked_node(list,old_tail).next=index;
    list.count=static_cast<std::uint16_t>(list.count+1u);
    return index;
}
} // namespace sin_compat::original_client_pvs_sixteen_list
