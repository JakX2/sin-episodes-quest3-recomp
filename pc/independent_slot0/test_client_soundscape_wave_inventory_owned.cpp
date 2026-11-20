// Phase 488: independent Win32 wave inventory and scene-plan test.
// All fixtures are synthetic, no original game sound effects or scripts.
#include "client_soundscape_wave_inventory_owned.h"
#include <cstdio>
#include <string>
#include <string_view>

namespace m=sin_independent::soundscape_manifest;
namespace c=sin_independent::soundscape_catalog;
namespace g=sin_independent::soundscape_graph;
namespace w=sin_independent::soundscape_waves;
static int Fail(int n){
    std::fprintf(stderr,"owned wave inventory fixture failed=%d\n",n);
    return n;
}
static const std::string manifest=
    "soundscaples_manifest { \"file\" \"scripts/s1.txt\" "
    "\"file\" \"scripts/s2.txt\" }";
static const std::string first=R"KV(
"A" {
 "playlooping" {
  "volume" "0.4" "wave" "*ambient/example.wav" "pitch" "90"
 }
 "playsoundscape" { "name" "B" }
 "playsoundscape" { "name" "missing" }
 "playsoundscape" { "name" "b" }
 "playrandom" {
  "time" "1.0,3.0" "volume" "0.5"
  "rndwave" {
    "wave" ")ambient/one.mp3"
    "wave" "*)ambient/two.wav"
  }
 }
}
)KV";
static const std::string second=R"KV(
"B" {
 "playlooping" { "wave" "ambient/three.wav" "volume" "1.0" }
}
"C" { "dsp" "3" }
)KV";

struct Provider {
    std::string a=first,b=second;
    bool Read(std::string_view path,std::string& out) const {
        if(path=="scripts/s1.txt"){out=a;return true;}
        if(path=="scripts/s2.txt"){out=b;return true;}
        return false;
    }
};
static bool Load(Provider& provider,m::OwnedSnapshot& snapshot,
                 c::OwnedCatalog& catalog) {
    const auto reader=[&](std::string_view path,std::string& out){
        return provider.Read(path,out);
    };
    return snapshot.Load(manifest,reader)==m::Status::Ready &&
           catalog.Build(snapshot.value())==c::Status::Ready;
}
int main(){
    std::string_view flags,path;
    if(!w::SafeWave("*)ambient/mix.wav",flags,path) ||
       flags!="*)" || path!="ambient/mix.wav" ||
       !w::SafeWave("*voice/hello.mp3",flags,path) ||
       flags!="*" || path!="voice/hello.mp3" ||
       w::SafeWave("../outside.wav",flags,path) ||
       w::SafeWave("*../outside.wav",flags,path) ||
       w::SafeWave("sound\\escape.wav",flags,path) ||
       w::SafeWave("C:/root.wav",flags,path) ||
       w::SafeWave("/root/file.wav",flags,path) ||
       w::SafeWave("ambient/bad.flac",flags,path) ||
       w::SafeWave("*)",flags,path))
        return Fail(1);

    Provider provider{};
    m::OwnedSnapshot snapshot{};
    c::OwnedCatalog catalog{};
    g::OwnedGraph graph{};
    w::OwnedWaves waves{};
    w::ScenePlan plan{};
    if(w::ExpandScene(waves,graph,"A",plan)!=w::SceneStatus::Unready)
        return Fail(2);
    if(!Load(provider,snapshot,catalog) ||
       graph.Build(catalog.value())!=g::BuildStatus::Ready ||
       waves.Build(catalog.value())!=w::BuildStatus::Ready)
        return Fail(3);
    const auto& s=waves.value();
    const auto& counts=s.summary;
    if(counts.definitions!=3||counts.definitions_with_waves!=2 ||
       counts.looping_groups!=2 ||counts.random_groups!=1 ||
       counts.rndwave_blocks!=1 ||counts.looping_waves!=2 ||
       counts.random_waves!=2 || counts.raw_wave_occurrences!=4 ||
       counts.unique_raw_waves!=4 ||counts.wav_occurrences!=3 ||
       counts.mp3_occurrences!=1 ||counts.star_leading!=2 ||
       counts.paren_leading!=1 ||counts.compound_star_paren!=1 ||
       counts.unmarked!=1 ||counts.maximum_random_candidates!=2 ||
       w::MatchesRetailWaveShape(counts))
        return Fail(4);
    if(s.groups.size()!=3||s.waves.size()!=4||
       s.definitions[0].group_indices.size()!=2 ||
       s.definitions[1].group_indices.size()!=1 ||
       !s.definitions[2].group_indices.empty())
        return Fail(5);
    if(s.waves[0].original_raw!="*ambient/example.wav" ||
       s.waves[0].leading_markers!="*" ||
       s.waves[0].asset_relative_path!="ambient/example.wav" ||
       s.waves[1].original_raw!=")ambient/one.mp3" ||
       s.waves[1].leading_markers!=")" ||
       s.waves[2].original_raw!="*)ambient/two.wav" ||
       s.waves[2].leading_markers!="*)" ||
       s.waves[3].original_raw!="ambient/three.wav" ||
       !s.waves[3].leading_markers.empty())
        return Fail(6);
    if(s.groups[0].kind!=w::Kind::Looping ||
       s.groups[0].wave_count!=1 ||
       s.groups[0].original_properties.size()!=2 ||
       s.groups[0].original_properties[0].key!="volume" ||
       s.groups[0].original_properties[0].raw_value!="0.4" ||
       s.groups[0].original_properties[1].key!="pitch" ||
       s.groups[1].kind!=w::Kind::RandomCandidate ||
       s.groups[1].rndwave_block_count!=1 ||
       s.groups[1].wave_count!=2 ||
       s.groups[1].original_properties.size()!=2 ||
       s.groups[1].original_properties[0].raw_value!="1.0,3.0")
        return Fail(7);

    if(w::ExpandScene(waves,graph,"a",plan)!=
           w::SceneStatus::ReadyWithMissing ||
       plan.root!="A" ||
       plan.groups.size()!=4 ||plan.wave_occurrences!=5 ||
       plan.missing_references!=1 ||
       plan.groups[0].group_index!=0 ||
       plan.groups[1].group_index!=1 ||
       plan.groups[2].group_index!=2 ||
       plan.groups[3].group_index!=2 ||
       plan.groups[0].dependency_depth!=1 ||
       plan.groups[2].dependency_depth!=2)
        return Fail(8);
    const auto previousScene=plan.groups.size();
    if(w::ExpandScene(waves,graph,"absent",plan)!=
           w::SceneStatus::GraphWalkRefused ||
       plan.groups.size()!=previousScene)return Fail(9);

    // Publisher must preserve preceding valid wave inventory when
    // rebuilt source contains malformed/untrusted asset path.
    provider.a=R"KV("A" { "playlooping" { "wave" "../bad.wav" } })KV";
    if(!Load(provider,snapshot,catalog) ||
       waves.Build(catalog.value())!=w::BuildStatus::InvalidWavePath ||
       waves.value().summary.raw_wave_occurrences!=4 ||
       !waves.ready())return Fail(10);
    provider.a=R"KV("A" { "wave" "ambient/wrong.wav" })KV";
    if(!Load(provider,snapshot,catalog) ||
       waves.Build(catalog.value())!=
           w::BuildStatus::InvalidWavePlacement ||
       waves.value().summary.raw_wave_occurrences!=4)return Fail(11);
    provider.a=R"KV("A" { "playrandom" { "time" "1" } })KV";
    if(!Load(provider,snapshot,catalog) ||
       waves.Build(catalog.value())!=w::BuildStatus::MalformedGroup ||
       waves.value().summary.raw_wave_occurrences!=4)return Fail(12);

    // A fabricated catalog with source-identical but renamed root
    // cannot be used as a scene plan with an unrelated graph.
    provider.a=first;
    if(!Load(provider,snapshot,catalog) ||
       waves.Build(catalog.value())!=w::BuildStatus::Ready)
        return Fail(13);
    c::CatalogState different=catalog.value();
    different.definitions[0].name="OTHER";
    if(graph.Build(different)!=g::BuildStatus::InvalidCatalog ||
       w::ExpandScene(waves,graph,"A",plan)!=
           w::SceneStatus::ReadyWithMissing)return Fail(14);

    // Public scale fixture: source-free 301 roots, 281 single WAV
    // looping groups and 150 random groups with 943 candidates.
    c::CatalogState large{};
    for(unsigned i=0;i<301;++i)
        large.definitions.push_back({
            "Synthetic"+std::to_string(i),"scripts/fake.txt",{}});
    for(unsigned i=0;i<281;++i){
        c::Node wave{"wave","ambient/loop"+std::to_string(i)+".wav",{},false};
        large.definitions[i%284].children.push_back({
            "playlooping","",{wave},true});
    }
    unsigned made=0;
    for(unsigned i=0;i<150;++i){
        const unsigned candidates=i==0?20:6+(i<=29?1:0);
        c::Node randomBlock{"rndwave","",{},true};
        for(unsigned j=0;j<candidates;++j){
            randomBlock.children.push_back({
                "wave","*ambient/rand"+std::to_string(made++)+".mp3",
                {},false});
        }
        large.definitions[(i+281)%284].children.push_back({
            "playrandom","",{randomBlock},true});
    }
    if(made!=943)return Fail(15);
    large.counts.definitions=301;
    if(waves.Build(large)!=w::BuildStatus::Ready ||
       waves.value().groups.size()!=431 ||
       waves.value().summary.definitions!=301 ||
       waves.value().summary.looping_groups!=281 ||
       waves.value().summary.random_groups!=150 ||
       waves.value().summary.random_waves!=943 ||
       waves.value().summary.raw_wave_occurrences!=1224 ||
       waves.value().summary.maximum_random_candidates!=20 ||
       waves.value().summary.definitions_with_waves!=284)
        return Fail(16);

    waves.Clear();
    graph.Clear();
    if(waves.ready()||!waves.value().waves.empty()||
       waves.value().summary.raw_wave_occurrences!=0 ||
       w::ExpandScene(waves,graph,"A",plan)!=w::SceneStatus::Unready)
        return Fail(17);
    std::puts("PASS synthetic soundscape wave inventory: 281/150/1224 scale, looping/random groups, markers, WAV/MP3, raw options, owned scene planner, path refusal and rollback");
    return 0;
}
