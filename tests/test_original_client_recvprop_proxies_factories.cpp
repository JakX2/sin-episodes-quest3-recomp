#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "sin_compat/original_client_recvprop_proxies_factories.hpp"

namespace native=sin_compat::original_client_recvprop_proxies_factories;
namespace particle=native::particle;

static void test_all_twelve_original_method_VAs_and_proxy_dispatch() {
    static_assert(native::kNewOriginalMethods==12u);
    static_assert(native::kOriginalNativeBytes==466u);
    static_assert(native::kTableDestructorVA==0x24126A20u);
    static_assert(native::kTableRegistrationVA==0x24126A30u);
    static_assert(native::kProxyFloatVA==0x24126C40u);
    static_assert(native::kProxyVector3VA==0x24126C50u);
    static_assert(native::kProxyByteVA==0x24126C70u);
    static_assert(native::kProxyWordVA==0x24126C80u);
    static_assert(native::kProxyInt32VA==0x24126C90u);
    static_assert(native::kProxyDispatchCtorVA==0x24126D00u);
    static_assert(native::kTableDefaultCtorVA==0x24126D30u);
    static_assert(native::kTableArgsCtorVA==0x24126D50u);
    static_assert(native::kFloatFactoryVA==0x24126D80u);
    static_assert(native::kIntegerFactoryVA==0x24126DF0u);
    static_assert(native::kRecvPropNativeStride==60u);
    static_assert(native::kRecvTableInitializedExtent==18u);
    static_assert(native::kProxySourceValueOffset==4u);
    static_assert(native::kDefaultProxyDispatch.size()==5u);
    static_assert(native::kDefaultProxyDispatch[0]==native::kProxyByteVA);
    static_assert(native::kDefaultProxyDispatch[1]==native::kProxyWordVA);
    static_assert(native::kDefaultProxyDispatch[2]==native::kProxyInt32VA);
    static_assert(native::kDefaultProxyDispatch[3]==native::kProxyFloatVA);
    static_assert(native::kDefaultProxyDispatch[4]==native::kProxyVector3VA);
}
static void test_all_original_proxies_copy_bits_not_arithmetic() {
    std::array<std::uint8_t,16> source{{
        0xEEu,0xEEu,0xEEu,0xEEu, // pointer metadata must be ignored
        0x01u,0x00u,0xC0u,0x7Fu, // float quiet NaN with payload
        0x00u,0x00u,0x00u,0x80u, // negative-zero FLOAT bits
        0xFFu,0xFFu,0x7Fu,0x7Fu
    }};
    for(const auto kind:{
        native::ProxyKind::Byte,native::ProxyKind::Word,
        native::ProxyKind::Int32,native::ProxyKind::Float,
        native::ProxyKind::Vector3}) {
        std::array<std::uint8_t,12> destination{};
        destination.fill(0xA5u);
        native::receive_proxy_copy(kind,source,destination);
        const auto size=native::proxy_copy_size(kind);
        assert(size==1u||size==2u||size==4u||size==12u);
        for(std::size_t i=0;i<size;++i)
            assert(destination[i]==source[i+4u]);
        for(std::size_t i=size;i<destination.size();++i)
            assert(destination[i]==0xA5u);
    }
    assert(native::proxy_copy_size(native::ProxyKind::Byte)==1u);
    assert(native::proxy_copy_size(native::ProxyKind::Word)==2u);
    assert(native::proxy_copy_size(native::ProxyKind::Int32)==4u);
    assert(native::proxy_copy_size(native::ProxyKind::Float)==4u);
    assert(native::proxy_copy_size(native::ProxyKind::Vector3)==12u);
    assert(native::proxy_native_VA(native::ProxyKind::Float)==native::kProxyFloatVA);
    assert(native::proxy_native_VA(native::ProxyKind::Int32)==native::kProxyInt32VA);
    bool bad=false;
    try{(void)native::proxy_copy_size(static_cast<native::ProxyKind>(99));}
    catch(const std::invalid_argument&){bad=true;}
    assert(bad);
    bad=false;
    try{(void)native::proxy_native_VA(static_cast<native::ProxyKind>(99));}
    catch(const std::invalid_argument&){bad=true;}
    assert(bad);
}
static void verify_common_known_bytes(
    const native::RecvPropKnown& p,std::uint32_t name,
    std::uint32_t kind,std::uint32_t off,
    std::uint32_t extra,std::uint32_t proxy) {
    assert(p.read32(0u)==name);
    assert(p.read32(4u)==kind);
    assert(p.read32(8u)==extra);
    assert(p.is_defined(16u)&&p.bytes[16u]==0u);
    for(const auto zero_off:{20u,24u,28u,36u,40u})
        assert(p.read32(zero_off)==0u);
    assert(p.read32(32u)==proxy);
    assert(p.read32(44u)==off);
    assert(p.read32(48u)==0xFFFFFFFFu);
    assert(p.read32(52u)==1u);
    assert(p.defined.count()==49u);
    // The retail code copied 15 DWORDs from an incompletely initialized
    // stack descriptor. It did NOT establish values for these 11 bytes.
    for(const auto unknown:{12u,13u,14u,15u,17u,18u,19u,56u,57u,58u,59u})
        assert(!p.is_defined(unknown));
    bool caught=false;
    try{(void)p.read32(12u);}
    catch(const std::logic_error&){caught=true;}
    assert(caught);
    caught=false;
    try{(void)p.read32(56u);}
    catch(const std::logic_error&){caught=true;}
    assert(caught);
}
static void test_float_factory_type_and_ignored_width_argument() {
    const auto p=native::construct_float_property(
        0x24278E80u,8u,4u,0x11223344u,native::kProxyFloatVA);
    verify_common_known_bytes(p,0x24278E80u,1u,8u,
                              0x11223344u,native::kProxyFloatVA);
    // Native 0x24126D80 never reads its fourth stack argument, so
    // varying that input cannot affect proven descriptor fields.
    const auto different_width=native::construct_float_property(
        0x24278E80u,8u,999u,0x11223344u,native::kProxyFloatVA);
    assert(different_width.defined==p.defined);
    assert(different_width.bytes==p.bytes);
    const auto no_proxy=native::construct_float_property(13u,24u,4u,0u,0u);
    verify_common_known_bytes(no_proxy,13u,1u,24u,0u,0u);
}
static void test_placeholder_fallback_widths_and_explicit_proxy() {
    for(const auto width:{0u,1u,2u,3u,4u,8u,16u}) {
        const auto result=native::construct_integer_placeholder_property(
            0x242762D8u,0u,width,7u,0u);
        const std::uint32_t expected=(width==2u)?native::kProxyWordVA:
             (width==4u)?native::kProxyInt32VA:native::kProxyByteVA;
        verify_common_known_bytes(result,0x242762D8u,0u,0u,7u,expected);
        assert(native::default_integer_proxy(width)==expected);
        // An explicitly supplied nonzero callback pointer overrides
        // all size-based default selection even for size=2 or size=4.
        const auto override_prop=native::construct_integer_placeholder_property(
            21u,12u,width,55u,0x1234ABCDu);
        verify_common_known_bytes(override_prop,21u,0u,12u,55u,0x1234ABCDu);
    }
    native::RecvPropKnown bytes;
    bool invalid=false;
    try{(void)bytes.read32(59u);}
    catch(const std::out_of_range&){invalid=true;}
    assert(invalid);
    invalid=false;
    try{bytes.write32(57u,3u);}
    catch(const std::out_of_range&){invalid=true;}
    assert(invalid);
    invalid=false;
    try{(void)bytes.is_defined(60u);}
    catch(const std::out_of_range&){invalid=true;}
    assert(invalid);
}
static void verify_table(const native::RecvTableKnown& table,
                         std::uint32_t first,std::uint32_t count,
                         std::uint32_t name) {
    assert(table.read32(0u)==first);
    assert(table.read32(4u)==count);
    assert(table.read32(8u)==0u);
    assert(table.read32(12u)==name);
    assert(table.is_defined(16u)&&table.bytes[16u]==0u);
    assert(table.is_defined(17u)&&table.bytes[17u]==0u);
    assert(table.defined.count()==18u);
}
static void test_table_default_arg_registration_and_trivial_destructor() {
    auto t=native::construct_default_table();
    verify_table(t,0u,0u,0u);
    auto args=native::table_from_args(0x12345678u,7u,0xABCD1234u);
    verify_table(args,0x12345678u,7u,0xABCD1234u);
    native::register_table(t,0x24360F8Cu,3u,0x24298EF8u);
    verify_table(t,0x24360F8Cu,3u,0x24298EF8u);
    native::destruct_table_noop(t);
    verify_table(t,0x24360F8Cu,3u,0x24298EF8u);
}
static void test_phase250_particletrail_init_uses_phase251_factories() {
    particle::Globals globals{};
    native::ParticleTrailRuntimeAdapter callbacks;
    particle::register_crt_table_lifetime(callbacks);
    assert(callbacks.object_constructed);
    assert(callbacks.cleanup_registered);
    assert(callbacks.registered_cleanup==particle::kNativeCleanupPointerVA);
    verify_table(callbacks.table,0u,0u,0u);
    particle::run_crt_table_initializer(globals,callbacks);
    assert(globals.once_flags==1u&&globals.crt_initialized_result==1u);
    assert(callbacks.register_count==1u);
    verify_table(callbacks.table,0x24360F8Cu,3u,0x24298EF8u);
    verify_common_known_bytes(callbacks.descriptors[0],
        particle::kNativePlaceholderNameVA,0u,0u,0u,native::kProxyInt32VA);
    for(std::size_t i=0;i<particle::kFloatProperties.size();++i) {
        const auto& field=particle::kFloatProperties[i];
        verify_common_known_bytes(callbacks.descriptors[i+1u],
            field.original_name_VA,1u,field.original_member_offset,0u,
            native::kProxyFloatVA);
    }
    const auto descriptor_snapshot=callbacks.descriptors[2].bytes;
    particle::run_crt_table_initializer(globals,callbacks);
    assert(callbacks.register_count==2u);
    assert(callbacks.descriptors[2].bytes==descriptor_snapshot);
    particle::cleanup_crt_table(callbacks);
    assert(callbacks.cleanup_called);
    verify_table(callbacks.table,0x24360F8Cu,3u,0x24298EF8u);
    bool rejected=false;
    try{(void)native::ParticleTrailRuntimeAdapter::index_for(0xDEADBEEFu);}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected);
    rejected=false;
    try{callbacks.construct_float(particle::kNativeFirstNetworkDescriptorVA,
                                  particle::kNativeLifetimeNameVA,8u,4u,0x1234u);}
    catch(const std::logic_error&){rejected=true;}
    assert(rejected);
}
int main() {
    test_all_twelve_original_method_VAs_and_proxy_dispatch();
    test_all_original_proxies_copy_bits_not_arithmetic();
    test_float_factory_type_and_ignored_width_argument();
    test_placeholder_fallback_widths_and_explicit_proxy();
    test_table_default_arg_registration_and_trivial_destructor();
    test_phase250_particletrail_init_uses_phase251_factories();
    return 0;
}
