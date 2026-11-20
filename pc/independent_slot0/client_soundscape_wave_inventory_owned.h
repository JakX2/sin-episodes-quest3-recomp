#pragma once
// Phase 488: independently owned wave declaration registry and portable
// structural soundscape resource plan. NOT Source sound emitter playback.
//
// SHA-authenticated original 2006 scripts have 281 playlooping groups
// and 150 playrandom/rndwave groups, 1,224 raw wave occurrences,
// 510 unique raw wave tokens, 782 WAV and 442 MP3 occurrences.
// Original leading sound-declaration characters (*, ), *)) must remain
// intact for later original engine semantics; stripped asset paths
// below are packaging metadata ONLY, not replacement mixer commands.
#include "client_soundscape_graph_owned.h"
#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sin_independent::soundscape_waves {
inline constexpr std::size_t kMaxIndependentGroups=8192;
inline constexpr std::size_t kMaxIndependentWaveEntries=16384;
inline constexpr std::size_t kMaxIndependentGroupWaves=128;
inline constexpr std::size_t kMaxIndependentSceneGroups=16384;
inline constexpr std::size_t kMaxIndependentSceneWaves=65536;
enum class Kind {Looping,RandomCandidate};
enum class BuildStatus {
    Ready, InvalidCatalog, InvalidWavePlacement, InvalidWavePath,
    MalformedGroup, GroupCap, WaveCap, AllocationRefused
};
inline const char* StatusName(BuildStatus s) noexcept {
    switch(s){
        case BuildStatus::Ready:return "owned-soundscape-wave-inventory-ready";
        case BuildStatus::InvalidCatalog:return "wave-source-catalog-refused";
        case BuildStatus::InvalidWavePlacement:return "wave-placement-refused";
        case BuildStatus::InvalidWavePath:return "wave-relative-path-refused";
        case BuildStatus::MalformedGroup:return "wave-group-structure-refused";
        case BuildStatus::GroupCap:return "wave-group-cap-refused";
        case BuildStatus::WaveCap:return "wave-entry-cap-refused";
        case BuildStatus::AllocationRefused:return "wave-allocation-refused";
    }
    return "unknown";
}
struct Scalar final {
    std::string key;
    std::string raw_value;
    bool nested_rndwave_property=false;
};
struct Wave final {
    std::string original_raw; // preserve Source control characters
    std::string leading_markers; // packaging observation, not playback
    std::string asset_relative_path; // no leading markers, no "sound/"
    std::size_t group_index=0;
    Kind kind=Kind::Looping;
};
struct Group final {
    std::size_t definition_index=0;
    Kind kind=Kind::Looping;
    std::size_t first_wave=0;
    std::size_t wave_count=0;
    std::size_t rndwave_block_count=0;
    std::vector<Scalar> original_properties; // never parse/modify floats
};
struct Definition final {
    std::string name;
    std::string source;
    std::vector<std::size_t> group_indices;
};
struct Summary final {
    std::size_t definitions=0;
    std::size_t definitions_with_waves=0;
    std::size_t looping_groups=0;
    std::size_t random_groups=0;
    std::size_t rndwave_blocks=0;
    std::size_t looping_waves=0;
    std::size_t random_waves=0;
    std::size_t raw_wave_occurrences=0;
    std::size_t unique_raw_waves=0;
    std::size_t wav_occurrences=0;
    std::size_t mp3_occurrences=0;
    std::size_t star_leading=0;
    std::size_t paren_leading=0;
    std::size_t compound_star_paren=0;
    std::size_t unmarked=0;
    std::size_t maximum_random_candidates=0;
};
struct Snapshot final {
    std::vector<Definition> definitions;
    std::vector<Group> groups;
    std::vector<Wave> waves;
    Summary summary{};
};
inline bool MatchesRetailWaveShape(const Summary& s) noexcept {
    return s.definitions==301 && s.definitions_with_waves==284 &&
       s.looping_groups==281 && s.random_groups==150 &&
       s.rndwave_blocks==150 && s.looping_waves==281 &&
       s.random_waves==943 && s.raw_wave_occurrences==1224 &&
       s.unique_raw_waves==510 && s.wav_occurrences==782 &&
       s.mp3_occurrences==442 && s.star_leading==449 &&
       s.paren_leading==91 && s.compound_star_paren==20 &&
       s.unmarked==684 && s.maximum_random_candidates==20;
}
inline bool SafeWave(std::string_view raw,
                     std::string_view& markers,
                     std::string_view& asset) noexcept {
    if(raw.empty()||raw.size()>240)return false;
    std::size_t start=0;
    while(start<raw.size() && (raw[start]=='*'||raw[start]==')'))
        ++start;
    if(start>2||start==raw.size())return false;
    markers=raw.substr(0,start);
    asset=raw.substr(start);
    if(asset.size()<6||asset.front()=='/'||
       !(soundscape_catalog::detail::Eq(asset.substr(asset.size()-4),".wav") ||
         soundscape_catalog::detail::Eq(asset.substr(asset.size()-4),".mp3")))
        return false;
    std::size_t begin=0;
    bool sawSlash=false;
    while(begin<asset.size()){
        const auto slash=asset.find('/',begin);
        const auto finish=slash==std::string_view::npos?asset.size():slash;
        const auto seg=asset.substr(begin,finish-begin);
        if(seg.empty()||seg=="."||seg=="..")return false;
        for(const auto ch:seg)
            if(!((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||
                 (ch>='0'&&ch<='9')||ch=='_'||ch=='-'||ch=='.'))
                return false;
        if(slash==std::string_view::npos)break;
        sawSlash=true;
        begin=slash+1;
    }
    return sawSlash;
}
namespace detail {
inline bool Is(std::string_view a,std::string_view b) noexcept {
    return soundscape_catalog::detail::Eq(a,b);
}
inline BuildStatus PushWave(Snapshot& out,Group& group,
                           std::size_t groupIndex,Kind kind,
                           std::string_view raw){
    std::string_view markers,asset;
    if(!SafeWave(raw,markers,asset))return BuildStatus::InvalidWavePath;
    if(out.waves.size()>=kMaxIndependentWaveEntries ||
       group.wave_count>=kMaxIndependentGroupWaves)
        return BuildStatus::WaveCap;
    out.waves.push_back({
        std::string(raw),std::string(markers),std::string(asset),
        groupIndex,kind
    });
    ++group.wave_count;
    return BuildStatus::Ready;
}
inline BuildStatus AddGroup(const soundscape_catalog::Node& block,
                            Snapshot& out,std::size_t owner) {
    if(out.groups.size()>=kMaxIndependentGroups)
        return BuildStatus::GroupCap;
    const Kind kind=Is(block.key,"playlooping")?
        Kind::Looping:Kind::RandomCandidate;
    const std::size_t id=out.groups.size();
    Group group{};
    group.definition_index=owner;
    group.kind=kind;
    group.first_wave=out.waves.size();
    for(const auto& node:block.children){
        if(kind==Kind::Looping){
            if(node.is_block)return BuildStatus::InvalidWavePlacement;
            if(Is(node.key,"wave")){
                const auto s=PushWave(out,group,id,kind,node.scalar);
                if(s!=BuildStatus::Ready)return s;
            }else group.original_properties.push_back({
                node.key,node.scalar,false
            });
            continue;
        }
        if(node.is_block && Is(node.key,"rndwave")){
            ++group.rndwave_block_count;
            for(const auto& child:node.children){
                if(child.is_block)return BuildStatus::InvalidWavePlacement;
                if(Is(child.key,"wave")){
                    const auto s=PushWave(out,group,id,kind,child.scalar);
                    if(s!=BuildStatus::Ready)return s;
                }else group.original_properties.push_back({
                    child.key,child.scalar,true
                });
            }
        }else if(!node.is_block){
            if(Is(node.key,"wave"))return BuildStatus::InvalidWavePlacement;
            group.original_properties.push_back({
                node.key,node.scalar,false
            });
        }else return BuildStatus::InvalidWavePlacement;
    }
    if(group.wave_count==0 ||
       (kind==Kind::RandomCandidate&&group.rndwave_block_count==0))
        return BuildStatus::MalformedGroup;
    out.definitions[owner].group_indices.push_back(id);
    out.groups.push_back(std::move(group));
    return BuildStatus::Ready;
}
inline BuildStatus Scan(const std::vector<soundscape_catalog::Node>& nodes,
                       Snapshot& out,std::size_t owner,unsigned depth=0) {
    if(depth>soundscape_catalog::kMaxDepth)
        return BuildStatus::InvalidCatalog;
    for(const auto& node:nodes){
        if(!node.is_block){
            if(Is(node.key,"wave"))return BuildStatus::InvalidWavePlacement;
            continue;
        }
        if(Is(node.key,"playlooping")||Is(node.key,"playrandom")){
            const auto status=AddGroup(node,out,owner);
            if(status!=BuildStatus::Ready)return status;
        }else{
            const auto status=Scan(node.children,out,owner,depth+1);
            if(status!=BuildStatus::Ready)return status;
        }
    }
    return BuildStatus::Ready;
}
inline std::string Fold(std::string_view raw) {
    std::string out(raw);
    for(auto& ch:out)
        if(ch>='A'&&ch<='Z')ch=static_cast<char>(ch-'A'+'a');
    return out;
}
} // namespace detail
class OwnedWaves final {
public:
    bool ready() const noexcept{return ready_;}
    const Snapshot& value() const noexcept{return snapshot_;}
    void Clear() noexcept{snapshot_={};ready_=false;}
    BuildStatus Build(const soundscape_catalog::CatalogState& input) noexcept {
        if(input.definitions.empty() ||
           input.counts.definitions!=input.definitions.size())
            return BuildStatus::InvalidCatalog;
        if(input.definitions.size()>soundscape_graph::kMaxIndependentDefinitions)
            return BuildStatus::InvalidCatalog;
        try {
            Snapshot stage{};
            stage.definitions.reserve(input.definitions.size());
            for(const auto& def:input.definitions){
                if(def.name.empty()||def.source.empty())
                    return BuildStatus::InvalidCatalog;
                stage.definitions.push_back({def.name,def.source,{}});
            }
            for(std::size_t i=0;i<input.definitions.size();++i){
                const auto status=detail::Scan(input.definitions[i].children,
                                               stage,i);
                if(status!=BuildStatus::Ready)return status;
            }
            auto& s=stage.summary;
            s.definitions=stage.definitions.size();
            s.raw_wave_occurrences=stage.waves.size();
            std::set<std::string> unique;
            for(const auto& d:stage.definitions)
                if(!d.group_indices.empty())++s.definitions_with_waves;
            for(const auto& g:stage.groups){
                if(g.kind==Kind::Looping){
                    ++s.looping_groups;
                    s.looping_waves+=g.wave_count;
                }else{
                    ++s.random_groups;
                    s.random_waves+=g.wave_count;
                    s.rndwave_blocks+=g.rndwave_block_count;
                    s.maximum_random_candidates=
                        (std::max)(s.maximum_random_candidates,g.wave_count);
                }
            }
            for(const auto& item:stage.waves){
                unique.insert(detail::Fold(item.original_raw));
                if(detail::Is(std::string_view(item.asset_relative_path).substr(
                                  item.asset_relative_path.size()-4),".wav"))
                    ++s.wav_occurrences;
                else ++s.mp3_occurrences;
                if(item.leading_markers.empty())++s.unmarked;
                else if(item.leading_markers.front()=='*')++s.star_leading;
                else if(item.leading_markers.front()==')')++s.paren_leading;
                if(item.leading_markers=="*)")++s.compound_star_paren;
            }
            s.unique_raw_waves=unique.size();
            snapshot_=std::move(stage);
            ready_=true;
            return BuildStatus::Ready;
        }catch(...){return BuildStatus::AllocationRefused;}
    }
private:
    Snapshot snapshot_{};
    bool ready_=false;
};
struct GroupOccurrence final {
    std::size_t group_index=0;
    std::size_t definition_index=0;
    std::size_t dependency_depth=0;
};
struct ScenePlan final {
    std::string root;
    std::vector<GroupOccurrence> groups;
    std::size_t wave_occurrences=0;
    std::size_t missing_references=0;
};
enum class SceneStatus {
    Ready, ReadyWithMissing, Unready, CatalogMismatch,
    GraphWalkRefused, SceneLimitRefused, AllocationRefused
};
// A resource-selection plan only: repeated nested definitions repeat
// their declarations, but no wave is played, randomized or parameterized.
// This intentionally does not assert Source DSP, time or volume parity.
inline SceneStatus ExpandScene(const OwnedWaves& inventory,
                               const soundscape_graph::OwnedGraph& graph,
                               std::string_view root,
                               ScenePlan& output) noexcept {
    if(!inventory.ready()||!graph.ready())return SceneStatus::Unready;
    const auto& src=inventory.value();
    const auto& nodes=graph.value().definitions;
    if(src.definitions.size()!=nodes.size())
        return SceneStatus::CatalogMismatch;
    for(std::size_t i=0;i<nodes.size();++i)
        if(!detail::Is(src.definitions[i].name,nodes[i].name) ||
           !detail::Is(src.definitions[i].source,nodes[i].source))
            return SceneStatus::CatalogMismatch;
    soundscape_graph::Expansion walk{};
    const auto status=graph.Expand(root,walk);
    if(status!=soundscape_graph::WalkStatus::Expanded &&
       status!=soundscape_graph::WalkStatus::ExpandedWithMissing)
        return SceneStatus::GraphWalkRefused;
    try {
        ScenePlan pending{};
        pending.root=walk.root;
        pending.missing_references=walk.missing_references;
        for(const auto& step:walk.steps){
            if(step.kind!=soundscape_graph::StepKind::DefinitionEntered)
                continue;
            const auto id=step.definition_index;
            if(id>=src.definitions.size())
                return SceneStatus::CatalogMismatch;
            for(const auto groupId:src.definitions[id].group_indices){
                if(pending.groups.size()>=kMaxIndependentSceneGroups)
                    return SceneStatus::SceneLimitRefused;
                const auto& group=src.groups[groupId];
                if(pending.wave_occurrences>
                   kMaxIndependentSceneWaves-group.wave_count)
                    return SceneStatus::SceneLimitRefused;
                pending.groups.push_back({groupId,id,step.depth});
                pending.wave_occurrences+=group.wave_count;
            }
        }
        output=std::move(pending);
        return output.missing_references?
            SceneStatus::ReadyWithMissing:SceneStatus::Ready;
    }catch(...){return SceneStatus::AllocationRefused;}
}
} // namespace sin_independent::soundscape_waves
