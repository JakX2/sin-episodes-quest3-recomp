// Phase 495: original 2006 C_SoundscapeSystem::Init tolerant control
// flow over an independent resource-provider fixture. Source evidence
// authenticated in audit_sin_original_soundscape_pathid.py. No engine
// IFileSystem*, commercial KeyValues or audio mixer is invoked.
#include "client_soundscape_original_bootstrap_owned.h"
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace s=sin_independent::soundscape_original_bootstrap;
namespace c=sin_independent::soundscape_catalog;
static int Fail(int n){
    std::fprintf(stderr,"owned-original-tolerant-soundscape-Init-test=%d\n",n);
    return n;
}
struct Fixture final {
    std::map<std::string,std::string> files{};
    bool throw_on_script=false;
    int manifest_reads=0;
    int null_scope_reads=0;
    int wrong_scopes=0;
    int null_scope_exists=0;
    bool Read(std::string_view path,const char* pathID,std::string& output) {
        if(path==sin_independent::soundscape_manifest::kManifestPath) {
            ++manifest_reads;
            if(!pathID||std::string_view(pathID)!="GAME"){
                ++wrong_scopes;return false;
            }
        }else{
            ++null_scope_reads;
            if(pathID!=nullptr){++wrong_scopes;return false;}
            if(throw_on_script)throw std::runtime_error("invented backend failure");
        }
        const auto found=files.find(std::string(path));
        if(found==files.end())return false;
        output=found->second;
        return true;
    }
    bool Exists(std::string_view path,const char* pathID) {
        ++null_scope_exists;
        if(pathID!=nullptr){++wrong_scopes;return false;}
        return files.find(std::string(path))!=files.end();
    }
};
int main(){
    s::OwnedInit owned{};
    Fixture input{};
    const auto read=[&](std::string_view name,const char* scope,
                        std::string& output){
        return input.Read(name,scope,output);
    };
    const auto exists=[&](std::string_view name,const char* scope){
        return input.Exists(name,scope);
    };
    const std::string manifest=
        "\"soundscaples_manifest\" { "
        "\"file\" \"scripts/soundscapes.txt\" "
        "\"unused_metadata\" \"ignored\" "
        "\"file\" \"scripts/soundscapes_intro.txt\" }";
    const std::string first=
        "\"A\" { \"playlooping\" { \"wave\" \"sound/a.wav\" } "
        "\"playsoundscape\" { \"name\" \"C\" } }";
    const std::string second=
        "\"B\" { \"playrandom\" { \"rndwave\" { \"wave\" \"sound/b.wav\" } } }";
    const std::string overrideDoc=
        "\"C\" { \"playlooping\" { \"wave\" \"sound/c.wav\" } }";
    input.files={
        {"scripts/soundscapes_manifest.txt",manifest},
        {"scripts/soundscapes.txt",first},
        {"scripts/soundscapes_intro.txt",second},
        {"scripts/soundscapes_c01.txt",overrideDoc}
    };

    // Original manifest rb/GAME, individual files rb/null, optional
    // map probe FileExists(path,null), then load via the same helper.
    auto result=owned.Initialize(read,exists,"c01");
    if(result.status!=s::Status::LoadedAllAvailable ||
       !result.original_return_value_true ||!result.state_published ||
       !result.manifest_found ||!result.manifest_parsed ||
       !result.map_candidate_requested ||!result.map_exists_checked ||
       !result.map_file_exists ||
       result.map_candidate_suppressed_by_manifest ||
       result.files_declared!=2 ||result.manifest_unknown_keys!=1 ||
       result.file_reads_attempted!=3 ||result.file_reads_succeeded!=3 ||
       result.file_documents_accepted!=3 ||result.file_documents_refused!=0 ||
       result.accepted_definitions!=3 ||
       result.resolved_references!=1 ||result.unresolved_references!=0 ||
       owned.value().field_cc!=0 ||!owned.value().manifest_loaded ||
       !owned.value().optional_map_candidate_loaded ||
       owned.value().manifest_root!="soundscaples_manifest" ||
       owned.value().catalog.counts.definitions!=3 ||
       owned.value().catalog.counts.wave_properties!=3 ||
       owned.value().loaded_paths.size()!=3 ||
       input.manifest_reads!=1 ||input.null_scope_reads!=3 ||
       input.null_scope_exists!=1 ||input.wrong_scopes!=0)
        return Fail(1);
    const auto rev1=owned.revision();

    // The real Init compares the optional path to every manifest
    // "file" value. A map-specific file already named in the
    // manifest must not be loaded a second time via FileExists.
    const std::string nestedManifest=
        "\"soundscaples_manifest\" { "
        "\"file\" \"scripts/soundscapes.txt\" "
        "\"file\" \"scripts/soundscapes_c01.txt\" }";
    input.files["scripts/soundscapes_manifest.txt"]=nestedManifest;
    input.null_scope_exists=0;
    result=owned.Initialize(read,exists,"c01");
    if(result.status!=s::Status::LoadedAllAvailable ||
       !result.map_candidate_suppressed_by_manifest ||
       result.map_exists_checked ||
       result.files_declared!=2 ||
       result.file_reads_attempted!=2 ||result.file_documents_accepted!=2 ||
       owned.value().loaded_paths.size()!=2 ||
       owned.value().catalog.counts.definitions!=2 ||
       input.null_scope_exists!=0 ||
       owned.revision()!=rev1+1)
        return Fail(2);

    // A missing manifest takes the original load-failure branch,
    // deletes temporary KeyValues and still returns AL=1. It does
    // NOT check the optional map-specific resource in that branch.
    input.files.erase("scripts/soundscapes_manifest.txt");
    input.null_scope_exists=0;
    result=owned.Initialize(read,exists,"c01");
    if(result.status!=s::Status::MissingManifestStillReturnsTrue ||
       !result.original_return_value_true||!result.state_published ||
       result.manifest_found ||result.map_exists_checked ||
       result.file_reads_attempted!=0 ||
       !owned.value().catalog.definitions.empty() ||
       owned.value().manifest_loaded ||input.null_scope_exists!=0)
        return Fail(3);

    // An empty but valid manifest still enters the optional-map
    // FileExists tail. The strict diagnostic parser rejects an
    // empty file list; this owned original-Init policy must not.
    input.files["scripts/soundscapes_manifest.txt"]=
        "\"soundscaples_manifest\" { }";
    result=owned.Initialize(read,exists,"c01");
    if(result.status!=s::Status::LoadedAllAvailable ||
       result.files_declared!=0 ||!result.map_exists_checked ||
       result.file_reads_attempted!=1 ||
       result.file_documents_accepted!=1 ||
       owned.value().catalog.definitions.size()!=1 ||
       owned.value().catalog.definitions[0].name!="C")
        return Fail(4);

    // Individual failed file loads DO NOT propagate a false return
    // through C_SoundscapeSystem::Init; successful following files
    // must still be visited in original manifest order.
    input.files["scripts/soundscapes_manifest.txt"]=manifest;
    input.files.erase("scripts/soundscapes_intro.txt");
    result=owned.Initialize(read,exists,"c01");
    if(result.status!=s::Status::PartialResourcesStillReturnsTrue ||
       !result.original_return_value_true ||!result.state_published ||
       result.file_reads_attempted!=3 ||result.file_reads_succeeded!=2 ||
       result.file_documents_refused!=1 ||
       result.file_documents_accepted!=2 ||
       owned.value().catalog.counts.definitions!=2 ||
       result.resolved_references!=1 ||
       input.wrong_scopes!=0)
        return Fail(5);
    input.files["scripts/soundscapes_intro.txt"]=second;

    // A source that opens but contains malformed nested KV text is
    // conservatively skipped by the owned parser. Original KeyValues
    // malformed-document semantics are NOT silently assumed identical.
    input.files["scripts/soundscapes_intro.txt"]="\"B\" { \"wave\" {";
    result=owned.Initialize(read,exists,"c01");
    if(result.status!=s::Status::PartialResourcesStillReturnsTrue ||
       result.file_reads_succeeded!=3 ||
       result.file_documents_refused!=1 ||
       owned.value().catalog.counts.definitions!=2)return Fail(6);
    input.files["scripts/soundscapes_intro.txt"]=second;

    // A syntactically invalid or unsafe manifest is not promoted
    // to a valid catalog. The model records the original Init's
    // true-return policy separately from resource success.
    input.files["scripts/soundscapes_manifest.txt"]=
        "\"soundscaples_manifest\" { \"file\" \"scripts/../escape.txt\" }";
    result=owned.Initialize(read,exists,"c01");
    if(result.status!=s::Status::InvalidManifestStillReturnsTrue ||
       !result.original_return_value_true ||result.manifest_parsed ||
       !owned.value().catalog.definitions.empty())return Fail(7);
    input.files["scripts/soundscapes_manifest.txt"]=manifest;

    // Unsafe map suffix refusal is an INDEPENDENT safety cap. The
    // ordinary manifest files still load and original style true
    // return is preserved, without probing a traversal path.
    input.null_scope_exists=0;
    result=owned.Initialize(read,exists,"../evil");
    if(result.status!=s::Status::UnsafeMapNameSkipped ||
       !result.original_return_value_true ||!result.unsafe_map_suffix ||
       result.map_exists_checked ||result.file_reads_attempted!=2 ||
       result.accepted_definitions!=2 ||
       result.unresolved_references!=1 ||
       input.null_scope_exists!=0)return Fail(8);

    // A throwing independent injected provider must preserve the
    // preceding valid state atomically, not publish an empty catalog
    // or falsely claim Source 2006 exception behavior.
    const auto beforeThrow=owned.revision();
    input.throw_on_script=true;
    result=owned.Initialize(read,exists,"c01");
    if(result.status!=s::Status::OwnedBackendRefused ||
       result.original_return_value_true ||result.state_published ||
       owned.revision()!=beforeThrow ||
       owned.value().catalog.counts.definitions!=2)
        return Fail(9);
    input.throw_on_script=false;

    // Original loader appends top-level name records in source order;
    // it does not perform our STRICT diagnostic's duplicate-name gate.
    input.files["scripts/soundscapes_intro.txt"]=
        "\"A\" { \"playlooping\" { \"wave\" \"sound/repeat.wav\" } }";
    result=owned.Initialize(read,exists,{});
    if(result.status!=s::Status::LoadedAllAvailable ||
       result.file_reads_attempted!=2 ||
       owned.value().catalog.counts.definitions!=2 ||
       owned.value().catalog.definitions[0].name!="A" ||
       owned.value().catalog.definitions[1].name!="A")
        return Fail(10);
    input.files["scripts/soundscapes_intro.txt"]=second;

    // The independent 256 KiB reader cap refuses a pathological file
    // while preserving later documents and true-return policy.
    input.files["scripts/soundscapes_intro.txt"]=
        std::string(s::kMaxIndividualFileBytes+1,'Z');
    result=owned.Initialize(read,exists,"c01");
    if(result.status!=s::Status::PartialResourcesStillReturnsTrue ||
       result.file_reads_succeeded!=3 ||
       result.file_documents_refused!=1 ||
       result.file_documents_accepted!=2 ||
       owned.value().catalog.counts.definitions!=2)
        return Fail(11);
    input.files["scripts/soundscapes_intro.txt"]=second;

    // No file keys and no optional map: source-compatible nonfatal
    // empty catalog with the retained manifest root spelling.
    input.files["scripts/soundscapes_manifest.txt"]=
        "\"soundscaples_manifest\" { }";
    result=owned.Initialize(read,exists,{});
    if(result.status!=s::Status::LoadedAllAvailable ||
       result.files_declared!=0 ||result.file_reads_attempted!=0 ||
       !result.manifest_parsed ||
       !owned.value().catalog.definitions.empty() ||
       owned.value().manifest_root!="soundscaples_manifest")
        return Fail(12);

    // Valid manifest with only unknown scalar/block keys still
    // returns true and reports both ignored keys. Even this path
    // must not silently change the original root spelling.
    input.files["scripts/soundscapes_manifest.txt"]=
        "\"soundscaples_manifest\" { "
        "\"unknown_scalar\" \"note\" "
        "\"unknown_block\" { \"inside\" \"yes\" } }";
    result=owned.Initialize(read,exists,{});
    if(result.status!=s::Status::LoadedAllAvailable ||
       !result.original_return_value_true ||
       result.files_declared!=0 ||
       result.manifest_unknown_keys!=2 ||
       result.file_reads_attempted!=0 ||
       owned.value().manifest_root!="soundscaples_manifest" ||
       !owned.value().catalog.definitions.empty())
        return Fail(14);

    owned.Clear();
    if(owned.value().manifest_loaded ||
       !owned.value().catalog.definitions.empty() ||
       !owned.value().loaded_paths.empty())return Fail(13);
    std::puts("PASS original 2006 Soundscape::Init bootstrap: rb/GAME manifest, rb/nullptr scripts, original true-return failure paths, map fallback/suppression, partial and empty catalogs, source order and atomic refusals");
    return 0;
}
