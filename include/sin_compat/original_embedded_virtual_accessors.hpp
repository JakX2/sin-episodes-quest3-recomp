#pragma once
// Phase229: independent C++17 source-grounded host views of twelve small
// original HLTVDirector001 and VClientEntityList003 virtual methods.
// All offsets are relative to the EMBEDDED INTERFACE 'this', NOT parent.
// No actual original C++ engine objects, proprietary bodies, or valid
// retail process pointers are created by this reference.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace sin_compat::original_embedded_virtual_accessors {

using Bytes=std::vector<std::uint8_t>;
inline constexpr std::uint32_t kServerHltvVtable=0x22453DA0u;
inline constexpr std::uint32_t kClientEntityListVtable=0x24297390u;
inline constexpr std::uint32_t kServerHltvEmbeddedOffset=0x08u;
inline constexpr std::uint32_t kClientEntityListEmbeddedOffset=0x10014u;
inline constexpr std::uint32_t kServerGlobalAddressVA=0x2251B054u;
inline constexpr std::uint32_t kServerOriginalBase=0x22000000u;
inline constexpr std::size_t kSelectedOriginalMethodCount=12u;
inline constexpr std::size_t kOriginalReviewedX86Bytes=112u;

struct Method {
    bool server{};
    std::uint32_t slot{};
    std::uint32_t native_va{};
    std::uint32_t native_code_bytes{};
    std::uint32_t return_pop_bytes{};
};
inline constexpr Method kMethods[]={
    {true,3u,0x2225F920u,4u,0u},
    {true,1u,0x2225F930u,9u,0u},
    {true,7u,0x2225F940u,4u,0u},
    {true,4u,0x2225F950u,4u,0u},
    {true,5u,0x2225F960u,4u,0u},
    {true,6u,0x2225F970u,30u,4u},
    {true,8u,0x2225F990u,6u,0u},
    {false,0u,0x241056F0u,11u,4u},
    {false,5u,0x24105700u,22u,4u},
    {false,7u,0x24105720u,10u,4u},
    {false,8u,0x24105730u,4u,0u},
    {false,6u,0x24105740u,4u,0u},
};
inline std::uint32_t load_u32(const Bytes& bytes,std::size_t offset) {
    if(offset>bytes.size() || bytes.size()-offset<4)
        throw std::out_of_range("host-only original native embedded pointer bounds");
    return std::uint32_t(bytes[offset]) |
        (std::uint32_t(bytes[offset+1u])<<8) |
        (std::uint32_t(bytes[offset+2u])<<16) |
        (std::uint32_t(bytes[offset+3u])<<24);
}
inline void write_u32(Bytes& bytes,std::size_t offset,std::uint32_t value) {
    if(offset>bytes.size() || bytes.size()-offset<4)
        throw std::out_of_range("host-only original native embedded write bounds");
    for(unsigned i=0;i<4u;i++)
        bytes[offset+i]=static_cast<std::uint8_t>((value>>(i*8))&0xFFu);
}
inline float load_f32(const Bytes& bytes,std::size_t offset) {
    static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,
                  "original x87 FLD DWORD requires IEEE754 binary32");
    const auto bits=load_u32(bytes,offset);
    float value{};
    std::memcpy(&value,&bits,sizeof(value));
    return value;
}
inline std::size_t host_checked_offset(std::size_t base,std::uint64_t advance) {
    if(advance>std::numeric_limits<std::size_t>::max() ||
       base>std::numeric_limits<std::size_t>::max()-static_cast<std::size_t>(advance))
        throw std::overflow_error("host-only original embedded offset arithmetic");
    return base+static_cast<std::size_t>(advance);
}
// Server HLTVDirector001 final embedded virtual slots. All fields are
// RELATIVE to the interface pointer (containing object + 8).
inline std::uint32_t server_get_04(const Bytes& view,std::size_t this_off=0) {
    return load_u32(view,host_checked_offset(this_off,4));
}
inline bool server_slot1_nonzero_04(const Bytes& view,std::size_t this_off=0) {
    return server_get_04(view,this_off)!=0u;
}
inline float server_slot7_f32_08(const Bytes& view,std::size_t this_off=0) {
    // Native returns via x87 ST(0), host returns the decoded float.
    return load_f32(view,host_checked_offset(this_off,8));
}
inline std::uint32_t server_slot4_get_0c(const Bytes& view,std::size_t this_off=0) {
    return load_u32(view,host_checked_offset(this_off,12));
}
inline std::uint32_t server_slot5_get_10(const Bytes& view,std::size_t this_off=0) {
    return load_u32(view,host_checked_offset(this_off,16));
}
// Native server slot6: source DWORDs at interface +0x14/+0x18/+0x1C
// copied sequentially to the caller's destination DWORD[0..2].
// Forward store order means overlapping source/dest may cascade.
inline void server_slot6_forward_three_dwords(
        const Bytes& source,std::size_t this_off,Bytes& destination,
        std::size_t destination_off) {
    for(std::size_t i=0;i<3;i++) {
        const auto src=host_checked_offset(this_off,0x14u+4u*i);
        const auto dst=host_checked_offset(destination_off,4u*i);
        const auto value=load_u32(source,src);
        write_u32(destination,dst,value);
    }
}
// Server slot8 MOV EAX, absolute immediate; the 4-byte immediate at
// native 0x2225F991 has a PE32 HIGHLOW relocation, so compute an
// expected rebased TOKEN, not a dereferenceable process pointer.
inline std::uint32_t server_slot8_rebased_global_token(
        std::uint32_t loaded_base=kServerOriginalBase) {
    constexpr std::uint32_t rva=kServerGlobalAddressVA-kServerOriginalBase;
    const std::uint64_t computed=std::uint64_t(loaded_base)+rva;
    if(computed>std::numeric_limits<std::uint32_t>::max())
        throw std::overflow_error("host-only PE32 HIGHLOW address overflow");
    return static_cast<std::uint32_t>(computed);
}
// Client VClientEntityList003 final embedded virtual methods. The
// interface this is containing entity-list parent + 0x10014.
inline std::uint32_t client_slot0_indexed_dword(
        const Bytes& view,std::size_t this_off,std::uint32_t index) {
    // Original MOV EAX,[ECX + index*8 + 0x14]; unsafe i386
    // integer wrapping/dereference is refused by the portable host.
    const auto relative=std::uint64_t(index)*8u+0x14u;
    return load_u32(view,host_checked_offset(this_off,relative));
}
inline std::uint32_t client_slot5_base_adjusted(
        const Bytes& view,std::size_t this_off,std::uint32_t selector) {
    const auto base=load_u32(view,host_checked_offset(this_off,4));
    // Original compares ONLY the low byte of selector to exactly 1.
    // Native ADD is a wrapping 32-bit integer addition.
    if((selector&0xFFu)==1u)
        return base+load_u32(view,host_checked_offset(this_off,12));
    return base;
}
inline std::uint32_t client_slot7_set_08(
        Bytes& view,std::size_t this_off,std::uint32_t value) {
    write_u32(view,host_checked_offset(this_off,8),value);
    return value; // Native retains EAX with supplied DWORD.
}
inline std::uint32_t client_slot8_get_08(
        const Bytes& view,std::size_t this_off) {
    return load_u32(view,host_checked_offset(this_off,8));
}
inline std::uint32_t client_slot6_get_10(
        const Bytes& view,std::size_t this_off) {
    return load_u32(view,host_checked_offset(this_off,16));
}

} // namespace sin_compat::original_embedded_virtual_accessors
