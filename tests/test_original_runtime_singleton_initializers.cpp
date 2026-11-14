#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_runtime_singleton_initializers.hpp"

namespace n=sin_compat::original_runtime_singleton_initializers;
struct Observer final:n::IObserver {
    std::vector<n::Event> events;
    void event(const n::Event& value) override { events.push_back(value); }
};

static void test_all_four_original_data_initializer_bindings() {
    static_assert(n::kBindings.size()==4u);
    static_assert(n::kNewlyReviewedOriginalFunctions==11u);
    static_assert(n::kNewNativeInstructionBytes==314u);
    static_assert(n::kRemainingUntracedRuntimeSingletons==2u);
    std::size_t server=0,client=0,ctors=0;
    for(const auto& b:n::kBindings) {
        assert(n::find(b.module,b.interface_name)==&b);
        assert(b.data_initializer_cell!=0u);
        assert(b.initializer_va!=0u && b.teardown_wrapper_va!=0u);
        assert(b.final_vtable_slot0!=0u && b.initializer_final_vptr!=0u);
        if(b.module==n::Module::Server)++server;
        else ++client;
        if(b.whole_constructor_sha_verified)++ctors;
    }
    assert(server==1u && client==3u && ctors==3u);
    assert(n::find(n::Module::Client,"clientleafsystem001")==nullptr);
    assert(n::find(n::Module::Client,"ClientLeafSystem001")!=nullptr);
    assert(n::find(n::Module::Server,"ClientLeafSystem001")==nullptr);
    assert(n::find(n::Module::Server,"HLTVDirector001")==nullptr);
    assert(n::find(n::Module::Client,"VClientEntityList003")==nullptr);
}
static void test_movement_construction_overwrites_vptr_once() {
    for(const auto& b:n::kBindings) {
        if(b.constructor_initial_vptr==b.initializer_final_vptr)continue;
        assert(b.interface_name==std::string_view{"GameMovement001"});
        n::State state;Observer o;
        n::observe_original_initializer(b,state,o);
        assert(state.initializer_ran && state.shutdown_registered);
        assert(!state.teardown_called);
        assert(state.observed_vptr_token==b.initializer_final_vptr);
        assert(o.events.size()==4u);
        assert(o.events[0].kind==n::EventKind::ConstructorCall);
        assert(o.events[0].target_va==b.constructor_va);
        assert(o.events[1].kind==n::EventKind::ConstructorVptrObserved);
        assert(o.events[1].vptr_token==b.constructor_initial_vptr);
        assert(o.events[2].kind==n::EventKind::PostConstructorVptrOverwrite);
        assert(o.events[2].vptr_token==b.initializer_final_vptr);
        assert(o.events[3].kind==n::EventKind::TeardownCallbackRegistered);
        assert(o.events[3].target_va==b.shutdown_registrar_va);
        assert(o.events[3].vptr_token==b.teardown_wrapper_va);
        if(b.module==n::Module::Server) {
            assert(b.constructor_va==0x221EBF40u);
            assert(b.constructor_initial_vptr==0x224433D4u);
            assert(b.initializer_final_vptr==0x2245EC04u);
            assert(b.shutdown_registrar_va==0x223A5639u);
        } else {
            assert(b.constructor_va==0x24136F80u);
            assert(b.constructor_initial_vptr==0x2429A684u);
            assert(b.initializer_final_vptr==0x242B1B5Cu);
            assert(b.shutdown_registrar_va==0x24248788u);
        }
        n::observe_original_shutdown(b,state,o);
        assert(o.events.size()==6u);
        assert(o.events[4].kind==n::EventKind::TeardownWrapperInvoked);
        assert(o.events[4].target_va==b.teardown_wrapper_va);
        assert(o.events[5].kind==n::EventKind::OriginalDestructorCalled);
        assert(o.events[5].target_va==b.teardown_method_va);
        assert(state.teardown_called);
    }
}
static void test_leaf_and_prediction_constructor_only_vptr() {
    for(const auto& b:n::kBindings) {
        if(b.constructor_initial_vptr!=b.initializer_final_vptr)continue;
        n::State state;Observer o;
        n::observe_original_initializer(b,state,o);
        assert(o.events.size()==3u);
        assert(o.events[0].kind==n::EventKind::ConstructorCall);
        assert(o.events[1].kind==n::EventKind::ConstructorVptrObserved);
        assert(o.events[2].kind==n::EventKind::TeardownCallbackRegistered);
        if(b.interface_name==std::string_view{"ClientLeafSystem001"}) {
            assert(!b.whole_constructor_sha_verified);
            assert(n::kOriginalClientLeafCtorVA==0x2410C790u);
            assert(b.constructor_initial_vptr==0x2429756Cu);
            assert(n::kOriginalClientLeafSubobjectVptrAt4==0x24297534u);
            assert(n::kOriginalClientLeafSubobjectVptrAt8==0x2429752Cu);
        } else {
            assert(b.interface_name==std::string_view{"VClientPrediction001"});
            assert(b.whole_constructor_sha_verified);
            assert(b.constructor_va==0x24195A10u);
            assert(b.constructor_initial_vptr==0x242A363Cu);
        }
    }
}
static void test_uninitialized_shutdown_and_duplicate_calls_refused() {
    for(const auto& b:n::kBindings) {
        n::State state;Observer o;
        bool refused=false;
        try {n::observe_original_shutdown(b,state,o);}
        catch(const std::logic_error&){refused=true;}
        assert(refused && o.events.empty());
        n::observe_original_initializer(b,state,o);
        const auto count=o.events.size();
        refused=false;
        try {n::observe_original_initializer(b,state,o);}
        catch(const std::logic_error&){refused=true;}
        assert(refused && o.events.size()==count);
        n::observe_original_shutdown(b,state,o);
        refused=false;
        try {n::observe_original_shutdown(b,state,o);}
        catch(const std::logic_error&){refused=true;}
        assert(refused);
    }
}
int main() {
    test_all_four_original_data_initializer_bindings();
    test_movement_construction_overwrites_vptr_once();
    test_leaf_and_prediction_constructor_only_vptr();
    test_uninitialized_shutdown_and_duplicate_calls_refused();
}
