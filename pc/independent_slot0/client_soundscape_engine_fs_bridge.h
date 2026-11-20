#pragma once
// Phase 484: original-data-driven soundscape resources through an owned
// VFileSystem012 read boundary. Independent authored data/parse code,
// NOT an implementation of Source C_SoundscapeSystem or live game Init.
//
// SHA-pinned client 0x24164A50 KeyValues::LoadFromFile invokes the
// original 2006 IFileSystem::Open slot 2 with "rb" (rdata 0x24289C0C).
// Original C_SoundscapeSystem::Init passes "GAME" and the manifest path;
// it also loads each manifest "file" via the same KeyValues helper.
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
    Status Load(void* fileSystem) noexcept {
        engine_fs::Methods methods{};
        if(!engine_fs::Resolve(fileSystem,methods,true))
            return Status::InvalidEngineAbi;
        try {
            std::string manifestText;
            const std::string path(soundscape_manifest::kManifestPath);
            if(!engine_fs::ReadOne(fileSystem,methods,path.c_str(),
                                   manifestText,"rb","GAME"))
                return Status::ManifestReadRefused;
            soundscape_manifest::OwnedSnapshot stagedScripts{};
            const auto readOne=[&](std::string_view relativePath,
                                   std::string& output)->bool {
                std::string name(relativePath);
                return engine_fs::ReadOne(
                    fileSystem,methods,name.c_str(),output,"rb","GAME");
            };
            if(stagedScripts.Load(manifestText,readOne)!=
                   soundscape_manifest::Status::Ready)
                return Status::ManifestOrDependencyRefused;
            soundscape_catalog::OwnedCatalog stagedCatalog{};
            if(stagedCatalog.Build(stagedScripts.value())!=
                   soundscape_catalog::Status::Ready)
                return Status::CatalogParseRefused;
            // Complete publication only after every resource has been
            // read, parsed and cross-file indexed. Original partial
            // KeyValues mutation on failure is not replicated.
            scripts_=std::move(stagedScripts);
            catalog_=std::move(stagedCatalog);
            return Status::OwnedCatalogReady;
        }catch(...){
            return Status::AllocationRefused;
        }
    }
private:
    soundscape_manifest::OwnedSnapshot scripts_{};
    soundscape_catalog::OwnedCatalog catalog_{};
};
} // namespace sin_independent::soundscape_engine_fs
