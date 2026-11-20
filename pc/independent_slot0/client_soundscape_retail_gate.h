#pragma once
// Phase 486: optional owner-retail read-only soundscape catalog milestone.
//
// Must ONLY run after an independently verified client Stats hook gate:
// original 21 interfaces, sound/VGUI prerequisites, genuine engine Stats
// reads and three independently bound CUserMessages message IDs.
// This gate does not invoke original C_SoundscapeSystem or play sounds.
// Real retail execution still requires a disposable, SHA-authenticated
// owner game copy and the reversible owner launcher.
#include "client_soundscape_engine_fs_bridge.h"
#include <cstddef>
#include <string_view>

namespace sin_independent::soundscape_retail_gate {
enum class Status {
    CatalogVerified, FilesystemOrParseRefused, OriginalCountsMismatch
};
inline const char* StatusName(Status status) noexcept {
    switch(status){
        case Status::CatalogVerified:
            return "owned-soundscape-catalog-verified-original-files";
        case Status::FilesystemOrParseRefused:
            return "owned-soundscape-read-or-parse-refused";
        case Status::OriginalCountsMismatch:
            return "owned-soundscape-original-counts-refused";
    }
    return "unknown";
}
struct Counts final {
    std::size_t files=0;
    std::size_t definitions=0;
    std::size_t nested_references=0;
    std::size_t unresolved_references=0;
};
struct Report final {
    Status status=Status::FilesystemOrParseRefused;
    Counts counts{};
    soundscape_engine_fs::Status bridge_result=
        soundscape_engine_fs::Status::InvalidEngineAbi;
    soundscape_engine_fs::Trace diagnostic{};
    bool verified() const noexcept {
        return status==Status::CatalogVerified;
    }
};
inline bool ValidateExpectedRetail(const Counts& n,
                                  std::string_view root) noexcept {
    // The seven original soundscape assets were SHA-inventoried privately.
    // They contain 301 case-insensitively unique root definitions,
    // 242 playsoundscape refs and 3 unresolved within those files.
    return n.files==7 && n.definitions==301 &&
           n.nested_references==242 && n.unresolved_references==3 &&
           root==soundscape_manifest::kObservedRetailRoot;
}
inline Report Verify(void* engineFileSystem) noexcept {
    Report report{};
    soundscape_engine_fs::Bridge bridge{};
    report.bridge_result=bridge.Load(engineFileSystem,&report.diagnostic);
    if(report.bridge_result!=
       soundscape_engine_fs::Status::OwnedCatalogReady)
        return report;
    report.counts={
        bridge.scripts().files.size(),
        bridge.catalog().counts.definitions,
        bridge.catalog().counts.nested_soundscape_blocks,
        bridge.catalog().counts.unresolved_references};
    report.status=ValidateExpectedRetail(
        report.counts,bridge.scripts().manifest.root)?
        Status::CatalogVerified:Status::OriginalCountsMismatch;
    return report;
}
} // namespace sin_independent::soundscape_retail_gate
