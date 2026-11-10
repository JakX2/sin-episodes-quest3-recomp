#include "sin_compat/original_server_entity_factory.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace e = sin_compat::original_server_entity_factory;
namespace {
struct Host {
    e::FactoryGlobals* globals = nullptr;
    std::vector<std::string> calls{};
    std::uintptr_t allocated = 0x12340u;
    std::uintptr_t edict = 0x98760u;
    std::uintptr_t factory = 0x87650u;
    std::uintptr_t expected_context_at_lookup = 0;
    bool reenter_during_construct = false;
    bool reenter_checked = false;
    unsigned allocations = 0;
    unsigned initializers = 0;
    unsigned destructors = 0;
    std::uintptr_t allocate_registry_storage(std::uint32_t bytes) {
        calls.emplace_back("allocate_800");
        assert(bytes==0x800u);
        assert(globals && (globals->registry.initialization_guard&1u));
        assert(globals->registry.storage.original_vtable_va==e::kRegistryVirtualTableVA);
        assert(globals->registry.storage.capacity_step_0c==0x80u);
        ++allocations;
        if (reenter_during_construct) {
            assert(e::original_registry_singleton(globals->registry,*this)
                   ==&globals->registry.storage);
            reenter_checked = true;
        }
        return allocated;
    }
    void initialize_registry_table(e::RegistryStorage& reg,
                                   std::uint32_t zero, std::uint32_t size,
                                   std::uint32_t one) {
        calls.emplace_back("table_init");
        ++initializers;
        assert(zero==0u&&size==0x80u&&one==1u);
        assert(reg.marker_14==0xffffu&&reg.marker_18==0xffffu);
        assert(reg.marker_16==0u&&reg.marker_1a==0u);
        assert(reg.allocation_08==allocated&&reg.allocation_alias_1c==allocated);
    }
    void register_registry_destructor(std::uint32_t va) {
        calls.emplace_back("register_destructor");
        assert(va==e::kRegistryDestructorVA);++destructors;
    }
    std::uintptr_t engine_create_edict(std::int32_t number) {
        calls.emplace_back("create_edict");
        assert(number==1||number==0||number==-2);
        return edict;
    }
    void report_edict_failure(const char* classname,std::int32_t slot,
                              std::uint32_t fmt) {
        calls.emplace_back("edict_warning");
        assert(std::string(classname)=="flak"&&slot==0);
        assert(fmt==e::kEdictFailureFormatVA);
        assert(globals->current_edict_context==0u);
    }
    std::uintptr_t lookup_class_factory(e::RegistryStorage& registry,
                                        const char* classname,
                                        std::uint32_t slot) {
        calls.emplace_back("lookup");
        assert(&registry==&globals->registry.storage);
        assert(slot==e::kFactoryLookupVirtualOffset);
        assert(std::string(classname)=="flak");
        assert(globals->current_edict_context==expected_context_at_lookup);
        return factory;
    }
    std::uintptr_t invoke_factory(std::uintptr_t addr,std::uint32_t slot) {
        calls.emplace_back(slot==0x24u?"dispatch24":"dispatch20");
        assert(addr==factory);
        assert(slot==0x24u||slot==0x20u);
        if (slot==0x24u) assert(globals->current_edict_context==0u);
        return slot==0x24u?0xabcd1234u:0x21212121u;
    }
};
void test_lazy_guard_reentry_and_native_constructor_fields(){
    e::FactoryGlobals state{};
    state.registry.initialization_guard=0x20u;
    Host host{};host.globals=&state;host.reenter_during_construct=true;
    const auto* value=e::original_registry_singleton(state.registry,host);
    assert(value==&state.registry.storage);
    assert(state.registry.initialization_guard==0x21u);
    assert(host.reenter_checked);
    assert(host.allocations==1u&&host.initializers==1u&&host.destructors==1u);
    assert((host.calls==std::vector<std::string>{"allocate_800","table_init","register_destructor"}));
    assert(value->original_vtable_va==e::kRegistryVirtualTableVA);
    assert(value->original_callback_va==e::kRegistryCallbackVA);
    assert(value->capacity_step_0c==0x80u);
    assert(value->marker_14==0xffffu&&value->marker_18==0xffffu);
    assert(value->marker_16==0&&value->marker_1a==0);
    assert(e::original_registry_singleton(state.registry,host)==value);
    assert(host.calls.size()==3u);
}
void test_factory_explicit_edict_and_default_index(){
    e::FactoryGlobals state{};
    Host host{};host.globals=&state;
    state.current_edict_context=0xdead;host.expected_context_at_lookup=0xdead;
    const auto created=e::original_create_entity_by_name(state,"flak",-1,host);
    assert(created==0xabcd1234u&&state.current_edict_context==0u);
    assert((host.calls==std::vector<std::string>{"allocate_800","table_init",
        "register_destructor","lookup","dispatch24"}));
    host.calls.clear();host.expected_context_at_lookup=host.edict;
    assert(e::original_create_entity_by_name(state,"flak",1,host)==0xabcd1234u);
    assert((host.calls==std::vector<std::string>{"create_edict","lookup","dispatch24"}));
    assert(host.allocations==1&&host.destructors==1);
}
void test_failed_edict_still_dispatches_and_lookup_failure_clears_context(){
    e::FactoryGlobals state{};
    Host host{};host.globals=&state;host.edict=0;
    assert(e::original_create_entity_by_name(state,"flak",0,host)==0xabcd1234u);
    assert((host.calls==std::vector<std::string>{"create_edict","edict_warning",
        "allocate_800","table_init","register_destructor","lookup","dispatch24"}));
    host.calls.clear();host.factory=0;
    state.current_edict_context=1234;host.expected_context_at_lookup=1234;
    assert(e::original_create_entity_by_name(state,"flak",-1,host)==0);
    assert(state.current_edict_context==0);
    assert((host.calls==std::vector<std::string>{"lookup"}));
}
void test_alternate_vtable_dispatch_does_not_modify_edict_context(){
    e::FactoryGlobals state{};
    Host host{};host.globals=&state;
    state.current_edict_context=0xbeef;host.expected_context_at_lookup=0xbeef;
    assert(e::original_secondary_factory_dispatch(state,"flak",host)==0x21212121u);
    assert(state.current_edict_context==0xbeef);
    assert((host.calls==std::vector<std::string>{"allocate_800","table_init",
        "register_destructor","lookup","dispatch20"}));
    host.calls.clear();host.factory=0;
    assert(e::original_secondary_factory_dispatch(state,"flak",host)==0);
    assert((host.calls==std::vector<std::string>{"lookup"}));
    assert(state.current_edict_context==0xbeef);
}
}
int main(){
    test_lazy_guard_reentry_and_native_constructor_fields();
    test_factory_explicit_edict_and_default_index();
    test_failed_edict_still_dispatches_and_lookup_failure_clears_context();
    test_alternate_vtable_dispatch_does_not_modify_edict_context();
}