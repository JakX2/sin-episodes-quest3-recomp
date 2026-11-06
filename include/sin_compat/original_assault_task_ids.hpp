#pragma once

// 11 task-symbol IDs recovered from original 2006 SiN server.dll's explicit
// registration calls to 0x2208C590. This is original Windows x86 metadata.
// TASK and SCHED IDs live in different registries and may have the same value.
// Do not use these 2006 integer IDs as native SourceVR/ARM64 runtime IDs.

#include "sin_compat/original_assault_schedule.hpp"

#include <array>
#include <optional>
#include <string_view>

namespace sin_compat {

struct OriginalAssaultTaskIdWitness {
    std::string_view name;
    LegacyAssaultTaskId original_task_id;
};

inline constexpr std::array<OriginalAssaultTaskIdWitness, 11> original_assault_task_ids{{
    {"TASK_GET_PATH_TO_RALLY_POINT", LegacyAssaultTaskId{0x186A0}},
    {"TASK_FACE_RALLY_POINT", LegacyAssaultTaskId{0x186A1}},
    {"TASK_GET_PATH_TO_ASSAULT_POINT", LegacyAssaultTaskId{0x186A2}},
    {"TASK_FACE_ASSAULT_POINT", LegacyAssaultTaskId{0x186A3}},
    {"TASK_HIT_ASSAULT_POINT", LegacyAssaultTaskId{0x186A4}},
    {"TASK_HIT_RALLY_POINT", LegacyAssaultTaskId{0x186A5}},
    {"TASK_AWAIT_CUE", LegacyAssaultTaskId{0x186A6}},
    {"TASK_AWAIT_ASSAULT_TIMEOUT", LegacyAssaultTaskId{0x186A7}},
    {"TASK_ANNOUNCE_CLEAR", LegacyAssaultTaskId{0x186A8}},
    {"TASK_WAIT_ASSAULT_DELAY", LegacyAssaultTaskId{0x186A9}},
    {"TASK_ASSAULT_DEFER_SCHEDULE_SELECTION", LegacyAssaultTaskId{0x186AB}},
}};

inline constexpr std::optional<LegacyAssaultTaskId> original_assault_task_id_for(
    std::string_view symbol) {
    for (const auto& item : original_assault_task_ids)
        if (item.name == symbol) return item.original_task_id;
    return std::nullopt;
}

inline constexpr std::optional<std::string_view> original_assault_task_name_for(
    LegacyAssaultTaskId id) {
    for (const auto& item : original_assault_task_ids)
        if (item.original_task_id.value == id.value) return item.name;
    return std::nullopt;
}

} // namespace sin_compat
