// Native MSVC x86 tests for real but tightly bounded original entity
// conversion semantics. Uses synthetic memory only; never loads retail DLLs.
#include "server_entity_conversion_core.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error MSVC Windows x86 required
#endif

class CBaseHandle { public: unsigned int value = 0; };
class CBaseEntity {
public:
    unsigned char prefix[sin_independent::kOriginalEntityEdictOffset] = {};
    edict_t* edict = nullptr;
};
struct edict_t {
    std::uint32_t stateFlags = 0;
    std::uint32_t networkSerialNumber = 0;
    IServerNetworkable* networkable = nullptr;
    sin_independent::ServerUnknownAbi* unknown = nullptr;
};
static_assert(offsetof(CBaseEntity,edict) == 0x16c,
              "Original server entity edict member offset");
static_assert(offsetof(edict_t,unknown) == 0x0c,
              "Original server edict unknown pointer offset");
static_assert(sizeof(edict_t) == 16, "Four original edict header pointers/words");

class SyntheticUnknown final : public sin_independent::ServerUnknownAbi {
public:
    explicit SyntheticUnknown(CBaseEntity* base) : base_(base) {}
    void __thiscall SetRefEHandle(const CBaseHandle& h) noexcept override {
        handle_ = h;
    }
    const CBaseHandle& __thiscall GetRefEHandle() const noexcept override {
        return handle_;
    }
    ICollideable* __thiscall GetCollideable() noexcept override {
        return nullptr;
    }
    IServerNetworkable* __thiscall GetNetworkable() noexcept override {
        return nullptr;
    }
    CBaseEntity* __thiscall GetBaseEntity() noexcept override {
        ++calls_;
        return base_;
    }
    int Calls() const { return calls_; }
private:
    CBaseEntity* base_;
    CBaseHandle handle_;
    int calls_ = 0;
};

static int fail(int code, const char* message) {
    std::fprintf(stderr, "FAIL original entity conversion core %d: %s\n",
                 code, message);
    return code;
}

int main() {
    sin_independent::ServerGameEntsConversionCore core;
    if (core.DebugEdictBase() != nullptr) return fail(1,"debug edict initial");
    edict_t first = {};
    edict_t second = {};
    core.SetDebugEdictBase(&first);
    if (core.DebugEdictBase() != &first) return fail(2,"debug edict set");
    core.SetDebugEdictBase(&second);
    if (core.DebugEdictBase() != &second) return fail(3,"debug edict replace");
    core.SetDebugEdictBase(nullptr);
    if (core.DebugEdictBase() != nullptr) return fail(4,"debug edict clear");

    CBaseEntity base = {};
    base.edict = &first;
    if (core.BaseEntityToEdict(nullptr) != nullptr)
        return fail(5,"null entity must yield null edict");
    if (core.BaseEntityToEdict(&base) != &first)
        return fail(6,"original CBaseEntity::edict offset 0x16c");
    base.edict = nullptr;
    if (core.BaseEntityToEdict(&base) != nullptr)
        return fail(7,"unset entity edict must yield null");
    base.edict = &second;
    if (core.BaseEntityToEdict(&base) != &second)
        return fail(8,"entity edict field update");

    if (core.EdictToBaseEntity(nullptr) != nullptr)
        return fail(9,"null edict must yield null entity");
    if (core.EdictToBaseEntity(&first) != nullptr)
        return fail(10,"edict with null unknown must yield null entity");
    SyntheticUnknown unknown(&base);
    first.unknown = &unknown;
    if (core.EdictToBaseEntity(&first) != &base || unknown.Calls() != 1)
        return fail(11,"original edict +0xc unknown GetBaseEntity vtable");

    // Original uses IServerUnknown::GetBaseEntity (virtual index 5 after
    // IHandleEntity's virtual destructor and two handle methods).
    void** vtable = *reinterpret_cast<void***>(&unknown);
    for (int i=0; i<6; ++i)
        if (vtable[i] == nullptr) return fail(12,"missing unknown slot");
    auto getter = reinterpret_cast<CBaseEntity* (__thiscall *)(void*)>(vtable[5]);
    std::uintptr_t before=0,after=0;
    CBaseEntity* fromSlot=nullptr;
    void* context = &unknown;
    __asm {
        mov eax, esp
        mov before, eax
        mov ecx, context
        mov eax, getter
        call eax
        mov fromSlot, eax
        mov eax, esp
        mov after, eax
    }
    if (fromSlot != &base || before != after || unknown.Calls() != 2)
        return fail(13,"native x86 unknown vtable slot-five GetBaseEntity ABI");

    SyntheticUnknown empty(nullptr);
    second.unknown = &empty;
    if (core.EdictToBaseEntity(&second) != nullptr || empty.Calls() != 1)
        return fail(14,"unknown with no base entity must return null");
    // Synthetic bridge lives entirely outside the game's DLL registration.
    std::puts("PASS isolated MSVC x86 server entity conversion: debug base, entity +0x16c, edict +0xc, unknown slot 5, null paths");
    return 0;
}
