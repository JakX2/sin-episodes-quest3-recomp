#pragma once
// Independent C++17 model of the authentic original 2006 SiN client
// string-token lifecycle. No original binary code, no native host pointers.
#include "sin_compat/original_client_string_token_tree.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace sin_compat::original_client_string_tokens::lifecycle {

inline constexpr std::uint32_t kGlobalTokenObjectNativeBytes=0x34;
inline constexpr std::uint16_t kGlobalTokenInitialNodes=32;
inline constexpr std::uint32_t kGlobalNodeArrayNativeBytes=0x180;
static_assert(kGlobalTokenInitialNodes*sizeof(Node)==kGlobalNodeArrayNativeBytes);

enum class NativeTableDefault : std::uint8_t {
    // Original global `0x24377044` -> constructor `0x241D47B0`
    GlobalTokenCaseSensitive,
    // Distinct Phase546 global converter registry `0x2432289C`
    HudConverterCaseFolded
};

// Original 0x241D3F30 overwrites only node children: smaller=index,
// larger=next-free. Parent/color/payload retain their previous values.
// Root becomes 0xFFFF, activeCount zero, freeHead=0 unless highwater=0.
struct ResetState {
    std::uint16_t root=kMissing;
    std::uint16_t active_count=0;
    std::uint16_t free_head=kMissing;
    std::uint16_t highwater_count=0;
};

// Rejecting a missing backing record is independent host safety, not an
// observed original x86 exception/return behavior.
inline bool rebuild_original_free_chain(std::vector<Node>& nodes,
                                        std::uint16_t highwater,
                                        ResetState& result) noexcept {
    if(highwater>nodes.size() || highwater==kMissing)return false;
    std::uint16_t next=kMissing;
    for(std::uint32_t i=highwater;i-->0;){
        auto& n=nodes[i];
        n.smaller=static_cast<std::uint16_t>(i);
        n.larger=next;
        next=static_cast<std::uint16_t>(i);
    }
    result.root=kMissing;
    result.active_count=0;
    result.free_head=next;
    result.highwater_count=highwater;
    return true;
}

struct NativeTokenDescriptor {
    std::uint16_t page_index=0;
    std::uint16_t byte_offset=0;
};
static_assert(sizeof(NativeTokenDescriptor)==4);

// Original 0x241D3D50: token 0xffff returns a *static empty C-string*.
// For other indices, uint16 page/offset comes from token-node payload +8;
// pagePointer[page_index] + 8 + byte_offset is the native string address.
// Here the page vectors are synthetic and bounded for host safety.
inline std::optional<std::string_view> string_from_token(
    const std::vector<Node>& records,
    const std::vector<std::vector<std::uint8_t>>& pages,
    std::uint16_t token) noexcept {
    if(token==kMissing)return std::string_view{};
    if(token>=records.size())return std::nullopt;
    const auto descriptor=records[token].payload_native32;
    const auto page=static_cast<std::uint16_t>(descriptor & 0xFFFF);
    const auto offset=static_cast<std::uint16_t>((descriptor>>16)&0xFFFF);
    if(page>=pages.size())return std::nullopt;
    const auto& data=pages[page];
    const std::size_t begin=static_cast<std::size_t>(8)+offset;
    if(begin>data.size())return std::nullopt;
    std::size_t end=begin;
    while(end<data.size() && data[end]!=0)++end;
    if(end==data.size())return std::nullopt;
    return std::string_view(reinterpret_cast<const char*>(data.data()+begin),end-begin);
}

} // namespace sin_compat::original_client_string_tokens::lifecycle