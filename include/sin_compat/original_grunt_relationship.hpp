#pragma once
// Independently authored, evidence-bounded model of original SiN grunt
// relationship override and hostility-mode condition gathering.
// The original flag name and activation path remain unknown.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sin_compat::ai::grunt {
using OriginalGruntEntityToken = std::uint64_t;

class OriginalGruntBeserkBehavior;
inline constexpr std::int32_t disposition_hate = 1;
inline constexpr std::int32_t disposition_neutral = 4;

enum class OriginalSharedAiCondition : std::uint8_t {
    SeeHate = 0x07, SeeDislike = 0x09, SeeEnemy = 0x0A,
    SeePlayer = 0x1F, SeeNemesis = 0x20, ScheduleDone = 0x22,
};

class IOriginalGruntRelationshipHost {
public:
    virtual ~IOriginalGruntRelationshipHost() = default;
    virtual std::int32_t parent_relation_type(
        OriginalGruntEntityToken target) const noexcept = 0;
    virtual bool target_is_npc(
        OriginalGruntEntityToken target) const noexcept = 0;
    virtual bool target_is_player(
        OriginalGruntEntityToken target) const noexcept = 0;

    // Host resolves the unidentified original +0x898 collection and preserves
    // its iteration order. Its concrete type/member name remain unknown.
    virtual std::vector<OriginalGruntEntityToken>
    relationship_targets_in_original_order() const = 0;
    virtual OriginalGruntEntityToken current_enemy() const noexcept = 0;
    virtual std::int32_t relation_priority(
        OriginalGruntEntityToken target) const noexcept = 0;
    virtual void set_ai_condition(
        OriginalSharedAiCondition condition) noexcept = 0;
    virtual void delegate_parent_condition_gather() noexcept = 0;

    // Explicit seam for the original per-target flag checks and virtual
    // +0x678 whose semantics have not yet been reconstructed.
    virtual void defer_unrecovered_target_condition_tail(
        OriginalGruntEntityToken target) noexcept = 0;
};

// Models original CNPC_GruntBase slot 240. The boolean represents the
// unidentified mode byte at original grunt +0xDB1; it is never enabled by
// default by this library.
std::int32_t original_grunt_relation_type(
    const IOriginalGruntRelationshipHost& host,
    bool hostility_mode_enabled,
    OriginalGruntEntityToken target) noexcept;

enum class OriginalGruntConditionGatherDisposition : std::uint8_t {
    ParentDelegated, PartialHostilityMode
};
struct OriginalGruntConditionGatherResult {
    OriginalGruntConditionGatherDisposition disposition =
        OriginalGruntConditionGatherDisposition::ParentDelegated;
    std::size_t targets_visited = 0;
    std::size_t neutral_targets_skipped = 0;
    std::size_t unresolved_tails_deferred = 0;
};

// Models only the verified subset of original slot 316. Mode-off delegates
// to CAI_BaseHumanoid; mode-on sets confirmed conditions and defers the
// still-unrecovered target tail explicitly.
OriginalGruntConditionGatherResult gather_original_grunt_conditions(
    IOriginalGruntRelationshipHost& host,
    bool hostility_mode_enabled);

std::int32_t original_grunt_relation_type(
    const IOriginalGruntRelationshipHost& host,
    const OriginalGruntBeserkBehavior& behavior,
    OriginalGruntEntityToken target) noexcept;

OriginalGruntConditionGatherResult gather_original_grunt_conditions(
    IOriginalGruntRelationshipHost& host,
    const OriginalGruntBeserkBehavior& behavior);
} // namespace sin_compat::ai::grunt
