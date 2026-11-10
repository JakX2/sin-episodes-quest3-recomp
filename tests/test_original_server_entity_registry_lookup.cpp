#include "sin_compat/original_server_entity_registry_lookup.hpp"
#include "sin_compat/original_server_entity_factory.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace r=sin_compat::original_server_entity_registry_lookup;
namespace e=sin_compat::original_server_entity_factory;
namespace {
struct Host {
    std::vector<std::string> calls{};
    std::uint16_t interned=0x345u;
    std::uint16_t index=3u;
    std::uintptr_t provider=0x11114000u;
    std::uintptr_t created_factory=0x99991000u;
    std::uintptr_t edict=0;
    e::FactoryGlobals* globals=nullptr;
    bool check_edict_context=false;
    std::uintptr_t expected_context=0;
    unsigned native_allocations=0;
    std::uint16_t intern_class_symbol(r::RegistryToken registry,const char* name) {
        calls.emplace_back("intern");
        assert(registry.native_instance==0x2258ffb0u);
        assert(std::string(name)=="flak");
        return interned;
    }
    std::uint16_t lookup_class_symbol_index(r::RegistryToken reg,std::uint16_t symbol,std::uint32_t zero) {
        calls.emplace_back("lookup_index");
        assert(reg.native_instance==0x2258ffb0u&&symbol==interned&&zero==0u);
        return index;
    }
    std::uintptr_t read_provider_from_record(r::RegistryToken reg,std::uint16_t slot,
        std::uint32_t stride,std::uint32_t word_offset) {
        calls.emplace_back("read_record");
        assert(reg.native_instance==0x2258ffb0u&&slot==index);
        assert(stride==16u&&word_offset==12u);
        return provider;
    }
    void report_unknown_create(const char* classname,std::uint32_t format){
        calls.emplace_back("unknown_create");
        assert(std::string(classname)=="flak"&&format==0x22455f50u);
    }
    void report_unknown_destroy(const char* classname,std::uint32_t format){
        calls.emplace_back("unknown_destroy");
        assert(std::string(classname)=="flak"&&format==0x22455f80u);
    }
    std::uintptr_t call_registered_provider_create(std::uintptr_t record,const char* classname){
        calls.emplace_back("provider_v0");
        assert(record==provider&&std::string(classname)=="flak");
        if(check_edict_context)assert(globals->current_edict_context==expected_context);
        return created_factory;
    }
    std::uintptr_t call_registered_provider_destroy(std::uintptr_t record,std::uintptr_t argument){
        calls.emplace_back("provider_v4");
        assert(record==provider&&argument==0x77889900u);
        return 0x33110044u;
    }
    // Phase170 upstream interface, to check the recovered lookup in situ.
    std::uintptr_t allocate_registry_storage(std::uint32_t size){
        calls.emplace_back("allocate_registry");assert(size==0x800u);
        ++native_allocations;return 0x30000u;
    }
    void initialize_registry_table(e::RegistryStorage& storage,std::uint32_t zero,
                                   std::uint32_t size,std::uint32_t one){
        calls.emplace_back("init_registry");
        assert(storage.original_vtable_va==r::kRegistryVtableVA);
        assert(zero==0u&&size==0x80u&&one==1u);
    }
    void register_registry_destructor(std::uint32_t callback){
        calls.emplace_back("destructor"); assert(callback==e::kRegistryDestructorVA);
    }
    std::uintptr_t engine_create_edict(std::int32_t number){
        calls.emplace_back("create_edict");assert(number==4);return edict;
    }
    void report_edict_failure(const char*,std::int32_t id,std::uint32_t){
        calls.emplace_back("edict_failure");assert(id==4);
    }
    std::uintptr_t lookup_class_factory(e::RegistryStorage& reg,const char* name,std::uint32_t slot){
        calls.emplace_back("registry_v4");
        assert(slot==4u&&reg.original_vtable_va==r::kRegistryVtableVA);
        return r::original_resolve_create_factory({e::kRegistrySingletonVA},name,*this);
    }
    std::uintptr_t invoke_factory(std::uintptr_t ptr,std::uint32_t slot){
        calls.emplace_back("factory_v24");
        assert(ptr==created_factory&&slot==0x24u);
        assert(globals->current_edict_context==0u);
        return 0xabcdu;
    }
};
void test_internal_record_hit_and_missing(){
    Host h{};r::RegistryToken reg{0x2258ffb0u};
    assert(r::original_find_registered_provider(reg,"flak",h)==h.provider);
    assert((h.calls==std::vector<std::string>{"intern","lookup_index","read_record"}));
    h.calls.clear();h.index=0xffffu;
    assert(r::original_find_registered_provider(reg,"flak",h)==0u);
    assert((h.calls==std::vector<std::string>{"intern","lookup_index"}));
}
void test_create_vs_destroy_wrapper(){
    Host h{};r::RegistryToken reg{0x2258ffb0u};
    assert(r::original_resolve_create_factory(reg,"flak",h)==h.created_factory);
    assert((h.calls==std::vector<std::string>{"intern","lookup_index","read_record","provider_v0"}));
    h.calls.clear();h.index=0xffffu;
    assert(r::original_resolve_create_factory(reg,"flak",h)==0u);
    assert((h.calls==std::vector<std::string>{"intern","lookup_index","unknown_create"}));
    h.calls.clear();h.index=2u;h.provider=0u;
    assert(r::original_resolve_create_factory(reg,"flak",h)==0u);
    assert((h.calls==std::vector<std::string>{"intern","lookup_index","read_record","unknown_create"}));
    h.calls.clear();h.provider=0x11114000u;
    assert(r::original_resolve_destroy_factory(reg,"flak",0x77889900u,h)==0x33110044u);
    assert((h.calls==std::vector<std::string>{"intern","lookup_index","read_record","provider_v4"}));
    h.calls.clear();h.index=0xffffu;
    assert(r::original_resolve_destroy_factory(reg,"flak",0x77889900u,h)==0u);
    assert((h.calls==std::vector<std::string>{"intern","lookup_index","unknown_destroy"}));
}
void test_original_phase170_factory_chain(){
    e::FactoryGlobals state{};
    Host h{};h.globals=&state;h.edict=0x876u;
    h.check_edict_context=true;h.expected_context=0x876u;
    assert(e::original_create_entity_by_name(state,"flak",4,h)==0xabcdu);
    assert(state.current_edict_context==0u&&h.native_allocations==1u);
    assert((h.calls==std::vector<std::string>{
        "create_edict","allocate_registry","init_registry","destructor",
        "registry_v4","intern","lookup_index","read_record","provider_v0","factory_v24"}));
    h.calls.clear();h.check_edict_context=false;h.index=0xffffu;
    assert(e::original_create_entity_by_name(state,"flak",-1,h)==0u);
    assert((h.calls==std::vector<std::string>{
        "registry_v4","intern","lookup_index","unknown_create"}));
    assert(state.current_edict_context==0u);
}
}
int main(){
    test_internal_record_hit_and_missing();
    test_create_vs_destroy_wrapper();
    test_original_phase170_factory_chain();
}