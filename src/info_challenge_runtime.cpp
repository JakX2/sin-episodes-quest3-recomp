#include "sin_compat/info_challenge_runtime.hpp"
#include <cmath>
namespace sin_compat {
bool InfoChallengeRuntime::register_entity(EntityRef e, float baseline) {
    if (!host_.is_live(e) || !std::isfinite(baseline)) return false;
    return baselines_.emplace(e,baseline).second;
}
void InfoChallengeRuntime::remove_entity(EntityRef e) { baselines_.erase(e); }
InfoChallengeOutput InfoChallengeRuntime::handle_input(EntityRef e,
                                                      const std::string& input) const {
    const auto it = baselines_.find(e);
    if (it == baselines_.end() || !host_.is_live(e))
        return {InfoChallengeQueryStatus::InvalidEntity,nullptr,0};
    const char* output = nullptr;
    float raw=0.0f;
    bool ok=false;
    if (input == "GetInitialChallenge") {
        output="OnInitialChallenge";
        ok=host_.initial_challenge(raw);
    } else if (input == "GetCurrentChallenge" || input == "GetChallengeDifference") {
        output=input == "GetCurrentChallenge" ? "OnCurrentChallenge" : "OnChallengeDifference";
        ok=host_.current_challenge(raw);
    } else return {InfoChallengeQueryStatus::UnknownInput,nullptr,0};
    if (!ok) return {InfoChallengeQueryStatus::ProviderUnavailable,nullptr,0};
    if (!std::isfinite(raw)) return {InfoChallengeQueryStatus::InvalidValue,nullptr,0};
    if (input == "GetChallengeDifference") {
        // Original x87 fsub [this+0x3dc]; fmul [100.0f].
        raw=(raw-it->second)*100.0f;
        if (!std::isfinite(raw)) return {InfoChallengeQueryStatus::InvalidValue,nullptr,0};
    }
    return {InfoChallengeQueryStatus::Emitted,output,raw};
}
} // namespace sin_compat
