#pragma once
// Independent host-neutral model of two original interpolation CALL SITES.
// Does not reconstruct all other original difficulty initialization effects.
#include "sin_compat/dynamic_difficulty_gpv.hpp"
namespace sin_compat {
class IGameplayDifficultySettingSource {
public:
    virtual ~IGameplayDifficultySettingSource() = default;
    // Engine host supplies the registered difficulty-setting ConVar value.
    virtual bool read_setting(float& value) const = 0;
};
enum class GameplayActivationStatus {
    Applied, AlreadyInitialized, MissingSetting, InvalidSetting,
    InvalidModel, InterpolationFailed
};
class GameplayDifficultyActivation final {
public:
    GameplayDifficultyActivation(GameplayDifficultyVariables& modifiers,
                                 IGameplayDifficultySettingSource& settings)
        : modifiers_(modifiers), settings_(settings) {}
    // 0x222BA360: only tracks +8 setting, +0x264 flag and interpolation.
    GameplayActivationStatus initialize_from_setting();
    // 0x222B9130: when +0x264 is 0, use cached +8,
    // set +0x0c to 0.5f, interpolate, set +0x264 to 1.
    GameplayActivationStatus restore_if_uninitialized();
    // Host syncs previously loaded original entity state; not a Save/Restore API.
    bool seed_cached_state(float cached_setting, bool initialized) noexcept;
    bool initialized() const noexcept { return initialized_; }
    float cached_setting() const noexcept { return cached_setting_; }
    float restore_half_marker() const noexcept { return restore_half_marker_; }
private:
    GameplayDifficultyVariables& modifiers_;
    IGameplayDifficultySettingSource& settings_;
    float cached_setting_ = 0.0f;
    float restore_half_marker_ = 0.0f;
    bool initialized_ = false;
};
} // namespace sin_compat
