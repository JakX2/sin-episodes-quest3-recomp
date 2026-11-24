#pragma once
// New port ABI, not an original Source engine interface.
// Calls are synchronous on the host's game thread. Output pointers must
// refer to caller-owned writable storage. Do not retain API pointers after
// unloading the module. No C++ objects or allocations cross the DLL boundary.
#include <cstdint>

#if defined(_MSC_VER)
#define SIN_FIXED_CALL __cdecl
#else
#define SIN_FIXED_CALL
#endif

namespace sin_port::fixed_service {
inline constexpr std::uint32_t abi_version=1;
inline constexpr std::uint32_t metric_count=14;
inline constexpr char export_name[]="SiN_GetFixedDifficultyApi";
enum ChallengeInput : std::uint32_t { Initial=0, Current=1, Difference=2 };

struct State {
    std::uint32_t skill, initial_skill, revision, trigger_count;
    std::uint32_t completed, adaptive_enabled;
};

struct ChallengeOutput {
    std::uint32_t original_field_offset;
    std::uint32_t activator, caller;
    float value;
};

struct Api {
    std::uint32_t version, size, metrics;
    int (SIN_FIXED_CALL *begin_campaign)(int) noexcept;
    int (SIN_FIXED_CALL *end_campaign)() noexcept;
    int (SIN_FIXED_CALL *set_skill)(int) noexcept;
    int (SIN_FIXED_CALL *get_state)(State*) noexcept;
    int (SIN_FIXED_CALL *get_metric)(std::uint32_t,float*) noexcept;
    int (SIN_FIXED_CALL *scale_damage_taken)(float,std::uint32_t,float*) noexcept;
    int (SIN_FIXED_CALL *scale_damage_done)(float,float*) noexcept;
    int (SIN_FIXED_CALL *npc_shoot_delay)(float,float*) noexcept;
    int (SIN_FIXED_CALL *observe_trigger)(std::int32_t,float,std::int32_t) noexcept;
    int (SIN_FIXED_CALL *capture_challenge)(float*) noexcept;
    // Snapshot is normalized 0/0.5/1, from capture_challenge. It is
    // required only for Difference. The result is an output descriptor,
    // NOT a dispatch into an original engine entity or its raw memory.
    int (SIN_FIXED_CALL *query_challenge)(std::uint32_t,float,
                                         std::uint32_t,ChallengeOutput*) noexcept;
};
// Win32 original VEngineCvar002 bridge. Explicit host call at campaign
// start or a game-thread synchronization boundary; no engine pointer is
// retained, no console object is created or modified. Not an original export.
inline constexpr char engine_skill_export_name[]="SiN_ApplyEngineSkill";
enum EngineSkillOperation : std::uint32_t { BeginFromEngine=0, SyncFromEngine=1 };
using ApplyEngineSkill=int (SIN_FIXED_CALL *)(void*,std::uint32_t) noexcept;
using GetApi=const Api* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
static_assert(sizeof(State)==24);
static_assert(sizeof(ChallengeOutput)==16);
} // namespace sin_port::fixed_service
