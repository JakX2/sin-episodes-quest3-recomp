#pragma once
// Phase213: four independently verified original 2006 SiN client.dll
// snapshot FIELD COPY HELPERS called by Phase212's type 1, 2, 5 and 6
// specialist cases. These are NOT complete cases, the source engine
// serializer, or an original 2006 ABI-compatible client.dll replacement.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace sin_compat::original_client_snapshot_field_copy {
inline constexpr std::uint32_t kCopyDwordsType5VA=0x24198a10u;
inline constexpr std::uint32_t kCopyBytesType6VA=0x24198a40u;
inline constexpr std::uint32_t kCopyDwordsType1VA=0x24198a70u;
inline constexpr std::uint32_t kCopyCStringType2VA=0x24198b40u;

// Original native callsite => complete original helper entry.
inline constexpr std::uint32_t kType5Callsite=0x241999cbu;
inline constexpr std::uint32_t kType6Callsite=0x24199993u;
inline constexpr std::uint32_t kType1Callsite=0x2419985bu;
inline constexpr std::uint32_t kType2Callsite=0x2419988fu;

struct CopyContext {
    // Native this+0x2C BYTE, NOT a reconstructed complete state object.
    std::uint8_t byte_2c{};
};
inline constexpr bool should_copy(CopyContext state,
                                   std::uint32_t first_stack_arg) noexcept {
    // Exact original: if (!BYTE PTR [ECX+0x2C] || arg1 == 1) skip.
    return state.byte_2c!=0u && first_stack_arg!=1u;
}

// Native REP MOVSD/MOVSB copies FORWARDS with DF assumed clear.
// Unlike memcpy, this can cause cascading byte overwrites for an
// overlapping destination after the source. Preserve this detail.
// Host-only bounds/length checks avoid the native unguarded memory faults.
inline void forward_copy(const std::vector<std::uint8_t>& src,
                         std::size_t source_off,
                         std::vector<std::uint8_t>& dst,
                         std::size_t dest_off,
                         std::size_t bytes) {
    if(source_off>src.size() || bytes>src.size()-source_off ||
       dest_off>dst.size() || bytes>dst.size()-dest_off)
        throw std::out_of_range("host-only native copy buffer bounds");
    // REP MOVSD transfers one complete four-byte word at a time. The
    // source word must be read before ANY of that DWORD is written:
    // for overlapping buffers, a naive byte loop is NOT equivalent.
    const auto word_count=bytes/4u;
    for(std::size_t w=0;w<word_count;++w) {
        const auto i=w*4u;
        const std::uint8_t word[4]{
            src[source_off+i],src[source_off+i+1u],
            src[source_off+i+2u],src[source_off+i+3u]
        };
        for(std::size_t j=0;j<4u;++j)
            dst[dest_off+i+j]=word[j];
    }
    // Original trailing REP MOVSB copies each remaining byte FORWARDS.
    for(std::size_t i=word_count*4u;i<bytes;++i)
        dst[dest_off+i]=src[source_off+i];
}

// Original 34-byte byte-for-byte SAME x86 body exists at two distinct
// addresses. The four native stack DWORDs are:
// (operation_flag, destination_pointer, source_pointer, DWORD_COUNT).
// Native REP MOVSD uses DWORD_COUNT, NOT a byte count.
inline void copy_dwords(CopyContext state,std::uint32_t operation_flag,
                        const std::vector<std::uint8_t>& source,
                        std::size_t source_off,
                        std::vector<std::uint8_t>& destination,
                        std::size_t destination_off,
                        std::uint32_t dword_count) {
    if(!should_copy(state,operation_flag))return;
    if(dword_count>std::numeric_limits<std::uint32_t>::max()/4u)
        throw std::overflow_error("host-only native DWORD copy size overflow");
    forward_copy(source,source_off,destination,destination_off,
                 static_cast<std::size_t>(dword_count)*4u);
}
inline void copy_dwords_type5(CopyContext state,std::uint32_t operation_flag,
                              const std::vector<std::uint8_t>& src,std::size_t src_off,
                              std::vector<std::uint8_t>& dst,std::size_t dst_off,
                              std::uint32_t dwords) {
    copy_dwords(state,operation_flag,src,src_off,dst,dst_off,dwords);
}
inline void copy_dwords_type1(CopyContext state,std::uint32_t operation_flag,
                              const std::vector<std::uint8_t>& src,std::size_t src_off,
                              std::vector<std::uint8_t>& dst,std::size_t dst_off,
                              std::uint32_t dwords) {
    copy_dwords(state,operation_flag,src,src_off,dst,dst_off,dwords);
}

// Original 46-byte type-6 helper receives an explicit BYTE count and
// uses REP MOVSD(count>>2) then REP MOVSB(count&3), hence preserving
// the final 1..3 bytes rather than rounding down to a DWORD boundary.
inline void copy_bytes_type6(CopyContext state,std::uint32_t operation_flag,
                             const std::vector<std::uint8_t>& src,std::size_t src_off,
                             std::vector<std::uint8_t>& dst,std::size_t dst_off,
                             std::uint32_t bytes) {
    if(!should_copy(state,operation_flag))return;
    forward_copy(src,src_off,dst,dst_off,bytes);
}

// Original 61-byte type-2 helper walks source BYTEs until a NUL, then
// copies strlen(source)+1 bytes, INCLUDING the terminating NUL, using
// REP MOVSD / REP MOVSB. The native length scan is unbounded; the
// host-only reference rejects unterminated buffers instead of reading
// unmapped memory. Original helper has THREE stack DWORD arguments.
inline void copy_c_string_type2(CopyContext state,std::uint32_t operation_flag,
                                const std::vector<std::uint8_t>& src,
                                std::size_t src_off,
                                std::vector<std::uint8_t>& dst,
                                std::size_t dst_off) {
    if(!should_copy(state,operation_flag))return;
    if(src_off>src.size())
        throw std::out_of_range("host-only original string source offset");
    const auto nul=std::find(src.begin()+static_cast<std::ptrdiff_t>(src_off),
                             src.end(),static_cast<std::uint8_t>(0u));
    if(nul==src.end())
        throw std::runtime_error("host-only native source string has no NUL");
    const auto count=static_cast<std::size_t>(
        std::distance(src.begin()+static_cast<std::ptrdiff_t>(src_off),nul))+1u;
    forward_copy(src,src_off,dst,dst_off,count);
}

} // namespace sin_compat::original_client_snapshot_field_copy
