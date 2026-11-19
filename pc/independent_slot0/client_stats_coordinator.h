#pragma once
// Clean-room Windows client C_StatsSystem Init coordinator.
// This orders three *partial* original-guided owned helpers and exposes
// the still-missing proprietary resource/layout/registration operations.
// DO NOT pass original retail client.dll objects or function addresses.
#include <cstddef>
#include "client_stats_storage_init.h"
#include "client_stats_catalog_init.h"

namespace sin_independent::stats_coordinator {
struct State final {
    stats_storage::Storage definitions;
    stats_catalog::Catalog catalog;
    State()=default;
    State(const State&)=delete;
    State& operator=(const State&)=delete;
    State(State&&)=delete;
    State& operator=(State&&)=delete;
};
struct Hooks {
    void* context=nullptr;
    stats_storage::DefinitionFactory make_definition=nullptr;
    stats_storage::AllowAllocation allow_definition=nullptr;
    stats_catalog::AllowAllocation allow_catalog=nullptr;
    stats_catalog::ContinueResourceLoad load_catalog_resources=nullptr;
    stats_storage::RemainingStep run_unreconstructed_step=nullptr;
};
enum class Outcome {
    Success,
    InvalidHost,
    MissingResourceLoader,
    ResourceLoaderRefused,
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
        case Outcome::MissingResourceLoader:return "missing-resource-loader";
        case Outcome::ResourceLoaderRefused:return "resource-loader-refused";
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
                break;
            case stats_system::StepKind::LoadDisplayLayout:
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
