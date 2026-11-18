#pragma once
// Terminal diagnostic for ONE authenticated server-only declaration.
// The C++ objects are independently owned. The engine pointer is used
// through verified Win32 slots, never as a guessed C++ implementation.
// Never continue game startup, unload this DLL or destroy the registered
// variable while the engine can still hold its address.
#include "server_native_cvar_registration.h"
namespace sin_independent::console_abi {
inline constexpr char kProbeVarName[]="dd_log_events";
enum class SingleCvarResult {
    Verified, InvalidInterface, ExistingName, InvalidList,
    BadObject, LookupMismatch, Relinked, OverridePresent
};
using RegisterSlot=void (__thiscall *)(void*,CommandBase*);
using OverrideSlot=const char* (__thiscall *)(void*,const char*);
using FindSlot=const void* (__thiscall *)(const void*,const char*);
using CommandsSlot=void* (__thiscall *)(void*);
using NameSlot=const char* (__thiscall *)(const void*);
using ProbeTrace=void (*)(const char*) noexcept;

inline const char* ResultName(SingleCvarResult result) noexcept {
    switch(result) {
    case SingleCvarResult::Verified:return "verified";
    case SingleCvarResult::InvalidInterface:return "invalid-interface";
    case SingleCvarResult::ExistingName:return "existing-name";
    case SingleCvarResult::InvalidList:return "invalid-list";
    case SingleCvarResult::BadObject:return "bad-owned-object";
    case SingleCvarResult::LookupMismatch:return "lookup-mismatch";
    case SingleCvarResult::Relinked:return "unexpected-relink";
    case SingleCvarResult::OverridePresent:return "override-present";
    }
    return "unknown";
}
inline SingleCvarResult RegisterSingleProbeVariable(void* engine,Variable& variable,
                                                    ProbeTrace trace) noexcept {
    if(!engine || !trace)return SingleCvarResult::InvalidInterface;
    auto** slots=*reinterpret_cast<void***>(engine);
    if(!slots || !slots[0] || !slots[1] || !slots[3] || !slots[4])
        return SingleCvarResult::InvalidInterface;
    auto reg=reinterpret_cast<RegisterSlot>(slots[0]);
    auto getOverride=reinterpret_cast<OverrideSlot>(slots[1]);
    auto find=reinterpret_cast<FindSlot>(slots[3]);
    auto commands=reinterpret_cast<CommandsSlot>(slots[4]);
    // No mutation before checking both the variable lookup and the
    // engine's ordinary linked command list for a name conflict.
    if(find(engine,kProbeVarName))return SingleCvarResult::ExistingName;
    auto* node=commands(engine);
    for(unsigned i=0;node;++i) {
        if(i==16384)return SingleCvarResult::InvalidList;
        auto** vt=*reinterpret_cast<void***>(node);
        if(!vt || !vt[4])return SingleCvarResult::InvalidList;
        const char* name=reinterpret_cast<NameSlot>(vt[4])(node);
        if(!name)return SingleCvarResult::InvalidList;
        if(SameName(name,kProbeVarName))return SingleCvarResult::ExistingName;
        // Verified original x86 ConCommandBase::next offset.
        void* next=nullptr;
        std::memcpy(&next,static_cast<const unsigned char*>(node)+4,sizeof(next));
        node=next;
    }
    trace("cvar-name-available");
    // Original constructor RVA 0x3C2FB1 -> 0x25BD20, no callback,
    // no limits, no help text, flags=0, default="0".
    if(variable.string || variable.registered || variable.next)
        return SingleCvarResult::BadObject;
    variable.CreateVariable(kProbeVarName,"0",0,nullptr,false,0,false,0,nullptr);
    if(!variable.string || variable.parent!=&variable || variable.integer ||
       variable.real!=0 || std::strcmp(variable.String(),"0"))
        return SingleCvarResult::BadObject;
    // Original accessor marks game-DLL, detaches, registers, queries
    // override, then marks registered. This one nonreplicated object
    // has no pre-link default restoration. Diagnostic refuses aliases
    // and unexpected overrides instead of mutating a different root.
    variable.AddFlags(4);
    variable.next=nullptr;
    const auto* storage=variable.string;
    trace("cvar-register-entered");
    reg(engine,&variable);
    trace("cvar-register-returned");
    if(variable.parent!=&variable || variable.string!=storage)
        return SingleCvarResult::Relinked;
    const char* overrideValue=getOverride(engine,kProbeVarName);
    if(overrideValue)return SingleCvarResult::OverridePresent;
    variable.registered=true;
    if(find(engine,kProbeVarName)!=&variable)
        return SingleCvarResult::LookupMismatch;
    if(variable.parent!=&variable || variable.string!=storage ||
       !variable.GetName() || std::strcmp(variable.GetName(),kProbeVarName) ||
       !variable.IsBitSet(4) || variable.integer!=0 ||
       variable.real!=0 || std::strcmp(variable.String(),"0"))
        return SingleCvarResult::BadObject;
    trace("cvar-registration-verified");
    return SingleCvarResult::Verified;
}
} // namespace sin_independent::console_abi
