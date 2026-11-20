#pragma once
// SiN Episodes original client.dll: SHA-authenticated 2006 stat-value
// factory at VA 0x24205BC0. It accepts type codes 0..15, constructs an
// original virtual value object, and the method at vtable offset 0x4c
// forwards to a *reader* virtual method. These are derived numeric
// metadata ONLY. Never call original addresses or cast proprietary
// CStatBufferReader structures in the independent client.
// Original client SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent::stats_value_types {
inline constexpr unsigned kTypeCount=16;
inline constexpr std::uint32_t kOriginalFactoryVA=0x24205bc0u;
inline constexpr std::uint32_t kOriginalJumpTableVA=0x24205df0u;
inline constexpr std::uint32_t kValueVirtualOffset=0x4cu;

// "config_bits" is the literal 32-bit argument pushed by the original
// factory for certain cases. Its actual interpretation must be proven
// from each constructor. Do NOT assume all 16 types are scalar floats.
// "reader_slot_offset" is a vtable byte displacement, NOT a wire bit
// count. Unknown reader virtual methods must remain opaque.
struct Case final {
    unsigned type;
    std::uint16_t allocation_bytes;
    std::uint32_t factory_case_va;
    std::uint32_t constructor_va; // 0 for inline factory construction
    std::uint32_t value_vtable_va;
    std::uint32_t value_update_va; // value_vtable_va[0x4c / 4]
    std::uint8_t reader_slot_offset;
    std::uint32_t config_bits;
};
inline constexpr std::array<Case,kTypeCount> kCases{{
    { 0,0x10,0x24205c00u,0,          0x242af384u,0x24205420u,0x24,0},
    { 1,0x10,0x24205bd9u,0,          0x242af3dcu,0x242054c0u,0x20,0},
    { 2,0x14,0x24205c47u,0x242054e0u,0x242af434u,0x24205580u,0x14,0},
    { 3,0x14,0x24205c27u,0x24205590u,0x242af48cu,0x24205600u,0x10,0},
    { 4,0x14,0x24205c67u,0x24205620u,0x242af4e4u,0x24205670u,0x0c,0},
    { 5,0x14,0x24205c87u,0x24205690u,0x242af53cu,0x242056b0u,0x0c,0},
    { 6,0x14,0x24205ca7u,0x242056c0u,0x242af594u,0x24205710u,0x08,0},
    { 7,0x14,0x24205cc7u,0x24205720u,0x242af5ecu,0x24205780u,0x1c,0},
    { 8,0x1c,0x24205ce7u,0x242057a0u,0x242af644u,0x24205820u,0x18,0x3f800000u},
    { 9,0x1c,0x24205d0cu,0x242057a0u,0x242af644u,0x24205820u,0x18,0x3c888889u},
    {10,0x1c,0x24205d31u,0x242057a0u,0x242af644u,0x24205820u,0x18,0x3991a2b4u},
    {11,0x20,0x24205d56u,0x24205840u,0x242af69cu,0x242058c0u,0x04,0x3f800000u},
    {12,0x10,0x24205d97u,0x242058d0u,0x242af6f4u,0x24205960u,0x20,0},
    {13,0x10,0x24205d7bu,0x24205980u,0x242af74cu,0x24205a10u,0x20,0},
    {14,0x14,0x24205dcfu,0x24205a30u,0x242af7a4u,0x24205960u,0x20,0},
    {15,0x14,0x24205db3u,0x24205ae0u,0x242af7fcu,0x24205a10u,0x20,0}
}};
constexpr bool IsKnown(unsigned index) noexcept {
    return index<kTypeCount;
}
constexpr const Case* TryGet(unsigned index) noexcept {
    return IsKnown(index)?&kCases[index]:nullptr;
}
constexpr bool SameReaderOperation(unsigned a,unsigned b) noexcept {
    return IsKnown(a) && IsKnown(b) &&
        kCases[a].reader_slot_offset==kCases[b].reader_slot_offset;
}
// A recovered original *indirection* chain, not a call implementation.
// Returns the owned type descriptor and reader slot to reconstruct.
// Never call or dereference value_update_va or any supplied engine ptr.
inline bool ResolveReaderSlot(unsigned type,
                              std::uint8_t& slot) noexcept {
    const auto* found=TryGet(type);
    if(!found) {
        slot=0xff;
        return false;
    }
    slot=found->reader_slot_offset;
    return true;
}
constexpr bool CheckTable() noexcept {
    for(unsigned i=0;i<kTypeCount;++i){
        const auto& item=kCases[i];
        if(item.type!=i || item.allocation_bytes<0x10 ||
           item.allocation_bytes>0x20 ||
           item.value_update_va<0x24001000u ||
           item.value_update_va>=0x24276000u ||
           item.reader_slot_offset<4u ||
           item.reader_slot_offset>0x24u ||
           (item.reader_slot_offset&3u)!=0u)
            return false;
    }
    return kCases[8].value_vtable_va==kCases[9].value_vtable_va &&
           kCases[9].value_vtable_va==kCases[10].value_vtable_va &&
           kCases[8].config_bits!=kCases[9].config_bits &&
           kCases[9].config_bits!=kCases[10].config_bits;
}
static_assert(CheckTable(),"original 16-case stat-value factory contract");
} // namespace sin_independent::stats_value_types
