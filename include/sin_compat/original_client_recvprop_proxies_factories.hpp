#pragma once
// Phase251: independent original-free C++17 reference for owner-original
// SiN Episodes: Emergence (2006) client RecvProxy/RecvProp/RecvTable methods.
// Entire native method SHA evidence is checked by a separate private audit.
// Do not treat this as a game-owned C++ object or a retail gameplay DLL.
#include <algorithm>
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "sin_compat/original_client_particle_trail_recvtable_crt.hpp"

namespace sin_compat::original_client_recvprop_proxies_factories {
namespace particle=sin_compat::original_client_particle_trail_recvtable_crt;

inline constexpr std::size_t kNewOriginalMethods=12u;
inline constexpr std::size_t kOriginalNativeBytes=466u;
inline constexpr std::uint32_t kTableDestructorVA=0x24126A20u;
inline constexpr std::uint32_t kTableRegistrationVA=0x24126A30u;
inline constexpr std::uint32_t kProxyFloatVA=0x24126C40u;
inline constexpr std::uint32_t kProxyVector3VA=0x24126C50u;
inline constexpr std::uint32_t kProxyByteVA=0x24126C70u;
inline constexpr std::uint32_t kProxyWordVA=0x24126C80u;
inline constexpr std::uint32_t kProxyInt32VA=0x24126C90u;
inline constexpr std::uint32_t kProxyDispatchCtorVA=0x24126D00u;
inline constexpr std::uint32_t kTableDefaultCtorVA=0x24126D30u;
inline constexpr std::uint32_t kTableArgsCtorVA=0x24126D50u;
inline constexpr std::uint32_t kFloatFactoryVA=0x24126D80u;
inline constexpr std::uint32_t kIntegerFactoryVA=0x24126DF0u;
inline constexpr std::size_t kRecvPropNativeStride=60u;
inline constexpr std::size_t kRecvTableInitializedExtent=18u;
inline constexpr std::size_t kProxySourceValueOffset=4u;
inline constexpr std::uint32_t kDefaultArrayIndex=0xFFFFFFFFu;

// The five pointers written by native constructor 0x24126D00 to
// table +0,+4,+8,+12,+16, in exact original storage order.
inline constexpr std::array<std::uint32_t,5> kDefaultProxyDispatch{{
    kProxyByteVA,kProxyWordVA,kProxyInt32VA,kProxyFloatVA,kProxyVector3VA
}};

enum class ProxyKind { Byte, Word, Int32, Float, Vector3 };
inline std::size_t proxy_copy_size(ProxyKind kind) {
    switch(kind) {
        case ProxyKind::Byte:return 1u;
        case ProxyKind::Word:return 2u;
        case ProxyKind::Int32:
        case ProxyKind::Float:return 4u;
        case ProxyKind::Vector3:return 12u;
    }
    throw std::invalid_argument("unsupported original RecvProxy type");
}
inline std::uint32_t proxy_native_VA(ProxyKind kind) {
    switch(kind) {
        case ProxyKind::Byte:return kProxyByteVA;
        case ProxyKind::Word:return kProxyWordVA;
        case ProxyKind::Int32:return kProxyInt32VA;
        case ProxyKind::Float:return kProxyFloatVA;
        case ProxyKind::Vector3:return kProxyVector3VA;
    }
    throw std::invalid_argument("unsupported original RecvProxy type");
}

// Authentic 0x24126C40/50/70/80/90 read from source struct BYTE+4,
// NOT from the pointer's first DWORD. They copy raw 1/2/4/12 bytes
// unchanged to an independent destination. The FLOAT and INT32 methods
// have identical native code SHA256 and preserve NaNs and -0.0 bits.
inline void receive_proxy_copy(ProxyKind kind,
                               const std::array<std::uint8_t,16>& incoming,
                               std::array<std::uint8_t,12>& destination) {
    const auto width=proxy_copy_size(kind);
    std::copy_n(incoming.begin()+kProxySourceValueOffset,width,
                destination.begin());
}

// Factory methods copy a stack-created 60-byte descriptor, BUT the
// original x86 does NOT write every byte before copying. Never claim
// unobserved bytes are initialized to zero or preserve a prior value:
// the host tracks exact known byte positions and treats all remaining
// bytes as UNDEFINED original-stack contents.
template<std::size_t N>
struct KnownNativeBytes {
    std::array<std::uint8_t,N> bytes{};
    std::bitset<N> defined{};
    void write8(std::size_t off,std::uint8_t value) {
        if(off>=N)throw std::out_of_range("native byte offset");
        bytes[off]=value;
        defined.set(off);
    }
    void write32(std::size_t off,std::uint32_t value) {
        if(off>N || N-off<4u)
            throw std::out_of_range("native DWORD offset");
        for(std::size_t i=0;i<4u;++i)
            write8(off+i,static_cast<std::uint8_t>(value>>(i*8u)));
    }
    std::uint32_t read32(std::size_t off) const {
        if(off>N || N-off<4u)
            throw std::out_of_range("native DWORD read");
        std::uint32_t v=0;
        for(std::size_t i=0;i<4u;++i) {
            if(!defined.test(off+i))
                throw std::logic_error("original x86 did not define this field");
            v|=std::uint32_t(bytes[off+i])<<(i*8u);
        }
        return v;
    }
    bool is_defined(std::size_t off) const {
        if(off>=N)throw std::out_of_range("native known-byte query");
        return defined.test(off);
    }
};
using RecvPropKnown=KnownNativeBytes<kRecvPropNativeStride>;
using RecvTableKnown=KnownNativeBytes<kRecvTableInitializedExtent>;

// Both original factories at 0x24126D80 (type1 FLOAT) and 0x24126DF0
// (type0 INTEGER) write these common fields in the temporary 60-byte
// descriptor. The fourth float-factory stack arg (data byte width) is
// NOT loaded by the original 0x24126D80 code; it is kept as an explicit
// ignored argument rather than inventing a field write.
inline RecvPropKnown common_recvprop(std::uint32_t name_token,
                                   std::uint32_t type,
                                   std::uint32_t member_offset,
                                   std::uint32_t extra_token,
                                   std::uint32_t proxy_token) {
    RecvPropKnown p{};
    p.write32(0u,name_token);         // pointer/name
    p.write32(4u,type);               // type 0=integer, 1=float
    p.write32(8u,extra_token);        // original fifth stack argument
    p.write8(16u,0u);                 // only low byte at +0x10
    p.write32(20u,0u);
    p.write32(24u,0u);
    p.write32(28u,0u);
    p.write32(32u,proxy_token);
    p.write32(36u,0u);
    p.write32(40u,0u);
    p.write32(44u,member_offset);
    p.write32(48u,kDefaultArrayIndex);
    p.write32(52u,1u);
    return p;
}
inline RecvPropKnown construct_float_property(
    std::uint32_t name_token,std::uint32_t member_offset,
    std::uint32_t /*width_arg_not_read*/,
    std::uint32_t extra_token,std::uint32_t explicit_proxy) {
    return common_recvprop(name_token,1u,member_offset,
                           extra_token,explicit_proxy);
}
inline std::uint32_t default_integer_proxy(std::uint32_t size) noexcept {
    if(size==2u)return kProxyWordVA;
    if(size==4u)return kProxyInt32VA;
    return kProxyByteVA; // width 1 and all other unsupported widths
}
inline RecvPropKnown construct_integer_placeholder_property(
    std::uint32_t name_token,std::uint32_t member_offset,
    std::uint32_t width,std::uint32_t extra_token,
    std::uint32_t explicit_proxy) {
    return common_recvprop(name_token,0u,member_offset,extra_token,
        explicit_proxy!=0u?explicit_proxy:default_integer_proxy(width));
}

// Original RecvTable 0x24126D30 default ctor and 0x24126D50
// argument-taking ctor touch only first four DWORD fields and bytes
// at +0x10/+0x11. Original registration 0x24126A30 likewise
// changes +0,+4,+8,+12,+16,+17; +0x08 is zero and NOT a pointer.
inline RecvTableKnown table_from_args(std::uint32_t first_descriptor,
                                     std::uint32_t count,
                                     std::uint32_t name) {
    RecvTableKnown t{};
    t.write32(0u,first_descriptor);
    t.write32(4u,count);
    t.write32(8u,0u);
    t.write32(12u,name);
    t.write8(16u,0u);
    t.write8(17u,0u);
    return t;
}
inline RecvTableKnown construct_default_table() {
    return table_from_args(0u,0u,0u);
}
inline void register_table(RecvTableKnown& t,std::uint32_t first_descriptor,
                           std::uint32_t count,std::uint32_t name) {
    t=table_from_args(first_descriptor,count,name);
}
inline void destruct_table_noop(RecvTableKnown& /*table*/) noexcept {}

// This adapter makes the PREVIOUSLY OPAQUE Phase250 callbacks execute
// the newly source-reviewed 0x24126D80/DF0/A30/D30/A20 reference
// semantics. Names and addresses remain original 32-bit process tokens.
// A standalone host test can cover the whole static initialization
// lifecycle without original executable game code or counterfeit pointers.
struct ParticleTrailRuntimeAdapter final:particle::RuntimeCallbacks {
    std::array<RecvPropKnown,4> descriptors{};
    RecvTableKnown table{};
    bool object_constructed{};
    bool cleanup_registered{};
    bool cleanup_called{};
    std::uint32_t registered_cleanup{};
    std::size_t register_count{};

    static std::size_t index_for(std::uint32_t va) {
        if(va==particle::kNativePlaceholderDescriptorVA)return 0u;
        if(va==particle::kNativeFirstNetworkDescriptorVA)return 1u;
        if(va==particle::kNativeSecondNetworkDescriptorVA)return 2u;
        if(va==particle::kNativeThirdNetworkDescriptorVA)return 3u;
        throw std::out_of_range("unknown original ParticleTrail RecvProp VA");
    }
    void construct_placeholder(
        std::uint32_t descriptor_va,std::uint32_t name_va,
        std::uint32_t member_offset,std::uint32_t size) override {
        descriptors[index_for(descriptor_va)]=
            construct_integer_placeholder_property(name_va,member_offset,
                size,0u,0u);
    }
    void construct_float(
        std::uint32_t descriptor_va,std::uint32_t name_va,
        std::uint32_t member_offset,std::uint32_t size,
        std::uint32_t proxy_va) override {
        if(proxy_va!=kProxyFloatVA)
            throw std::logic_error("unexpected original float proxy VA");
        descriptors[index_for(descriptor_va)]=
            construct_float_property(name_va,member_offset,size,0u,proxy_va);
    }
    void register_table(
        std::uint32_t object_va,std::uint32_t first,
        std::uint32_t count,std::uint32_t name_va) override {
        if(object_va!=particle::kNativeRecvTableObjectVA)
            throw std::logic_error("incorrect original table global");
        original_client_recvprop_proxies_factories::register_table(
            table,first,count,name_va);
        ++register_count;
    }
    void construct_table_object(std::uint32_t va) override {
        if(va!=particle::kNativeRecvTableObjectVA)
            throw std::logic_error("incorrect original table object");
        table=construct_default_table();
        object_constructed=true;
    }
    void register_termination_handler(std::uint32_t cleanup_va) override {
        registered_cleanup=cleanup_va;
        cleanup_registered=true;
    }
    void destruct_table_object(std::uint32_t va) override {
        if(va!=particle::kNativeRecvTableObjectVA)
            throw std::logic_error("incorrect original table destructor object");
        destruct_table_noop(table);
        cleanup_called=true;
    }
};

} // namespace sin_compat::original_client_recvprop_proxies_factories
