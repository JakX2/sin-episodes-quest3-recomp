#pragma once
// Phase 484: original-data-driven soundscape resources through an owned
// VFileSystem012 read boundary. Independent authored data/parse code,
// NOT an implementation of Source C_SoundscapeSystem or live game Init.
//
// SHA-pinned client 0x24164A50 KeyValues::LoadFromFile invokes the
// original 2006 IFileSystem::Open slot 2 with "rb" (rdata 0x24289C0C).
// Original C_SoundscapeSystem::Init passes "GAME" for the manifest;
// the separate per-file loader at 0x240F4070 passes a null pathID
// through the same KeyValues::LoadFromFile method. Never collapse
// these two search scopes into a single mode.
// The independently rebuilt VFileSystem012 slots 0,2,3,7 are reused
// via client_engine_filesystem_read.h. Actual engine calls must be
// separately approved and verified in a disposable owner process.
#if !defined(_M_IX86)
#error Windows MSVC x86-only Source 2004 filesystem ABI fixture
#endif
#include "client_engine_filesystem_read.h"
#include "client_soundscape_catalog_owned.h"
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace sin_independent::soundscape_engine_fs {
enum class Status {
    OwnedCatalogReady, InvalidEngineAbi, ManifestReadRefused,
    ManifestOrDependencyRefused, CatalogParseRefused, AllocationRefused
};
inline const char* StatusName(Status status) noexcept {
    switch(status){
        case Status::OwnedCatalogReady:
            return "owned-engine-soundscape-catalog-ready-no-game-init";
        case Status::InvalidEngineAbi:return "engine-filesystem-abi-refused";
        case Status::ManifestReadRefused:return "soundscape-manifest-read-refused";
        case Status::ManifestOrDependencyRefused:
            return "soundscape-manifest-or-file-refused";
        case Status::CatalogParseRefused:return "soundscape-definition-refused";
        case Status::AllocationRefused:return "soundscape-resource-allocation-refused";
    }
    return "unknown";
}
// Owner-retail diagnostic evidence: coarse failure boundaries, no paths,
// original text, pointers or resource data leaked into the trace.
enum class Stage {
    ResolveAbi, ReadManifest, ValidateManifest, ReadScript,
    ScriptBudget, BuildCatalog, Complete
};
inline const char* StageName(Stage stage) noexcept {
    switch(stage) {
        case Stage::ResolveAbi:return "resolve-abi";
        case Stage::ReadManifest:return "read-manifest";
        case Stage::ValidateManifest:return "validate-manifest";
        case Stage::ReadScript:return "read-script";
        case Stage::ScriptBudget:return "script-budget";
        case Stage::BuildCatalog:return "build-catalog";
        case Stage::Complete:return "complete";
    }
    return "unknown";
}
struct Trace final {
    Stage stage=Stage::ResolveAbi;
    soundscape_manifest::Status manifest_status=
        soundscape_manifest::Status::Ready;
    soundscape_catalog::Status catalog_status=
        soundscape_catalog::Status::Ready;
    std::size_t script_reads_attempted=0;
    std::size_t script_reads_succeeded=0;
};
static_assert(std::is_nothrow_copy_assignable<Trace>::value,
              "trace publishing must never throw");
class Bridge final {
public:
    const soundscape_manifest::Snapshot& scripts() const noexcept {
        return scripts_.value();
    }
    const soundscape_catalog::CatalogState& catalog() const noexcept {
        return catalog_.value();
    }
    void Clear() noexcept {
        catalog_.Clear();
        scripts_.Clear();
    }
    Status Load(void* fileSystem,Trace* observed=nullptr) noexcept {
        Trace trace{};
        const auto finish=[&](Status status) noexcept {
            if(observed)*observed=trace;
            return status;
        };
        engine_fs::Methods methods{};
        if(!engine_fs::Resolve(fileSystem,methods,true))
            return finish(Status::InvalidEngineAbi);
        try {
            trace.stage=Stage::ReadManifest;
            std::string manifestText;
            const std::string path(soundscape_manifest::kManifestPath);
            if(!engine_fs::ReadOne(fileSystem,methods,path.c_str(),
                                   manifestText,"rb","GAME"))
                return finish(Status::ManifestReadRefused);
            trace.stage=Stage::ValidateManifest;
            soundscape_manifest::OwnedSnapshot stagedScripts{};
            const auto readOne=[&](std::string_view relativePath,
                                   std::string& output)->bool {
                trace.stage=Stage::ReadScript;
                ++trace.script_reads_attempted;
                std::string name(relativePath);
                // Original SHA-matched 2006 per-script helper:
                // 0x240F409C pushes null pathID and 0x240F40A6
                // calls KeyValues::LoadFromFile at 0x24164A50.
                // The manifest in Init (0x240F4160) uses "GAME".
                // Keep the two search scopes separate.
                const bool ok=engine_fs::ReadOne(
                    fileSystem,methods,name.c_str(),output,"rb",nullptr);
                if(ok)++trace.script_reads_succeeded;
                return ok;
            };
            trace.manifest_status=stagedScripts.Load(manifestText,readOne);
            if(trace.manifest_status!=soundscape_manifest::Status::Ready) {
                if(trace.manifest_status==soundscape_manifest::Status::FileTooLarge ||
                   trace.manifest_status==soundscape_manifest::Status::TotalSizeExceeded)
                    trace.stage=Stage::ScriptBudget;
                else if(trace.manifest_status!=soundscape_manifest::Status::MissingFile)
                    trace.stage=Stage::ValidateManifest;
                return finish(Status::ManifestOrDependencyRefused);
            }
            trace.stage=Stage::BuildCatalog;
            soundscape_catalog::OwnedCatalog stagedCatalog{};
            trace.catalog_status=stagedCatalog.Build(stagedScripts.value());
            if(trace.catalog_status!=soundscape_catalog::Status::Ready)
                return finish(Status::CatalogParseRefused);
            // Publish BOTH complete snapshots only after every read/parse.
            scripts_=std::move(stagedScripts);
            catalog_=std::move(stagedCatalog);
            trace.stage=Stage::Complete;
            return finish(Status::OwnedCatalogReady);
        }catch(...){
            return finish(Status::AllocationRefused);
        }
    }
private:
    soundscape_manifest::OwnedSnapshot scripts_{};
    soundscape_catalog::OwnedCatalog catalog_{};
};
} // namespace sin_independent::soundscape_engine_fs
