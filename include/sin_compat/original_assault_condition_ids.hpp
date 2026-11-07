#pragma once

// Source-verified names + original condition IDs from the owner's genuine
// 2006 SiN server.dll: original x86 registration helper at 0x22073760.
// Cross-checked against native CAssaultBehavior / ai_goal_assault call sites.
// These numeric IDs are historical metadata, NEVER new SourceVR engine IDs.
// The actual target must register and resolve its own condition namespace.

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace sin_compat {

struct LegacyAssaultConditionId {
    explicit constexpr LegacyAssaultConditionId(std::uint32_t id) : value(id) {}
    std::uint32_t value;
};

enum class OriginalAssaultCondition : std::uint8_t {
    NoPrimaryAmmo,
    SeeEnemy,
    CanRangeAttack1,
    Provoked,
    NewEnemy,
    HearDanger,
    HearCombat,
    HearBulletImpact,
    ReceivedOrders,
};

struct OriginalAssaultConditionWitness {
    std::string_view symbol;
    LegacyAssaultConditionId original_x86_id;
    OriginalAssaultCondition semantic;
};

inline constexpr std::array<OriginalAssaultConditionWitness, 9>
original_assault_condition_ids{{
    {"COND_NO_PRIMARY_AMMO", LegacyAssaultConditionId{0x04}, OriginalAssaultCondition::NoPrimaryAmmo},
    {"COND_SEE_ENEMY", LegacyAssaultConditionId{0x0A}, OriginalAssaultCondition::SeeEnemy},
    {"COND_CAN_RANGE_ATTACK1", LegacyAssaultConditionId{0x14}, OriginalAssaultCondition::CanRangeAttack1},
    {"COND_PROVOKED", LegacyAssaultConditionId{0x18}, OriginalAssaultCondition::Provoked},
    {"COND_NEW_ENEMY", LegacyAssaultConditionId{0x19}, OriginalAssaultCondition::NewEnemy},
    {"COND_HEAR_DANGER", LegacyAssaultConditionId{0x30}, OriginalAssaultCondition::HearDanger},
    {"COND_HEAR_COMBAT", LegacyAssaultConditionId{0x33}, OriginalAssaultCondition::HearCombat},
    {"COND_HEAR_BULLET_IMPACT", LegacyAssaultConditionId{0x36}, OriginalAssaultCondition::HearBulletImpact},
    {"COND_RECEIVED_ORDERS", LegacyAssaultConditionId{0x3B}, OriginalAssaultCondition::ReceivedOrders},
}};

inline constexpr LegacyAssaultConditionId original_assault_condition_id(
    OriginalAssaultCondition semantic) {
    for (const auto& witness : original_assault_condition_ids)
        if (witness.semantic == semantic) return witness.original_x86_id;
    // Invalid enum never becomes an existing original ID: 0xFFFFFFFF
    // is intentionally not a legal Source condition from this table.
    return LegacyAssaultConditionId{0xFFFFFFFFu};
}

inline constexpr std::optional<OriginalAssaultCondition>
decode_original_assault_condition(LegacyAssaultConditionId original) {
    for (const auto& witness : original_assault_condition_ids)
        if (witness.original_x86_id.value == original.value)
            return witness.semantic;
    return std::nullopt;
}

inline constexpr std::string_view original_assault_condition_name(
    OriginalAssaultCondition semantic) {
    for (const auto& witness : original_assault_condition_ids)
        if (witness.semantic == semantic) return witness.symbol;
    return {};
}

} // namespace sin_compat
