#pragma once
// Phase209: independently reconstructed, SHA-verified ORIGINAL SiN 2006
// server/client ALLOCATOR CALLERS, not merely duplicate allocator wrappers.
// All pointers are host-controlled opaque 32-bit tokens, NEVER live 2006
// game heap pointers or SourceVR ABI objects.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include "sin_compat/original_tier0_allocator_bridges.hpp"

namespace sin_compat::original_allocator_game_callers {
namespace bridge = original_tier0_allocator_bridges;
namespace cross = original_cross_dll_tier0_allocator;

inline constexpr std::uint32_t kServerStringCallerVA = 0x22358870u;
inline constexpr std::uint32_t kClientBufferInitVA = 0x240c8720u;
inline constexpr std::uint32_t kServerRandomIntIATVA = 0x223d21acu;
inline constexpr std::uint32_t kServerGameVirtualMethodByteOffset = 0x5b0u;
inline constexpr std::uint32_t kClientDescriptorVirtualMethodByteOffset = 0x40u;
inline constexpr std::uint32_t kClientOptionalPreinitHelperVA = 0x240c57b0u;
inline constexpr std::uint32_t kClientPointerArrayObjectByteOffset = 0x2ecu;
inline constexpr std::uint32_t kClientSentinelObjectByteOffset = 0x454u;
inline constexpr std::uint32_t kClientBufferCount = 90u;
inline constexpr std::uint32_t kClientTotalAllocationCount = 91u;
inline constexpr std::uint32_t kClientMinimumBufferBytes = 4u;

// Original server method (52 native bytes): allocate THREE bytes via
// previously SHA-verified +0x2226E820 alias, then write '_' and a single
// byte formed from the LOW BYTE of vstdlib.dll!RandomInt(0,3) + 'A',
// followed by NUL. Invoke object's virtual method at vtable byte +0x5B0
// with this buffer as argument. Original does NOT free the token in this
// method; actual transfer/ownership is unresolved. Native also does not
// check a null allocation; this host reference rejects null before writing.
template<class Host>
decltype(auto) server_make_and_send_token(cross::ModuleState& allocator,
                                          Host& host) {
    const std::uint32_t token=bridge::allocate_cached_alias_b(allocator,host,3u);
    if (token==0u)
        throw std::runtime_error("host-only original server string allocation failed");
    host.store_byte(token,0u,static_cast<std::uint8_t>('_'));
    const std::uint32_t random=static_cast<std::uint32_t>(host.random_int(0,3));
    const auto letter=static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(random)+static_cast<std::uint8_t>('A'));
    host.store_byte(token,1u,letter);
    host.store_byte(token,2u,static_cast<std::uint8_t>(0u));
    return host.dispatch_virtual_5b0(token);
}

// Original client method: 156 native bytes. At object+0x454, a nonzero
// pointer is a guard and skips all work. Otherwise query virtual +0x40;
// if that descriptor exists and its byte at +0x11 is ZERO, invoke original
// 0x240C57B0 on it. Query +0x40 AGAIN, read its signed DWORD at +0x14,
// clamp to a minimum FOUR, allocate/zero and store one sentinel buffer,
// then allocate/zero 90 MORE and store their tokens in consecutive object
// DWORDs from +0x2EC to +0x450. The original source-level member/class
// names and real descriptor meaning remain UNKNOWN.
//
// The 91 pointers live in one contiguous original object offset span.
// This C++ struct models that span, not the preceding 0x2EC-byte object.
struct ClientBufferSpan32 {
    std::array<std::uint32_t,kClientBufferCount> pointers_2ec_to_450{};
    std::uint32_t sentinel_454{};
};
static_assert(sizeof(ClientBufferSpan32)==kClientTotalAllocationCount*4u);
static_assert(offsetof(ClientBufferSpan32,sentinel_454)==
              kClientSentinelObjectByteOffset-kClientPointerArrayObjectByteOffset);

struct ClientDescriptor {
    bool present{};
    std::uint8_t flag_at_11{};
    std::int32_t signed_size_at_14{};
};

// Host must expose:
//   ClientDescriptor query_virtual_40();
//   void invoke_helper_240c57b0(const ClientDescriptor&);
//   void zero_bytes(uint32_t token,uint32_t count);
// Plus the same allocation callbacks as the Phase208 bridge model.
//
// The original machine code DOES NOT check descriptor pointer dereference
// on the SECOND query or allocation result before REP STOSD/STOSB. Guards
// here are intentionally HOST-ONLY and do not claim x86 behavioral parity
// for OOM, corrupted objects or dangling pointers.
template<class Host>
void client_init_91_buffers(ClientBufferSpan32& memory,
                             cross::ModuleState& allocator,
                             Host& host) {
    if (memory.sentinel_454 != 0u) return;
    const ClientDescriptor first=host.query_virtual_40();
    if (first.present && first.flag_at_11==0u)
        host.invoke_helper_240c57b0(first);

    const ClientDescriptor second=host.query_virtual_40();
    if (!second.present)
        throw std::runtime_error("host-only original client descriptor missing");
    const std::uint32_t bytes =
        second.signed_size_at_14 > static_cast<std::int32_t>(kClientMinimumBufferBytes)
            ? static_cast<std::uint32_t>(second.signed_size_at_14)
            : kClientMinimumBufferBytes;
    // Original has no memory bound; avoid allocating huge host fixtures.
    if (bytes>1048576u)
        throw std::overflow_error("host-only original client buffer too large");

    auto create_buffer=[&]() -> std::uint32_t {
        const auto token=bridge::allocate_cached_alias_b(allocator,host,bytes);
        if (token==0u)
            throw std::runtime_error("host-only original client allocation failed");
        host.zero_bytes(token,bytes);
        return token;
    };
    memory.sentinel_454=create_buffer();
    for (std::uint32_t i=0;i<kClientBufferCount;++i)
        memory.pointers_2ec_to_450[i]=create_buffer();
}

} // namespace sin_compat::original_allocator_game_callers
