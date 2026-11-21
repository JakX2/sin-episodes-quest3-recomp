#pragma once
// Phase536. Native 2006 CNPC_MobGrunt virtual +0x848 and selector state=3.
// Independent host-driven behavioral reference, not an engine CBaseNPC.
#include <cstdint>
#include <string_view>

namespace sin_compat::original_grunt_sidestep {

inline constexpr std::uint32_t kGruntVtableVA = 0x2249AAA4;
inline constexpr std::uint32_t kOriginalGateVA = 0x22359620;
inline constexpr std::uint32_t kOriginalSelectorVA = 0x2235E4E0;
inline constexpr std::uint32_t kNativeGateVirtualOffset = 0x848;
inline constexpr std::uint32_t kNativeFallbackVirtualOffset = 0x844;
inline constexpr std::uint32_t kOriginalPositiveScheduleId = 0x55;
// Verified as one name/ID registration pair in original 0x223600C1..0x223600DC.
inline constexpr std::string_view kOriginalPositiveScheduleName =
    "SCHED_GRUNT_MOBILITY_SIDESTEP";
inline constexpr std::uint8_t kInPlayerSightsCondition = 0x4D;

// Source offsets are recovered; member names are purely descriptive.
struct NativeFacts {
    bool byte_ca0_nonzero = false;
    bool byte_988_bit2_set = false;
    bool native_field_8c_nonzero = false;
};

enum class Exit : std::uint8_t {
    Eligible, Ca0Set, Flag988Set, Condition4Present,
    Condition3Present, Field8cNonzero, OwnerVirtual304Rejected,
    ManagerVirtual300Rejected, MobilityRuleRejected, HostException,
    Reentrant
};
struct Decision {
    Exit exit = Exit::HostException;
    float requested_mobility_factor = 0.0f;
    bool eligible() const noexcept { return exit == Exit::Eligible; }
};

class IHost {
public:
    virtual ~IHost() = default;
    // The same original NPC condition method 0x220397C0 is called with
    // numeric conditions 4, 3, and optionally 0x4D in that order.
    virtual bool has_original_condition(std::uint8_t condition) = 0;
    // The first virtual dispatch is Grunt vslot +0x304, with an argument
    // supplied through original 0x2202E310 / 0x220092D0. Do not guess its type.
    virtual bool call_owner_virtual_304_with_original_context() = 0;
    // The second virtual dispatch is on an object returned from original
    // 0x2202E310, vslot +0x300, with this NPC passed as argument.
    virtual bool call_manager_virtual_300_with_owner() = 0;
    // Original global at 0x22572914 virtual +0x1A0, push float factor.
    // The engine host provides real CSE1Rules and RNG behavior.
    virtual bool original_rules_mobility_decision(float factor) = 0;
};

class EligibilityGate final {
public:
    explicit EligibilityGate(IHost& host) noexcept : host_(host) {}
    EligibilityGate(const EligibilityGate&) = delete;
    EligibilityGate& operator=(const EligibilityGate&) = delete;
    Decision evaluate(NativeFacts facts) noexcept;
private:
    IHost& host_;
    bool busy_ = false;
};

struct SelectorState3Result {
    // No decision in this isolated state=3 branch means the original
    // full selector continues its other branches / inherited fallback.
    bool handled = false;
    std::uint32_t original_schedule_id = 0;
    Decision gate{};
};
class IState3Fallback {
public:
    virtual ~IState3Fallback() = default;
    // Grunt vslot +0x844 may return a nonzero original schedule ID.
    virtual std::uint32_t invoke_original_virtual_844() = 0;
};
SelectorState3Result select_original_state_three(
    std::uint32_t field_850, EligibilityGate& gate, NativeFacts facts,
    IState3Fallback& fallback) noexcept;

} // namespace sin_compat::original_grunt_sidestep