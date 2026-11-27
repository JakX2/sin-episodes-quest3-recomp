// Phase 1654: true native Win32 x86 virtual-call ABI, fake game exports
// only. The original owner client.dll is NOT linked/loaded by this test.
// Exact four slot positions were authenticated separately from SHA-pinned
// original PE32 data. No GameClientExports001 factory is registered.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <type_traits>
#if !defined(_M_IX86)
#error Phase 1654 requires MSVC native Windows x86
#endif

namespace {
struct State {
    unsigned destroyed=0,slot1_calls=0,slot2_calls=0,slot3_calls=0;
    std::uint32_t last1=0,last2=0,last3=0;
};
static State g{};

// Original slot 0 is MSVC x86 scalar deleting destructor (flags&1, ret 4).
// The three remaining original vtable slots each accept one DWORD stack
// argument and use thiscall RET 4. We do not infer original C++ types or
// give this fake object to a running original engine.
class FakeGameClientExports {
public:
    explicit FakeGameClientExports(State& state) noexcept:state_(&state) {}
    virtual ~FakeGameClientExports() noexcept {
        ++state_->destroyed;
    }
    virtual std::uint32_t __thiscall Slot1(std::uint32_t value) noexcept {
        ++state_->slot1_calls;
        state_->last1=value;
        return value^0x5A5A1234u;
    }
    virtual void __thiscall Slot2(std::uint32_t value) noexcept {
        ++state_->slot2_calls;
        state_->last2=value;
    }
    virtual void __thiscall Slot3(std::uint32_t value) noexcept {
        ++state_->slot3_calls;
        state_->last3=value;
    }
private:
    State* state_;
};

static_assert(sizeof(void*)==4,"original PE32 pointers required");
static_assert(std::has_virtual_destructor<FakeGameClientExports>::value,
              "native deleting destructor must occupy virtual slot zero");
using ExpectedSlot1=std::uint32_t (__thiscall FakeGameClientExports::*)(
    std::uint32_t) noexcept;
using ExpectedSlotVoid=void (__thiscall FakeGameClientExports::*)(
    std::uint32_t) noexcept;
static_assert(std::is_same<decltype(&FakeGameClientExports::Slot1),
                           ExpectedSlot1>::value,"one DWORD ABI");
static_assert(std::is_same<decltype(&FakeGameClientExports::Slot2),
                           ExpectedSlotVoid>::value,"one DWORD ABI");
static_assert(std::is_same<decltype(&FakeGameClientExports::Slot3),
                           ExpectedSlotVoid>::value,"one DWORD ABI");

int fail(int code,const char* why) {
    std::fprintf(stderr,
                 "FAIL original-free GameClientExports001 x86 ABI %d: %s\n",
                 code,why);
    return code;
}

bool invoke_dword(void* instance,void* slot,std::uint32_t value,
                  std::uint32_t expected) noexcept {
    std::uintptr_t before=0,after=0;
    std::uint32_t returned=0;
    __asm {
        mov eax,esp
        mov before,eax
        push value
        mov ecx,instance
        mov eax,slot
        call eax
        mov returned,eax
        mov eax,esp
        mov after,eax
    }
    return before==after&&returned==expected;
}

bool invoke_void(void* instance,void* slot,std::uint32_t value) noexcept {
    std::uintptr_t before=0,after=0;
    __asm {
        mov eax,esp
        mov before,eax
        push value
        mov ecx,instance
        mov eax,slot
        call eax
        mov eax,esp
        mov after,eax
    }
    return before==after;
}

bool invoke_deleting_destructor(void* instance,void* slot) noexcept {
    // The fixture is heap-owned; flags=1 makes MSVC destroy AND free it.
    // The original destructor is not invoked. No use of instance afterward.
    std::uintptr_t before=0,after=0;
    __asm {
        mov eax,esp
        mov before,eax
        push 1
        mov ecx,instance
        mov eax,slot
        call eax
        mov eax,esp
        mov after,eax
    }
    return before==after;
}
} // namespace

int main() {
    // Keep an object with four virtual slots and an independent 3-argument
    // test; no original game process or third-party engine factory is used.
    auto* obj=new FakeGameClientExports(g);
    auto** slots=*reinterpret_cast<void***>(obj);
    if(!slots||!slots[0]||!slots[1]||!slots[2]||!slots[3])
        return fail(1,"native four-slot vtable not populated");
    // All four functions must be distinct; this is a real compiler-allocated
    // vtable, not a manually filled array of arbitrary raw void pointers.
    for(unsigned i=0;i<4;++i)
        for(unsigned j=0;j<i;++j)
            if(slots[i]==slots[j])
                return fail(2,"MSVC virtual methods aliased");
    const auto slot1_value=std::uint32_t{0x82310FF0};
    const auto slot2_value=std::uint32_t{0x9F0A0021};
    const auto slot3_value=std::uint32_t{0x04323001};
    if(!invoke_dword(
            obj,slots[1],slot1_value,slot1_value^0x5A5A1234u)||
       !invoke_void(obj,slots[2],slot2_value)||
       !invoke_void(obj,slots[3],slot3_value))
        return fail(3,"MSVC x86 one-DWORD virtual call stack imbalance");
    if(g.slot1_calls!=1||g.last1!=slot1_value||
       g.slot2_calls!=1||g.last2!=slot2_value||
       g.slot3_calls!=1||g.last3!=slot3_value||
       g.destroyed!=0)
        return fail(4,"fake virtual methods dispatched to wrong slot");
    if(!invoke_deleting_destructor(obj,slots[0])||g.destroyed!=1)
        return fail(5,"scalar deleting destructor ABI/flags mismatch");
    std::puts("PASS original-free Win32 x86 GameClientExports001 four-slot ABI: scalar deleting destructor flags=1, three one-DWORD __thiscall RET4 virtual methods, balanced stack and exact dispatch");
    return 0;
}
