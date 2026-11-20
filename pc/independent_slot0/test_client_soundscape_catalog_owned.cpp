// Phase 483 independent Win32 soundscape KeyValues catalog native test.
// All fixture scripts are invented; retail soundscape assets stay private.
#include "client_soundscape_catalog_owned.h"
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>

namespace m=sin_independent::soundscape_manifest;
namespace c=sin_independent::soundscape_catalog;
static int Fail(int id){
    std::fprintf(stderr,"soundscape owned catalog test failed %d\n",id);
    return id;
}
static const std::string manifest=R"KV(
soundscaples_manifest
{
  "file" "scripts/soundscapes.txt"
  "file" "scripts/soundscapes_extra.txt"
}
)KV";
static const std::string first=R"KV(
"A"
{
  "dsp" "1"
  "playlooping" { "wave" "ambient/a.wav" }
  "playlooping" { "wave" "ambient/b.wav" }
  "playsoundscape" { "name" "B" "volume" "0.5" }
  "playsoundscape" { "name" "missing" }
  "playrandom" {
    "rndwave" {
      "wave" "ambient/c.wav"
      "wave" "ambient/d.wav"
    }
  }
}
)KV";
static const std::string second=R"KV(
"B" {
  "dsp" "2"
  "playsoundscape" { "name" "c" }
}
"C" {
  "playlooping" { "wave" "ambient/e.wav" }
}
)KV";
struct Provider {
    std::string a=first,b=second;
    bool Read(std::string_view path,std::string& out){
        if(path=="scripts/soundscapes.txt"){out=a;return true;}
        if(path=="scripts/soundscapes_extra.txt"){out=b;return true;}
        return false;
    }
};
int main() {
    Provider source;
    m::OwnedSnapshot scripts;
    const auto read=[&](std::string_view path,std::string& out){
        return source.Read(path,out);
    };
    if(scripts.Load(manifest,read)!=m::Status::Ready)return Fail(1);
    c::OwnedCatalog catalog;
    if(catalog.Build(scripts.value())!=c::Status::Ready)return Fail(2);
    const auto& state=catalog.value();
    if(state.counts.documents!=2 || state.counts.definitions!=3 ||
       state.counts.looping_blocks!=3 || state.counts.random_blocks!=1 ||
       state.counts.random_wave_blocks!=1 ||
       state.counts.wave_properties!=5 ||
       state.counts.nested_soundscape_blocks!=3 ||
       state.references.size()!=3 || state.unresolved.size()!=1 ||
       state.unresolved[0].owner!="A" ||
       state.unresolved[0].target!="missing")
        return Fail(3);
    const auto* a=catalog.Find("a");
    const auto* b=catalog.Find("B");
    const auto* cRef=catalog.Find("c");
    if(!a||!b||!cRef||catalog.Find("unknown")||
       a->source!="scripts/soundscapes.txt" ||
       a->children.size()!=6 || b->children.size()!=2 ||
       cRef->children.size()!=1)return Fail(4);
    if(!a->children[1].is_block || !a->children[2].is_block ||
       a->children[1].key!="playlooping" ||
       a->children[2].key!="playlooping" ||
       a->children[4].children[0].scalar!="missing")
        return Fail(5);

    // Duplicate root names across different documents, including
    // capitalization differences, cannot silently overwrite the
    // previous valid owned snapshot.
    source.b=R"KV("a" { "dsp" "3" })KV";
    if(scripts.Load(manifest,read)!=m::Status::Ready ||
       catalog.Build(scripts.value())!=c::Status::DuplicateSoundscapeName ||
       catalog.Find("C")==nullptr ||
       catalog.value().counts.definitions!=3)return Fail(6);

    source.b=R"KV("B" { "dsp" "2" })KV";
    source.a=R"KV("A" { "playlooping" { "wave" "x.wav" } )KV";
    if(scripts.Load(manifest,read)!=m::Status::Ready ||
       catalog.Build(scripts.value())!=c::Status::MalformedDefinition ||
       catalog.value().counts.definitions!=3)return Fail(7);

    source.a=first;
    source.b=" // no definitions here \r\n ";
    if(scripts.Load(manifest,read)!=m::Status::Ready ||
       catalog.Build(scripts.value())!=c::Status::EmptyDefinitionFile ||
       catalog.value().counts.definitions!=3)return Fail(8);

    // Bound recursion depth independent of Source engine KeyValues.
    source.b=second;
    source.a="\"A\" { ";
    for(unsigned depth=0;depth<c::kMaxDepth+2;++depth)
        source.a+="\"nest\" { ";
    source.a+="\"data\" \"value\" ";
    for(unsigned depth=0;depth<c::kMaxDepth+2;++depth)
        source.a+="} ";
    source.a+="}";
    if(scripts.Load(manifest,read)!=m::Status::Ready ||
       catalog.Build(scripts.value())!=c::Status::NestingTooDeep ||
       catalog.value().counts.definitions!=3)return Fail(9);

    // Property cap must refuse a massive but valid fixture without
    // corrupting any previous accepted catalog.
    source.a="\"A\" { ";
    for(unsigned i=0;i<c::kMaxNodes+1;++i)
        source.a+="\"k\" \"v\" ";
    source.a+="}";
    if(scripts.Load(manifest,read)!=m::Status::Ready ||
       catalog.Build(scripts.value())!=c::Status::TooManyNodes ||
       catalog.value().counts.definitions!=3)return Fail(10);

    source.a=first;
    if(scripts.Load(manifest,read)!=m::Status::Ready ||
       catalog.Build(scripts.value())!=c::Status::Ready ||
       catalog.value().counts.definitions!=3)return Fail(11);

    c::OwnedCatalog none;
    m::Snapshot missing{};
    if(none.Build(missing)!=c::Status::MissingManifestSnapshot)
        return Fail(12);
    missing=scripts.value();
    missing.manifest.file_paths.pop_back();
    if(none.Build(missing)!=c::Status::MissingManifestSnapshot)
        return Fail(13);

    catalog.Clear();
    if(!catalog.value().definitions.empty() ||
       !catalog.value().unresolved.empty() ||
       catalog.value().counts.definitions!=0)
        return Fail(14);

    std::puts("PASS independent soundscape catalog: nested KV trees, repeated blocks, 3 refs, 1 missing, casefold lookup, atomic rollback, depth/node/duplicate caps");
    return 0;
}
