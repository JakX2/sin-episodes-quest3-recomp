// Phase 487, compiled native Win32. All names and waves are invented.
// Real seven-file dependency graph was analyzed privately read-only.
#include "client_soundscape_graph_owned.h"
#include <cstdio>
#include <string>
#include <string_view>

namespace m=sin_independent::soundscape_manifest;
namespace c=sin_independent::soundscape_catalog;
namespace g=sin_independent::soundscape_graph;

static int Fail(int n){
    std::fprintf(stderr,"soundscape owned DAG fixture failed=%d\n",n);
    return n;
}
static const std::string manifest=R"KV(
soundscaples_manifest {
 "file" "scripts/test_one.txt"
 "file" "scripts/test_two.txt"
}
)KV";
static const std::string first=R"KV(
"A" {
 "playsoundscape" { "name" "B" "volume" "0.25" }
 "playsoundscape" { "name" "missing" }
 "playsoundscape" { "name" "b" "volume" "1" }
}
"D" { "playlooping" { "wave" "synthetic.wav" } }
)KV";
static const std::string second=R"KV(
"B" { "playsoundscape" { "name" "C" } }
"C" { "dsp" "3" }
)KV";

struct Provider {
    std::string one=first,two=second;
    bool Read(std::string_view path,std::string& text) const {
        if(path=="scripts/test_one.txt"){text=one;return true;}
        if(path=="scripts/test_two.txt"){text=two;return true;}
        return false;
    }
};
static bool Matches(const g::Snapshot& s,const g::Step& step,
                    g::StepKind kind,std::string_view name,
                    std::string_view parent,std::size_t depth) {
    if(step.kind!=kind||step.depth!=depth)return false;
    if(kind==g::StepKind::DefinitionEntered) {
        if(step.definition_index>=s.definitions.size() ||
           !g::Same(s.definitions[step.definition_index].name,name))
            return false;
    }else if(step.missing_target!=name ||
             step.definition_index!=g::kInvalidIndex)return false;
    if(parent.empty())return step.parent_index==g::kInvalidIndex;
    return step.parent_index<s.definitions.size() &&
           g::Same(s.definitions[step.parent_index].name,parent);
}
int main(){
    m::OwnedSnapshot assets{};
    c::OwnedCatalog catalog{};
    g::OwnedGraph graph{};
    g::Expansion expansion{};
    if(graph.Expand("A",expansion)!=g::WalkStatus::NotInitialized ||
       graph.ready())return Fail(1);

    Provider source{};
    const auto read=[&](std::string_view path,std::string& into){
        return source.Read(path,into);
    };
    if(assets.Load(manifest,read)!=m::Status::Ready ||
       catalog.Build(assets.value())!=c::Status::Ready ||
       graph.Build(catalog.value())!=g::BuildStatus::Ready)
        return Fail(2);
    const auto& s=graph.value();
    const auto& q=s.summary;
    if(q.definitions!=4||q.reference_occurrences!=4 ||
       q.resolved_occurrences!=3 || q.unresolved_occurrences!=1 ||
       q.distinct_resolved_targets!=2 ||
       q.definitions_with_resolved_outgoing!=2 ||
       q.definitions_without_resolved_outgoing!=2 ||
       q.cross_file_resolved_occurrences!=2 ||
       q.maximum_dependency_depth!=3 ||
       g::OriginalGraphShape(q))return Fail(3);

    // Duplicate source entries must preserve both B links, including
    // the lowercase repeat. Missing targets are structural events,
    // not fatal for a valid original-like partial catalog.
    if(graph.Expand("a",expansion)!=g::WalkStatus::ExpandedWithMissing ||
       expansion.root!="A" ||
       expansion.definitions_entered!=5 ||
       expansion.missing_references!=1 ||
       expansion.deepest!=3 ||
       expansion.steps.size()!=6)return Fail(4);
    if(!Matches(s,expansion.steps[0],g::StepKind::DefinitionEntered,
                "A","",1) ||
       !Matches(s,expansion.steps[1],g::StepKind::DefinitionEntered,
                "B","A",2) ||
       !Matches(s,expansion.steps[2],g::StepKind::DefinitionEntered,
                "C","B",3) ||
       !Matches(s,expansion.steps[3],g::StepKind::MissingReference,
                "missing","A",2) ||
       !Matches(s,expansion.steps[4],g::StepKind::DefinitionEntered,
                "B","A",2) ||
       !Matches(s,expansion.steps[5],g::StepKind::DefinitionEntered,
                "C","B",3))return Fail(5);
    const auto published=expansion.steps.size();
    if(graph.Expand("no-such-root",expansion)!=g::WalkStatus::UnknownRoot ||
       expansion.steps.size()!=published ||
       graph.Expand("D",expansion)!=g::WalkStatus::Expanded ||
       expansion.steps.size()!=1 || expansion.definitions_entered!=1)
        return Fail(6);

    // A -> B -> A is a synthetic cycle: build refuses atomically
    // instead of risking recursive soundscape entry at runtime.
    source.two=R"KV("B" { "playsoundscape" { "name" "a" } }
                       "C" { "dsp" "3" })KV";
    if(assets.Load(manifest,read)!=m::Status::Ready ||
       catalog.Build(assets.value())!=c::Status::Ready ||
       graph.Build(catalog.value())!=g::BuildStatus::CyclicReferences ||
       graph.value().summary.resolved_occurrences!=3 ||
       graph.Expand("A",expansion)!=g::WalkStatus::ExpandedWithMissing)
        return Fail(7);
    source.two=second;

    // Duplicate definitions, wrong references and invalid count
    // metadata cannot replace a previous coherent snapshot.
    c::CatalogState forged=catalog.value();
    forged.definitions[1].name="a";
    if(graph.Build(forged)!=g::BuildStatus::DuplicateName ||
       !graph.ready())return Fail(8);
    forged=catalog.value();
    forged.counts.definitions=0;
    if(graph.Build(forged)!=g::BuildStatus::InvalidCatalog ||
       graph.value().summary.definitions!=4)return Fail(9);

    // 301-definition / 242-reference manufactured scale fixture,
    // NOT the original game's actual dependency topology.
    c::CatalogState large{};
    for(unsigned i=0;i<301;++i)
        large.definitions.push_back({
            "synthetic"+std::to_string(i),
            i<151?"scripts/a.txt":"scripts/b.txt",{}});
    for(unsigned i=0;i<239;++i)
        large.references.push_back({
            "synthetic0","synthetic1","scripts/a.txt"});
    for(unsigned i=0;i<3;++i)
        large.references.push_back({
            "synthetic0","unknown"+std::to_string(i),"scripts/a.txt"});
    large.counts.definitions=301;
    large.counts.nested_soundscape_blocks=242;
    large.counts.unresolved_references=3;
    large.unresolved={large.references[239],large.references[240],
                      large.references[241]};
    if(graph.Build(large)!=g::BuildStatus::Ready ||
       graph.value().summary.definitions!=301 ||
       graph.value().summary.reference_occurrences!=242 ||
       graph.value().summary.resolved_occurrences!=239 ||
       graph.value().summary.maximum_dependency_depth!=2 ||
       g::OriginalGraphShape(graph.value().summary))
        return Fail(10);
    if(graph.Expand("SYNTHETIC0",expansion)!=
           g::WalkStatus::ExpandedWithMissing ||
       expansion.definitions_entered!=240 ||
       expansion.missing_references!=3)return Fail(11);

    // An exponential but acyclic graph can have very few definitions
    // and yet an unsafe number of repeated expansion occurrences.
    // Refuse without publishing a truncated plan.
    c::CatalogState boom{};
    for(int i=0;i<17;++i)
        boom.definitions.push_back({
            "N"+std::to_string(i),"scripts/fixture.txt",{}});
    for(int i=0;i<16;++i)
        for(int j=0;j<2;++j)
            boom.references.push_back({
                "N"+std::to_string(i),"N"+std::to_string(i+1),
                "scripts/fixture.txt"});
    boom.counts.definitions=17;
    boom.counts.nested_soundscape_blocks=32;
    if(graph.Build(boom)!=g::BuildStatus::Ready)return Fail(12);
    const auto oldSteps=expansion.steps.size();
    if(graph.Expand("N0",expansion)!=g::WalkStatus::ExpansionRefused ||
       expansion.steps.size()!=oldSteps)return Fail(13);

    // Extremely long acyclic chains are also refused at runtime
    // rather than using unbounded native recursion.
    c::CatalogState deep{};
    for(int i=0;i<258;++i)
        deep.definitions.push_back({
            "D"+std::to_string(i),"scripts/fixture.txt",{}});
    for(int i=0;i<257;++i)
        deep.references.push_back({
            "D"+std::to_string(i),"D"+std::to_string(i+1),
            "scripts/fixture.txt"});
    deep.counts.definitions=258;
    deep.counts.nested_soundscape_blocks=257;
    if(graph.Build(deep)!=g::BuildStatus::Ready ||
       graph.value().summary.maximum_dependency_depth!=258 ||
       graph.Expand("D0",expansion)!=g::WalkStatus::DepthRefused ||
       expansion.steps.size()!=oldSteps)return Fail(14);

    graph.Clear();
    if(graph.ready()||!graph.value().definitions.empty()||
       graph.value().summary.definitions!=0 ||
       graph.Expand("A",expansion)!=g::WalkStatus::NotInitialized)
        return Fail(15);

    std::puts("PASS independent soundscape DAG: repeated references, ordered DFS, missing targets, acyclic topo, same-file/cross-file, 301-scale, cycle rollback, walk cap, depth guard and cleanup");
    return 0;
}
