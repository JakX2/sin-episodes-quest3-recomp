#pragma once
// Independently reconstructed call boundary, not the full IVEngineVGui ABI.
// Evidence: original SHA-pinned client.dll 0x2422ECF0 and caller 0x241EDFB0.
// Slots/argument are authenticated. SDK2004 identifies this as GetPanel(PANEL_CLIENTDLL), returning VPANEL.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include "client_vgui_vtable_probe.h"
namespace sin_independent::engine_vgui {
enum class Slot1Result { InvalidObject, InvalidVtable, InvalidMethod, Zero, Nonzero };
inline const char* Name(Slot1Result s) noexcept {
    switch(s) {
        case Slot1Result::InvalidObject:return "invalid-object";
        case Slot1Result::InvalidVtable:return "invalid-vtable";
        case Slot1Result::InvalidMethod:return "invalid-method";
        case Slot1Result::Zero:return "zero";
        case Slot1Result::Nonzero:return "nonzero";
    }
    return "unexpected";
}
inline Slot1Result CallSlot1Arg2(void* object) noexcept {
    if(!vgui_layout::Readable(object,sizeof(void*)))
        return Slot1Result::InvalidObject;
    void** table=nullptr;
    std::memcpy(&table,object,sizeof(table));
    if(!table || !vgui_layout::Readable(table,2*sizeof(void*)))
        return Slot1Result::InvalidVtable;
    HMODULE vtableModule=nullptr;
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(table),&vtableModule))
        return Slot1Result::InvalidVtable;
    void* address=nullptr;
    std::memcpy(&address,table+1,sizeof(address));
    if(!vgui_layout::Executable(address) ||
       !vgui_layout::OwnedBy(vtableModule,address))
        return Slot1Result::InvalidMethod;
    using Method = std::uintptr_t (__thiscall *)(void*,int);
    const auto fn=reinterpret_cast<Method>(address);
    // Original game: ECX=VEngineVGui001; push PANEL_CLIENTDLL (2); call vtable + 4.
    // SDK2004 calls this GetPanel; the VPANEL handle stays opaque.
    return fn(object,2)!=0 ? Slot1Result::Nonzero : Slot1Result::Zero;
}
} // namespace sin_independent::engine_vgui
