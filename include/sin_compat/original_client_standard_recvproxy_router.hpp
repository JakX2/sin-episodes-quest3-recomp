#pragma once
// Phase1708: HOST-OWNED dispatch of original source-standard receiver
// callback IDENTITIES to independently reconstructed C++17 semantics.
// This does not call original x86 PE32 method addresses, consume a
// packet, return live RecvProp pointers or enable VClient011.slot5.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>
#include "sin_compat/original_client_native32_recvtable_manifest.hpp"
#include "sin_compat/original_client_native_member_offsets.hpp"
#include "sin_compat/original_client_recvprop_proxies_factories.hpp"
#include "sin_compat/original_client_source_proxy_catalog.hpp"
#include "sin_compat/original_client_source_string_sizes.hpp"
#include "sin_compat/original_client_standard_recvproxy_host.hpp"

namespace sin_compat::original_client_standard_recvproxy_router {
namespace manifest=sin_compat::original_client_native32_recvtable_manifest;
namespace members=sin_compat::original_client_native_member_offsets;
namespace source=sin_compat::original_client_source_proxy_catalog;
namespace strings=sin_compat::original_client_source_string_sizes;
namespace copies=sin_compat::original_client_recvprop_proxies_factories;
namespace owned=sin_compat::original_client_standard_recvproxy_host;

struct DecodedOwnedInput final {
    // The original CRecvProxyData union value begins at byte +4.
    // Remaining four source bytes are never executable pointers.
    std::array<std::uint8_t,16> originalProxyData{};
    // Explicit, memory-safe decoded string slice INCLUDING any NUL byte.
    std::string_view decodedStringBytes{};
    // Bounded 32-bit native datatable object TOKEN, not a process pointer.
    std::uint32_t dataTableObjectToken=0u;
};
enum class Status : std::uint8_t {
    Applied, UnsupportedSourceCallback, RefusedBadOwnedMemory
};

inline constexpr bool CanPublishRetailProxyPointers() noexcept {return false;}

// All 999 original source-standard callback identities can dispatch
// through the seven source-backed method families. The 83 custom and
// 28 specialized methods NEVER silently fall back to raw memcpy.
inline Status ApplySourceStandardProperty(
    std::size_t sourceTableOrdinal,std::size_t slot,
    const DecodedOwnedInput& incoming,
    std::vector<std::uint8_t>& originalObjectBytes,
    std::uint32_t& dataTableOutputToken) {
    if(sourceTableOrdinal>=manifest::kStaticTables ||
       slot>=manifest::kCounts[sourceTableOrdinal])
        return Status::RefusedBadOwnedMemory;
    const auto index=manifest::kTypePrefixes[sourceTableOrdinal]+slot;
    const auto nativeType=manifest::kTypes[index];
    const auto nativeSourceVA=source::SourceVAForSlot(sourceTableOrdinal,slot);
    if(nativeSourceVA==0u || !source::IsStandardOriginalSourceVA(nativeSourceVA))
        return Status::UnsupportedSourceCallback;

    if(nativeType==5u && nativeSourceVA==owned::kOriginalDataTableProxyVA) {
        owned::ApplyDataTableToken(incoming.dataTableObjectToken,
                                    dataTableOutputToken);
        return Status::Applied;
    }
    const auto offset=members::kSourceNativeMemberOffsets[index];
    if(offset==members::kNotDirectlyRecovered)
        return Status::RefusedBadOwnedMemory;
    const auto start=static_cast<std::size_t>(offset);
    if(start>originalObjectBytes.size())
        return Status::RefusedBadOwnedMemory;

    if(nativeType==3u && nativeSourceVA==owned::kOriginalStringProxyVA) {
        const auto capacity=strings::SourceCapacity(sourceTableOrdinal,slot);
        if(capacity==0u ||
           static_cast<std::size_t>(capacity)>originalObjectBytes.size()-start)
            return Status::RefusedBadOwnedMemory;
        std::vector<std::uint8_t> output(
            originalObjectBytes.begin()+start,
            originalObjectBytes.begin()+start+capacity);
        const auto status=owned::ApplyString(
            incoming.decodedStringBytes,output,capacity);
        if(status!=owned::Status::Applied)
            return Status::RefusedBadOwnedMemory;
        std::copy(output.begin(),output.end(),
                  originalObjectBytes.begin()+start);
        return Status::Applied;
    }

    copies::ProxyKind kind=copies::ProxyKind::Byte;
    if(nativeType==0u) {
        if(nativeSourceVA==copies::kProxyByteVA)
            kind=copies::ProxyKind::Byte;
        else if(nativeSourceVA==copies::kProxyWordVA)
            kind=copies::ProxyKind::Word;
        else if(nativeSourceVA==copies::kProxyInt32VA)
            kind=copies::ProxyKind::Int32;
        else return Status::UnsupportedSourceCallback;
    }else if(nativeType==1u && nativeSourceVA==copies::kProxyFloatVA) {
        kind=copies::ProxyKind::Float;
    }else if(nativeType==2u && nativeSourceVA==copies::kProxyVector3VA) {
        kind=copies::ProxyKind::Vector3;
    }else return Status::UnsupportedSourceCallback;
    const auto width=copies::proxy_copy_size(kind);
    if(width>originalObjectBytes.size()-start)
        return Status::RefusedBadOwnedMemory;
    std::array<std::uint8_t,12> copied{};
    copies::receive_proxy_copy(kind,incoming.originalProxyData,copied);
    std::copy_n(copied.begin(),width,originalObjectBytes.begin()+start);
    return Status::Applied;
}
} // namespace sin_compat::original_client_standard_recvproxy_router
