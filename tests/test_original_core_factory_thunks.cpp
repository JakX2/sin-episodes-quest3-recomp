#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>
#include "sin_compat/original_core_factory_thunks.hpp"
namespace n=sin_compat::original_core_factory_thunks;

static void test_inventory_complete_without_double_count() {
    static_assert(n::kOriginalThunkBytes==6u);
    static_assert(n::kOriginalPostThunkINT3Bytes==10u);
    static_assert(n::kOriginalTotalCoreFactories==18u);
    static_assert(n::kPreviouslyReviewedPhase178Bodies==9u);
    static_assert(n::kNewPhase226FactoryBodies==9u);
    static_assert(n::kSeparateClientAllocatingMaterialProxies==47u);
    assert(n::kBindings.size()==18u && n::newly_reviewed_count()==9u);
    std::size_t server=0,client=0,initialized=0,new_server=0,new_client=0;
    for(const auto& row:n::kBindings) {
        if(row.module==n::Module::Server) {
            ++server;if(row.newly_verified_phase226)++new_server;
        } else {
            ++client;if(row.newly_verified_phase226)++new_client;
        }
        if(row.original_vptr_va!=0u) {
            ++initialized;
            assert(row.original_first_virtual_method_va!=0u);
        } else assert(row.original_first_virtual_method_va==0u);
        assert(n::registered_core_factory(row.module,row.interface_name)==&row);
        assert(n::singleton_return_token(row,n::original_image_base(row.module))==row.singleton_va);
    }
    assert(server==10u && client==8u);
    assert(new_server==6u && new_client==3u);
    assert(initialized==12u);
}

static void test_original_interface_case_and_version_strictness() {
    using n::Module;
    assert(n::registered_core_factory(Module::Server,"ServerGameDLL003")!=nullptr);
    assert(n::registered_core_factory(Module::Client,"ServerGameDLL003")==nullptr);
    assert(n::registered_core_factory(Module::Client,"VClient011")!=nullptr);
    assert(n::registered_core_factory(Module::Client,"vclient011")==nullptr);
    assert(n::registered_core_factory(Module::Client,"VClient01")==nullptr);
    assert(n::registered_core_factory(Module::Client,"VClient011tail")==nullptr);
    assert(n::registered_core_factory(Module::Server,"GameMovement001")!=nullptr);
    assert(n::registered_core_factory(Module::Client,"GameMovement001")!=nullptr);
    assert(n::registered_core_factory(Module::Client,"Alpha_IMaterialProxy003")==nullptr);
    assert(n::registered_core_factory(Module::Server,"unknown")==nullptr);
}

static void test_duplicate_server_interface_versions_same_singleton() {
    using n::Module;
    const auto* first=n::registered_core_factory(Module::Server,"PlayerInfoManager001");
    const auto* second=n::registered_core_factory(Module::Server,"PlayerInfoManager002");
    assert(first!=nullptr && second!=nullptr && first!=second);
    assert(first->factory_va!=second->factory_va);
    assert(first->singleton_va==0x2251A048u);
    assert(second->singleton_va==0x2251A048u);
    assert(first->original_vptr_va==second->original_vptr_va);
    const std::uint32_t rebased=0x26000000u;
    assert(n::singleton_return_token(*first,rebased)==
           n::singleton_return_token(*second,rebased));
    assert(n::singleton_return_token(*first,rebased)==0x2651A048u);
}

static void test_original_relocation_requirements_and_module_independence() {
    using n::Module;
    const auto* server=n::registered_core_factory(Module::Server,"IEffects001");
    const auto* client=n::registered_core_factory(Module::Client,"IEffects001");
    assert(server!=nullptr && client!=nullptr);
    assert(server->singleton_va!=client->singleton_va);
    assert(n::original_image_base(Module::Server)==0x22000000u);
    assert(n::original_image_base(Module::Client)==0x24000000u);
    assert(n::singleton_return_token(*server,0x23000000u)==0x234ECE00u);
    assert(n::singleton_return_token(*client,0x25000000u)==0x252FB554u);
    assert(n::file_backed_vptr_token(*server,0x23000000u)==0x234059B8u);
    assert(n::file_backed_vptr_token(*client,0x25000000u)==0x25298E54u);
    // Static singleton address is a token, never a dereferenceable host pointer.
    const auto* leaf=n::registered_core_factory(Module::Client,"ClientLeafSystem001");
    assert(leaf!=nullptr && leaf->original_vptr_va==0u);
    bool refused=false;
    try {n::file_backed_vptr_token(*leaf,0x24000000u);}
    catch(const std::logic_error&) {refused=true;}
    assert(refused);
    refused=false;
    try {n::rebased_address_token(Module::Client,0x21FFFFFFu,0x24000000u);}
    catch(const std::out_of_range&) {refused=true;}
    assert(refused);
    refused=false;
    try {n::singleton_return_token(*client,0xFFFFFFFFu);}
    catch(const std::overflow_error&) {refused=true;}
    assert(refused);
}

int main() {
    test_inventory_complete_without_double_count();
    test_original_interface_case_and_version_strictness();
    test_duplicate_server_interface_versions_same_singleton();
    test_original_relocation_requirements_and_module_independence();
}
