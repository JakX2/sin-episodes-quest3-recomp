// Standalone Windows x86 ABI test, using only independently authored objects.
// Never loads, executes, forwards to, or exposes the retail server.dll.
//
// The 7 original ServerGameEnts001 method RVAs were SHA-authenticated
// independently. This test checks native MSVC pointer arguments, returns,
// ordered slots, one virtual deleting destructor, and stack cleanup.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "server_gameents_abi.h"
#include <cstdint>
#include <cstdio>
#include <new>
#include <type_traits>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error MSVC native Windows x86 required
#endif

using sin_independent::ServerGameEntsAbi;
using DebugSig = void (__thiscall ServerGameEntsAbi::*)(edict_t*) noexcept;
using TouchSig = void (__thiscall ServerGameEntsAbi::*)(edict_t*,edict_t*) noexcept;
using FreeSig = void (__thiscall ServerGameEntsAbi::*)(edict_t*) noexcept;
using BaseToEdictSig = edict_t* (__thiscall ServerGameEntsAbi::*)(CBaseEntity*) noexcept;
using EdictToBaseSig = CBaseEntity* (__thiscall ServerGameEntsAbi::*)(edict_t*) noexcept;
using TransmitSig = void (__thiscall ServerGameEntsAbi::*)(
    CCheckTransmitInfo*,const unsigned short*,int) noexcept;
static_assert(std::is_same<decltype(&ServerGameEntsAbi::SetDebugEdictBase),DebugSig>::value,
              "original slot 1 calling convention or pointer type");
static_assert(std::is_same<decltype(&ServerGameEntsAbi::MarkEntitiesAsTouching),TouchSig>::value,
              "original slot 2 calling convention or pointer types");
static_assert(std::is_same<decltype(&ServerGameEntsAbi::FreeContainingEntity),FreeSig>::value,
              "original slot 3 calling convention or pointer type");
static_assert(std::is_same<decltype(&ServerGameEntsAbi::BaseEntityToEdict),BaseToEdictSig>::value,
              "original slot 4 pointer return or parameter");
static_assert(std::is_same<decltype(&ServerGameEntsAbi::EdictToBaseEntity),EdictToBaseSig>::value,
              "original slot 5 pointer return or parameter");
static_assert(std::is_same<decltype(&ServerGameEntsAbi::CheckTransmit),TransmitSig>::value,
              "original slot 6 arguments or calling convention");
static_assert(sizeof(void*) == 4, "the original game is Windows PE32");

static int calls[7] = {};
static edict_t* debugBase = nullptr;
static edict_t* touching1 = nullptr;
static edict_t* touching2 = nullptr;
static edict_t* freed = nullptr;
static CBaseEntity* knownBase = nullptr;
static edict_t* knownEdict = nullptr;
static CCheckTransmitInfo* transmitInfo = nullptr;
static const unsigned short* transmitIndices = nullptr;
static int transmitCount = -1;

class SyntheticGameEnts final : public ServerGameEntsAbi {
public:
    ~SyntheticGameEnts() noexcept override { ++calls[0]; }
    void __thiscall SetDebugEdictBase(edict_t* p) noexcept override {
        ++calls[1]; debugBase = p;
    }
    void __thiscall MarkEntitiesAsTouching(
        edict_t* a, edict_t* b) noexcept override {
        ++calls[2]; touching1 = a; touching2 = b;
    }
    void __thiscall FreeContainingEntity(edict_t* p) noexcept override {
        ++calls[3]; freed = p;
    }
    edict_t* __thiscall BaseEntityToEdict(
        CBaseEntity* p) noexcept override {
        ++calls[4]; return p == knownBase ? knownEdict : nullptr;
    }
    CBaseEntity* __thiscall EdictToBaseEntity(
        edict_t* p) noexcept override {
        ++calls[5]; return p == knownEdict ? knownBase : nullptr;
    }
    void __thiscall CheckTransmit(
        CCheckTransmitInfo* info, const unsigned short* indices,
        int count) noexcept override {
        ++calls[6]; transmitInfo = info;
        transmitIndices = indices; transmitCount = count;
    }
};
static_assert(sizeof(SyntheticGameEnts) == sizeof(void*),
              "synthetic singleton must contain only one virtual-table pointer");

static int fail(int code, const char* text) {
    std::fprintf(stderr,"FAIL gameents x86 ABI %d: %s\n",code,text);
    return code;
}

// Compile with native MSVC x86: a mismatched __cdecl / __stdcall / argument
// count leaves ESP inconsistent across these *actual virtual-slot calls*.
#define REQUIRE_BALANCED(expr, code) do {                         \
    std::uintptr_t before = 0;                                   \
    std::uintptr_t after = 0;                                    \
    __asm { mov before, esp }                                     \
    expr;                                                         \
    __asm { mov after, esp }                                      \
    if (before != after) return fail(code,"stack cleanup mismatch"); \
} while (false)

int main() {
    // Opaque, distinct tokens. No user-game entity memory is dereferenced.
    int edictToken1 = 0;
    int edictToken2 = 0;
    int baseToken = 0;
    int transmitToken = 0;
    unsigned short indexTokens[] = {5, 7};
    auto* e1 = reinterpret_cast<edict_t*>(&edictToken1);
    auto* e2 = reinterpret_cast<edict_t*>(&edictToken2);
    auto* base = reinterpret_cast<CBaseEntity*>(&baseToken);
    auto* info = reinterpret_cast<CCheckTransmitInfo*>(&transmitToken);
    knownBase = base;
    knownEdict = e1;

    auto* object = new (std::nothrow) SyntheticGameEnts;
    if (!object) return fail(1,"allocation");
    auto** slots = *reinterpret_cast<void***>(object);
    for (int i = 0; i < 7; ++i) {
        if (!slots[i]) return fail(2,"missing original-contract virtual slot");
    }
    // [0] MSVC scalar deleting destructor; test last.
    auto setDebug = reinterpret_cast<void (__thiscall *)(void*,edict_t*)>(slots[1]);
    auto markTouching = reinterpret_cast<void (__thiscall *)(void*,edict_t*,edict_t*)>(slots[2]);
    auto freeEntity = reinterpret_cast<void (__thiscall *)(void*,edict_t*)>(slots[3]);
    auto baseToEdict = reinterpret_cast<edict_t* (__thiscall *)(void*,CBaseEntity*)>(slots[4]);
    auto edictToBase = reinterpret_cast<CBaseEntity* (__thiscall *)(void*,edict_t*)>(slots[5]);
    auto checkTransmit = reinterpret_cast<void (__thiscall *)(
        void*,CCheckTransmitInfo*,const unsigned short*,int)>(slots[6]);
    REQUIRE_BALANCED(setDebug(object,e1),3);
    if (calls[1] != 1 || debugBase != e1) return fail(4,"slot 1 SetDebugEdictBase");
    REQUIRE_BALANCED(markTouching(object,e1,e2),5);
    if (calls[2] != 1 || touching1 != e1 || touching2 != e2)
        return fail(6,"slot 2 MarkEntitiesAsTouching");
    REQUIRE_BALANCED(freeEntity(object,e2),7);
    if (calls[3] != 1 || freed != e2) return fail(8,"slot 3 FreeContainingEntity");
    edict_t* outEdict = nullptr;
    REQUIRE_BALANCED(outEdict = baseToEdict(object,base),9);
    if (calls[4] != 1 || outEdict != e1)
        return fail(10,"slot 4 BaseEntityToEdict");
    CBaseEntity* outBase = nullptr;
    REQUIRE_BALANCED(outBase = edictToBase(object,e1),11);
    if (calls[5] != 1 || outBase != base)
        return fail(12,"slot 5 EdictToBaseEntity");
    REQUIRE_BALANCED(checkTransmit(object,info,indexTokens,2),13);
    if (calls[6] != 1 || transmitInfo != info ||
        transmitIndices != indexTokens || transmitCount != 2)
        return fail(14,"slot 6 CheckTransmit");
    // The first virtual entry's hidden 4-byte deleting-destructor flag
    // matches the original server's ret 4. Flag 1 deletes our HEAP object.
    auto deletingDestructor =
        reinterpret_cast<void* (__thiscall *)(void*,unsigned int)>(slots[0]);
    REQUIRE_BALANCED((void)deletingDestructor(object,1u),15);
    if (calls[0] != 1) return fail(16,"slot 0 scalar deleting destructor");

    std::puts("PASS synthetic MSVC x86 ServerGameEnts001: 7 typed virtual slots, return values, pointer order, stack cleanup");
    return 0;
}
