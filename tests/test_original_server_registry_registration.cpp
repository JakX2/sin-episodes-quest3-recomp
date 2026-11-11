#include "sin_compat/original_server_registry_registration.hpp"
#include "sin_compat/original_server_entity_factory.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace s=sin_compat::original_server_registry_registration;
namespace f=sin_compat::original_server_entity_factory;
namespace {
struct Host {
    std::vector<std::string> calls{};
    std::uint16_t symbol=0x31;
    std::uint16_t existing_slot=3;
    std::uint16_t registered_slot=3;
    std::uint32_t registered_value=0;
    std::uint32_t* mutable_source=nullptr;
    bool change_source_after_slot=false;
    std::uintptr_t expected_key=0x2258ffb0;
    f::FactoryGlobals* globals=nullptr;
    std::uintptr_t observed_edict=0;
    std::uint32_t write_count=0;
    std::uint32_t registered_count=0;
    std::uint16_t intern_symbol(s::RegistryKey key,const char* classname) {
        calls.emplace_back("intern");
        assert(key.registry_instance==expected_key);
        assert(std::string(classname)=="flak");
        return symbol;
    }
    std::uint16_t find_existing_slot(s::RegistryKey key,
                                     std::uint16_t sym,std::uint32_t zero){
        calls.emplace_back("find_existing");
        assert(key.registry_instance==expected_key&&sym==symbol&&zero==0);
        return existing_slot;
    }
    std::uint16_t find_or_insert_slot(s::RegistryKey key,std::uint16_t sym){
        calls.emplace_back("find_or_insert");
        assert(key.registry_instance==expected_key&&sym==symbol);
        if(change_source_after_slot&&mutable_source)*mutable_source=0x77665544u;
        return registered_slot;
    }
    void write_provider_word(s::RegistryKey key,std::uint16_t slot,
                             std::uint32_t stride,std::uint32_t off,
                             std::uint32_t value){
        calls.emplace_back("write_provider");
        assert(key.registry_instance==expected_key);
        assert(slot==registered_slot&&stride==16u&&off==12u);
        registered_value=value; ++write_count;
    }
    // Phase170 integration: exposes registered provider through existing
    // original CreateEntityByName entry without assuming native allocator.
    std::uintptr_t allocate_registry_storage(std::uint32_t n){
        calls.emplace_back("allocate");assert(n==0x800u);return 0x8888u;
    }
    void initialize_registry_table(f::RegistryStorage& reg,
                                   std::uint32_t a,std::uint32_t b,std::uint32_t c){
        calls.emplace_back("init");assert(a==0&&b==0x80u&&c==1u);
        assert(reg.original_vtable_va==f::kRegistryVirtualTableVA);
    }
    void register_registry_destructor(std::uint32_t va){
        calls.emplace_back("destructor_register");assert(va==f::kRegistryDestructorVA);
    }
    std::uintptr_t engine_create_edict(std::int32_t id){
        calls.emplace_back("edict");assert(id==4);return 0x5555u;
    }
    void report_edict_failure(const char*,std::int32_t,std::uint32_t){
        assert(false&&"unexpected edict failure");
    }
    std::uintptr_t lookup_class_factory(f::RegistryStorage&,const char* name,std::uint32_t slot){
        calls.emplace_back("factory_lookup");
        assert(std::string(name)=="flak"&&slot==4u);
        assert(globals);
        observed_edict=globals->current_edict_context;
        // The registered provider +0 would turn this record into a factory
        // object. The value below is an opaque mock return from that virtual.
        return registered_value ? 0x9999u : 0u;
    }
    std::uintptr_t invoke_factory(std::uintptr_t fac,std::uint32_t slot){
        calls.emplace_back("factory_v24");
        assert(fac==0x9999u&&slot==0x24u);
        assert(globals&&globals->current_edict_context==0u);
        return 0x12345678u;
    }
};
void test_hash_native_signed_chars_and_wrap(){
    assert(s::original_classname_hash("")==0x1505u);
    assert(s::original_classname_hash("A")==0x2b5e6u);
    assert(s::original_classname_hash("abc")==0xb885c8bu);
    assert(s::original_classname_hash("flak")==0x7c96d683u);
    const char high[]={static_cast<char>(0x80),0};
    assert(s::original_classname_hash(high)==0x2b525u);
    const char ff[]={static_cast<char>(0xff),0};
    assert(s::original_classname_hash(ff)==0x2b5a4u);
    assert(s::original_classname_hash("AAAAAAAAAAAAAAAAAAAAAAAA")==0xb0da1c9du);
}
void test_existing_slot_and_missing_sentinel(){
    Host h{};s::RegistryKey reg{h.expected_key};
    assert(s::original_lookup_existing_slot(reg,"flak",h)==3u);
    assert((h.calls==std::vector<std::string>{"intern","find_existing"}));
    h.calls.clear();h.existing_slot=0xffffu;
    assert(s::original_lookup_existing_slot(reg,"flak",h)==0xffffu);
    assert((h.calls==std::vector<std::string>{"intern","find_existing"}));
}
void test_direct_and_indirect_registration_order(){
    Host h{};s::RegistryKey reg{h.expected_key};
    assert(s::original_register_direct_provider(reg,"flak",0x11223344u,h)==3);
    assert(h.registered_value==0x11223344u&&h.write_count==1u);
    assert((h.calls==std::vector<std::string>{"intern","find_or_insert","write_provider"}));
    h.calls.clear();
    std::uint32_t word=0xabcdef01u;
    h.mutable_source=&word;h.change_source_after_slot=true;
    // Native indirect path reads *second_arg only after get-or-insert.
    assert(s::original_register_indirect_provider(reg,"flak",&word,h)==3);
    assert(word==0x77665544u&&h.registered_value==0x77665544u);
    assert((h.calls==std::vector<std::string>{"intern","find_or_insert","write_provider"}));
    // Both native paths lack bounds checks and still attempt a write to
    // +0x0c even if host lookup returned the invalid 16-bit sentinel.
    h.calls.clear();h.registered_slot=0xffffu;
    assert(s::original_register_direct_provider(reg,"flak",5u,h)==0xffffu);
    assert(h.write_count==3u&&h.registered_value==5u);
}
void test_phase170_created_flak_uses_record(){
    f::FactoryGlobals globals{};
    Host h{};h.globals=&globals;
    s::RegistryKey reg{h.expected_key};
    s::original_register_direct_provider(reg,"flak",0x43434343u,h);
    h.calls.clear();
    assert(f::original_create_entity_by_name(globals,"flak",4,h)==0x12345678u);
    assert(h.observed_edict==0x5555u&&globals.current_edict_context==0u);
    assert((h.calls==std::vector<std::string>{"edict","allocate","init",
            "destructor_register","factory_lookup","factory_v24"}));
    h.calls.clear();
    assert(f::original_create_entity_by_name(globals,"flak",-1,h)==0x12345678u);
    assert((h.calls==std::vector<std::string>{"factory_lookup","factory_v24"}));
}
}
int main(){
    test_hash_native_signed_chars_and_wrap();
    test_existing_slot_and_missing_sentinel();
    test_direct_and_indirect_registration_order();
    test_phase170_created_flak_uses_record();
}