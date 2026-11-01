#pragma once
// Independent C++17 reconstruction of original 2006 difficulty category
// factor table 0x22521DE0: six rows of current/saved low and high factors.
#include <array>
#include <cstddef>
namespace sin_compat {
struct DifficultyBandPair {
    float low_current = 1.0f;
    float low_saved = 1.0f;
    float high_current = 1.0f;
    float high_saved = 1.0f;
};
enum class DifficultyBandStatus {
    Applied, NotConfigured, InvalidCategory, InvalidFactor, InvalidDelta
};
class DifficultyCategoryBands final {
public:
    static constexpr std::size_t kCategories = 6;
    static constexpr float kMinimum = 0.05f;
    static constexpr float kMaximum = 2.0f;
    // Independent host setup, not original savegame parsing.
    bool configure(const std::array<DifficultyBandPair,kCategories>& factors) noexcept;
    bool configured() const noexcept { return configured_; }
    const DifficultyBandPair* category(std::size_t index) const noexcept;
    // Original 0x222C07A0: parameter <2 selects low, >=2 selects high.
    bool get(std::size_t index, int selector, float& result) const noexcept;
    // Original 0x222C0810: clamp absolute factor to [0.05,2.0].
    DifficultyBandStatus set(std::size_t index, int selector, float value) noexcept;
    // Original 0x222C07C0: add delta to current factor and clamp.
    DifficultyBandStatus add(std::size_t index, int selector, float delta) noexcept;
    // Original 0x222C0860: copy saved low/high to current without clamping.
    DifficultyBandStatus reset_from_saved() noexcept;
private:
    std::array<DifficultyBandPair,kCategories> factors_{};
    bool configured_=false;
};
} // namespace sin_compat
