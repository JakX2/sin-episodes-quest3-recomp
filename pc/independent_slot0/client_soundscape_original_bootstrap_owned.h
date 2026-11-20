#pragma once
// Phase 495: independently owned C_SoundscapeSystem::Init startup policy.
// SHA-authenticated original 343-byte Init VA 0x240F4160 and 233-byte
// per-file helper VA 0x240F4070 (see original pathID auditor).
//
// Original evidence: manifest rb/GAME, each file rb/null pathID,
// unknown manifest keys ignored, optional map-specific file check
// using a null pathID, per-file failures do not propagate, and
// both original Init return sites set AL=1 even after manifest
// failure. This owned reconstruction NEVER calls original KeyValues
// or engine filesystem, and is distinct from the strict owner
// 0x53494E76 soundscape *diagnostic* requiring all seven files.
#include "client_soundscape_catalog_owned.h"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sin_independent::soundscape_original_bootstrap {
namespace manifest=soundscape_manifest;
namespace catalog=soundscape_catalog;
inline constexpr std::uint32_t kOriginalInitVA=0x240F4160u;
inline constexpr std::uint32_t kOriginalSingleFileHelperVA=0x240F4070u;
inline constexpr std::uint32_t kOriginalOptionalExistenceCallVA=0x240F4277u;
inline constexpr std::uint32_t kOriginalInitFlagOffset=0xCCu;
inline constexpr std::string_view kOriginalOptionalPrefix="scripts/soundscapes_";
inline constexpr std::string_view kOriginalOptionalSuffix=".txt";
inline constexpr std::size_t kMaxOwnedMapSuffix=64u;
inline constexpr std::size_t kMaxIndividualFileBytes=256u*1024u;
// This is the stricter bound of the separate VFileSystem012 ReadOne
// adapter, not a limit claimed for original 2006 KeyValues.
enum class Status {
    LoadedAllAvailable, MissingManifestStillReturnsTrue,
    InvalidManifestStillReturnsTrue, PartialResourcesStillReturnsTrue,
    UnsafeMapNameSkipped, OwnedBackendRefused
};
inline const char* Name(Status status) noexcept {
    switch(status) {
        case Status::LoadedAllAvailable:return "owned-original-init-resources-loaded";
        case Status::MissingManifestStillReturnsTrue:
            return "original-init-true-manifest-missing";
        case Status::InvalidManifestStillReturnsTrue:
            return "original-init-true-manifest-refused";
        case Status::PartialResourcesStillReturnsTrue:
            return "original-init-true-partial-files";
        case Status::UnsafeMapNameSkipped:return "owned-map-name-unsafe-skipped";
        case Status::OwnedBackendRefused:return "owned-original-init-refused";
    }
    return "unknown";
}
struct Summary final {
    Status status=Status::LoadedAllAvailable;
    bool original_return_value_true=false;
    bool state_published=false;
    bool manifest_found=false;
    bool manifest_parsed=false;
    bool map_candidate_suppressed_by_manifest=false;
    bool map_candidate_requested=false;
    bool map_exists_checked=false;
    bool map_file_exists=false;
    bool unsafe_map_suffix=false;
    std::size_t manifest_unknown_keys=0;
    std::size_t files_declared=0;
    std::size_t file_reads_attempted=0;
    std::size_t file_reads_succeeded=0;
    std::size_t file_documents_accepted=0;
    std::size_t file_documents_refused=0;
    std::size_t accepted_definitions=0;
    // These counters are derived from *owned* parsers, not engine
    // KeyValues memory and not evidence of actual audio playback.
    std::size_t resolved_references=0;
    std::size_t unresolved_references=0;
};
struct State final {
    std::string manifest_root;
    catalog::CatalogState catalog{};
    std::vector<std::string> loaded_paths{};
    std::uint32_t field_cc=0;
    bool manifest_loaded=false;
    bool optional_map_candidate_loaded=false;
};
inline bool SafeMapSuffix(std::string_view input) noexcept {
    if(input.empty()||input.size()>kMaxOwnedMapSuffix)return false;
    for(const char ch:input)
        if(!((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||
             (ch>='0'&&ch<='9')||ch=='_'||ch=='-'))
            return false;
    return true;
}
inline bool ParsePermissiveEmptyManifest(std::string_view text,
                                         manifest::Manifest& out) noexcept {
    // The strict owned manifest parser returns NoFileEntries for a
    // syntactically valid empty manifest. The original 2006 function
    // still checks for an optional map-specific soundscape after
    // seeing zero "file" keys, so preserve that source branch.
    auto status=manifest::Parse(text,out);
    if(status==manifest::Status::Ready)return true;
    if(status!=manifest::Status::NoFileEntries)return false;
    manifest::detail::Lexer lexer(text);
    const auto root=lexer.Next();
    if(root.kind!=manifest::detail::Kind::Word ||
       root.text.empty())return false;
    out={};
    out.root=root.text; // full grammar was already checked by Parse
    return true;
}
class OwnedInit final {
public:
    const State& value() const noexcept{return state_;}
    std::uint64_t revision() const noexcept{return revision_;}
    void Clear() noexcept {state_={};++revision_;}
    // Read(path, pathID, string& output): bool.
    // Exists(path, pathID): bool.
    // These callbacks only receive independent strings and a null or
    // static "GAME" pathID. No actual engine objects or pointers
    // are ever passed into this model.
    template<class Read,class Exists>
    Summary Initialize(Read&& read,Exists&& exists,
                       std::string_view mapSuffix={}) noexcept {
        Summary result{};
        State next{};
        next.field_cc=0; // original Init starts with this+0xCC=0
        try {
            std::string optionalPath;
            if(!mapSuffix.empty()) {
                if(!SafeMapSuffix(mapSuffix))result.unsafe_map_suffix=true;
                else {
                    optionalPath.reserve(kOriginalOptionalPrefix.size()+
                                         mapSuffix.size()+
                                         kOriginalOptionalSuffix.size());
                    optionalPath.assign(kOriginalOptionalPrefix);
                    optionalPath.append(mapSuffix);
                    optionalPath.append(kOriginalOptionalSuffix);
                    if(!manifest::detail::SafePath(optionalPath)) {
                        optionalPath.clear();
                        result.unsafe_map_suffix=true;
                    }
                }
            }
            result.map_candidate_requested=!optionalPath.empty();
            std::string rawManifest;
            if(!read(manifest::kManifestPath,"GAME",rawManifest)) {
                // The actual original emits an error and executes
                // a second AL=1 return path. Optional per-level
                // existence lookup is NOT reached.
                result.status=Status::MissingManifestStillReturnsTrue;
                result.original_return_value_true=true;
                state_=std::move(next);
                ++revision_;
                result.state_published=true;
                return result;
            }
            result.manifest_found=true;
            manifest::Manifest fileList{};
            if(!ParsePermissiveEmptyManifest(rawManifest,fileList)) {
                result.status=Status::InvalidManifestStillReturnsTrue;
                result.original_return_value_true=true;
                state_=std::move(next);
                ++revision_;
                result.state_published=true;
                return result;
            }
            result.manifest_parsed=true;
            result.files_declared=fileList.file_paths.size();
            result.manifest_unknown_keys=fileList.unknown_keys;
            next.manifest_root=std::move(fileList.root);
            next.manifest_loaded=true;
            bool skippedAny=false;
            auto loadSingle=[&](std::string_view path,bool mapSpecific)->void {
                ++result.file_reads_attempted;
                std::string data;
                if(!read(path,nullptr,data)) {
                    ++result.file_documents_refused;
                    skippedAny=true;
                    return;
                }
                ++result.file_reads_succeeded;
                if(data.empty()||data.size()>kMaxIndividualFileBytes) {
                    ++result.file_documents_refused;
                    skippedAny=true;
                    return;
                }
                catalog::CatalogState one{};
                const auto syntax=catalog::detail::ParseDocument(
                    data,path,one);
                if(syntax!=catalog::Status::Ready ||
                   one.definitions.size()>catalog::kMaxDefinitions-
                       next.catalog.definitions.size() ||
                   one.counts.property_nodes>catalog::kMaxNodes-
                       next.catalog.counts.property_nodes) {
                    ++result.file_documents_refused;
                    skippedAny=true;
                    return;
                }
                const auto newNodes=one.counts.property_nodes;
                next.catalog.definitions.insert(
                    next.catalog.definitions.end(),
                    std::make_move_iterator(one.definitions.begin()),
                    std::make_move_iterator(one.definitions.end()));
                next.catalog.counts.property_nodes+=newNodes;
                next.loaded_paths.emplace_back(path);
                ++next.catalog.counts.documents;
                ++result.file_documents_accepted;
                if(mapSpecific)next.optional_map_candidate_loaded=true;
            };
            for(const auto& path:fileList.file_paths) {
                if(!optionalPath.empty() &&
                   manifest::detail::EqualsAsciiIgnoreCase(
                       path,optionalPath)) {
                    result.map_candidate_suppressed_by_manifest=true;
                    optionalPath.clear();
                }
                loadSingle(path,false);
            }
            if(!optionalPath.empty()) {
                // Original virtual IFileSystem::FileExists slot 10,
                // called with pathID == nullptr after manifest loop.
                result.map_exists_checked=true;
                if(exists(optionalPath,nullptr)) {
                    result.map_file_exists=true;
                    loadSingle(optionalPath,true);
                }
            }
            next.catalog.counts.definitions=
                next.catalog.definitions.size();
            for(const auto& definition:next.catalog.definitions)
                catalog::detail::Visit(definition.children,definition,
                                       next.catalog);
            // Resolve references only AFTER all loaded source docs,
            // so a forward nested name can refer to a later file.
            for(const auto& edge:next.catalog.references) {
                bool found=false;
                for(const auto& definition:next.catalog.definitions)
                    if(manifest::detail::EqualsAsciiIgnoreCase(
                        edge.target,definition.name)) {
                        found=true;break;
                    }
                if(found)++result.resolved_references;
                else {
                    next.catalog.unresolved.push_back(edge);
                    ++result.unresolved_references;
                }
            }
            next.catalog.counts.unresolved_references=
                next.catalog.unresolved.size();
            result.accepted_definitions=
                next.catalog.counts.definitions;
            // Missing documents are *not fatal* to original Init.
            // Unsafe map suffix is our own conservative policy.
            result.status=result.unsafe_map_suffix?
                Status::UnsafeMapNameSkipped:
                skippedAny?Status::PartialResourcesStillReturnsTrue:
                           Status::LoadedAllAvailable;
            result.original_return_value_true=true;
            state_=std::move(next);
            ++revision_;
            result.state_published=true;
            return result;
        }catch(...){
            // Independent-only safety policy: never publish partially
            // reconstructed state if an owned callback or allocation
            // throws. The real 2006 method's SEH/memory-failure
            // behavior has not been authenticated.
            result.status=Status::OwnedBackendRefused;
            return result;
        }
    }
private:
    State state_{};
    std::uint64_t revision_=0;
};
} // namespace sin_independent::soundscape_original_bootstrap
