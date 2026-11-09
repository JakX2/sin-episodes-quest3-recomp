#pragma once

// Independently authored, partial reconstruction of confirmed branches in
// the original SiN Episodes (2006) CNPC_GruntBase slot-269 selector.
// This is not Ritual source and does not expose the old x86 ABI.

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace sin_compat::ai::grunt {

// These IDs belong to the original grunt-local condition namespace. They are
// not global Source condition IDs and must not be reused for other NPC classes.
enum class GruntConditionId : std::uint8_t {
    ShouldPatrol = 0x44,
    OnFire = 0x46,
    ShotInKnee = 0x47,
    ShotInHelmet = 0x48,
    HelpUp = 0x49,
    SquadmateNeedsHelp = 0x4A,
    HelperFoundPlayer = 0x4B,
    KickableGrenade = 0x4C,
    InPlayerSights = 0x4D
};

// Only schedule IDs whose string/number registration was recovered in Phase 7
// and whose return is identified in the preserved selector trace are listed.
// These are original SiN-local IDs, never native SourceVR schedule IDs.
enum class GruntSchedule : std::uint8_t {
    Patrol = 0x64,
    BurningStand = 0x68,
    Stunned = 0x6B,
    HelpStunned = 0x6C,
    Dodge = 0x6D,
    MoveToPlayer = 0x6E,
    Sitting = 0x6F,
    GetUp = 0x71,
    KickGrenade = 0x73,
    HelmetFlinch = 0x75
};

struct OriginalGruntScheduleId {
    std::uint8_t value = 0;
};

constexpr OriginalGruntScheduleId original_schedule_id(
    GruntSchedule schedule) noexcept {
    return {static_cast<std::uint8_t>(schedule)};
}

constexpr bool operator==(OriginalGruntScheduleId a,
                          OriginalGruntScheduleId b) noexcept {
    return a.value == b.value;
}

constexpr bool operator!=(OriginalGruntScheduleId a,
                          OriginalGruntScheduleId b) noexcept {
    return !(a == b);
}

class GruntConditionSet {
public:
    void set(GruntConditionId id, bool present = true) noexcept {
        const auto index = static_cast<std::size_t>(id);
        if (present) bits_.set(index);
        else bits_.reset(index);
    }

    void clear(GruntConditionId id) noexcept {
        bits_.reset(static_cast<std::size_t>(id));
    }

    bool has(GruntConditionId id) const noexcept {
        return bits_.test(static_cast<std::size_t>(id));
    }

private:
    std::bitset<128> bits_{};
};

struct GruntSelectorState {
    // Raw byte and one-shot flag identified by the Phase 7 selector trace.
    // The descriptive C++ field name for +0xCB0 is not an original symbol.
    std::uint8_t npc_flags_b6 = 0;
    bool sitting_handled_cb0 = false;

    GruntConditionSet conditions{};

    // The selector tests shared condition 0x37 before the squad-help branch.
    // Its semantic name was not recovered in the grunt-local registry.
    bool has_unidentified_shared_condition_0x37 = false;

    // Descriptive host inputs for two guards whose original field names or
    // exact offsets are not established by the preserved report.
    bool squadmate_help_guard_field_nonzero = false;
    bool patrol_guard_satisfied = false;
};

enum class UnrecoveredSelectorGap : std::uint8_t {
    BetweenEarlyPrioritiesAndDodge,
    BetweenDodgeAndSquadHelp,
    BetweenHelperFoundPlayerAndPatrol
};

// The original selector contains additional branches in these gaps. The
// adapter must implement any recovered branches there and return their
// original SiN-local schedule ID. Returning nullopt means no decision in
// that region. It must not treat these IDs as native engine schedule IDs.
class IGruntSelectorExtension {
public:
    virtual ~IGruntSelectorExtension() = default;
    virtual std::optional<OriginalGruntScheduleId> select_unrecovered(
        UnrecoveredSelectorGap gap,
        GruntSelectorState& state) = 0;
};

// The original KickableGrenade selector branch also resolves the tracked
// grenade, checks the original eligibility helper, transfers the handle, and
// only then returns schedule 0x73. This gate lets the portable selector keep
// that operation coupled to the verified branch without importing engine ABI.
class IGruntKickScheduleGate {
public:
    virtual ~IGruntKickScheduleGate() = default;
    virtual bool prepare_kick_schedule() = 0;
};

// Runs only the branch segments supported by the preserved Phase 7 trace.
// The extension is called at each gap containing unmodeled original logic.
// A null result after the last segment means the caller must delegate to the
// CAI_BaseHumanoid slot-269 selector; this function does not call an engine.
std::optional<OriginalGruntScheduleId> select_verified_grunt_schedule(
    GruntSelectorState& state,
    IGruntSelectorExtension& extension);
std::optional<OriginalGruntScheduleId> select_verified_grunt_schedule(
    GruntSelectorState& state,
    IGruntSelectorExtension& extension,
    IGruntKickScheduleGate& kick_gate);

} // namespace sin_compat::ai::grunt
