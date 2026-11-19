#pragma once
// Original-content validation checkpoint for the independent Windows client.
// Runs after the owner-verified sound-system callback only when opted in.
// This does NOT invoke original C_StatsSystem::Init, IFileSystem or event ABI.
#include "client_stats_asset_loader.h"
#include <cstddef>
#include <filesystem>
#include <string>

namespace sin_independent::stats_live_probe {
struct Counts {
    std::size_t definitions=0;
    std::size_t percentage_refs=0;
    std::size_t display_pairs=0;
    std::size_t dividers=0;
};
enum class Result { Verified, MissingAbsoluteRoot, ReadOrParseRefused,
                    UnexpectedRetailCounts };
inline const char* Name(Result result) noexcept {
    switch(result) {
        case Result::Verified:return "verified-original-counts";
        case Result::MissingAbsoluteRoot:return "missing-absolute-root";
        case Result::ReadOrParseRefused:return "asset-read-or-parse-refused";
        case Result::UnexpectedRetailCounts:return "unexpected-retail-counts";
    }
    return "unknown";
}
inline Result Validate(const std::filesystem::path* content_root,
                       Counts* counts=nullptr) noexcept {
    if(counts) *counts={};
    try {
        if(!content_root || content_root->empty() ||
           !content_root->is_absolute())
            return Result::MissingAbsoluteRoot;
        stats_assets::Documents docs;
        std::string error;
        if(!docs.LoadContentRoot(*content_root,error))
            return Result::ReadOrParseRefused;
        Counts observed;
        observed.definitions=docs.definitions().size();
        for(const auto& def:docs.definitions())
            if(!def.percentage_of.empty())++observed.percentage_refs;
        for(const auto& entry:docs.display()) {
            if(entry.divider)++observed.dividers;
            else ++observed.display_pairs;
        }
        if(counts) *counts=observed;
        // Owner originals, SHA-pinned in OWNER_CLIENT_STATS_SCRIPTS.md.
        if(observed.definitions!=62 || observed.percentage_refs!=8 ||
           observed.display_pairs!=156 || observed.dividers!=66)
            return Result::UnexpectedRetailCounts;
        return Result::Verified;
    } catch(...) {
        // A malformed filesystem path or allocation failure must never
        // terminate a diagnostic process via a noexcept coordinator.
        return Result::ReadOrParseRefused;
    }
}
} // namespace sin_independent::stats_live_probe
