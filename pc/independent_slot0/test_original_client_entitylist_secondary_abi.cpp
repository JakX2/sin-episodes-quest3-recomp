// Phase 1656. Native MSVC Win32 x86 fixture for an EMBEDDED nine-slot
// VClientEntityList003-style object. Original SiN DLLs are NEVER linked,
// loaded, or invoked. The real client factory is NOT registered here.
//
// Prior original PE32 evidence: the owner starts at +0, the secondary
// interface is +0x10014, with a separate three-slot parent vtable.
// Slots 0..5 and 7 take one 4-byte argument (RET 4); slots 6/8 take
// none (RET). This fixture checks the actual compiler-generated vftables.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <type_traits>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Requires actual MSVC native Windows x86.
#endif

namespace {
constexpr std::uintptr_t kSecondaryOffset=0x10014u;
constexpr std::uint32_t kBaseValue=0x69F00000u;

struct Calls {
    unsigned called[9]{};
    std::uint32_t argument[9]{};
    bool receiver_error=false;
    unsigned parent_destructions=0;
};
Calls g{};
void* g_expected_secondary=nullptr;

std::uint32_t note(unsigned slot,void* receiver,std::uint32_t value) noexcept {
    if(slot>=9u) {
        g.receiver_error=true;
        return 0;
    }
    if(receiver!=g_expected_secondary)g.receiver_error=true;
    ++g.called[slot];
    g.argument[slot]=value;
    return kBaseValue^(value+slot*0x103u);
}

// A primary object with its OWN virtuals. Destruction remains entirely
// local to this synthetic fixture. Original parent virtuals are not called.
class Parent3 {
public:
    virtual std::uint32_t __thiscall Register(std::uint32_t a,
                                              std::uint32_t b) noexcept {
        return a^b^0xCE11u;
    }
    virtual std::uint32_t __thiscall Unregister(std::uint32_t a,
                                                std::uint32_t b) noexcept {
        return a+b+0x3355u;
    }
    virtual ~Parent3() noexcept {++g.parent_destructions;}
};

// Deliberately NO virtual destructor: original entity-list vtable slot 0
// is a real one-argument indexed method, not a deleting destructor.
class Embedded9 {
public:
    virtual std::uint32_t __thiscall Slot0(std::uint32_t v) noexcept {
        return note(0,this,v);
    }
    virtual std::uint32_t __thiscall Slot1(std::uint32_t v) noexcept {
        return note(1,this,v);
    }
    virtual std::uint32_t __thiscall Slot2(std::uint32_t v) noexcept {
        return note(2,this,v);
    }
    virtual std::uint32_t __thiscall Slot3(std::uint32_t v) noexcept {
        return note(3,this,v);
    }
    virtual std::uint32_t __thiscall Slot4(std::uint32_t v) noexcept {
        return note(4,this,v);
    }
    virtual std::uint32_t __thiscall Slot5(std::uint32_t v) noexcept {
        return note(5,this,v);
    }
    virtual std::uint32_t __thiscall Slot6() noexcept {
        return note(6,this,0);
    }
    virtual void __thiscall Slot7(std::uint32_t v) noexcept {
        (void)note(7,this,v);
    }
    virtual std::uint32_t __thiscall Slot8() noexcept {
        return note(8,this,0);
    }
};

// Model the original parent/secondary vptr positioning with owned memory,
// without attempting to recreate any opaque CClientEntityList state fields.
struct OwnedFixture {
    Parent3 parent;                    // owner +0x00000
    std::uint8_t unmodeled[0x10010]{}; // owns no Source entities or engine ptrs
    Embedded9 embedded;                // owner +0x10014
};

static_assert(sizeof(void*)==4,"original PE32 pointer width");
static_assert(sizeof(Parent3)==4,"primary object owns one PE32 vptr");
static_assert(sizeof(Embedded9)==4,"secondary object owns one PE32 vptr");
static_assert(sizeof(OwnedFixture)==0x10018,"secondary layout drift");
static_assert(std::has_virtual_destructor<Parent3>::value,
              "native primary destructor must occupy a virtual slot");
static_assert(!std::has_virtual_destructor<Embedded9>::value,
              "secondary slot zero must NOT be a deleting destructor");
using Unary=std::uint32_t (__thiscall Embedded9::*)(std::uint32_t) noexcept;
using Nullary=std::uint32_t (__thiscall Embedded9::*)() noexcept;
using Setter=void (__thiscall Embedded9::*)(std::uint32_t) noexcept;
static_assert(std::is_same<decltype(&Embedded9::Slot0),Unary>::value,
              "slot0 one DWORD ABI");
static_assert(std::is_same<decltype(&Embedded9::Slot5),Unary>::value,
              "slot5 one DWORD ABI");
static_assert(std::is_same<decltype(&Embedded9::Slot6),Nullary>::value,
              "slot6 RET0 ABI");
static_assert(std::is_same<decltype(&Embedded9::Slot7),Setter>::value,
              "slot7 setter RET4 ABI");
static_assert(std::is_same<decltype(&Embedded9::Slot8),Nullary>::value,
              "slot8 RET0 ABI");

int fail(int code,const char* why) {
    std::fprintf(stderr,
        "FAIL Phase1656 native VClientEntityList003 secondary x86 ABI "
        "%d: %s\n",code,why);
    return code;
}

bool call_unary(void* self,void* method,std::uint32_t argument,
                std::uint32_t& result) noexcept {
    std::uintptr_t before=0,after=0;
    std::uint32_t returned=0;
    __asm {
        mov before,esp
        push argument
        mov ecx,self
        mov eax,method
        call eax
        mov returned,eax
        mov after,esp
    }
    result=returned;
    return before==after;
}

bool call_setter(void* self,void* method,
                 std::uint32_t argument) noexcept {
    std::uintptr_t before=0,after=0;
    __asm {
        mov before,esp
        push argument
        mov ecx,self
        mov eax,method
        call eax
        mov after,esp
    }
    return before==after;
}

bool call_nullary(void* self,void* method,
                  std::uint32_t& result) noexcept {
    std::uintptr_t before=0,after=0;
    std::uint32_t returned=0;
    __asm {
        mov before,esp
        mov ecx,self
        mov eax,method
        call eax
        mov returned,eax
        mov after,esp
    }
    result=returned;
    return before==after;
}
} // namespace

int main() {
    {
        auto owner=std::make_unique<OwnedFixture>();
        auto* const parent=static_cast<void*>(&owner->parent);
        auto* const secondary=static_cast<void*>(&owner->embedded);
        const auto parentAddress=reinterpret_cast<std::uintptr_t>(parent);
        const auto secondaryAddress=reinterpret_cast<std::uintptr_t>(secondary);
        if(secondaryAddress-parentAddress!=kSecondaryOffset)
            return fail(1,"synthetic secondary pointer is not +0x10014");
        if(parent==secondary)
            return fail(2,"returned secondary interface aliases primary owner");
        g_expected_secondary=secondary;
        auto** const parentTable=*reinterpret_cast<void***>(parent);
        auto** const secondaryTable=*reinterpret_cast<void***>(secondary);
        if(!parentTable || !secondaryTable || parentTable==secondaryTable)
            return fail(3,"independent primary/secondary MSVC x86 vtables");
        for(unsigned slot=0;slot<3;slot++)
            if(parentTable[slot]==nullptr)
                return fail(4,"missing primary parent virtual slot");
        for(unsigned slot=0;slot<9;slot++)
            if(secondaryTable[slot]==nullptr)
                return fail(5,"missing secondary virtual slot");
        // Methods with distinct native slot identities must not fold into
        // the same method pointer. Every function captures its own index.
        for(unsigned slot=0;slot<9;slot++)
            for(unsigned previous=0;previous<slot;previous++)
                if(secondaryTable[slot]==secondaryTable[previous])
                    return fail(6,"secondary native methods aliased");

        for(unsigned slot=0;slot<9;slot++) {
            const std::uint32_t argument=0x7F120000u+slot*0x1121u;
            const auto used=(slot==6u || slot==8u)?0u:argument;
            std::uint32_t result=0;
            bool balanced=false;
            if(slot==6u || slot==8u)
                balanced=call_nullary(secondary,secondaryTable[slot],result);
            else if(slot==7u)
                balanced=call_setter(secondary,secondaryTable[slot],argument);
            else
                balanced=call_unary(secondary,secondaryTable[slot],argument,result);
            if(!balanced)
                return fail(7,"ECX thiscall stack unbalanced at virtual slot");
            if(slot!=7u && result!=(kBaseValue^(used+slot*0x103u)))
                return fail(8,"wrong synthetic native virtual return");
            if(g.called[slot]!=1u || g.argument[slot]!=used || g.receiver_error)
                return fail(9,"wrong ECX receiver, argument or slot identity");
        }
        if(g.parent_destructions!=0u)
            return fail(10,"unexpected primary deletion while dispatching secondary");
        // Deliberately never call a slot through a forged primary pointer:
        // the original engine would invoke an unrelated parent virtual.
    }
    if(g.parent_destructions!=1u)
        return fail(11,"native primary fixture destruction did not happen once");
    std::puts("PASS Phase1656 original-free MSVC Win32 x86: "
              "secondary pointer owner+0x10014, distinct primary/secondary "
              "vftables, nine original-position callback slots, 7 RET4 "
              "one-DWORD slots and 2 RET0 getters, exact ECX/ESP/dispatch");
    return 0;
}
