#pragma once
// Win32 x86 exact original SiN client post-VGUI surface query boundary.
// Original SHA-pinned 0x240FE3D7 calls an IAppSystem subobject at offset +4,
// vtable slot 2: QueryInterface("MatSystemSurface003").
// SDK 2004 ISurface inherits IBaseInterface, IAppSystem in that order.
// This is NOT the full ISurface implementation or client Init.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include "client_vgui_vtable_probe.h"

namespace sin_independent::mat_surface {
enum class Result {
    InvalidSurface,
    InvalidAppSystemSubobject,
    InvalidVtable,
    InvalidMethod,
    MissingInterface,
    Found
};
inline const char* Name(Result result) noexcept {
    switch(result) {
        case Result::InvalidSurface:return "invalid-surface";
        case Result::InvalidAppSystemSubobject:return "invalid-appsystem-subobject";
        case Result::InvalidVtable:return "invalid-vtable";
        case Result::InvalidMethod:return "invalid-method";
        case Result::MissingInterface:return "missing-interface";
        case Result::Found:return "found";
    }
    return "unexpected";
}
inline Result Query(void* surface,void** resultOut=nullptr) noexcept {
    if(resultOut)*resultOut=nullptr;
    if(!vgui_layout::Readable(surface,2*sizeof(void*)))
        return Result::InvalidSurface;
    // Original x86 lea ecx,[eax+4]; the adjustment is one 32-bit vptr.
    auto* appSystem=reinterpret_cast<unsigned char*>(surface)+sizeof(void*);
    if(!vgui_layout::Readable(appSystem,sizeof(void*)))
        return Result::InvalidAppSystemSubobject;
    void** table=nullptr;
    std::memcpy(&table,appSystem,sizeof(table));
    if(!table || !vgui_layout::Readable(table,3*sizeof(void*)))
        return Result::InvalidVtable;
    HMODULE owner=nullptr;
    if(!GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(table),&owner))
        return Result::InvalidVtable;
    void* method=nullptr;
    std::memcpy(&method,table+2,sizeof(method));
    if(!vgui_layout::Executable(method) ||
       !vgui_layout::OwnedBy(owner,method))
        return Result::InvalidMethod;
    using QueryFn=void* (__thiscall *)(void*,const char*);
    auto* interfacePointer=reinterpret_cast<QueryFn>(method)(
        appSystem,"MatSystemSurface003");
    if(!interfacePointer)return Result::MissingInterface;
    if(resultOut)*resultOut=interfacePointer;
    return Result::Found;
}
} // namespace sin_independent::mat_surface
