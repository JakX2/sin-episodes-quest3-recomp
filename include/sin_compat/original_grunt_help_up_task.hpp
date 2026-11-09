#pragma once

// Independently authored, evidence-bounded reconstruction of the original
// SiN Episodes (2006) CNPC_GruntBase TASK_GRUNT_HELP_UP dispatch paths.
// This is not Ritual source, an engine ABI, or a native SourceVR task.

#include <cstdint>

namespace sin_compat::ai::grunt {

// Grunt TASK IDs are a separate, build-local namespace from schedule IDs,
// condition IDs, and the other NPC classes' task registries.
struct OriginalGruntTaskId {
    std::uint32_t value = 0;
};

inline constexpr OriginalGruntTaskId task_grunt_help_up{0x94U};

constexpr bool operator==(OriginalGruntTaskId lhs,
                          OriginalGruntTaskId rhs) noexcept {
    return lhs.value == rhs.value;
}

constexpr bool operator!=(OriginalGruntTaskId lhs,
                          OriginalGruntTaskId rhs) noexcept {
    return !(lhs == rhs);
}

// Portable host-owned identity for the original m_pWounded class pointer.
// The original datamap records this member as FIELD_CLASSPTR (type code 12),
// not FIELD_EHANDLE. This token is neither the old pointer nor an engine handle.
struct HostWoundedObjectToken {
    std::uint64_t value = 0;

    explicit constexpr operator bool() const noexcept {
        return value != 0;
    }
};

enum class GruntTaskDispatchDisposition : std::uint8_t {
    Handled,
    DelegateToHumanoid,
    UnresolvedGruntTask,
};

class IOriginalGruntTaskHost {
public:
    virtual ~IOriginalGruntTaskHost() = default;

    // Returns the host's opaque identity for original CNPC_GruntBase::m_pWounded
    // at +0xCA4, or zero when the original pointer is null.
    virtual HostWoundedObjectToken wounded_object_token() const noexcept = 0;

    // Host-owned reconstruction seam for the call made at original VA
    // 0x2235A0F0. Its exact semantic name/side effects remain unresolved.
    virtual void invoke_wounded_object_helper(
        HostWoundedObjectToken target) noexcept = 0;

    // Writes null to the host representation of original m_pWounded (+0xCA4).
    virtual void clear_wounded_object() noexcept = 0;

    // Host seam for the common task-completion-style helper at original VA
    // 0x2203D2D0. The recovered TASK_GRUNT_HELP_UP paths pass argument zero.
    virtual void complete_task_with_argument(std::int32_t argument) noexcept = 0;

    // The original slot-273/slot-275 dispatchers use the inherited humanoid
    // implementation for tasks without a grunt-specific case.
    virtual void delegate_start_task_to_humanoid(
        OriginalGruntTaskId task) noexcept = 0;
    virtual void delegate_run_task_to_humanoid(
        OriginalGruntTaskId task) noexcept = 0;
};

// Handles the recovered slot-273 StartTask-style path for TASK_GRUNT_HELP_UP.
// Other registered grunt tasks are reported as unresolved so a complete
// grunt dispatcher can continue its own custom case; only unregistered IDs
// use the inherited humanoid fallback.
GruntTaskDispatchDisposition start_original_grunt_task(
    OriginalGruntTaskId task,
    IOriginalGruntTaskHost& host) noexcept;

// Handles the recovered slot-275 RunTask-style path for TASK_GRUNT_HELP_UP.
// Other explicit grunt update cases remain unresolved; tasks without a custom
// update case (including 0x9B..0x9E outside the update range) use the parent.
GruntTaskDispatchDisposition run_original_grunt_task(
    OriginalGruntTaskId task,
    IOriginalGruntTaskHost& host) noexcept;

} // namespace sin_compat::ai::grunt
