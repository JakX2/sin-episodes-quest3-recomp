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
#include "sin_compat/original_client_custom_handle_proxy.hpp"
#include "sin_compat/original_client_custom_color_integer_proxies.hpp"
#include "sin_compat/original_client_custom_movement_fov_proxies.hpp"
#include "sin_compat/original_client_custom_scale_transition_proxies.hpp"

namespace sin_compat::original_client_standard_recvproxy_router {
namespace manifest=sin_compat::original_client_native32_recvtable_manifest;
namespace members=sin_compat::original_client_native_member_offsets;
namespace source=sin_compat::original_client_source_proxy_catalog;
namespace strings=sin_compat::original_client_source_string_sizes;
namespace copies=sin_compat::original_client_recvprop_proxies_factories;
namespace owned=sin_compat::original_client_standard_recvproxy_host;
namespace handle=sin_compat::original_client_custom_handle_proxy;
namespace colorInt=sin_compat::original_client_custom_color_integer_proxies;
namespace moveFov=sin_compat::original_client_custom_movement_fov_proxies;
namespace scale=sin_compat::original_client_custom_scale_transition_proxies;

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

// Phase1709: the 999 original standard identities dispatch through seven
// source-backed families; 35 source-proven original custom handle/moveparent
// assignments use their exact reconstructed 49-byte method behavior.
// Phase1710 now adds four byte-reordering color assignments and five
// original integer decrement assignments. Phase1711 handles three original direct movement/FOV
// state changes, bringing host-reconstructed routes to 1046; the remaining
// Phase1712 adds two source float-scale transition routines, preserving
// original x87 ordered comparisons; 34 custom and all 28 special
// constructor paths stay blocked.
// This is still a bounded, HOST-OWNED function, not live Source networking.
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
    if(nativeSourceVA==0u ||
       (!source::IsStandardOriginalSourceVA(nativeSourceVA) &&
        !handle::IsOriginalHandleProxy(nativeSourceVA) &&
        !colorInt::IsOriginalCustomColorOrDecrement(nativeSourceVA) &&
        !moveFov::IsOriginalMovementOrFovProxy(nativeSourceVA) &&
        !scale::IsSourceScaleProxy(nativeSourceVA)))
        return Status::UnsupportedSourceCallback;

    if(nativeType==5u && nativeSourceVA==owned::kOriginalDataTableProxyVA) {
        owned::ApplyDataTableToken(incoming.dataTableObjectToken,
                                    dataTableOutputToken);
        return Status::Applied;
    }
    // Phase1711: three source-attested custom callbacks use fixed
    // object fields independent of their RecvProp +44 metadata. In
    // particular movetype/movecollide have original +44=0 but write to
    // object+0x144/+0x148, so generic offset-based copying is WRONG.
    // The FOV callback additionally resets +0x2C only when +0x38
    // actually changes. The helper validates all fields before writing.
    if(moveFov::IsOriginalMovementOrFovProxy(nativeSourceVA)) {
        if(nativeType!=0u)return Status::UnsupportedSourceCallback;
        const auto applied=moveFov::ApplySourceOriginalEffect(
            sourceTableOrdinal,slot,nativeSourceVA,
            incoming.originalProxyData,originalObjectBytes);
        if(applied==moveFov::Status::Applied)
            return Status::Applied;
        if(applied==moveFov::Status::RefusedBadOwnedMemory)
            return Status::RefusedBadOwnedMemory;
        return Status::UnsupportedSourceCallback;
    }

    // Phase1712: original x87 m_flScale transition callbacks for
    // DT_Plasma/DT_FireSmoke. Extra transition fields and ordered
    // +0/-0 equivalence are required; do not reduce to generic memcpy.
    // The host refuses NaNs because the old x87 payload/quieting
    // store semantics have not been independently reconstructed.
    if(scale::IsSourceScaleProxy(nativeSourceVA)) {
        if(nativeType!=1u)return Status::UnsupportedSourceCallback;
        const auto applied=scale::ApplySourceOriginalEffect(
            sourceTableOrdinal,slot,nativeSourceVA,
            incoming.originalProxyData,originalObjectBytes);
        if(applied==scale::Status::Applied)
            return Status::Applied;
        if(applied==scale::Status::RefusedBadOwnedMemory)
            return Status::RefusedBadOwnedMemory;
        return Status::UnsupportedSourceCallback;
    }

    const auto offset=members::kSourceNativeMemberOffsets[index];
    if(offset==members::kNotDirectlyRecovered)
        return Status::RefusedBadOwnedMemory;
    const auto start=static_cast<std::size_t>(offset);
    if(start>originalObjectBytes.size())
        return Status::RefusedBadOwnedMemory;

    if(colorInt::IsOriginalCustomColorOrDecrement(nativeSourceVA)) {
        // Original routines are 2006 PE32 38/17-byte methods:
        // both read one DWORD from original CRecvProxyData+4.
        // Color writes four separate BYTEs in high-to-low order,
        // decrement writes one native DWORD of raw-1 (wraparound).
        if(nativeType!=0u)return Status::UnsupportedSourceCallback;
        constexpr std::size_t kWidth=4u;
        if(kWidth>originalObjectBytes.size()-start)
            return Status::RefusedBadOwnedMemory;
        std::uint32_t raw=0u;
        for(std::size_t i=0u;i<kWidth;++i)
            raw|=std::uint32_t(incoming.originalProxyData[
                colorInt::kOriginalDecodedProxyValueOffset+i])<<(i*8u);
        if(nativeSourceVA==colorInt::kNativeColorByteReverseVA) {
            const auto mapped=colorInt::SourceColorBytes(raw);
            for(std::size_t i=0u;i<kWidth;++i)
                originalObjectBytes[start+i]=mapped[i];
        } else {
            const auto mapped=colorInt::SourceDecrementDword(raw);
            for(std::size_t i=0u;i<kWidth;++i)
                originalObjectBytes[start+i]=
                    static_cast<std::uint8_t>(mapped>>(i*8u));
        }
        return Status::Applied;
    }

    if(handle::IsOriginalHandleProxy(nativeSourceVA)) {
        // Original handle and moveparent callbacks take 32-bit decoded
        // raw value from native CRecvProxyData+4. They perform signed
        // SAR 11, SHL 12, OR index11 and invalid-handle special case.
        // On this host the *source* is an OWNED sixteen-byte record and
        // the destination is bounded original member byte storage.
        if(nativeType!=0u)return Status::UnsupportedSourceCallback;
        constexpr std::size_t kWidth=4u;
        if(kWidth>originalObjectBytes.size()-start)
            return Status::RefusedBadOwnedMemory;
        std::uint32_t raw=0u;
        for(std::size_t i=0u;i<kWidth;++i)
            raw|=std::uint32_t(incoming.originalProxyData[
                handle::kOriginalSourceValueOffset+i])<<(i*8u);
        const auto mapped=handle::DecodeSourceHandle(raw);
        for(std::size_t i=0u;i<kWidth;++i)
            originalObjectBytes[start+i]=
                static_cast<std::uint8_t>(mapped>>(i*8u));
        return Status::Applied;
    }

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
