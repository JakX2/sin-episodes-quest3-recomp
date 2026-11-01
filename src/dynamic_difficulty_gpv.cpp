#include "sin_compat/dynamic_difficulty_gpv.hpp"
#include <algorithm>
#include <cmath>
#include <utility>
namespace sin_compat {
namespace {
bool valid(const GameplayVariableDefinition& row) {
    return !row.name.empty() && row.name.size() < 256 &&
           std::isfinite(row.current) && std::isfinite(row.low_target) &&
           std::isfinite(row.high_target) && std::isfinite(row.rating_low) &&
           std::isfinite(row.rating_high) && row.rating_low != row.rating_high;
}
}
bool GameplayDifficultyVariables::configure(
    std::vector<GameplayVariableDefinition> definitions, std::string* error) {
    rows_.clear(); ids_.clear();
    std::string reason;
    if (definitions.empty() || definitions.size() > 14)
        reason = "expected between one and fourteen GPV rows";
    if (reason.empty()) {
        for (std::size_t i=0;i<definitions.size();++i) {
            if (!valid(definitions[i])) {reason="nonfinite, empty or zero-range GPV";break;}
            if (!ids_.emplace(definitions[i].name,i).second) {
                reason="duplicate GPV name";break;
            }
        }
    }
    if (!reason.empty()) {
        ids_.clear();
        if(error) *error=reason;
        return false;
    }
    rows_=std::move(definitions);
    if(error) error->clear();
    return true;
}
GameplayDifficultyStatus GameplayDifficultyVariables::interpolate(float slider) noexcept {
    if (rows_.empty()) return GameplayDifficultyStatus::InvalidTable;
    if (!std::isfinite(slider)) return GameplayDifficultyStatus::InvalidSlider;
    // Validate every proposed value before mutating any row.
    // double avoids overflowing intermediate interpolation products.
    std::vector<float> next; next.reserve(rows_.size());
    for (const auto& row:rows_) {
        const double value=(1.0-static_cast<double>(slider))*row.low_target+
                           static_cast<double>(slider)*row.high_target;
        if (!std::isfinite(value) || std::fabs(value) > 3.4028234663852886e38)
            return GameplayDifficultyStatus::NonFiniteCalculation;
        next.push_back(static_cast<float>(value));
    }
    for (std::size_t i=0;i<rows_.size();++i) rows_[i].current=next[i];
    return GameplayDifficultyStatus::Ok;
}
GameplayChallengeResult GameplayDifficultyVariables::current_challenge(
    const std::vector<std::string>& names) const noexcept {
    if (rows_.empty()) return {GameplayDifficultyStatus::InvalidTable,0,0,0};
    if (names.empty()) return {GameplayDifficultyStatus::Ok,0,0,0};
    double sum=0.0;
    std::size_t known=0;
    for (const auto& name:names) {
        const auto it=ids_.find(name);
        if(it==ids_.end()) continue; // Original invalid-name path adds zero.
        const auto& row=rows_[it->second];
        const double numerator=static_cast<double>(row.current)-row.rating_low;
        const double denominator=static_cast<double>(row.rating_high)-row.rating_low;
        if (!std::isfinite(numerator) || !std::isfinite(denominator) || denominator==0.0)
            return {GameplayDifficultyStatus::NonFiniteCalculation,0,names.size(),known};
        const double t=numerator/denominator;
        if(!std::isfinite(t))
            return {GameplayDifficultyStatus::NonFiniteCalculation,0,names.size(),known};
        sum+=std::clamp(t,0.0,1.0);
        ++known;
    }
    const double rating=sum/static_cast<double>(names.size())*100.0;
    if (!std::isfinite(rating))
        return {GameplayDifficultyStatus::NonFiniteCalculation,0,names.size(),known};
    return {GameplayDifficultyStatus::Ok,static_cast<float>(rating),names.size(),known};
}
const GameplayVariableDefinition* GameplayDifficultyVariables::find(
    const std::string& name) const noexcept {
    const auto it=ids_.find(name);
    return it==ids_.end()?nullptr:&rows_[it->second];
}
} // namespace sin_compat
