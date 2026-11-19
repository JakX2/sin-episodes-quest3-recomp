#pragma once
// Original 2006 SiN client.dll twelve direct AutoGameSystem registrations,
// now traced to actual MSVC RTTI and virtual Init slot0 addresses.
// This is an evidence map, NOT an array of live original-game pointers.
#include <array>
#include <cstdint>
#include "client_auto_gamesystem_init_core.h"
namespace sin_independent::gamesystem_sequence {
struct Entry {
    std::uint32_t callsite_va;
    const char* class_name;
    std::uint32_t original_object_va;
    std::uint32_t registered_subobject_va;
    std::uint32_t original_vtable_va;
    std::uint32_t original_init_va;
    bool init_body_reconstructed;
};
inline constexpr std::array<Entry,12> kOriginalOrder{{
    {0x240FE3FCu,"CGameStringPool",0x2436212Cu,0x2436212Cu,0x2429AD94u,0x240566C0u,true},
    {0x240FE407u,"CSoundEmitterSystem",0x242FFC1Cu,0x242FFC1Cu,0x242A6BE8u,0x241BC950u,true},
    {0x240FE420u,"CClientLeafSystem",0x24357F30u,0x24357F34u,0x24297534u,0x2410CA00u,true},
    {0x240FE42Bu,"CDetailObjectSystem",0x243589A8u,0x243589A8u,0x24298694u,0x24126300u,true},
    {0x240FE436u,"CViewportClientSystem",0x2431DD84u,0x2431DD84u,0x242AC284u,0x241ED310u,true},
    {0x240FE441u,"CClientEffectPrecacheSystem",0x2433FD9Cu,0x2433FD9Cu,0x24297334u,0x241055A0u,true},
    {0x240FE44Du,"CClientShadowMgr",0x24358280u,0x24358280u,0x24297C4Cu,0x24118880u,false},
    {0x240FE457u,"C_StatsSystem",0x24378DE8u,0x24378DE8u,0x242AE124u,0x241FCAA0u,false},
    {0x240FE461u,"C_DynamicDifficulty",0x24378D18u,0x24378D18u,0x242ADF34u,0x241FAC90u,false},
    {0x240FE46Bu,"CClientThinkList",0x242FA6B8u,0x242FA6B8u,0x24297300u,0x24103DF0u,true},
    {0x240FE476u,"C_SoundscapeSystem",0x2433E4F8u,0x2433E4F8u,0x24293C34u,0x240F4160u,false},
    {0x240FE481u,"CPerfVisualBenchmark",0x242FC9DCu,0x242FC9DCu,0x242A209Cu,0x24182800u,false}
}};
constexpr unsigned ReconstructedCount() noexcept {
    unsigned n=0;for(const auto& s:kOriginalOrder)if(s.init_body_reconstructed)++n;
    return n;
}
constexpr unsigned PendingCount() noexcept {
    return static_cast<unsigned>(kOriginalOrder.size())-ReconstructedCount();
}
constexpr bool MatchesVerifiedCalls() noexcept {
    if(kOriginalOrder.size()!=auto_gamesystem::kOriginalDirectRegistrations.size())
        return false;
    for(std::size_t i=0;i<kOriginalOrder.size();++i) {
        if(kOriginalOrder[i].callsite_va!=
           auto_gamesystem::kOriginalDirectRegistrations[i].callsite_va)
            return false;
    }
    return true;
}
static_assert(MatchesVerifiedCalls(),"Original ABI order must match callsite audit");
static_assert(ReconstructedCount()==7,"Seven complete original Init bodies");
static_assert(PendingCount()==5,"Five game-system Init bodies remain");
} // namespace sin_independent::gamesystem_sequence
