#pragma once
// Reconstructed original VGui_InitInterfacesList dependencies.
// The original 2006 client.dll and Source SDK 2004 agree on the names
// and the VGUI_ivgui006 virtual slot-1 Init(Factory*, 1) signature.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "client_vgui_vtable_probe.h"

namespace sin_independent::vgui_controls {
using Factory = void* (__cdecl *)(const char*,int*);
using Trace = void (*)(const char*,const char*) noexcept;
constexpr std::array<const char*,8> kNames={{
    "VGUI_ivgui006","VGUI_Panel009","VGUI_Surface028","VGUI_Scheme010",
    "VGUI_System010","VGUI_Input005","VGUI_Localize003","VFileSystem012"
}};
enum class Result {
    MissingFactory, MissingInterface, InvalidCoreObject,
    InvalidCoreVtable, InvalidInitMethod, InitReturnedTrue, InitReturnedFalse
};
inline const char* Name(Result value) noexcept {
    switch(value) {
        case Result::MissingFactory: return "missing-factory";
        case Result::MissingInterface: return "missing-interface";
        case Result::InvalidCoreObject: return "invalid-core-object";
        case Result::InvalidCoreVtable: return "invalid-core-vtable";
        case Result::InvalidInitMethod: return "invalid-init-method";
        case Result::InitReturnedTrue: return "init-true";
        case Result::InitReturnedFalse: return "init-false";
    }
    return "unexpected";
}
inline bool Completed(Result r) noexcept {
    // Original caller ignores the IVGui.Init boolean result. The
    // important observation is that the authenticated ABI call returned.
    return r==Result::InitReturnedTrue || r==Result::InitReturnedFalse;
}
inline Result Start(Factory factory,Trace trace,void** surfaceOut=nullptr) noexcept {
    if(surfaceOut)*surfaceOut=nullptr;
    if(!factory) return Result::MissingFactory;
    std::array<void*,kNames.size()> objects{};
    for(std::size_t i=0;i<kNames.size();++i) {
        if(trace)trace("vgui-controls-request",kNames[i]);
        objects[i]=factory(kNames[i],nullptr);
        if(!objects[i]) {
            if(trace)trace("vgui-controls-missing",kNames[i]);
        } else if(trace) {
            trace("vgui-controls-found",kNames[i]);
        }
    }
    // Fail closed if an interface is missing before invoking a live
    // engine method. Original calls IVGui.Init before checking the
    // remaining seven, but we deliberately don't risk a partial set.
    for(void* p:objects)if(!p)return Result::MissingInterface;
    if(surfaceOut)*surfaceOut=objects[2];  // VGUI_Surface028
    void* core=objects[0];
    if(!vgui_layout::Readable(core,sizeof(void*)))
        return Result::InvalidCoreObject;
    void** vtable=nullptr;
    std::memcpy(&vtable,core,sizeof(vtable));
    if(!vtable || !vgui_layout::Readable(vtable,2*sizeof(void*)))
        return Result::InvalidCoreVtable;
    HMODULE module=nullptr;
    if(!GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(vtable),&module))
        return Result::InvalidCoreVtable;
    void* address=nullptr;
    std::memcpy(&address,vtable+1,sizeof(address));
    if(!vgui_layout::Executable(address) ||
       !vgui_layout::OwnedBy(module,address))
        return Result::InvalidInitMethod;
    using Init = bool (__thiscall *)(void*,Factory*,int);
    Factory factories[1]={factory};
    if(trace)trace("vgui-controls-init-entered","VGUI_ivgui006:factory-count-1");
    const bool returned=reinterpret_cast<Init>(address)(core,factories,1);
    if(trace)trace("vgui-controls-init-returned",returned?"true":"false");
    return returned?Result::InitReturnedTrue:Result::InitReturnedFalse;
}
} // namespace sin_independent::vgui_controls
