#pragma once
// Phase1695: original SiN 2006 ClientLeafSystem001 interface FACTORY ONLY.
// Exact original singleton factory 0x24108B20 returns CClientLeafSystem
// primary +0. Original source MSVC RTTI has primary 22 virtual entries,
// AutoGameSystem secondary subobject +4 with 12 virtual entries, and a
// third subobject +8 with one virtual entry. The 2006 engine may retain
// and later call those pointers. NO renderer or leaf behavior is modeled.
//
// Every attempted virtual method is a TERMINAL diagnostic. __fastcall
// guard functions do NOT emulate original __thiscall cleanup: they MUST
// NEVER return and never invent a source ABI/result for unknown slots.
// This is an opt-in NEGOTIATION boundary, not a functional leaf service.
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1695 ClientLeafSystem001 cold interface requires MSVC x86
#endif

namespace sin_independent::native_client_leaf_cold_factory {
inline constexpr char kInterfaceName[]="ClientLeafSystem001";
inline constexpr unsigned kOriginalPrimarySlots=22u;
inline constexpr unsigned kOriginalAutoSystemSlots=12u;
inline constexpr unsigned kOriginalAuxiliarySlots=1u;
inline constexpr unsigned kOriginalAutoSystemOffset=4u;
inline constexpr unsigned kOriginalAuxiliaryOffset=8u;
using RefuseCallback=void (*)(const char*) noexcept;
// The two register arguments allow the fatal thunks to be entered from
// native x86 __thiscall dispatch without reading an unknown stack ABI.
using FatalThunk=void (__fastcall *)(void*,void*) noexcept;
static_assert(sizeof(void*)==4u && sizeof(FatalThunk)==4u,
              "native client leaf x86 vtable pointer width changed");

class Cold final {
public:
    explicit Cold(RefuseCallback refuse) noexcept:
        primary_(kPrimary),autosystem_(kAuto),auxiliary_(kAux),
        refuse_(refuse) {}

    void BindCold() noexcept {active_=true;}
    void Revoke() noexcept {active_=false;}
    bool Active() const noexcept {return active_;}
    void* Interface() noexcept {return this;}
    const void* PrimaryTable() const noexcept {return primary_;}
    const void* AutoTable() const noexcept {return autosystem_;}
    const void* AuxiliaryTable() const noexcept {return auxiliary_;}
    bool SourceColdOffsets() const noexcept {
        const auto* base=reinterpret_cast<const unsigned char*>(this);
        return reinterpret_cast<const unsigned char*>(&primary_)==base &&
               reinterpret_cast<const unsigned char*>(&autosystem_)==base+4u &&
               reinterpret_cast<const unsigned char*>(&auxiliary_)==base+8u;
    }
private:
    // These functions never return. A function's exact number of native
    // stack arguments is deliberately UNKNOWN, not guessed as void().
    [[noreturn]] static void Refuse(void* self,unsigned adjustment,
                                   const char* label) noexcept {
        if(self) {
            auto* owner=reinterpret_cast<Cold*>(
                static_cast<unsigned char*>(self)-adjustment);
            if(owner->refuse_)owner->refuse_(label);
        }
        std::abort(); // only if the caller's terminal callback returns
    }
    template<unsigned I>
    [[noreturn]] static void __fastcall PrimaryGuard(void* self,void*) noexcept {
        char label[104]={};
        (void)std::snprintf(label,sizeof(label),
                            "ClientLeafSystem001.primary.slot%u.unimplemented",I);
        Refuse(self,0u,label);
    }
    template<unsigned I>
    [[noreturn]] static void __fastcall AutoGuard(void* self,void*) noexcept {
        char label[104]={};
        (void)std::snprintf(label,sizeof(label),
                            "ClientLeafSystem001.auto-game-system.slot%u.unimplemented",I);
        Refuse(self,4u,label);
    }
    template<unsigned I>
    [[noreturn]] static void __fastcall AuxGuard(void* self,void*) noexcept {
        char label[104]={};
        (void)std::snprintf(label,sizeof(label),
                            "ClientLeafSystem001.third-subobject.slot%u.unimplemented",I);
        Refuse(self,8u,label);
    }
    inline static FatalThunk kPrimary[kOriginalPrimarySlots]={
        &Cold::PrimaryGuard<0>,
        &Cold::PrimaryGuard<1>,
        &Cold::PrimaryGuard<2>,
        &Cold::PrimaryGuard<3>,
        &Cold::PrimaryGuard<4>,
        &Cold::PrimaryGuard<5>,
        &Cold::PrimaryGuard<6>,
        &Cold::PrimaryGuard<7>,
        &Cold::PrimaryGuard<8>,
        &Cold::PrimaryGuard<9>,
        &Cold::PrimaryGuard<10>,
        &Cold::PrimaryGuard<11>,
        &Cold::PrimaryGuard<12>,
        &Cold::PrimaryGuard<13>,
        &Cold::PrimaryGuard<14>,
        &Cold::PrimaryGuard<15>,
        &Cold::PrimaryGuard<16>,
        &Cold::PrimaryGuard<17>,
        &Cold::PrimaryGuard<18>,
        &Cold::PrimaryGuard<19>,
        &Cold::PrimaryGuard<20>,
        &Cold::PrimaryGuard<21>
    };
    inline static FatalThunk kAuto[kOriginalAutoSystemSlots]={
        &Cold::AutoGuard<0>,
        &Cold::AutoGuard<1>,
        &Cold::AutoGuard<2>,
        &Cold::AutoGuard<3>,
        &Cold::AutoGuard<4>,
        &Cold::AutoGuard<5>,
        &Cold::AutoGuard<6>,
        &Cold::AutoGuard<7>,
        &Cold::AutoGuard<8>,
        &Cold::AutoGuard<9>,
        &Cold::AutoGuard<10>,
        &Cold::AutoGuard<11>
    };
    inline static FatalThunk kAux[kOriginalAuxiliarySlots]={
        &Cold::AuxGuard<0>
    };
    // These are the only three native layout fields copied from the 2006
    // primary object. NOT the original CClientLeafSystem member layout.
    FatalThunk* primary_;
    FatalThunk* autosystem_;
    FatalThunk* auxiliary_;
    RefuseCallback refuse_=nullptr; // independent-only metadata +0x0C
    bool active_=false;             // independent-only metadata
};
} // namespace sin_independent::native_client_leaf_cold_factory
