#include "sin_compat/dynamic_difficulty_activation.hpp"
#include <cmath>
namespace sin_compat {
namespace {
GameplayActivationStatus convert(GameplayDifficultyStatus result) {
    switch (result) {
    case GameplayDifficultyStatus::Ok: return GameplayActivationStatus::Applied;
    case GameplayDifficultyStatus::InvalidTable: return GameplayActivationStatus::InvalidModel;
    case GameplayDifficultyStatus::InvalidSlider: return GameplayActivationStatus::InvalidSetting;
    case GameplayDifficultyStatus::NonFiniteCalculation: return GameplayActivationStatus::InterpolationFailed;
    }
    return GameplayActivationStatus::InterpolationFailed;
}
}
bool GameplayDifficultyActivation::seed_cached_state(float cached, bool is_initialized) noexcept {
    if (!std::isfinite(cached)) return false;
    cached_setting_ = cached;
    initialized_ = is_initialized;
    return true;
}
GameplayActivationStatus GameplayDifficultyActivation::initialize_from_setting() {
    float value = 0.0f;
    if (!settings_.read_setting(value)) return GameplayActivationStatus::MissingSetting;
    if (!std::isfinite(value)) return GameplayActivationStatus::InvalidSetting;
    const auto result = convert(modifiers_.interpolate(value));
    if (result != GameplayActivationStatus::Applied) return result;
    cached_setting_ = value;
    initialized_ = true;
    return result;
}
GameplayActivationStatus GameplayDifficultyActivation::restore_if_uninitialized() {
    if (initialized_) return GameplayActivationStatus::AlreadyInitialized;
    const auto result = convert(modifiers_.interpolate(cached_setting_));
    if (result != GameplayActivationStatus::Applied) return result;
    restore_half_marker_ = 0.5f;
    initialized_ = true;
    return result;
}
} // namespace sin_compat
