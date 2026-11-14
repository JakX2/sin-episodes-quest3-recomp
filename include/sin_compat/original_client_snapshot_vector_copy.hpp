#pragma once
// Phase221 clean-room host reference for two complete SHA-reviewed original
// 2006 SiN client.dll snapshot field copy methods. No proprietary code.
// No engine/record ownership or runnable gameplay client.dll is implied.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "sin_compat/original_client_snapshot_field_copy.hpp"

namespace sin_compat::original_client_snapshot_vector_copy {
namespace original_copy=original_client_snapshot_field_copy;
using Bytes=std::vector<std::uint8_t>;
using CopyContext=original_copy::CopyContext;
inline constexpr std::uint32_t kType3CopyVA=0x24198DA0u;
inline constexpr std::uint32_t kType4CopyVA=0x24198DE0u;
inline constexpr std::uint32_t kType3RouterCall=0x241998C3u;
inline constexpr std::uint32_t kType4RouterCall=0x241998FDu;
inline constexpr std::uint32_t kType3RecordBytes=12u;
inline constexpr std::uint32_t kType4RecordBytes=16u;

// Native i386 uses DWORD arithmetic to compute count*stride, then
// REP MOVSD followed by REP MOVSB for remaining bytes, DF assumed clear.
// The raw 32-bit multiplication can wrap. Host-only model deliberately
// rejects that unsafe case rather than simulating x86 memory corruption.
// Original skip gates happen before any pointer or count access.
inline void copy_fixed_stride(CopyContext context,std::uint32_t status,
        const Bytes& net,std::size_t net_offset,
        Bytes& predicted,std::size_t predicted_offset,
        std::uint32_t record_count,std::uint32_t record_bytes) {
    if(!original_copy::should_copy(context,status))return;
    if(record_bytes==0u ||
       record_count>std::numeric_limits<std::uint32_t>::max()/record_bytes)
        throw std::overflow_error("host-only native count*record_bytes overflow");
    const auto count=static_cast<std::size_t>(record_count)*record_bytes;
    original_copy::forward_copy(net,net_offset,predicted,predicted_offset,count);
}

// Original complete 53-byte 0x24198DA0, RET 16.
// LEA count*3; double twice = count*12 modulo 2^32; copy forwards.
inline void copy_type3(CopyContext context,std::uint32_t status,
        const Bytes& net,std::size_t net_offset,
        Bytes& predicted,std::size_t predicted_offset,
        std::uint32_t record_count) {
    copy_fixed_stride(context,status,net,net_offset,predicted,predicted_offset,
                      record_count,kType3RecordBytes);
}

// Original complete 49-byte 0x24198DE0, RET 16.
// SHL count by 4 = count*16 modulo 2^32; copy forwards.
inline void copy_type4(CopyContext context,std::uint32_t status,
        const Bytes& net,std::size_t net_offset,
        Bytes& predicted,std::size_t predicted_offset,
        std::uint32_t record_count) {
    copy_fixed_stride(context,status,net,net_offset,predicted,predicted_offset,
                      record_count,kType4RecordBytes);
}

} // namespace sin_compat::original_client_snapshot_vector_copy
