#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "sin_compat/original_client_particle_trail_recvtable_crt.hpp"

namespace particle=sin_compat::original_client_particle_trail_recvtable_crt;

enum class Kind { Placeholder, Float, Table, Construct, RegisterExit, Destroy };
struct Event {
    Kind kind;
    std::uint32_t a{};
    std::uint32_t b{};
    std::uint32_t c{};
    std::uint32_t d{};
    std::uint32_t e{};
};
struct Spy final:particle::RuntimeCallbacks {
    std::vector<Event> events;
    bool fail_first_placeholder{};
    void construct_placeholder(std::uint32_t desc,std::uint32_t name,
                               std::uint32_t member,std::uint32_t size) override {
        events.push_back({Kind::Placeholder,desc,name,member,size,0u});
        if(fail_first_placeholder)
            throw std::runtime_error("synthetic descriptor factory failure");
    }
    void construct_float(std::uint32_t desc,std::uint32_t name,
                         std::uint32_t member,std::uint32_t size,
                         std::uint32_t proxy) override {
        events.push_back({Kind::Float,desc,name,member,size,proxy});
    }
    void register_table(std::uint32_t object,std::uint32_t descriptors,
                        std::uint32_t count,std::uint32_t name) override {
        events.push_back({Kind::Table,object,descriptors,count,name,0u});
    }
    void construct_table_object(std::uint32_t object) override {
        events.push_back({Kind::Construct,object});
    }
    void register_termination_handler(std::uint32_t cleanup) override {
        events.push_back({Kind::RegisterExit,cleanup});
    }
    void destruct_table_object(std::uint32_t object) override {
        events.push_back({Kind::Destroy,object});
    }
};
static void test_authentic_native_method_addresses_and_recv_fields() {
    static_assert(particle::kWholeNativeMethods==4u);
    static_assert(particle::kWholeNativeCodeBytes==199u);
    static_assert(particle::kNativeRecvtableInitVA==0x241281D0u);
    static_assert(particle::kNativeCrtLifetimeRegistrationVA==0x24268960u);
    static_assert(particle::kNativeCrtInitializeVA==0x24268980u);
    static_assert(particle::kNativeCrtCleanupThunkVA==0x242736B0u);
    static_assert(particle::kNativePlaceholderFactoryVA==0x24126DF0u);
    static_assert(particle::kNativeFloatFactoryVA==0x24126D80u);
    static_assert(particle::kNativeFloatRecvProxyVA==0x24126C40u);
    static_assert(particle::kNativeTableRegisterVA==0x24126A30u);
    static_assert(particle::kNativeTableConstructVA==0x24126D30u);
    static_assert(particle::kNativeTableDestructVA==0x24126A20u);
    static_assert(particle::kNativeCrtTerminationRegistrationVA==0x24248788u);
    static_assert(particle::kNativeInitOnceFlagsVA==0x24361040u);
    static_assert(particle::kNativePlaceholderDescriptorVA==0x24360F50u);
    static_assert(particle::kNativeRecvTableObjectVA==0x24361044u);
    static_assert(particle::kNativeCrtResultVA==0x24361058u);
    static_assert(particle::kNativeCrtPointer0VA==0x242F3A38u);
    static_assert(particle::kNativeCrtPointer1VA==0x242F3A3Cu);
    static_assert(particle::kNativeRealRecvPropCount==3u);
    static_assert(particle::kNativeRecvPropDescriptorStride==60u);
    static_assert(particle::kNativeFloatSizeBytes==4u);
    static_assert(particle::kFloatProperties.size()==3u);
    static_assert(particle::kFloatProperties[0].original_member_offset==8u);
    static_assert(particle::kFloatProperties[1].original_member_offset==12u);
    static_assert(particle::kFloatProperties[2].original_member_offset==16u);
    static_assert(particle::kFloatProperties[1].original_descriptor_VA-
                  particle::kFloatProperties[0].original_descriptor_VA==60u);
    static_assert(particle::kFloatProperties[2].original_descriptor_VA-
                  particle::kFloatProperties[1].original_descriptor_VA==60u);
    assert(std::string(particle::kNativeRecvTableName)=="DT_EntityParticleTrailInfo");
    assert(std::string(particle::kNativePlaceholderName)=="should_never_see_this");
    assert(std::string(particle::kFloatProperties[0].property_name)=="m_flLifetime");
    assert(std::string(particle::kFloatProperties[1].property_name)=="m_flStartSize");
    assert(std::string(particle::kFloatProperties[2].property_name)=="m_flEndSize");
}
static void test_first_registration_is_placeholder_three_float_props_then_recvtable() {
    particle::Globals globals{};
    Spy spy;
    assert(particle::initialize_recv_table(globals,spy)==1u);
    assert(globals.once_flags==1u);
    assert(globals.crt_initialized_result==0u); // direct call did NOT cache it
    assert(spy.events.size()==5u);
    const auto& p=spy.events[0];
    assert(p.kind==Kind::Placeholder);
    assert(p.a==particle::kNativePlaceholderDescriptorVA);
    assert(p.b==particle::kNativePlaceholderNameVA);
    assert(p.c==0u&&p.d==4u);
    for(std::size_t i=0;i<3u;++i) {
        const auto& e=spy.events[i+1u];
        const auto& field=particle::kFloatProperties[i];
        assert(e.kind==Kind::Float);
        assert(e.a==field.original_descriptor_VA);
        assert(e.b==field.original_name_VA);
        assert(e.c==field.original_member_offset);
        assert(e.d==particle::kNativeFloatSizeBytes);
        assert(e.e==particle::kNativeFloatRecvProxyVA);
    }
    const auto& table=spy.events[4u];
    assert(table.kind==Kind::Table);
    assert(table.a==particle::kNativeRecvTableObjectVA);
    assert(table.b==particle::kNativeFirstNetworkDescriptorVA);
    assert(table.c==3u&&table.d==particle::kNativeTableNameVA);
    // The placeholder occupies its own descriptor and must never be
    // added to the table's first-pointer+count=3 contiguous range.
    assert(particle::kNativePlaceholderDescriptorVA+60u==
           particle::kNativeFirstNetworkDescriptorVA);
}
static void test_repeated_initializer_preserves_other_flag_bits_and_re_registers() {
    particle::Globals g{};
    g.once_flags=0xA0u;
    Spy spy;
    assert(particle::initialize_recv_table(g,spy)==1u);
    assert(g.once_flags==0xA1u);
    assert(spy.events.size()==5u);
    assert(particle::initialize_recv_table(g,spy)==1u);
    assert(g.once_flags==0xA1u);
    assert(spy.events.size()==6u);
    assert(spy.events.back().kind==Kind::Table);
    // With flag already set, the original skips all 4 construction
    // callbacks but still calls native recvtable registration.
    particle::Globals already{};
    already.once_flags=3u;
    Spy other;
    assert(particle::initialize_recv_table(already,other)==1u);
    assert(already.once_flags==3u);
    assert(other.events.size()==1u);
    assert(other.events[0].kind==Kind::Table);
}
static void test_real_crt_initializer_and_lifetime_callbacks_are_separate() {
    particle::Globals g{};
    Spy spy;
    particle::register_crt_table_lifetime(spy);
    assert(spy.events.size()==2u);
    assert(spy.events[0].kind==Kind::Construct);
    assert(spy.events[0].a==particle::kNativeRecvTableObjectVA);
    assert(spy.events[1].kind==Kind::RegisterExit);
    assert(spy.events[1].a==particle::kNativeCleanupPointerVA);
    particle::run_crt_table_initializer(g,spy);
    assert(g.once_flags==1u&&g.crt_initialized_result==1u);
    assert(spy.events.size()==7u);
    assert(spy.events[2].kind==Kind::Placeholder);
    assert(spy.events[6].kind==Kind::Table);
    particle::cleanup_crt_table(spy);
    assert(spy.events.size()==8u);
    assert(spy.events[7].kind==Kind::Destroy);
    assert(spy.events[7].a==particle::kNativeRecvTableObjectVA);
    // The original initializer pointer table shows the two wrapper
    // function pointers in adjacent .data locations, not a guarantee
    // that these independent callbacks are never invoked again.
    particle::run_crt_table_initializer(g,spy);
    assert(g.crt_initialized_result==1u&&spy.events.size()==9u);
    assert(spy.events[8].kind==Kind::Table);
}
static void test_original_once_bit_is_set_before_opaque_factory_call() {
    particle::Globals g{};
    Spy spy;spy.fail_first_placeholder=true;
    bool caught=false;
    try{(void)particle::initialize_recv_table(g,spy);}
    catch(const std::runtime_error&){caught=true;}
    assert(caught);
    assert(g.once_flags==1u);
    assert(spy.events.size()==1u);
    assert(spy.events[0].kind==Kind::Placeholder);
    // This mirrors original OR flag before CALL: retries should NOT
    // recreate partly constructed descriptors. This is an exceptional
    // injected synthetic host scenario, not a claim about real engine error.
    spy.fail_first_placeholder=false;
    assert(particle::initialize_recv_table(g,spy)==1u);
    assert(spy.events.size()==2u);
    assert(spy.events[1].kind==Kind::Table);
}
int main() {
    test_authentic_native_method_addresses_and_recv_fields();
    test_first_registration_is_placeholder_three_float_props_then_recvtable();
    test_repeated_initializer_preserves_other_flag_bits_and_re_registers();
    test_real_crt_initializer_and_lifetime_callbacks_are_separate();
    test_original_once_bit_is_set_before_opaque_factory_call();
    return 0;
}
