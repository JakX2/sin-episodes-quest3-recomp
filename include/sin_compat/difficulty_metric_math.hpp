#pragma once
// Original SiN Episodes 2006 PE32 data/code derived reference, not a Source ABI.
// Named metric records are static binary initializers, not savegame fields.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::difficulty {
inline constexpr std::size_t original_metric_count = 14;
inline constexpr std::size_t original_modifier_count = 10;
struct MetricInitializer {
    std::string_view name;
    // Seven consecutive float32 members, original server 0x22523478 + i*28.
    // 0 = current, 1/2 = interpolation inputs; 3..6 left unlabelled here.
    std::array<float, 7> fields;
};
inline constexpr std::array<MetricInitializer, original_metric_count> original_metrics{{
    {"DAMAGE_TAKEN_SCALE",              {1.075f, 0.75f, 1.4f, 0.5f, 1.5f, 0.f, 2.f}},
    {"DAMAGE_DONE_SCALE",               {1.f,   1.f,    1.f,   0.5f, 1.5f, 0.5f, 1.5f}},
    {"AMMO_QUANTITY_SCALE",             {1.f,   1.f,    1.f,   1.5f, 0.5f, 1.5f, 0.5f}},
    {"NUM_FORCED_HEALTH_KITS",          {0.f,   0.f,    0.f,   0.f, 3.402823466e38f, 0.f, 3.402823466e38f}},
    {"PERCENT_HEALTH_KITS_DROP",        {0.4f,  0.5f,  0.3f, 0.8f, 0.2f, 0.9f, 0.f}},
    {"PERCENT_NPC_2ND_FIRE",            {0.25f, 0.f,    0.5f, 0.f, 0.7f, 0.f, 1.f}},
    {"PERCENT_NPC_MAKE_DIFFERENT",      {0.15f,-0.1f,   0.4f,-0.5f, 0.5f,-0.75f,0.75f}},
    {"PERCENT_NPC_CHANGE_SPAWN_NUM",    {0.15f,-0.1f,   0.4f,-0.5f, 0.5f,-0.75f,0.75f}},
    {"PERCENT_NPC_WEARS_HELMET",        {0.35000002f, 0.1f,   0.6f, 0.f, 0.8f, 0.f, 1.f}},
    {"PERCENT_NPC_HELMET_SHOT_OFF",     {0.6f,  0.8f,   0.4f, 0.9f, 0.3f, 1.f, 0.2f}},
    {"PERCENT_NPC_TAKES_COVER",         {0.2f,  0.2f,   0.2f, 0.f, 1.f, 0.f, 1.f}},
    {"RANDOM_NPC_WAIT_BEFORE_SHOOT",    {0.f,   0.1f,  -0.1f, 0.25f,-0.15f,0.3f,-0.2f}},
    {"PERCENT_NPC_ACCURATE",            {0.75f, 1.f,    0.5f, 1.f, 0.3f, 1.f, 0.f}},
    {"PERCENT_NPC_MOBILE",              {0.5f,  0.3f,   0.7f, 0.2f, 0.8f, 0.f, 1.f}}
}};

// Recovered original strcmp-style name search at 0x222D3370.
inline int metric_index(std::string_view name) noexcept {
    for (std::size_t i=0; i<original_metrics.size(); ++i)
        if (original_metrics[i].name == name) return static_cast<int>(i);
    return -1;
}

// Original 0x222D3490 computes current=(1-t)*fields[1]+t*fields[2]
// for all 14 records. Unlike a guessed curve, this routine does not clamp t.
// C++ float arithmetic is a reference, not x87 bit-exact for all inputs.
inline float blended_metric(const MetricInitializer& m, float t) noexcept {
    return (1.0f-t)*m.fields[1]+t*m.fields[2];
}
inline std::array<float,original_metric_count> blended_metrics(float t) noexcept {
    std::array<float,original_metric_count> result{};
    for (std::size_t i=0;i<original_metric_count;++i)
        result[i]=blended_metric(original_metrics[i],t);
    return result;
}

// Original 0x222C07A0 chooses one of two float modifier cells:
// selector < 2 => +0, selector >= 2 => +8, 16 bytes per indexed entry.
// 0x222C07C0 adds a delta and clamps to [0.05f, 2.0f].
// 0x222C0810 assigns an absolute value with the same bounds.
inline std::size_t modifier_side(std::int32_t selector) noexcept {
    return selector < 2 ? 0u : 1u;
}
inline float clamp_modifier_finite(float value) noexcept {
    return value >= 2.0f ? 2.0f : value <= 0.05f ? 0.05f : value;
}
inline float update_modifier_finite(float current, float delta) noexcept {
    return clamp_modifier_finite(current+delta);
}
inline float set_modifier_finite(float value) noexcept {
    return clamp_modifier_finite(value);
}
} // namespace sin_compat::difficulty
