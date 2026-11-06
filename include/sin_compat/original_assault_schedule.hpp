#pragma once

// Semantic, engine-neutral assault-schedule identifiers recovered from the
// original 2006 SiN server.dll. No SourceVR entity/AI schedule is registered.
// The legacy 0x186A0..0x186A8 values are for static decompilation evidence
// only. In a future ARM64 Source-compatible build, register fresh native IDs.

#include <array>
#include <cstdint>
#include <optional>

namespace sin_compat {

// Never use raw legacy numeric IDs as native schedule IDs.
enum class AssaultSchedule : std::uint8_t {
    MoveToRallyPoint,
    AssaultFailedToMove,
    FailMoveToRallyPoint,
    MoveToAssaultPoint,
    HoldRallyPoint,
    HoldAssaultPoint,
    WaitAndClear,
    AssaultMoveAway,
    ClearAssaultPoint,
};

struct LegacyAssaultScheduleId {
    explicit constexpr LegacyAssaultScheduleId(std::uint32_t original) : value(original) {}
    std::uint32_t value;
};

struct LegacyAssaultTaskId {
    explicit constexpr LegacyAssaultTaskId(std::uint32_t original) : value(original) {}
    std::uint32_t value;
};

// The original task registry may reuse a numeric ID from schedule registry.
// Separate types make accidental cross-registry lookups a compile error.
struct AssaultScheduleWitness {
    AssaultSchedule semantic;
    LegacyAssaultScheduleId original_x86_id;
    const char* original_symbol_name;
};

inline constexpr std::array<AssaultScheduleWitness, 9> original_assault_schedules{{
    {AssaultSchedule::MoveToRallyPoint, LegacyAssaultScheduleId{0x186A0}, "SCHED_MOVE_TO_RALLY_POINT"},
    {AssaultSchedule::AssaultFailedToMove, LegacyAssaultScheduleId{0x186A1}, "SCHED_ASSAULT_FAILED_TO_MOVE"},
    {AssaultSchedule::FailMoveToRallyPoint, LegacyAssaultScheduleId{0x186A2}, "SCHED_FAIL_MOVE_TO_RALLY_POINT"},
    {AssaultSchedule::MoveToAssaultPoint, LegacyAssaultScheduleId{0x186A3}, "SCHED_MOVE_TO_ASSAULT_POINT"},
    {AssaultSchedule::HoldRallyPoint, LegacyAssaultScheduleId{0x186A4}, "SCHED_HOLD_RALLY_POINT"},
    {AssaultSchedule::HoldAssaultPoint, LegacyAssaultScheduleId{0x186A5}, "SCHED_HOLD_ASSAULT_POINT"},
    {AssaultSchedule::WaitAndClear, LegacyAssaultScheduleId{0x186A6}, "SCHED_WAIT_AND_CLEAR"},
    {AssaultSchedule::AssaultMoveAway, LegacyAssaultScheduleId{0x186A7}, "SCHED_ASSAULT_MOVE_AWAY"},
    {AssaultSchedule::ClearAssaultPoint, LegacyAssaultScheduleId{0x186A8}, "SCHED_CLEAR_ASSAULT_POINT"},
}};

inline constexpr std::optional<AssaultSchedule> decode_original_assault_schedule(
    LegacyAssaultScheduleId original) {
    for (const auto& witness : original_assault_schedules) {
        if (witness.original_x86_id.value == original.value)
            return witness.semantic;
    }
    return std::nullopt;
}

inline constexpr const char* original_assault_schedule_name(AssaultSchedule schedule) {
    for (const auto& witness : original_assault_schedules) {
        if (witness.semantic == schedule) return witness.original_symbol_name;
    }
    return nullptr;
}

} // namespace sin_compat
