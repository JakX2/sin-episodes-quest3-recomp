#pragma once
// Read-only, engine-neutral reference to original 2006 SiN assault schedule
// task ordering and interrupt declarations. Independently cross-checked with
// server.dll script strings by audit_sin_assault_schedule_scripts.py.
// This table does NOT execute Source engine AI tasks or register schedules.

#include "sin_compat/original_assault_schedule.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace sin_compat {

struct AssaultTaskWitness {
    AssaultSchedule schedule;
    std::uint8_t position;
    std::string_view original_task_symbol;
    std::string_view original_parameter;
};

struct AssaultInterruptWitness {
    AssaultSchedule schedule;
    std::string_view original_condition_symbol;
};

inline constexpr std::array<AssaultTaskWitness, 29> original_assault_task_order{{
    {AssaultSchedule::AssaultMoveAway, 0, "TASK_MOVE_AWAY_PATH", "120"},
    {AssaultSchedule::AssaultMoveAway, 1, "TASK_RUN_PATH", "0"},
    {AssaultSchedule::AssaultMoveAway, 2, "TASK_WAIT_FOR_MOVEMENT", "0"},
    {AssaultSchedule::ClearAssaultPoint, 0, "TASK_ANNOUNCE_CLEAR", "0"},
    {AssaultSchedule::WaitAndClear, 0, "TASK_FACE_ASSAULT_POINT", "0"},
    {AssaultSchedule::WaitAndClear, 1, "TASK_SET_ACTIVITY", "ACTIVITY:ACT_IDLE"},
    {AssaultSchedule::WaitAndClear, 2, "TASK_AWAIT_ASSAULT_TIMEOUT", "0"},
    {AssaultSchedule::WaitAndClear, 3, "TASK_ANNOUNCE_CLEAR", "0"},
    {AssaultSchedule::MoveToAssaultPoint, 0, "TASK_SET_FAIL_SCHEDULE", "SCHEDULE:SCHED_ASSAULT_FAILED_TO_MOVE"},
    {AssaultSchedule::MoveToAssaultPoint, 1, "TASK_GATHER_CONDITIONS", "0"},
    {AssaultSchedule::MoveToAssaultPoint, 2, "TASK_GET_PATH_TO_ASSAULT_POINT", "0"},
    {AssaultSchedule::MoveToAssaultPoint, 3, "TASK_RUN_PATH", "0"},
    {AssaultSchedule::MoveToAssaultPoint, 4, "TASK_WAIT_FOR_MOVEMENT", "0"},
    {AssaultSchedule::MoveToAssaultPoint, 5, "TASK_FACE_ASSAULT_POINT", "0"},
    {AssaultSchedule::MoveToAssaultPoint, 6, "TASK_HIT_ASSAULT_POINT", "0"},
    {AssaultSchedule::HoldAssaultPoint, 0, "TASK_SET_ACTIVITY", "ACTIVITY:ACT_IDLE"},
    {AssaultSchedule::HoldAssaultPoint, 1, "TASK_WAIT", "3"},
    {AssaultSchedule::HoldRallyPoint, 0, "TASK_AWAIT_CUE", "0"},
    {AssaultSchedule::HoldRallyPoint, 1, "TASK_WAIT_ASSAULT_DELAY", "0"},
    {AssaultSchedule::FailMoveToRallyPoint, 0, "TASK_WAIT", "1"},
    {AssaultSchedule::AssaultFailedToMove, 0, "TASK_ASSAULT_DEFER_SCHEDULE_SELECTION", "1"},
    {AssaultSchedule::MoveToRallyPoint, 0, "TASK_SET_FAIL_SCHEDULE", "SCHEDULE:SCHED_ASSAULT_FAILED_TO_MOVE"},
    {AssaultSchedule::MoveToRallyPoint, 1, "TASK_GET_PATH_TO_RALLY_POINT", "0"},
    {AssaultSchedule::MoveToRallyPoint, 2, "TASK_RUN_PATH", "0"},
    {AssaultSchedule::MoveToRallyPoint, 3, "TASK_WAIT_FOR_MOVEMENT", "0"},
    {AssaultSchedule::MoveToRallyPoint, 4, "TASK_STOP_MOVING", "0"},
    {AssaultSchedule::MoveToRallyPoint, 5, "TASK_FACE_RALLY_POINT", "0"},
    {AssaultSchedule::MoveToRallyPoint, 6, "TASK_HIT_RALLY_POINT", "0"},
    {AssaultSchedule::MoveToRallyPoint, 7, "TASK_SET_SCHEDULE", "SCHEDULE:SCHED_HOLD_RALLY_POINT"},
}};

inline constexpr std::array<AssaultInterruptWitness, 35> original_assault_interrupts{{
    {AssaultSchedule::WaitAndClear, "COND_NEW_ENEMY"},
    {AssaultSchedule::WaitAndClear, "COND_CAN_RANGE_ATTACK1"},
    {AssaultSchedule::WaitAndClear, "COND_CAN_MELEE_ATTACK1"},
    {AssaultSchedule::WaitAndClear, "COND_CAN_RANGE_ATTACK2"},
    {AssaultSchedule::WaitAndClear, "COND_CAN_MELEE_ATTACK2"},
    {AssaultSchedule::WaitAndClear, "COND_HEAR_DANGER"},
    {AssaultSchedule::WaitAndClear, "COND_HEAR_BULLET_IMPACT"},
    {AssaultSchedule::MoveToAssaultPoint, "COND_NO_PRIMARY_AMMO"},
    {AssaultSchedule::MoveToAssaultPoint, "COND_HEAR_DANGER"},
    {AssaultSchedule::HoldAssaultPoint, "COND_NEW_ENEMY"},
    {AssaultSchedule::HoldAssaultPoint, "COND_ENEMY_DEAD"},
    {AssaultSchedule::HoldAssaultPoint, "COND_CAN_RANGE_ATTACK1"},
    {AssaultSchedule::HoldAssaultPoint, "COND_CAN_MELEE_ATTACK1"},
    {AssaultSchedule::HoldAssaultPoint, "COND_CAN_RANGE_ATTACK2"},
    {AssaultSchedule::HoldAssaultPoint, "COND_CAN_MELEE_ATTACK2"},
    {AssaultSchedule::HoldAssaultPoint, "COND_TOO_CLOSE_TO_ATTACK"},
    {AssaultSchedule::HoldAssaultPoint, "COND_LOST_ENEMY"},
    {AssaultSchedule::HoldAssaultPoint, "COND_HEAR_DANGER"},
    {AssaultSchedule::HoldAssaultPoint, "COND_HEAR_BULLET_IMPACT"},
    {AssaultSchedule::HoldAssaultPoint, "COND_NO_PRIMARY_AMMO"},
    {AssaultSchedule::HoldRallyPoint, "COND_NEW_ENEMY"},
    {AssaultSchedule::HoldRallyPoint, "COND_CAN_RANGE_ATTACK1"},
    {AssaultSchedule::HoldRallyPoint, "COND_CAN_MELEE_ATTACK1"},
    {AssaultSchedule::HoldRallyPoint, "COND_LIGHT_DAMAGE"},
    {AssaultSchedule::HoldRallyPoint, "COND_HEAVY_DAMAGE"},
    {AssaultSchedule::HoldRallyPoint, "COND_PLAYER_PUSHING"},
    {AssaultSchedule::HoldRallyPoint, "COND_HEAR_DANGER"},
    {AssaultSchedule::HoldRallyPoint, "COND_HEAR_BULLET_IMPACT"},
    {AssaultSchedule::HoldRallyPoint, "COND_NO_PRIMARY_AMMO"},
    {AssaultSchedule::FailMoveToRallyPoint, "COND_HEAR_DANGER"},
    {AssaultSchedule::FailMoveToRallyPoint, "COND_CAN_RANGE_ATTACK1"},
    {AssaultSchedule::FailMoveToRallyPoint, "COND_CAN_MELEE_ATTACK1"},
    {AssaultSchedule::MoveToRallyPoint, "COND_HEAR_DANGER"},
    {AssaultSchedule::MoveToRallyPoint, "COND_PROVOKED"},
    {AssaultSchedule::MoveToRallyPoint, "COND_NO_PRIMARY_AMMO"},
}};

constexpr std::size_t original_task_count_for(AssaultSchedule id) {
    std::size_t found = 0;
    for (const auto& task : original_assault_task_order)
        if (task.schedule == id) ++found;
    return found;
}

constexpr std::size_t original_interrupt_count_for(AssaultSchedule id) {
    std::size_t found = 0;
    for (const auto& interrupt : original_assault_interrupts)
        if (interrupt.schedule == id) ++found;
    return found;
}

} // namespace sin_compat
