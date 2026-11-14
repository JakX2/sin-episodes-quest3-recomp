#pragma once
// Phase228: source-grounded C++17 event-only reference for two original
// 2006 SiN Source game embedded interface initialization paths.
// All addresses are original PE32 metadata TOKENS. No engine objects,
// original methods, vtables, or commercial DLL code is executed.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace sin_compat::original_embedded_interface_initializers {
enum class Module { Server, Client };
struct VptrWrite {
    std::uint32_t object_offset{};
    std::uint32_t vtable_va{};
};
struct Binding {
    Module module{};
    const char* interface_name{};
    std::uint32_t initializer_cell{};
    std::uint32_t initializer_va{};
    std::uint32_t containing_object_va{};
    std::uint32_t embedded_offset{};
    std::uint32_t factory_va{};
    std::uint32_t constructor_va{};
    std::uint32_t teardown_wrapper_va{};
    std::uint32_t destructor_va{};
    std::uint32_t shutdown_registrar_va{};
    std::uint32_t final_interface_vtable_va{};
    std::uint32_t final_interface_method0_va{};
    std::array<VptrWrite,5> writes{};
    std::size_t write_count{};
};
inline constexpr std::array<Binding,2> kBindings{{
    {Module::Server,"HLTVDirector001",0x224D7A04u,0x223C1B40u,
     0x2257AE98u,0x08u,0x2225F8C0u,0x222625E0u,
     0x223CE6A0u,0x222626A0u,0x223A5639u,
     0x22453DA0u,0x22262690u,
     {{{4u,0x224313FCu},{8u,0x22453CB4u},
       {0u,0x22453E04u},{4u,0x22453DC8u},
       {8u,0x22453DA0u}}},5u},
    {Module::Client,"VClientEntityList003",0x242F3998u,0x24268370u,
     0x2433FDC8u,0x10014u,0x241056B0u,0x24108190u,
     0x242734C0u,0x24107430u,0x24248788u,
     0x24297390u,0x241056F0u,
     {{{0x10014u,0x24297368u},{0u,0x242973B8u},
       {0x10014u,0x24297390u},{0u,0u},{0u,0u}}},3u},
}};
inline constexpr std::size_t kNewFullyShaReviewedOriginalMethods=6u;
inline constexpr std::size_t kNewOriginalNativeCodeBytes=486u;
inline constexpr std::size_t kOriginalCoreUnbackedStartupChainsTotal=6u;
inline constexpr std::size_t kOriginalCoreUnbackedStartupChainsUntraced=0u;

inline const Binding* find(Module module,std::string_view exact) noexcept {
    for(const auto& b:kBindings)
        if(b.module==module && exact==b.interface_name)return &b;
    return nullptr;
}
inline std::uint32_t original_interface_return_va(const Binding& b) noexcept {
    return b.containing_object_va+b.embedded_offset;
}
enum class EventKind { ConstructorCall,VptrStore,TeardownRegistered,
                       TeardownWrapper,OriginalDestructorTailcall };
struct Event {
    EventKind kind{};
    std::uint32_t code_va{};
    std::uint32_t containing_object{};
    std::uint32_t object_offset{};
    std::uint32_t pointer_va{};
};
struct IObserver {
    virtual ~IObserver()=default;
    virtual void on_event(const Event& e)=0;
};
struct State {
    bool initialized{};
    bool teardown_registered{};
    bool teardown_observed{};
    std::uint32_t final_embedded_vptr{};
};
// A verified event sequence, NOT a native constructor execution.
// The original Windows CRT callback order among independent objects
// and the actual side effects of nested methods are not modeled.
inline void observe_initializer(const Binding& b,State& s,IObserver& observer) {
    if(s.initialized)throw std::logic_error("host refuses duplicate original static initialization");
    observer.on_event({EventKind::ConstructorCall,b.constructor_va,
                       b.containing_object_va,0u,0u});
    std::uint32_t final=0u;
    for(std::size_t i=0;i<b.write_count;i++){
        const auto& w=b.writes[i];
        observer.on_event({EventKind::VptrStore,b.constructor_va,
                           b.containing_object_va,w.object_offset,w.vtable_va});
        if(w.object_offset==b.embedded_offset) final=w.vtable_va;
    }
    if(final!=b.final_interface_vtable_va)
        throw std::logic_error("original embedded interface never got expected final vptr");
    observer.on_event({EventKind::TeardownRegistered,b.shutdown_registrar_va,
                       b.containing_object_va,0u,b.teardown_wrapper_va});
    s.final_embedded_vptr=final;
    s.teardown_registered=true;
    s.initialized=true;
}
inline void observe_teardown(const Binding& b,State& s,IObserver& observer) {
    if(!s.teardown_registered || s.teardown_observed)
        throw std::logic_error("host refuses unregistered or repeated teardown");
    observer.on_event({EventKind::TeardownWrapper,b.teardown_wrapper_va,
                       b.containing_object_va,0u,0u});
    observer.on_event({EventKind::OriginalDestructorTailcall,b.destructor_va,
                       b.containing_object_va,0u,0u});
    s.teardown_observed=true;
}
} // namespace sin_compat::original_embedded_interface_initializers
