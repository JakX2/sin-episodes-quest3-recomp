#include "sin_compat/original_client_static_recvtable_links.hpp"
#include "sin_compat/original_client_class_catalog.hpp"
#include "sin_compat/original_client_aux_recvtable_metadata.hpp"
#include "sin_compat/original_client_recvtable_preflight.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace links=sin_compat::original_client_static_recvtable_links;
namespace catalog=sin_compat::original_client_class_catalog;
namespace aux=sin_compat::original_client_aux_recvtable_metadata;
namespace graph=sin_compat::original_client_recvtable_preflight;

static bool CheckRealSourceGraph(
    const std::array<links::Edge,links::kOriginalStaticNestedEdges>& edges) {
    std::array<std::vector<std::size_t>,links::kOriginalStaticTableCount> adj{};
    std::unordered_set<std::uint32_t> parentAndSlot;
    std::size_t classEdges=0,auxEdges=0;
    for(const auto& edge:edges) {
        if(edge.parentTableOrdinal>=links::kOriginalStaticTableCount ||
           edge.childTableOrdinal>=links::kOriginalStaticTableCount)
            return false;
        const auto key=std::uint32_t(edge.parentTableOrdinal)*256u+
                       edge.parentPropertySlot;
        if(!parentAndSlot.insert(key).second)return false;
        adj[edge.parentTableOrdinal].push_back(edge.childTableOrdinal);
        if(edge.parentTableOrdinal<links::kOriginalClassTableCount)
            ++classEdges;
        else
            ++auxEdges;
    }
    if(classEdges!=191u || auxEdges!=1u || parentAndSlot.size()!=192u)
        return false;
    std::array<std::uint8_t,links::kOriginalStaticTableCount> state{};
    std::array<int,links::kOriginalStaticTableCount> memo{};
    std::function<int(std::size_t)> depth=[&](std::size_t index) -> int {
        if(state[index]==1u)return -1; // cycle in source nested table graph
        if(state[index]==2u)return memo[index];
        state[index]=1u;
        int maximum=0;
        for(const auto child:adj[index]) {
            const int d=depth(child);
            if(d<0)return -1;
            if(d>maximum)maximum=d;
        }
        state[index]=2u;
        memo[index]=1+maximum;
        return memo[index];
    };
    int maximum=0;
    for(std::size_t i=0u;i<state.size();++i) {
        const int d=depth(i);
        if(d<0)return false;
        if(d>maximum)maximum=d;
    }
    return maximum==static_cast<int>(links::kOriginalStaticGraphMaxDepth);
}

static void test_real_source_192_edges_not_synthetic(const char* manifest) {
    static_assert(links::kOriginalClassTableCount==191u);
    static_assert(links::kOriginalAuxTableCount==20u);
    static_assert(links::kOriginalStaticTableCount==211u);
    static_assert(links::kOriginalStaticNestedEdges==192u);
    static_assert(links::kOriginalDynamicGenericNestedWrappers==14u);
    static_assert(links::kOriginalSpecialOverlayNestedWrapper==1u);
    static_assert(links::kOriginalNativeTypeFiveProps==207u);
    static_assert(links::kOriginalStaticGraphMaxDepth==9u);
    assert(!links::CanExposeToRetailEngine());

    std::ifstream input(manifest);
    assert(input.good());
    catalog::Catalog originalClasses;
    assert(originalClasses.Load(input) && originalClasses.Size()==191u);

    graph::Graph realGraph;
    std::array<std::uint32_t,links::kOriginalStaticTableCount> realSourceVAs{};
    std::vector<std::size_t> roots;
    std::size_t n=0u;
    std::unordered_set<std::uint32_t> distinctNativeTableVAs;
    for(const auto* node=originalClasses.Head();node;node=node->next) {
        assert(n<191u);
        realSourceVAs[n]=node->originalRecvTableVA;
        assert(distinctNativeTableVAs.insert(node->originalRecvTableVA).second);
        const auto index=realGraph.AddTable(node->networkName);
        assert(index==n);
        roots.push_back(index);
        ++n;
    }
    assert(n==191u);
    for(const auto& source:aux::kOriginalAuxiliaryTables) {
        realSourceVAs[n]=source.originalTableVA;
        assert(distinctNativeTableVAs.insert(source.originalTableVA).second);
        const auto index=realGraph.AddTable(std::string(source.networkTableName));
        assert(index==n);
        roots.push_back(index);
        ++n;
    }
    assert(n==211u && roots.size()==211u);
    assert(distinctNativeTableVAs.size()==211u && realGraph.TableCount()==211u);
    assert(realSourceVAs[0]==0x2437F720u);   // original CWeaponMelee
    assert(realSourceVAs[203]==0x24338E8Cu); // original DT_LocalPlayerExclusive
    assert(realSourceVAs[202]==0x24338E74u); // original DT_Local

    // All 192 source real static type5 edges, not synthetic class/slot data.
    assert(CheckRealSourceGraph(links::kOriginalSourceEdges));
    for(const auto& edge:links::kOriginalSourceEdges) {
        const auto source=edge.parentTableOrdinal;
        const auto target=edge.childTableOrdinal;
        const auto* before=realGraph.FindTable(source);
        assert(before);
        const auto childPosition=before->props.size();
        assert(realGraph.AddProperty(
            source,"real_source_property_slot_"+std::to_string(edge.parentPropertySlot),
            5));
        assert(realGraph.LinkNested(source,childPosition,target));
    }
    const auto report=realGraph.Validate(roots);
    assert(report.good() && report.visitedTables==211u &&
           report.seenProperties==192u && report.nestedEdges==192u);
    assert(!realGraph.CanExposeToRetailEngine());
    // The authentic original extra auxiliary link (not one of 191
    // registered class links) is LocalPlayerExclusive.m_Local -> Local.
    const auto& last=links::kOriginalSourceEdges.back();
    assert(last.parentTableOrdinal==203u &&
           last.parentPropertySlot==0u &&
           last.childTableOrdinal==202u);
    // Adversarial modifications to authentic graph topology MUST fail.
    {
        auto wrong=links::kOriginalSourceEdges;
        wrong[0].childTableOrdinal=wrong[0].parentTableOrdinal;
        assert(!CheckRealSourceGraph(wrong)); // self-cycle
        wrong=links::kOriginalSourceEdges;
        wrong[0]=wrong[1];
        assert(!CheckRealSourceGraph(wrong)); // duplicate parent slot
        wrong=links::kOriginalSourceEdges;
        wrong.back().childTableOrdinal=0xFFu;
        assert(!CheckRealSourceGraph(wrong)); // invalid original index
        wrong=links::kOriginalSourceEdges;
        wrong.back().parentTableOrdinal=0u;
        assert(!CheckRealSourceGraph(wrong)); // class/aux distribution changes
    }
}

int main(int argc,char** argv) {
    assert(argc==2);
    test_real_source_192_edges_not_synthetic(argv[1]);
    std::puts("PASS Phase1699 REAL original 211-table/192-static-type5-edge graph: source ordinals, native VAs, acyclic depth9, 14 dynamic plus one overlay classification, mutations refused; retail ABI gated");
    return 0;
}
