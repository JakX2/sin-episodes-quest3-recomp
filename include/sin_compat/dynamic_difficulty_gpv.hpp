#pragma once
// Portable, independently authored reconstruction of two pinned 2006 x86
// routines: GPV interpolation (0x222D3490) and rating (0x222D3730).
// Original table data must be supplied privately by an authorized game host.
#include <cstddef>
#include <map>
#include <string>
#include <vector>
namespace sin_compat {
struct GameplayVariableDefinition {
    std::string name;
    float current = 0.0f;    // Original table +0x00, mutable
    float low_target = 0.0f; // +0x04: interpolation endpoint
    float high_target = 0.0f;// +0x08: interpolation endpoint
    float rating_low = 0.0f; // +0x14: normalization start
    float rating_high = 1.0f;// +0x18: end, may be lower than start
};
enum class GameplayDifficultyStatus {
    Ok, InvalidTable, InvalidSlider, NonFiniteCalculation
};
struct GameplayChallengeResult {
    GameplayDifficultyStatus status = GameplayDifficultyStatus::InvalidTable;
    float rating_percent = 0.0f;
    std::size_t entries_visited = 0;
    std::size_t recognized_entries = 0;
    explicit operator bool() const noexcept {
        return status == GameplayDifficultyStatus::Ok;
    }
};
class GameplayDifficultyVariables final {
public:
    // Original array has exactly 14 entries. Smaller self-authored test
    // fixtures are permitted; the host supplies and owns original values.
    bool configure(std::vector<GameplayVariableDefinition> definitions,
                   std::string* error = nullptr);
    // 0x222D3490: current=(1-slider)*low_target+slider*high_target.
    // The original instruction sequence does not clamp slider.
    GameplayDifficultyStatus interpolate(float slider) noexcept;
    // 0x222D3730: sum normalized, clamp [0,1], divide by number of GPV
    // references (INCLUDING unknown entries), multiply by 100.
    GameplayChallengeResult current_challenge(
        const std::vector<std::string>& average_stat_names) const noexcept;
    const GameplayVariableDefinition* find(const std::string& name) const noexcept;
    std::size_t count() const noexcept {return rows_.size();}
private:
    std::vector<GameplayVariableDefinition> rows_;
    std::map<std::string,std::size_t> ids_;
};
} // namespace sin_compat
