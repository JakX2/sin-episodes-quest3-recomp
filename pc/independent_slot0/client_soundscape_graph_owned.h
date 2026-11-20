#pragma once
// Phase 487: original-data-informed soundscape reference graph, authored
// independently for a future portable soundscape runtime.
//
// Real SiN 2006 scripts.zip was privately authenticated and its 7
// soundscape documents yield 301 unique names, 242 playsoundscape blocks,
// 239 resolved references, 3 unresolved, 83 distinct resolved targets,
// 120 definitions with resolved outgoing edges, four cross-file resolved
// edges, no directed cycles and maximum dependency depth of five.
// These are graph observations, NOT original C_SoundscapeSystem playback
// rules or original KeyValues ABI.
#include "client_soundscape_catalog_owned.h"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sin_independent::soundscape_graph {
inline constexpr std::size_t kInvalidIndex=
    (std::numeric_limits<std::size_t>::max)();
inline constexpr std::size_t kMaxIndependentDefinitions=4096;
inline constexpr std::size_t kMaxIndependentEdges=16384;
inline constexpr std::size_t kMaxIndependentWalkSteps=16384;
inline constexpr std::size_t kMaxIndependentWalkDepth=256;

enum class BuildStatus {
    Ready, InvalidCatalog, TooManyDefinitions, TooManyReferences,
    DuplicateName, CyclicReferences, AllocationRefused
};
inline const char* BuildStatusName(BuildStatus status) noexcept {
    switch(status){
        case BuildStatus::Ready:return "owned-acyclic-graph-ready";
        case BuildStatus::InvalidCatalog:return "graph-source-catalog-refused";
        case BuildStatus::TooManyDefinitions:return "graph-definition-cap";
        case BuildStatus::TooManyReferences:return "graph-reference-cap";
        case BuildStatus::DuplicateName:return "graph-duplicate-definition";
        case BuildStatus::CyclicReferences:return "graph-dependency-cycle";
        case BuildStatus::AllocationRefused:return "graph-allocation-refused";
    }
    return "unknown";
}
enum class WalkStatus {
    Expanded, ExpandedWithMissing, NotInitialized, UnknownRoot,
    DepthRefused, ExpansionRefused, AllocationRefused
};
inline const char* WalkStatusName(WalkStatus status) noexcept {
    switch(status){
        case WalkStatus::Expanded:return "graph-structural-expanded-no-audio";
        case WalkStatus::ExpandedWithMissing:return "graph-structural-expanded-with-missing-no-audio";
        case WalkStatus::NotInitialized:return "graph-not-initialized";
        case WalkStatus::UnknownRoot:return "graph-root-unavailable";
        case WalkStatus::DepthRefused:return "graph-walk-depth-refused";
        case WalkStatus::ExpansionRefused:return "graph-walk-cap-refused";
        case WalkStatus::AllocationRefused:return "graph-walk-allocation-refused";
    }
    return "unknown";
}
struct Edge final {
    std::string target;
    std::size_t resolved_index=kInvalidIndex;
    bool resolved() const noexcept{return resolved_index!=kInvalidIndex;}
};
struct Definition final {
    std::string name;
    std::string source;
    std::vector<Edge> outgoing; // retain EVERY original ordered reference
};
struct Summary final {
    std::size_t definitions=0;
    std::size_t reference_occurrences=0;
    std::size_t resolved_occurrences=0;
    std::size_t unresolved_occurrences=0;
    std::size_t distinct_resolved_targets=0;
    std::size_t definitions_with_resolved_outgoing=0;
    std::size_t definitions_without_resolved_outgoing=0;
    std::size_t cross_file_resolved_occurrences=0;
    std::size_t maximum_dependency_depth=0;
};
struct Snapshot final {
    std::vector<Definition> definitions;
    Summary summary{};
};
enum class StepKind {DefinitionEntered, MissingReference};
struct Step final {
    StepKind kind=StepKind::DefinitionEntered;
    std::size_t definition_index=kInvalidIndex; // valid only on Entered
    std::size_t parent_index=kInvalidIndex; // source of edge, none for root
    std::size_t depth=0; // root is depth one
    std::string missing_target; // populated only when MissingReference
};
struct Expansion final {
    std::string root;
    std::vector<Step> steps;
    std::size_t definitions_entered=0;
    std::size_t missing_references=0;
    std::size_t deepest=0;
};
inline bool Same(std::string_view a,std::string_view b) noexcept {
    return soundscape_catalog::detail::Eq(a,b);
}
inline std::size_t Locate(const std::vector<Definition>& nodes,
                          std::string_view name) noexcept {
    for(std::size_t i=0;i<nodes.size();++i)
        if(Same(nodes[i].name,name))return i;
    return kInvalidIndex;
}
inline bool OriginalGraphShape(const Summary& s) noexcept {
    return s.definitions==301 && s.reference_occurrences==242 &&
        s.resolved_occurrences==239 && s.unresolved_occurrences==3 &&
        s.distinct_resolved_targets==83 &&
        s.definitions_with_resolved_outgoing==120 &&
        s.definitions_without_resolved_outgoing==181 &&
        s.cross_file_resolved_occurrences==4 &&
        s.maximum_dependency_depth==5;
}
class OwnedGraph final {
public:
    bool ready() const noexcept{return ready_;}
    const Snapshot& value() const noexcept{return state_;}
    void Clear() noexcept{
        state_={};
        ready_=false;
    }
    BuildStatus Build(const soundscape_catalog::CatalogState& catalog) noexcept {
        if(catalog.definitions.empty() ||
           catalog.counts.definitions!=catalog.definitions.size() ||
           catalog.counts.nested_soundscape_blocks!=catalog.references.size() ||
           catalog.counts.unresolved_references!=catalog.unresolved.size())
            return BuildStatus::InvalidCatalog;
        if(catalog.definitions.size()>kMaxIndependentDefinitions)
            return BuildStatus::TooManyDefinitions;
        if(catalog.references.size()>kMaxIndependentEdges)
            return BuildStatus::TooManyReferences;
        try {
            Snapshot next{};
            const std::size_t n=catalog.definitions.size();
            next.definitions.reserve(n);
            for(const auto& source:catalog.definitions){
                if(source.name.empty()||source.source.empty())
                    return BuildStatus::InvalidCatalog;
                if(Locate(next.definitions,source.name)!=kInvalidIndex)
                    return BuildStatus::DuplicateName;
                next.definitions.push_back({source.name,source.source,{}});
            }
            std::size_t missing=0;
            for(const auto& ref:catalog.references){
                const auto src=Locate(next.definitions,ref.owner);
                if(src==kInvalidIndex ||
                   !Same(next.definitions[src].source,ref.source))
                    return BuildStatus::InvalidCatalog;
                const auto target=Locate(next.definitions,ref.target);
                if(target==kInvalidIndex)++missing;
                next.definitions[src].outgoing.push_back({
                    ref.target,target
                });
            }
            if(missing!=catalog.unresolved.size())
                return BuildStatus::InvalidCatalog;
            Summary stats{};
            stats.definitions=n;
            stats.reference_occurrences=catalog.references.size();
            stats.unresolved_occurrences=missing;
            stats.resolved_occurrences=stats.reference_occurrences-missing;
            std::vector<std::size_t> indegree(n,0),depth(n,1);
            std::vector<bool> uniqueTargets(n,false);
            for(std::size_t i=0;i<n;++i){
                bool outgoingResolved=false;
                for(const auto& edge:next.definitions[i].outgoing){
                    if(!edge.resolved())continue;
                    outgoingResolved=true;
                    ++indegree[edge.resolved_index];
                    if(!uniqueTargets[edge.resolved_index]){
                        uniqueTargets[edge.resolved_index]=true;
                        ++stats.distinct_resolved_targets;
                    }
                    if(!Same(next.definitions[i].source,
                             next.definitions[edge.resolved_index].source))
                        ++stats.cross_file_resolved_occurrences;
                }
                if(outgoingResolved)++stats.definitions_with_resolved_outgoing;
                else ++stats.definitions_without_resolved_outgoing;
            }
            // Kahn's topological sort (iterative, no recursive stack):
            // duplicate edges are retained and decrement indegree once
            // EACH, so repeated playsoundscape links are never erased.
            std::vector<std::size_t> ready;
            ready.reserve(n);
            for(std::size_t i=0;i<n;++i)
                if(indegree[i]==0)ready.push_back(i);
            std::size_t visited=0;
            for(std::size_t at=0;at<ready.size();++at){
                const auto from=ready[at];
                ++visited;
                for(const auto& edge:next.definitions[from].outgoing){
                    if(!edge.resolved())continue;
                    auto target=edge.resolved_index;
                    depth[target]=(std::max)(depth[target],depth[from]+1);
                    if(--indegree[target]==0)ready.push_back(target);
                }
            }
            if(visited!=n)return BuildStatus::CyclicReferences;
            for(const auto value:depth)
                stats.maximum_dependency_depth=
                    (std::max)(stats.maximum_dependency_depth,value);
            next.summary=stats;
            state_=std::move(next);
            ready_=true;
            return BuildStatus::Ready;
        }catch(...){return BuildStatus::AllocationRefused;}
    }
    WalkStatus Expand(std::string_view root,Expansion& output) const noexcept {
        if(!ready_)return WalkStatus::NotInitialized;
        const auto index=Locate(state_.definitions,root);
        if(index==kInvalidIndex)return WalkStatus::UnknownRoot;
        try {
            struct Frame{
                std::size_t node=0;
                std::size_t next_edge=0;
                std::size_t depth=0;
            };
            Expansion pending{};
            pending.root=state_.definitions[index].name;
            pending.steps.push_back({
                StepKind::DefinitionEntered,index,kInvalidIndex,1,{}});
            pending.definitions_entered=1;
            pending.deepest=1;
            std::vector<Frame> frames;
            frames.push_back({index,0,1});
            while(!frames.empty()){
                auto& frame=frames.back();
                const auto& src=state_.definitions[frame.node];
                if(frame.next_edge>=src.outgoing.size()){
                    frames.pop_back();
                    continue;
                }
                const auto& edge=src.outgoing[frame.next_edge++];
                if(pending.steps.size()>=kMaxIndependentWalkSteps)
                    return WalkStatus::ExpansionRefused;
                if(!edge.resolved()){
                    pending.steps.push_back({
                        StepKind::MissingReference,kInvalidIndex,
                        frame.node,frame.depth+1,edge.target
                    });
                    ++pending.missing_references;
                    continue;
                }
                if(frame.depth>=kMaxIndependentWalkDepth)
                    return WalkStatus::DepthRefused;
                const auto nextDepth=frame.depth+1;
                const auto parent=frame.node;
                pending.steps.push_back({
                    StepKind::DefinitionEntered,edge.resolved_index,
                    parent,nextDepth,{}});
                ++pending.definitions_entered;
                pending.deepest=(std::max)(pending.deepest,nextDepth);
                frames.push_back({edge.resolved_index,0,nextDepth});
            }
            output=std::move(pending); // atomic publication on full walk
            return output.missing_references?
                WalkStatus::ExpandedWithMissing:WalkStatus::Expanded;
        }catch(...){return WalkStatus::AllocationRefused;}
    }
private:
    Snapshot state_{};
    bool ready_=false;
};
} // namespace sin_independent::soundscape_graph
