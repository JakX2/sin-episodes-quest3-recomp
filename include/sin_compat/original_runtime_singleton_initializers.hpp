#pragma once
// Phase227: independent C++17 event model of original SiN 2006 singleton
// startup/cleanup paths. These are ORIGINAL VA TOKENS ONLY; no original
// Source C++ objects are constructed and no game module is loaded.
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace sin_compat::original_runtime_singleton_initializers {

enum class Module { Server, Client };
enum class EventKind {
    ConstructorCall, ConstructorVptrObserved, PostConstructorVptrOverwrite,
    TeardownCallbackRegistered, TeardownWrapperInvoked, OriginalDestructorCalled
};
struct Binding {
    Module module{};
    const char* interface_name{};
    std::uint32_t data_initializer_cell{};
    std::uint32_t initializer_va{};
    std::uint32_t singleton_va{};
    std::uint32_t constructor_va{};
    std::uint32_t constructor_initial_vptr{};
    std::uint32_t initializer_final_vptr{};
    std::uint32_t teardown_wrapper_va{};
    std::uint32_t teardown_method_va{};
    std::uint32_t shutdown_registrar_va{};
    std::uint32_t final_vtable_slot0{};
    bool whole_constructor_sha_verified{};
};
inline constexpr std::array<Binding,4> kBindings{{
  {Module::Server,"GameMovement001",0x224D7CC0u,0x223C33F0u,
   0x225934B0u,0x221EBF40u,0x224433D4u,0x2245EC04u,
   0x223CF2C0u,0x221EAAC0u,0x223A5639u,0x222D01B0u,true},
  {Module::Client,"ClientLeafSystem001",0x242F39ACu,0x24268440u,
   0x24357F30u,0x2410C790u,0x2429756Cu,0x2429756Cu,
   0x24273500u,0x2410CA80u,0x24248788u,0x2410D8E0u,false},
  {Module::Client,"GameMovement001",0x242F4304u,0x2426E940u,
   0x24379920u,0x24136F80u,0x2429A684u,0x242B1B5Cu,
   0x242757A0u,0x24136270u,0x24248788u,0x2420FC60u,true},
  {Module::Client,"VClientPrediction001",0x242F480Cu,0x24271430u,
   0x2437FA00u,0x24195A10u,0x242A363Cu,0x242A363Cu,
   0x24275E50u,0x24195700u,0x24248788u,0x24195A50u,true},
}};

struct Event {
    EventKind kind{};
    std::uint32_t target_va{};
    std::uint32_t singleton_va{};
    std::uint32_t vptr_token{};
};
struct IObserver {
    virtual ~IObserver()=default;
    virtual void event(const Event& value)=0;
};
struct State {
    bool initializer_ran{};
    bool shutdown_registered{};
    bool teardown_called{};
    std::uint32_t observed_vptr_token{};
};
inline const Binding* find(Module module,std::string_view exact_name) noexcept {
    for(const auto& b:kBindings)
        if(b.module==module && b.interface_name==exact_name)
            return &b;
    return nullptr;
}

// This is an event timeline constrained by the original x86 CALLs and
// selected vptr MOVs, NOT execution of original C++ constructors. In
// particular the full client leaf-system constructor body remains opaque.
inline void observe_original_initializer(const Binding& b,State& state,
                                          IObserver& observer) {
    if(state.initializer_ran)
        throw std::logic_error("host-only duplicate initializer refused");
    observer.event({EventKind::ConstructorCall,b.constructor_va,b.singleton_va,0u});
    state.observed_vptr_token=b.constructor_initial_vptr;
    observer.event({EventKind::ConstructorVptrObserved,b.constructor_va,
                    b.singleton_va,b.constructor_initial_vptr});
    if(b.constructor_initial_vptr!=b.initializer_final_vptr) {
        state.observed_vptr_token=b.initializer_final_vptr;
        observer.event({EventKind::PostConstructorVptrOverwrite,b.initializer_va,
                        b.singleton_va,b.initializer_final_vptr});
    }
    observer.event({EventKind::TeardownCallbackRegistered,b.shutdown_registrar_va,
                    b.singleton_va,b.teardown_wrapper_va});
    state.shutdown_registered=true;
    state.initializer_ran=true;
}

// The original teardown wrapper at the stored address is only a
// MOV ECX,singleton; JMP original destructor. We do NOT simulate
// destructors' object mutations or any CRT atexit registration order.
inline void observe_original_shutdown(const Binding& b,State& state,
                                       IObserver& observer) {
    if(!state.shutdown_registered || state.teardown_called)
        throw std::logic_error("host-only invalid teardown order");
    observer.event({EventKind::TeardownWrapperInvoked,b.teardown_wrapper_va,
                    b.singleton_va,0u});
    observer.event({EventKind::OriginalDestructorCalled,b.teardown_method_va,
                    b.singleton_va,0u});
    state.teardown_called=true;
}
inline constexpr std::uint32_t kOriginalClientLeafSubobjectVptrAt4=0x24297534u;
inline constexpr std::uint32_t kOriginalClientLeafSubobjectVptrAt8=0x2429752Cu;
inline constexpr std::uint32_t kOriginalClientLeafCtorVA=0x2410C790u;
inline constexpr std::size_t kNewlyReviewedOriginalFunctions=11u;
inline constexpr std::size_t kNewNativeInstructionBytes=314u;
inline constexpr std::size_t kRemainingUntracedRuntimeSingletons=2u;

} // namespace sin_compat::original_runtime_singleton_initializers
