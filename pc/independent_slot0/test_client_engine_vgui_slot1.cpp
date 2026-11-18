// Native Windows x86 synthetic ABI test. No proprietary engine is loaded.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "client_engine_vgui_slot1_probe.h"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

static unsigned requests=0;
static std::uintptr_t outcome=23;
class OwnedVGui {
public:
    virtual void __thiscall NotSlot1() noexcept {std::abort();}
    virtual std::uintptr_t __thiscall Query(int kind) noexcept {
        if(kind!=2) std::abort();
        ++requests;
        return outcome;
    }
};
int main() {
    OwnedVGui object;
    using sin_independent::engine_vgui::CallSlot1Arg2;
    using sin_independent::engine_vgui::Slot1Result;
    if(CallSlot1Arg2(nullptr)!=Slot1Result::InvalidObject)return 1;
    int empty=0;
    if(CallSlot1Arg2(&empty)!=Slot1Result::InvalidVtable)return 2;
    // A readable, module-owned table with a null slot must be rejected without a call.
    static void* null_method_table[2] = {nullptr, nullptr};
    void** fake_table = null_method_table;
    if(CallSlot1Arg2(&fake_table)!=Slot1Result::InvalidMethod || requests!=0)return 6;
    // A non-executable data pointer is not a callable virtual method.
    static int not_executable = 0;
    static void* data_method_table[2] = {nullptr, &not_executable};
    fake_table = data_method_table;
    if(CallSlot1Arg2(&fake_table)!=Slot1Result::InvalidMethod || requests!=0)return 7;
    // Neither the object nor its vtable may point to an unreadable page.
    void* inaccessible=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);
    if(!inaccessible)return 8;
    if(CallSlot1Arg2(inaccessible)!=Slot1Result::InvalidObject || requests!=0)return 9;
    fake_table = static_cast<void**>(inaccessible);
    if(CallSlot1Arg2(&fake_table)!=Slot1Result::InvalidVtable || requests!=0)return 10;
    VirtualFree(inaccessible,0,MEM_RELEASE);
    if(CallSlot1Arg2(&object)!=Slot1Result::Nonzero || requests!=1)return 3;
    outcome=0;
    if(CallSlot1Arg2(&object)!=Slot1Result::Zero || requests!=2)return 4;
    if(std::strcmp(sin_independent::engine_vgui::Name(Slot1Result::Nonzero),"nonzero"))return 5;
    std::puts("PASS independently reconstructed VEngineVGui001 slot1 arg2 ABI, zero/nonzero, invalid object/table/method, no-call refusals");
    return 0;
}
