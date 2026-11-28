#pragma once
// Phase1707: source-authenticated 2006 PE32 standard STRING and DATATABLE
// RecvProxy method semantics reconstructed in original-free C++17.
// Original binary methods: 0x24126CA0 (59B) and 0x24126CE0 (11B).
// This is ONLY host-owned, bounds-checked data behavior; it does NOT
// install live 2006 Source client/network ABI callbacks or original VAs.
// Original string RecvProp +12 is a signed string destination capacity;
// the source bytes are indirect through CRecvProxyData+4. The original
// writes a final NUL byte to destination[capacity-1].
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sin_compat::original_client_standard_recvproxy_host {
inline constexpr std::uint32_t kOriginalStringProxyVA=0x24126CA0u;
inline constexpr std::uint32_t kOriginalDataTableProxyVA=0x24126CE0u;
inline constexpr std::size_t kOriginalStringProxyBytes=59u;
inline constexpr std::size_t kOriginalDataTableProxyBytes=11u;
inline constexpr std::uint32_t kOriginalRecvPropStringBufferOffset=12u;
inline constexpr std::uint32_t kOriginalReceivedValuePointerOffset=4u;
inline constexpr std::uint32_t kOriginalDataTableOutputArgumentOffset=8u;
inline constexpr std::uint32_t kOriginalDataTableSourceArgumentOffset=12u;
enum class Status : std::uint8_t { Applied, SourceNoOp, Refused };

// Mirrors native 0x24126CA0's observable writes using bounded OWNED bytes.
// sourceBytes includes any terminating NUL as an actual byte, never a
// pointer/length inferred by calling strlen on unknown original memory.
// A short, non-NUL-terminated source refuses without changing output.
// Native routine does not clear unrelated destination bytes. A positive
// native stringBufferSize always forces destination[size-1] to NUL.
// Source x86 treats nonpositive int32 string size as a NO-OP.
inline Status ApplyString(std::string_view sourceBytes,
                          std::vector<std::uint8_t>& destination,
                          std::int32_t nativeStringBufferSize) {
    if(nativeStringBufferSize<=0)return Status::SourceNoOp;
    const auto capacity=static_cast<std::size_t>(nativeStringBufferSize);
    if(capacity>destination.size())return Status::Refused;

    // Preflight exact accesses before writing anything. This also handles
    // embedded NUL correctly, without reading beyond sourceBytes.
    std::size_t copied=0u;
    while(copied<capacity) {
        if(copied>=sourceBytes.size())return Status::Refused;
        const auto c=static_cast<std::uint8_t>(sourceBytes[copied]);
        ++copied;
        if(c==0u)break;
    }
    for(std::size_t i=0u;i<copied;++i)
        destination[i]=static_cast<std::uint8_t>(sourceBytes[i]);
    destination[capacity-1u]=0u;
    return Status::Applied;
}

// Original 0x24126CE0 does:
//   EAX <- [ESP+0x0C], ECX <- [ESP+0x08], [ECX] <- EAX.
// Tokens here are owned 32-bit VALUES, not dereferenceable original
// source process addresses. This intentionally does not touch engine
// object identities, allocator ownership or live pointer validity.
inline void ApplyDataTableToken(std::uint32_t sourceToken,
                                std::uint32_t& outputToken) noexcept {
    outputToken=sourceToken;
}
inline constexpr bool CanPublishRetailProxyPointers() noexcept {return false;}
} // namespace sin_compat::original_client_standard_recvproxy_host
