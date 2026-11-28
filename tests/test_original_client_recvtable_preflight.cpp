#include "sin_compat/original_client_class_catalog.hpp"
#include "sin_compat/original_client_recvtable_preflight.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace catalog=sin_compat::original_client_class_catalog;
namespace schema=sin_compat::original_client_recvtable_preflight;

static void check_original_engine_constants() {
    static_assert(schema::kClassRootLimit==1024u);
    static_assert(schema::kOriginalClassRoots==191u);
    static_assert(schema::kOriginalAttachedPropSlots==939u);
    static_assert(schema::kOriginalRecvPropStride==60u);
    static_assert(schema::kOriginalClassRecvTableOffset==12u);
    static_assert(schema::kOriginalClassNextOffset==16u);
    static_assert(schema::kOriginalRecvTableVisitedByteOffset==17u);
    static_assert(schema::kOriginalRecvPropTypeOffset==4u);
    static_assert(schema::kOriginalNestedRecvTableOffset==40u);
    static_assert(schema::kNativeNestedTableType==5);
}

static void test_known_cycle_and_unresolved_rejection() {
    schema::Graph graph;
    const auto a=graph.AddTable("DT_FabricatedA");
    const auto b=graph.AddTable("DT_FabricatedB");
    const auto c=graph.AddTable("DT_FabricatedC");
    assert(a==0 && b==1 && c==2);
    assert(graph.TableCount()==3 && !graph.CanExposeToRetailEngine());
    assert(graph.AddProperty(a,"baseclass",5));
    assert(graph.AddProperty(a,"m_fHeight",1));
    assert(graph.AddProperty(b,"baseclass",5));
    assert(graph.AddProperty(b,"m_nState",0));
    assert(graph.AddProperty(c,"m_flUnresolved",-1));
    assert(!graph.LinkNested(a,1,b)); // type1 is NOT nested
    assert(!graph.LinkNested(a,0,77)); // out of range
    auto r=graph.Validate({a});
    assert(!r.good() && r.problem==schema::Graph::Problem::MissingNestedTable);
    assert(graph.LinkNested(a,0,b));
    assert(!graph.LinkNested(a,0,b)); // cannot silently change target
    r=graph.Validate({a});
    assert(!r.good() && r.problem==schema::Graph::Problem::MissingNestedTable);
    assert(graph.LinkNested(b,0,a)); // original mark-at-table+17 prevents infinite recursion
    r=graph.Validate({a,b});
    assert(r.good() && r.visitedTables==2 && r.seenProperties==4);
    assert(r.nestedEdges==2 && r.revisitEdges>=1);
    r=graph.Validate({c});
    assert(!r.good() && r.problem==schema::Graph::Problem::UnknownType);
    r=graph.Validate({99});
    assert(!r.good() && r.problem==schema::Graph::Problem::InvalidRoot);
    r=graph.Validate({});
    assert(!r.good() && r.problem==schema::Graph::Problem::EmptyRoots);
    std::vector<std::size_t> huge(1024,0u);
    r=graph.Validate(huge);
    assert(!r.good() && r.problem==schema::Graph::Problem::TooManyRoots);
    assert(!graph.AddProperty(999,"oops",0));
    assert(!graph.AddProperty(a,"unknown-type",6));
    assert(!graph.AddProperty(a,"",0));
    assert(!graph.FindTable(900));
}

static void test_source_class_count_and_synthetic_939_property_traversal(
    const char* source_manifest) {
    std::ifstream input(source_manifest);
    assert(input.good());
    catalog::Catalog originalClasses;
    assert(originalClasses.Load(input) && originalClasses.Size()==191);
    schema::Graph graph;
    std::vector<std::size_t> roots;
    std::size_t originalOrder=0;
    for(const auto* item=originalClasses.Head();item;item=item->next) {
        roots.push_back(graph.AddTable("DT_TEST_"+item->networkName));
        assert(roots.back()==originalOrder);
        // The 191 authentic names/order come from source. These
        // 4/5 synthetic properties PER CLASS are NOT claimed to be
        // original descriptors or the source class distribution.
        const std::size_t number=4u+(originalOrder<175u?1u:0u);
        for(std::size_t slot=0;slot<number;++slot) {
            const bool nested=(originalOrder==0 && slot==0);
            const int type=nested?5:static_cast<int>(slot%5u);
            assert(graph.AddProperty(originalOrder,
                  "synthetic_field_"+std::to_string(slot),type));
        }
        ++originalOrder;
    }
    assert(originalOrder==191 && graph.TableCount()==191);
    assert(graph.LinkNested(0,0,1));
    const auto r=graph.Validate(roots);
    assert(r.good() && r.visitedTables==191 && r.seenProperties==939);
    assert(r.nestedEdges==1 && r.revisitEdges>=1);
    assert(!graph.CanExposeToRetailEngine());
}

int main(int argc,char** argv) {
    assert(argc==2);
    check_original_engine_constants();
    test_known_cycle_and_unresolved_rejection();
    test_source_class_count_and_synthetic_939_property_traversal(argv[1]);
    std::puts("PASS Phase1697 191-source-class metadata + 939 synthetic type-safe property graph: "
              "nested type5, 60-byte stride contract, +17 visited cycles, 1024 root refusal, "
              "unknown types fail closed; no executable retail RecvTable pointers");
    return 0;
}
