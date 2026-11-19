#pragma once
// Three complete, SHA-audited original 2006 SiN client AutoGameSystem
// virtual Init method behaviors. These are independent ABI adapters,
// NOT dispatches into a retail game's original client.dll.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include "client_vgui_vtable_probe.h"
namespace sin_independent::known_game_system {
enum class CallResult {InvalidObject,InvalidVtable,InvalidMethod,CalledFalse,CalledTrue};
inline const char* Name(CallResult r) noexcept {
    switch(r) {
        case CallResult::InvalidObject:return "invalid-object";
        case CallResult::InvalidVtable:return "invalid-vtable";
        case CallResult::InvalidMethod:return "invalid-method";
        case CallResult::CalledFalse:return "called-false";
        case CallResult::CalledTrue:return "called-true";
    }
    return "unexpected";
}
// The original sound system Init at 0x241BC950 tail-jumps to virtual
// slot 1 of the engine sound emitter acquired earlier in client Init.
inline void* VerifiedMethod(void* object,std::size_t slot,
                            CallResult& refused) noexcept {
    if(!vgui_layout::Readable(object,sizeof(void*))) {
        refused=CallResult::InvalidObject;return nullptr;
    }
    void** table=nullptr;
    std::memcpy(&table,object,sizeof(table));
    if(!table || slot>(static_cast<std::size_t>(-1)/sizeof(void*))-1 ||
       !vgui_layout::Readable(table,(slot+1)*sizeof(void*))) {
        refused=CallResult::InvalidVtable;return nullptr;
    }
    HMODULE owner=nullptr;
    if(!GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(table),&owner)) {
        refused=CallResult::InvalidVtable;return nullptr;
    }
    void* method=nullptr;
    std::memcpy(&method,table+slot,sizeof(method));
    if(!vgui_layout::Executable(method) ||
       !vgui_layout::OwnedBy(owner,method)) {
        refused=CallResult::InvalidMethod;return nullptr;
    }
    return method;
}
inline CallResult InitSoundEmitter(void* soundEmitter) noexcept {
    CallResult refused=CallResult::InvalidObject;
    void* slot=VerifiedMethod(soundEmitter,1,refused);
    if(!slot)return refused;
    using Init=bool (__thiscall *)(void*);
    // Original CSoundEmitterSystem::Init is a tail call, and the
    // delegated method's boolean value controls the whole system gate.
    return reinterpret_cast<Init>(slot)(soundEmitter)?
        CallResult::CalledTrue:CallResult::CalledFalse;
}
// The original CViewportClientSystem::Init at 0x241ED310 calls
// virtual slot 7 of the original global at 0x2437F8A0, then returns true
// regardless of that method's result.
inline CallResult InitViewport(void* viewport) noexcept {
    CallResult refused=CallResult::InvalidObject;
    void* slot=VerifiedMethod(viewport,7,refused);
    if(!slot)return refused;
    using Setup=void (__thiscall *)(void*);
    reinterpret_cast<Setup>(slot)(viewport);
    return CallResult::CalledTrue;
}
inline bool InitClientThinkList(void* state) noexcept {
    // Original 0x24103DF0 does two writes at +0x48 and +0x4C,
    // returning bool true. Guard our independent memory owner.
    if(!state || !vgui_layout::Readable(state,0x4Du))return false;
    MEMORY_BASIC_INFORMATION area{};
    if(VirtualQuery(state,&area,sizeof(area))!=sizeof(area) ||
       area.State!=MEM_COMMIT || (area.Protect & (PAGE_GUARD|PAGE_NOACCESS)))
        return false;
    auto prot=area.Protect & 0xffu;
    if(prot!=PAGE_READWRITE && prot!=PAGE_EXECUTE_READWRITE &&
       prot!=PAGE_WRITECOPY && prot!=PAGE_EXECUTE_WRITECOPY)return false;
    auto* pointer=static_cast<unsigned char*>(state);
    const std::uint32_t zero=0;
    std::memcpy(pointer+0x48,&zero,sizeof(zero));
    pointer[0x4C]=0;
    return true;
}
} // namespace sin_independent::known_game_system
