#include "sin_compat/original_client_aux_recvtable_metadata.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <string>
#include <unordered_set>

namespace aux=sin_compat::original_client_aux_recvtable_metadata;

int main() {
    static_assert(aux::kOriginalAuxiliaryTableCount==20);
    static_assert(aux::kOriginalAuxiliaryPropSlots==171);
    static_assert(aux::kOriginalStaticTableCount==211);
    static_assert(aux::kOriginalStaticPropSlots==1110);
    static_assert(aux::CountOriginalAuxiliarySlots()==171);
    assert(!aux::CanExposeToRetailEngine());
    std::size_t properties=0;
    std::uint32_t preceding=0;
    std::unordered_set<std::string> seen;
    for(const auto& item:aux::kOriginalAuxiliaryTables) {
        assert(item.originalTableVA>preceding);
        preceding=item.originalTableVA;
        assert(item.originalTableVA>=0x24000000u &&
               item.originalTableVA<0x24390000u);
        assert(item.originalPropertyArrayVA>=0x24000000u &&
               item.originalPropertyArrayVA<0x24390000u);
        assert(seen.emplace(std::string(item.networkTableName)).second);
        assert(aux::Find(item.networkTableName)==&item);
        properties+=item.declaredPropertyCount;
    }
    assert(seen.size()==20 && properties==171);
    assert(!aux::Find("DT_FabricatedUnknown"));
    const auto* local=aux::Find("DT_LocalPlayerExclusive");
    const auto* weapon=aux::Find("DT_LocalWeaponData");
    const auto* rules=aux::Find("DT_SE1GameRules");
    const auto* hit=aux::Find("DT_EnvHeadcrabCanisterShared");
    assert(local && local->declaredPropertyCount==27);
    assert(weapon && weapon->declaredPropertyCount==5);
    assert(rules && rules->declaredPropertyCount==0);
    assert(aux::kOriginalTotalOrdinaryNestedLinks==192u);
    assert(aux::kOriginalClassOrdinaryNestedLinks==191u);
    assert(aux::kOriginalAuxOrdinaryNestedLinks==1u);
    assert(local->originalTableVA==aux::kOriginalLocalPlayerTableVA);
    assert(aux::Find("DT_Local")->originalTableVA==
           aux::kOriginalLocalChildTableVA);
    assert(aux::kOriginalLocalNativeMemberOffset==0x0d68u);
    assert(hit && hit->declaredPropertyCount==15);
    assert(aux::kOriginalTypedIntegerAux==75 &&
           aux::kOriginalTypedFloatAux==62 &&
           aux::kOriginalTypedVectorAux==28 &&
           aux::kOriginalTypedArrayAux==1 &&
           aux::kOriginalTypedNestedAux==5);
    std::puts("PASS Phase1698 original 20 auxiliary RecvTables: 171 native property slots, all 211 static tables / 1110 slots accounted, unique ordered source VA labels, and retail ABI gated");
    return 0;
}
