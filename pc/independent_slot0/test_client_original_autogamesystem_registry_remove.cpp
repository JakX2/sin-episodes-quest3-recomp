// Original 2006 SiN base game-system registry removal: source-free
// x86/native fabricated pointer-ID list. No engine memory dereference.
#include "client_original_autogamesystem_registry_remove.h"
#include <array>
#include <cstdio>

namespace o=sin_independent::original_gamesystem_registry;
static int Fail(int n){
    std::fprintf(stderr,"2006 original base registry destructor fixture %d\n",n);
    return n;
}
int main(){
    static_assert(o::kOriginalBaseDestructorVA==0x24156820u);
    static_assert(o::kOriginalRemoveVA==0x241565E0u);
    static_assert(o::kOriginalRegistryVA==0x242FBFA8u);
    static_assert(o::kOriginalRegistryCountVA==0x242FBFB4u);
    static_assert(o::kOriginalBaseVtableVA==0x24293B1Cu);
    o::OwnedRegistry registry{};
    const auto empty=registry.RemoveFirst(123u);
    if(empty.result!=o::RemoveResult::Empty ||
       empty.count_before!=0 ||empty.count_after!=0 ||
       registry.size()!=0)return Fail(1);

    // Source removal scans from index 0 and removes the FIRST
    // matching identity only, retaining stable order and any
    // duplicate entries still present in the registry.
    constexpr std::uint32_t ids[]={11u,22u,33u,22u};
    if(registry.Seed(ids,4)!=o::SeedResult::Accepted)
        return Fail(2);
    const auto startRevision=registry.revision();
    auto result=registry.RemoveFirst(22u);
    if(result.result!=o::RemoveResult::Removed ||
       result.index!=1 ||result.elements_shifted!=2 ||
       result.count_before!=4 ||result.count_after!=3 ||
       registry.At(0)!=11u ||registry.At(1)!=33u ||
       registry.At(2)!=22u ||registry.size()!=3 ||
       registry.Raw(3)!=22u || // original leaves stale tail intact
       registry.revision()!=startRevision+1)
        return Fail(3);
    result=registry.RemoveFirst(22u);
    if(result.result!=o::RemoveResult::Removed ||
       result.index!=2 ||result.elements_shifted!=0 ||
       result.count_before!=3 ||result.count_after!=2 ||
       registry.At(0)!=11u ||registry.At(1)!=33u ||
       registry.Raw(2)!=22u)return Fail(4);
    const auto unchangedRevision=registry.revision();
    result=registry.RemoveFirst(999u);
    if(result.result!=o::RemoveResult::NotFound ||
       result.count_before!=2 ||result.count_after!=2 ||
       registry.revision()!=unchangedRevision ||
       registry.At(0)!=11u ||registry.At(1)!=33u)
        return Fail(5);

    // Source base destructor always writes base vtable first,
    // then invokes registry-removal with the address of this.
    // Even if object identity was never registered, base
    // destructor still reaches its return path.
    const std::uint32_t duplicateIds[]={10u,20u,10u};
    registry.Seed(duplicateIds,3);
    o::BaseObject object{};
    object.owned_identity=10u;
    const auto destroyed=o::DestroyBaseOwned(registry,object);
    if(destroyed.result!=o::DestructorResult::Applied ||
       !destroyed.original_base_vtable_written ||
       !destroyed.originally_registered ||
       destroyed.registry.result!=o::RemoveResult::Removed ||
       destroyed.registry.index!=0 ||
       destroyed.registry.elements_shifted!=2 ||
       object.original_vtable_metadata!=o::kOriginalBaseVtableVA ||
       !object.independently_destroyed ||
       registry.size()!=2 ||registry.At(0)!=20u ||
       registry.At(1)!=10u)
        return Fail(6);
    const auto duplicateRev=registry.revision();
    const auto again=o::DestroyBaseOwned(registry,object);
    if(again.result!=o::DestructorResult::AlreadyDestroyed ||
       registry.revision()!=duplicateRev ||
       registry.At(1)!=10u)return Fail(7);

    o::BaseObject unregistered{};
    unregistered.owned_identity=99u;
    const auto noMatch=o::DestroyBaseOwned(registry,unregistered);
    if(noMatch.result!=o::DestructorResult::Applied ||
       !noMatch.original_base_vtable_written ||
       noMatch.originally_registered ||
       noMatch.registry.result!=o::RemoveResult::NotFound ||
       unregistered.original_vtable_metadata!=o::kOriginalBaseVtableVA ||
       !unregistered.independently_destroyed ||
       registry.size()!=2)return Fail(8);

    // Source pointer equality permits zero or duplicate DWORD
    // values; no invented uniqueness/non-null guard in registry.
    const std::uint32_t zeroIds[]={0u,1u,0u};
    registry.Seed(zeroIds,3);
    result=registry.RemoveFirst(0u);
    if(result.result!=o::RemoveResult::Removed ||
       registry.size()!=2 ||registry.At(0)!=1u ||
       registry.At(1)!=0u)return Fail(9);

    // Cap is independently owned safety policy and cannot
    // partially mutate an accepted prior registry.
    std::array<std::uint32_t,o::kOwnedRegistryMax+1> oversized{};
    const auto before=registry.revision();
    if(registry.Seed(oversized.data(),oversized.size())!=
       o::SeedResult::TooLarge ||
       registry.revision()!=before ||
       registry.size()!=2)return Fail(10);
    std::puts("PASS SHA-original 2006 global game-system registry first-match stable removal, duplicate retention, memmove source order, stale trailing slot, base-vptr transition, unregistered and repeated teardown safety");
    return 0;
}
