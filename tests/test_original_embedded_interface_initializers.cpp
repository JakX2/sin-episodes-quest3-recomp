#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>
#include "sin_compat/original_embedded_interface_initializers.hpp"

namespace native=sin_compat::original_embedded_interface_initializers;
struct Recorder final:native::IObserver {
    std::vector<native::Event> events;
    void on_event(const native::Event& value) override {events.push_back(value);}
};
static void original_embedded_factory_and_object_layout() {
    static_assert(native::kBindings.size()==2u);
    static_assert(native::kNewFullyShaReviewedOriginalMethods==6u);
    static_assert(native::kNewOriginalNativeCodeBytes==486u);
    static_assert(native::kOriginalCoreUnbackedStartupChainsTotal==6u);
    static_assert(native::kOriginalCoreUnbackedStartupChainsUntraced==0u);
    for(const auto& b:native::kBindings) {
        assert(native::find(b.module,b.interface_name)==&b);
        assert(native::original_interface_return_va(b)==
               b.containing_object_va+b.embedded_offset);
        assert(b.final_interface_method0_va!=0u);
    }
    const auto* s=native::find(native::Module::Server,"HLTVDirector001");
    const auto* c=native::find(native::Module::Client,"VClientEntityList003");
    assert(s!=nullptr && c!=nullptr);
    assert(s->containing_object_va==0x2257AE98u);
    assert(s->embedded_offset==0x08u);
    assert(native::original_interface_return_va(*s)==0x2257AEA0u);
    assert(s->factory_va==0x2225F8C0u);
    assert(s->initializer_cell==0x224D7A04u);
    assert(s->initializer_va==0x223C1B40u);
    assert(s->constructor_va==0x222625E0u);
    assert(s->teardown_wrapper_va==0x223CE6A0u);
    assert(s->destructor_va==0x222626A0u);
    assert(s->final_interface_vtable_va==0x22453DA0u);
    assert(s->final_interface_method0_va==0x22262690u);
    assert(c->containing_object_va==0x2433FDC8u);
    assert(c->embedded_offset==0x10014u);
    assert(native::original_interface_return_va(*c)==0x2434FDDCu);
    assert(c->factory_va==0x241056B0u);
    assert(c->initializer_cell==0x242F3998u);
    assert(c->initializer_va==0x24268370u);
    assert(c->constructor_va==0x24108190u);
    assert(c->teardown_wrapper_va==0x242734C0u);
    assert(c->destructor_va==0x24107430u);
    assert(c->final_interface_vtable_va==0x24297390u);
    assert(c->final_interface_method0_va==0x241056F0u);
    assert(native::find(native::Module::Client,"vcliententitylist003")==nullptr);
    assert(native::find(native::Module::Server,"VClientEntityList003")==nullptr);
}
static void verify_original_native_constructor_vptr_order() {
    const auto& s=native::kBindings[0];
    const auto& c=native::kBindings[1];
    assert(s.write_count==5u && c.write_count==3u);
    assert(s.writes[0].object_offset==4u && s.writes[0].vtable_va==0x224313FCu);
    assert(s.writes[1].object_offset==8u && s.writes[1].vtable_va==0x22453CB4u);
    assert(s.writes[2].object_offset==0u && s.writes[2].vtable_va==0x22453E04u);
    assert(s.writes[3].object_offset==4u && s.writes[3].vtable_va==0x22453DC8u);
    assert(s.writes[4].object_offset==8u && s.writes[4].vtable_va==0x22453DA0u);
    assert(c.writes[0].object_offset==0x10014u &&
           c.writes[0].vtable_va==0x24297368u);
    assert(c.writes[1].object_offset==0u &&
           c.writes[1].vtable_va==0x242973B8u);
    assert(c.writes[2].object_offset==0x10014u &&
           c.writes[2].vtable_va==0x24297390u);
}
static void verify_initializer_and_teardown_callback_order() {
    for(const auto& b:native::kBindings) {
        Recorder rec;native::State state;
        native::observe_initializer(b,state,rec);
        assert(state.initialized && state.teardown_registered);
        assert(state.final_embedded_vptr==b.final_interface_vtable_va);
        assert(rec.events.size()==b.write_count+2u);
        assert(rec.events.front().kind==native::EventKind::ConstructorCall);
        assert(rec.events.front().code_va==b.constructor_va);
        assert(rec.events.front().containing_object==b.containing_object_va);
        for(std::size_t i=0;i<b.write_count;i++) {
            const auto& e=rec.events[1u+i];
            assert(e.kind==native::EventKind::VptrStore);
            assert(e.containing_object==b.containing_object_va);
            assert(e.object_offset==b.writes[i].object_offset);
            assert(e.pointer_va==b.writes[i].vtable_va);
        }
        const auto& reg=rec.events.back();
        assert(reg.kind==native::EventKind::TeardownRegistered);
        assert(reg.code_va==b.shutdown_registrar_va);
        assert(reg.pointer_va==b.teardown_wrapper_va);
        native::observe_teardown(b,state,rec);
        assert(state.teardown_observed);
        assert(rec.events.size()==b.write_count+4u);
        const auto& wrap=rec.events[rec.events.size()-2u];
        const auto& dtor=rec.events.back();
        assert(wrap.kind==native::EventKind::TeardownWrapper);
        assert(wrap.code_va==b.teardown_wrapper_va);
        assert(wrap.containing_object==b.containing_object_va);
        assert(dtor.kind==native::EventKind::OriginalDestructorTailcall);
        assert(dtor.code_va==b.destructor_va);
        assert(dtor.containing_object==b.containing_object_va);
        // No call is permitted to substitute the embedded interface
        // pointer for the *containing object's* this pointer.
        assert(b.containing_object_va!=native::original_interface_return_va(b));
    }
}
static void host_refuses_invalid_atexit_or_duplicate_initialization() {
    for(const auto& b:native::kBindings) {
        Recorder recorder;native::State state;
        bool rejected=false;
        try{native::observe_teardown(b,state,recorder);}
        catch(const std::logic_error&){rejected=true;}
        assert(rejected && recorder.events.empty());
        native::observe_initializer(b,state,recorder);
        const auto n=recorder.events.size();
        rejected=false;
        try{native::observe_initializer(b,state,recorder);}
        catch(const std::logic_error&){rejected=true;}
        assert(rejected && recorder.events.size()==n);
        native::observe_teardown(b,state,recorder);
        rejected=false;
        try{native::observe_teardown(b,state,recorder);}
        catch(const std::logic_error&){rejected=true;}
        assert(rejected);
    }
}
int main() {
    original_embedded_factory_and_object_layout();
    verify_original_native_constructor_vptr_order();
    verify_initializer_and_teardown_callback_order();
    host_refuses_invalid_atexit_or_duplicate_initialization();
}
