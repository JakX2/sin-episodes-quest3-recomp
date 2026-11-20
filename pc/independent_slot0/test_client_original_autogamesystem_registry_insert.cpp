// Original 2006 global AutoGameSystem source positional insert and
// append wrapper. Synthetic IDs only; no engine allocator or pointers.
#include "client_original_autogamesystem_registry_remove.h"
#include <array>
#include <cstdio>

namespace o=sin_independent::original_gamesystem_registry;
static int Fail(int n){
    std::fprintf(stderr,"original AutoGameSystem ordered insert fixture %d\n",n);
    return n;
}
int main(){
    static_assert(o::kOriginalInsertVA==0x24156640u);
    static_assert(o::kOriginalAppendVA==0x24156800u);
    static_assert(o::kOriginalEnsureCapacityVA==0x241562F0u);
    static_assert(o::kOriginalMemmoveVA==0x24248B50u);
    o::OwnedRegistry registry{};
    const auto first=registry.Append(11u);
    if(first.result!=o::InsertResult::Inserted ||
       first.index!=0 ||first.count_before!=0 ||
       first.count_after!=1 ||first.elements_shifted!=0 ||
       first.original_capacity_growth_simulated ||
       !registry.mirror_points_to_owned_array() ||
       registry.size()!=1 ||registry.At(0)!=11u)
        return Fail(1);
    constexpr std::uint32_t seed[]={10u,20u,30u};
    if(registry.Seed(seed,3)!=o::SeedResult::Accepted ||
       registry.mirror_points_to_owned_array())
        return Fail(2);
    const auto before=registry.revision();
    const auto middle=registry.InsertAt(1,99u);
    if(middle.result!=o::InsertResult::Inserted ||
       middle.index!=1 ||middle.elements_shifted!=2 ||
       middle.count_before!=3 ||middle.count_after!=4 ||
       registry.size()!=4 ||registry.At(0)!=10u ||
       registry.At(1)!=99u ||registry.At(2)!=20u ||
       registry.At(3)!=30u ||
       registry.revision()!=before+1 ||
       !registry.mirror_points_to_owned_array())
        return Fail(3);

    // The original append wrapper uses count as the insert index.
    // It never shifts an already registered pointer.
    const auto appended=registry.Append(99u);
    if(appended.result!=o::InsertResult::Inserted ||
       appended.index!=4 ||appended.elements_shifted!=0 ||
       registry.At(4)!=99u ||registry.size()!=5)
        return Fail(4);
    const auto front=registry.InsertAt(0,0u);
    if(front.result!=o::InsertResult::Inserted ||
       front.index!=0 ||front.elements_shifted!=5 ||
       registry.At(0)!=0u ||registry.At(1)!=10u ||
       registry.At(2)!=99u ||registry.At(3)!=20u ||
       registry.At(4)!=30u ||registry.At(5)!=99u)
        return Fail(5);
    const auto revision=registry.revision();
    const auto invalid=registry.InsertAt(999,1234u);
    if(invalid.result!=o::InsertResult::InvalidIndex ||
       registry.size()!=6 ||registry.revision()!=revision ||
       registry.At(5)!=99u)return Fail(6);

    // Reverse contract: erase removes only the FIRST duplicate,
    // keeps the append duplicate in the same relative order,
    // and decrements the active count without blanket clearing.
    const auto erased=registry.RemoveFirst(99u);
    if(erased.result!=o::RemoveResult::Removed ||
       erased.index!=2 ||erased.elements_shifted!=3 ||
       registry.size()!=5 ||
       registry.At(0)!=0u ||registry.At(1)!=10u ||
       registry.At(2)!=20u ||registry.At(3)!=30u ||
       registry.At(4)!=99u ||
       !registry.mirror_points_to_owned_array())
        return Fail(7);

    // Use the bounded independent capacity rather than claiming
    // the original 2006 EnsureCapacity realloc behavior. At limit,
    // the registry must remain unchanged and must not write OOB.
    std::array<std::uint32_t,o::kOwnedRegistryMax> full{};
    for(std::size_t i=0;i<full.size();++i)
        full[i]=static_cast<std::uint32_t>(i+1u);
    if(registry.Seed(full.data(),full.size())!=
       o::SeedResult::Accepted)return Fail(8);
    const auto capRevision=registry.revision();
    const auto refused=registry.Append(3u);
    if(refused.result!=o::InsertResult::OwnedCapacityRefused ||
       refused.count_before!=o::kOwnedRegistryMax ||
       refused.count_after!=o::kOwnedRegistryMax ||
       registry.revision()!=capRevision ||
       registry.size()!=o::kOwnedRegistryMax ||
       registry.At(o::kOwnedRegistryMax-1u)!=
           static_cast<std::uint32_t>(o::kOwnedRegistryMax))
        return Fail(9);
    std::puts("PASS SHA-original 2006 AutoGameSystem positional insert and append: ordered 4-byte shifts, duplicates, first-match erase, mirror+10 source write, invalid-index and independent capacity refusal");
    return 0;
}
