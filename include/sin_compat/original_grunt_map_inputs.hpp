#pragma once

// Independently authored, evidence-bounded reconstruction of original
// CNPC_GruntBase map inputs. The original PE32 offsets are evidence labels;
// the native host owns all ARM64 object storage and engine calls.

#include "sin_compat/original_grunt_relationship.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace sin_compat::ai::grunt {

enum class OriginalGruntMapInput : std::uint8_t {
    LookOff,
    LookOn,
    StartPatrolling,
    StopPatrolling,
    Assault,
    ThrowGrenades
};

enum class OriginalGruntMapInputResult : std::uint8_t {
    Applied,
    MissingEntity,
    MissingInputValue,
    InvalidInput
};

inline constexpr std::uint32_t original_grunt_look_flag_mask = 0x00010000U;
inline constexpr std::int32_t original_grunt_assault_helper_argument = 3;
inline constexpr float original_grunt_throw_grenade_height_reset = 0.0F;

// Only the two typed input payloads need values. An engaged empty string is
// distinct from a missing FIELD_STRING input; false is a valid FIELD_BOOLEAN.
struct OriginalGruntMapInputValue {
    std::optional<std::string_view> string_value{};
    std::optional<bool> boolean_value{};
};

// The host maps these operations onto its own entity storage and engine APIs.
// The original LookOn/LookOff target flag-word offset and Assault helper
// semantics remain unresolved, so those operations stay explicit callbacks.
class IOriginalGruntMapInputHost {
public:
    virtual ~IOriginalGruntMapInputHost() = default;
    virtual bool is_live(OriginalGruntEntityToken entity) const noexcept = 0;

    // Original m_bShouldPatrol at +0xC65.
    virtual void set_should_patrol(
        OriginalGruntEntityToken entity, bool enabled) noexcept = 0;

    // Original LookOff sets, and LookOn clears, bit 0x10000. The host locates
    // the correct native flag word; this API does not assume the old x86 offset.
    virtual void update_look_flag(
        OriginalGruntEntityToken entity,
        std::uint32_t mask,
        bool enabled) noexcept = 0;

    // InputAssault's FIELD_STRING payload is preserved. The original handler
    // calls the helper at 0x220CFC00 with integer argument 3; helper semantics
    // are not guessed or replaced with a fabricated schedule ID.
    virtual void invoke_assault_helper(
        OriginalGruntEntityToken entity,
        std::string_view input,
        std::int32_t argument) noexcept = 0;

    // The verified FIELD_BOOLEAN path writes m_flGrenadeHeight=0.0f at +0xC70
    // before writing the input value to m_bGrenadeHappy at +0xC6C.
    virtual void set_grenade_height(
        OriginalGruntEntityToken entity, float height) noexcept = 0;
    virtual void set_grenade_happy(
        OriginalGruntEntityToken entity, bool enabled) noexcept = 0;
};

OriginalGruntMapInputResult dispatch_original_grunt_map_input(
    OriginalGruntEntityToken entity,
    OriginalGruntMapInput input,
    const OriginalGruntMapInputValue& value,
    IOriginalGruntMapInputHost& host) noexcept;

} // namespace sin_compat::ai::grunt
