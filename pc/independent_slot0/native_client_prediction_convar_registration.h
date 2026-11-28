// Phase1688: opt-in registered client-owned cl_predict ConVar source.
//
// Original retail 2006 PE32 client.dll static ConVar registration at
// 0x2426C550 creates cl_predict default "1", flags 0x200 and
// self-parent at +0x18. Client-side ConVar accessor adds flag
// 0x08, calls VEngineCvar002 virtual +0x00 RegisterConCommandBase,
// and preserves its owner for the engine lifetime. The full
// original registration wrapper, 64B constructor and 244B base
// constructor are whole-SHA-authenticated by Phase1658.
//
// The independent client DLL lacks the original static ConVar
// list. This host-owned adaptation performs the same registration
// strictly AFTER explicit owner diagnostic startup; unlike retail
// static initialization timing, it does not pretend to run the
// original .CRT constructor sequence. It never uses the original
// client.dll or copies executable bytes.
//
// The object is allocated once and deliberately retained for
// process lifetime: after any RegisterConCommandBase invocation,
// the engine may retain its pointer even when other validation
// fails. Do not retry failed registrations or delete it on
// VClient011 Shutdown.
#pragma once
#include "server_console_object_abi.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1688 original client ConVar registration requires MSVC x86
#endif

namespace sin_independent {
namespace native_client_prediction_convar_registration {
namespace ca=console_abi;
constexpr char kOriginalName[]="cl_predict";
constexpr char kOriginalDefault[]="1";
constexpr std::int32_t kOriginalDefaultInt=1;
constexpr int kOriginalCtorFlags=0x200;
constexpr int kOriginalClientRegisteredFlags=0x208;
constexpr unsigned kOriginalConVarParentOffset=0x18u;
constexpr unsigned kOriginalConVarNumericOffset=0x2Cu;
constexpr unsigned kOriginalCvarRegisterVirtualSlot=0u;
constexpr unsigned kOriginalCvarFindConstVirtualSlot=3u;
static_assert(sizeof(ca::Variable)==0x44u &&
              sizeof(void*)==4u,
              "native retail ConVar ownership/ABI drift");

// A real production instance lives in a deliberately leaked
// process-lifetime heap record. The original owner engine may
// retain a pointer after Registration and before DLL unload.
struct Owner {
    ca::Variable variable{};
    bool attempted=false;
    bool registered=false;
    unsigned register_calls=0u;
    unsigned refused=0u;

    // The calling site has validated engine/virtual pointers
    // using VirtualQuery and earlier VEngineCvar002 acquisition.
    // find and reg are actual __thiscall Win32 vtable methods.
    using Register=void (__thiscall *)(void*,ca::CommandBase*);
    using Find=const void* (__thiscall *)(const void*,const char*);

    bool RegisterMissing(void* cvar,Register reg,Find find) noexcept {
        if(attempted || !cvar || !reg || !find) {
            ++refused;
            return false;
        }
        attempted=true; // terminal if registration produces side effects
        if(find(cvar,kOriginalName)) {
            ++refused; // existing root must be reused, not replaced
            return false;
        }
        variable.CreateVariable(
            kOriginalName,kOriginalDefault,kOriginalCtorFlags,
            nullptr,false,0.0f,false,0.0f,nullptr);
        if(variable.parent!=&variable || !variable.string ||
           variable.integer!=kOriginalDefaultInt ||
           variable.registered || variable.next ||
           variable.flags!=kOriginalCtorFlags ||
           std::strcmp(variable.String(),kOriginalDefault)) {
            ++refused;
            return false;
        }
        const auto* const initialString=variable.string;
        // Exact original ConVar accessor order: add client flag 8,
        // detach linked list pointer, register, then mark local
        // registered. Engine may have retained this pointer.
        variable.AddFlags(8);
        variable.next=nullptr;
        ++register_calls;
        reg(cvar,&variable);
        if(variable.parent!=&variable ||
           variable.string!=initialString ||
           variable.flags!=kOriginalClientRegisteredFlags ||
           find(cvar,kOriginalName)!=&variable) {
            ++refused;
            return false;
        }
        variable.registered=true;
        registered=true;
        return true;
    }
};
} // namespace native_client_prediction_convar_registration
} // namespace sin_independent
