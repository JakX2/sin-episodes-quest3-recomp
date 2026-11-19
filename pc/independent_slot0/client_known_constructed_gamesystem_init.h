#pragma once
// Independent implementations of SHA-audited 2006 SiN direct
// AutoGameSystem Init methods. No original object or engine ABI assumed.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "client_auto_gamesystem_init_core.h"
#include "client_vgui_vtable_probe.h"

namespace sin_independent::constructed_gamesystem {
enum class TrivialSystem { GameStringPool, ClientLeafSecondary, ClientEffectPrecache };
// Exact 3-byte native methods at 0x240566C0, 0x2410CA00 and
// 0x241055A0 all perform "mov al,1; ret".
inline bool InitTrivial(TrivialSystem) noexcept { return true; }

// Original CDetailObjectSystem::Init at 0x24126300 (31 bytes)
// loads DWORD [0x24358930]+0x28 -> [this+0x806C], then
// DWORD [0x243588E8]+0x28 -> [this+0x8070], returns true.
// Host supplies independently owned provider memory rather than
// dereferencing retail-global addresses from another DLL.
inline bool MemoryWritable(const void* address,std::size_t count) noexcept {
    if(!address || !count)return false;
    MEMORY_BASIC_INFORMATION info{};
    if(VirtualQuery(address,&info,sizeof(info))!=sizeof(info) ||
       info.State!=MEM_COMMIT ||
       (info.Protect & (PAGE_NOACCESS|PAGE_GUARD)))return false;
    const auto prot=info.Protect&0xffu;
    if(prot!=PAGE_READWRITE && prot!=PAGE_WRITECOPY &&
       prot!=PAGE_EXECUTE_READWRITE && prot!=PAGE_EXECUTE_WRITECOPY)
        return false;
    const auto start=reinterpret_cast<std::uintptr_t>(address);
    const auto base=reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    return start>=base && count<=info.RegionSize &&
           start-base<=info.RegionSize-count;
}
inline bool InitDetailObject(void* own, const void* worldDetail,
                             const void* staticDetail) noexcept {
    constexpr std::size_t kObjectBytes=0x8074;
    constexpr std::size_t kSourceBytes=0x2C;
    if(!MemoryWritable(own,kObjectBytes) ||
       !vgui_layout::Readable(worldDetail,kSourceBytes) ||
       !vgui_layout::Readable(staticDetail,kSourceBytes))
        return false;
    auto* dst=static_cast<unsigned char*>(own);
    const auto* first=static_cast<const unsigned char*>(worldDetail);
    const auto* second=static_cast<const unsigned char*>(staticDetail);
    std::uint32_t firstValue=0,secondValue=0;
    std::memcpy(&firstValue,first+0x28,sizeof(firstValue));
    std::memcpy(&secondValue,second+0x28,sizeof(secondValue));
    std::memcpy(dst+0x806c,&firstValue,sizeof(firstValue));
    std::memcpy(dst+0x8070,&secondValue,sizeof(secondValue));
    return true;
}

// Each is a real reconstructed method outcome, not a runtime pointer to
// the original client's BSS memory. The secondary leaf interface must
// be registered at +4 on PE32, not as the primary leaf object.
struct StringPoolSystem: auto_gamesystem::System {
    bool Init() noexcept override {return InitTrivial(TrivialSystem::GameStringPool);}
};
struct ClientLeafSecondarySystem: auto_gamesystem::System {
    bool Init() noexcept override {return InitTrivial(TrivialSystem::ClientLeafSecondary);}
};
struct EffectPrecacheSystem: auto_gamesystem::System {
    bool Init() noexcept override {return InitTrivial(TrivialSystem::ClientEffectPrecache);}
};
struct LeafComposite {
    void* primary=nullptr;
    ClientLeafSecondarySystem game_system;
};
} // namespace sin_independent::constructed_gamesystem
