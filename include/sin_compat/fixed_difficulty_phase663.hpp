#pragma once
// Phase663: independent, opt-in fixed-difficulty policy for reconstructed
// SiN Episodes gameplay. The original 2006 metric names, initialization
// values and consumer functions are retained; the Easy/Normal/Hard tuning
// values are NEW port policy, not claimed to be Ritual's original presets.
// Does not modify original retail DLLs, maps or original trigger entities.
#include "difficulty_metric_consumers.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::difficulty::fixed_phase663 {
inline constexpr std::string_view kEngineSkillConVar="skill";
inline constexpr int kEasySkill=1;
inline constexpr int kNormalSkill=2;
inline constexpr int kHardSkill=3;

enum class Level : std::uint8_t { Easy=1, Normal=2, Hard=3 };
enum class Selection : std::uint8_t { Applied, Unchanged, InvalidSkill };

struct Profile {
    Level level;
    std::string_view label;
    RuntimeMetrics metrics;
};

// The original 14 positional metric names are retained verbatim. Current
// NORMAL values are their exact initial f32 values at original server
// 0x22523478+i*28, rather than guessed (or rounded) midpoint expressions.
// Easy and Hard deliberately use INDIVIDUAL values, not one dd_challenge t;
// otherwise harder enemy damage would perversely reduce accuracy.
Profile make_profile(Level level) noexcept;
bool valid_skill(int raw) noexcept;
Level level_for_skill(int raw) noexcept;
const char* profile_name(Level level) noexcept;

struct TriggerObservation {
    std::int32_t estimated_difficulty=0; // original EstimatedDifficulty key
    float estimated_time=0.f;             // original EstimatedTime key
    std::int32_t end_of_game=0;           // original EndOfGame integer
};

class FixedDifficultyRuntime final {
public:
    FixedDifficultyRuntime() noexcept;
    // Use console/menu skill 1/2/3. A rejected value changes no state.
    Selection set_skill(int new_skill) noexcept;
    int skill() const noexcept {return static_cast<int>(level_);}
    Level level() const noexcept {return level_;}
    std::string_view name() const noexcept {return profile_name(level_);}
    const RuntimeMetrics& metrics() const noexcept {return metrics_;}
    float metric_at(std::size_t index) const noexcept;
    // Unknown, differently cased and truncated original names are rejected.
    bool lookup(std::string_view original_name,float& out) const noexcept;
    std::uint32_t revision() const noexcept {return revision_;}
    std::uint32_t trigger_count() const noexcept {return trigger_count_;}
    bool playthrough_completed() const noexcept {return completed_;}

    // The map-trigger entity and its Source events/removal remain the
    // responsibility of original_sin_trigger_entities. This new observer
    // records completion with the original low-byte EndOfGame test but
    // explicitly NEVER mutates combat metrics or selected difficulty.
    void observe_dynamic_trigger(const TriggerObservation& event) noexcept;

    // Reapply a user-selected skill after the ORIGINAL manager's archive
    // has read 14 mutable metric floats. This deliberately ignores those
    // archived adaptive floats; a game host must call this after restore,
    // or saved adaptive values would silently override the fixed profile.
    Selection reapply_after_archive(int saved_skill) noexcept;

    // Performance/advisor reports are deliberately non-adaptive here.
    // Existing client-side DD messages may still require protocol handlers.
    void observe_performance_sample(float /*unused*/) const noexcept {}
private:
    Level level_=Level::Normal;
    RuntimeMetrics metrics_{};
    std::uint32_t revision_=0u;
    std::uint32_t trigger_count_=0u;
    bool completed_=false;
};
} // namespace sin_compat::difficulty::fixed_phase663
