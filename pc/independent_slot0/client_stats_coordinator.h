#pragma once
// Clean-room Windows client C_StatsSystem Init coordinator.
// This orders three *partial* original-guided owned helpers and exposes
// the still-missing proprietary resource/layout/registration operations.
// DO NOT pass original retail client.dll objects or function addresses.
#include <cstddef>
#include "client_stats_storage_init.h"
#include "client_stats_catalog_init.h"
#include "client_stats_asset_loader.h"
#include "client_stats_original_catalog_binding.h"

namespace sin_independent::stats_coordinator {
struct State final {
    stats_storage::Storage definitions;
    stats_catalog::Catalog catalog;
    stats_assets::Documents scripts;
    // Independently owned 66-index source text projection.
    stats_binding::Projection catalog_text;
    std::string last_script_error;
    State()=default;
    State(const State&)=delete;
    State& operator=(const State&)=delete;
    State(State&&)=delete;
    State& operator=(State&&)=delete;
};
using AssetReader=bool (*)(void* context,std::string& definitions,
                           std::string& layout,std::string& error) noexcept;
using StagedResourceReader=bool (*)(void* filesystem,const char* resource,
                                   std::string& text,
                                   std::string& error) noexcept;
struct Hooks {
    void* context=nullptr;
    stats_storage::DefinitionFactory make_definition=nullptr;
    stats_storage::AllowAllocation allow_definition=nullptr;
    stats_catalog::AllowAllocation allow_catalog=nullptr;
    stats_catalog::ContinueResourceLoad load_catalog_resources=nullptr;
    stats_storage::RemainingStep run_unreconstructed_step=nullptr;
    // Optional content root; disabled by default for prior ABI fixtures.
    // This is a standard C++17 filesystem path, NOT engine IFileSystem.
    const std::filesystem::path* game_content_root=nullptr;
    // Alternative to game_content_root. The engine-specific adapter only
    // supplies bytes; this coordinator owns parsing, rollback and ordering.
    // Exactly one asset input source can be selected per Init attempt.
    AssetReader read_engine_documents=nullptr;
    // The filesystem interface belongs to the engine, not the game stats
    // resource-provider host. Never reinterpret hooks.context as IFileSystem.
    void* asset_reader_context=nullptr;
    // Source-authenticated third/fourth stage version: the definitions
    // document is fetched/bound during catalog Init, and the layout
    // document fetched during the next stage.
    StagedResourceReader read_stats_resource=nullptr;
    void* stats_resource_context=nullptr;
};
enum class Outcome {
    Success,
    InvalidHost,
    ConflictingAssetSources,
    ConflictingResourceLoaders,
    MissingAssetReaderContext,
    MissingResourceLoader,
    ResourceLoaderRefused,
    AssetLoadRefused,
    MissingLaterStep,
    LaterStepRefused
};
struct Result {
    Outcome outcome=Outcome::InvalidHost;
    std::size_t completed=0; // number of original 7 Init callsites
    std::size_t attempted=0; // includes a refusing action
    bool succeeded() const noexcept {return outcome==Outcome::Success;}
};
inline const char* OutcomeName(Outcome result) noexcept {
    switch(result) {
        case Outcome::Success:return "success";
        case Outcome::InvalidHost:return "invalid-host";
        case Outcome::ConflictingAssetSources:return "conflicting-asset-sources";
        case Outcome::ConflictingResourceLoaders:return "conflicting-resource-loaders";
        case Outcome::MissingAssetReaderContext:return "missing-asset-reader-context";
        case Outcome::MissingResourceLoader:return "missing-resource-loader";
        case Outcome::ResourceLoaderRefused:return "resource-loader-refused";
        case Outcome::AssetLoadRefused:return "asset-load-refused";
        case Outcome::MissingLaterStep:return "missing-later-step";
        case Outcome::LaterStepRefused:return "later-step-refused";
    }
    return "unknown";
}
// Original C_StatsSystem::Init unconditionally returns true. This
// independent coordinator instead fails closed when an unresolved
// dependency is unavailable or explicitly refuses to run. A refusal
// retains earlier completed stages and never rolls back partial work;
// retry starts at step zero and resets all three owned collections.
inline Result Initialize(State& state,void* system,void* registry,
                         Hooks hooks) noexcept {
    Result result{};
    if(!system || !registry || !hooks.context) {
        result.outcome=Outcome::InvalidHost;
        return result;
    }
    if(hooks.game_content_root && hooks.read_engine_documents) {
        // Reject ambiguous IO sources before *any* catalog/storage writes.
        result.outcome=Outcome::ConflictingAssetSources;
        return result;
    }
    if(hooks.read_engine_documents && !hooks.asset_reader_context) {
        result.outcome=Outcome::MissingAssetReaderContext;
        return result;
    }
    if(hooks.read_stats_resource &&
       (hooks.game_content_root || hooks.read_engine_documents)) {
        result.outcome=Outcome::ConflictingAssetSources;
        return result;
    }
    if(hooks.read_stats_resource && hooks.load_catalog_resources) {
        result.outcome=Outcome::ConflictingResourceLoaders;
        return result;
    }
    if(hooks.read_stats_resource && !hooks.stats_resource_context) {
        result.outcome=Outcome::MissingAssetReaderContext;
        return result;
    }
    // Per-attempt transaction: the original third stage consumes
    // definitions, whereas the fourth stage consumes display layout.
    // Preserve published Documents until the fourth stage validates.
    std::string stagedDefinitions;
    for(const auto& step:stats_system::kInitSteps) {
        ++result.attempted;
        switch(step.kind) {
            case stats_system::StepKind::InitializeStaticData:
                state.definitions.ResetStatic(
                    hooks.context,hooks.make_definition,
                    hooks.allow_definition);
                break;
            case stats_system::StepKind::InitializeDynamicData:
                state.definitions.ResetDynamic(
                    hooks.context,hooks.allow_definition);
                break;
            case stats_system::StepKind::InitializeOwner:
                if(hooks.read_stats_resource) {
                    // Original 0x241FC5A0 fills 66 owner records, reads
                    // scripts/stats/stats_definitions.txt via KeyValues,
                    // and binds names to compiled IDs. The host instead
                    // validates independent text without constructing
                    // unknown original engine KeyValues objects.
                    state.catalog.Rebuild(hooks.context,hooks.allow_catalog);
                    bool valid=false;
                    try {
                        std::vector<stats_scripts::Definition> parsed;
                        valid=hooks.read_stats_resource(
                                  hooks.stats_resource_context,
                                  "scripts/stats/stats_definitions.txt",
                                  stagedDefinitions,state.last_script_error) &&
                              stats_scripts::ParseDefinitions(
                                  stagedDefinitions,parsed,state.last_script_error) &&
                              stats_binding::Bind(
                                  state.catalog,state.catalog_text,parsed,
                                  state.last_script_error);
                    } catch(...) {
                        state.last_script_error.clear();
                    }
                    if(!valid) {
                        result.outcome=Outcome::ResourceLoaderRefused;
                        return result;
                    }
                } else {
                    if(!hooks.load_catalog_resources) {
                        result.outcome=Outcome::MissingResourceLoader;
                        return result;
                    }
                    if(!stats_catalog::BeginOwnerSetup(
                            state.catalog,hooks.context,
                            hooks.load_catalog_resources,hooks.allow_catalog)) {
                        result.outcome=Outcome::ResourceLoaderRefused;
                        return result;
                    }
                }
                break;
            case stats_system::StepKind::LoadDisplayLayout:
                // The recovered on-disk parser validates both original
                // source files here. The remaining 2006 layout helper
                // and engine-side hookup still require host callbacks.
                if(hooks.read_stats_resource) {
                    bool loaded=false;
                    try {
                        std::string layout;
                        loaded=hooks.read_stats_resource(
                                   hooks.stats_resource_context,
                                   "scripts/stats/stats_display_layout.txt",
                                   layout,state.last_script_error) &&
                               state.scripts.LoadTexts(
                                   stagedDefinitions,layout,
                                   state.last_script_error);
                    } catch(...) {
                        state.last_script_error.clear();
                    }
                    if(!loaded) {
                        result.outcome=Outcome::AssetLoadRefused;
                        return result;
                    }
                } else if(hooks.game_content_root || hooks.read_engine_documents) {
                    bool loaded=false;
                    try {
                        if(hooks.read_engine_documents) {
                            std::string definitions,layout;
                            // The reader cannot mutate State::scripts.
                            // Parse into temporary vectors inside LoadTexts,
                            // then publish only when BOTH inputs are valid.
                            loaded=hooks.read_engine_documents(
                                hooks.asset_reader_context,definitions,layout,
                                state.last_script_error) &&
                                state.scripts.LoadTexts(
                                    definitions,layout,state.last_script_error);
                        } else {
                            loaded=state.scripts.LoadContentRoot(
                                *hooks.game_content_root,state.last_script_error);
                        }
                    } catch(...) {
                        // Filesystem, parser, or allocation exceptions must
                        // produce a refusal, never unwind through noexcept Init.
                        state.last_script_error.clear();
                    }
                    if(!loaded) {
                        result.outcome=Outcome::AssetLoadRefused;
                        return result;
                    }
                }
                if(!hooks.run_unreconstructed_step) {
                    result.outcome=Outcome::MissingLaterStep;
                    return result;
                }
                if(!hooks.run_unreconstructed_step(system,step)) {
                    result.outcome=Outcome::LaterStepRefused;
                    return result;
                }
                break;
            case stats_system::StepKind::RegisterCallback:
                if(!hooks.run_unreconstructed_step) {
                    result.outcome=Outcome::MissingLaterStep;
                    return result;
                }
                if(!hooks.run_unreconstructed_step(
                    step.kind==stats_system::StepKind::RegisterCallback?
                        registry:system,step)) {
                    result.outcome=Outcome::LaterStepRefused;
                    return result;
                }
                break;
        }
        ++result.completed;
    }
    result.outcome=Outcome::Success;
    return result;
}
} // namespace sin_independent::stats_coordinator
