#pragma once
// Independent C++17 behavioral reconstruction of a specific original 2006
// CNPC_Jessica scripting chain. Not Ritual source, a native Source AI class,
// or a claim that the original campaign has run on an Android engine.
#include "sin_compat/original_jessica_toss.hpp"
#include "sin_compat/original_jessica_impulse.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace sin_compat::jessica {

// Original datamap input handlers test variant type 2 for string handles
// and 5 for integer payloads; mismatched types result in empty/zero.
struct InputValue {
    std::int32_t original_type = -1;
    std::string text;
    std::int32_t integer = 0;
    static InputValue string_value(std::string text);
    static InputValue integer_value(std::int32_t number) noexcept;
};

// This is a host-owned entity identity, NOT a 2006 Source EHANDLE encoding.
using SpawnedEntity = std::uint64_t;

class IHost {
public:
    virtual ~IHost() = default;
    // The engine owns condition storage and all actual NPC scheduling.
    virtual void set_condition(int original_condition) = 0;
    virtual bool has_condition(int original_condition) const = 0;
    virtual void clear_condition(int original_condition) = 0;
    virtual int select_companion_schedule() = 0;
    virtual void on_companion_animation_event(int event_id) = 0;

    // The original fetches anim_attachment_RH before creating the object.
    // A host cannot assume it has a player or an attachment. It must fail
    // closed instead of inventing an attachment or silently dropping errors.
    virtual std::optional<SpawnedEntity> spawn_at_attachment(
        std::string_view classname, std::string_view attachment) = 0;
    virtual bool is_combat_weapon(SpawnedEntity entity) = 0;
    virtual void set_primary_ammo(SpawnedEntity weapon, std::int32_t count) = 0;
    virtual void set_secondary_ammo(SpawnedEntity weapon, std::int32_t count) = 0;

    // Kept for older mock-host tests. A real host should supply a separate
    // IOriginalTossProvider to apply the independently recovered equation.
    virtual bool launch_toward_player(SpawnedEntity entity) = 0;
    // The original HolsterWeapon input resolves the local player first.
    virtual bool holster_local_player_weapon() = 0;
};

// Source/Quest engine adapter chooses the attachment and player target
// positions; no original memory layouts or stale EHANDLE pointers cross
// this seam. Missing/invalid physics data fails closed.
class IOriginalTossProvider : public IOriginalImpulseHost {
public:
    virtual ~IOriginalTossProvider()=default;
    virtual std::optional<TossGeometry> observe_toss(
        SpawnedEntity entity,std::string_view attachment_name)=0;
    // The x86 routine applies a velocity IMPULSE, not a replacement.
    // All branch effects are supplied through IOriginalImpulseHost.
};

enum class AnimationResult {
    ForwardedToCompanion,
    NoRegisteredEvent,
    SpawnRejected,
    LaunchRejected,
    Launched
};

class ThrowRuntime final {
public:
    explicit ThrowRuntime(IHost& host,
                          IOriginalTossProvider* toss_provider=nullptr) noexcept
        : host_(host), toss_provider_(toss_provider) {}
    // The animation registry ID is assigned at runtime, not permanently
    // equal to 0x59 or 0x47 (those are the separate schedule/condition).
    void bind_animation_event(int registered_id) noexcept {
        registered_event_id_ = registered_id;
    }
    void unbind_animation_event() noexcept { registered_event_id_.reset(); }

    void input_throw_item_at_player(const InputValue& value);
    void input_set_primary_ammo_count_to_throw(const InputValue& value) noexcept;
    void input_set_secondary_ammo_count_to_throw(const InputValue& value) noexcept;
    bool input_holster_weapon();

    int select_schedule();
    AnimationResult on_animation_event(int event_id);

    std::string_view pending_classname() const noexcept { return throw_name_; }
    std::int32_t pending_primary_ammo() const noexcept { return primary_ammo_; }
    std::int32_t pending_secondary_ammo() const noexcept { return secondary_ammo_; }

    static constexpr int kThrowCondition = 0x47;
    static constexpr int kThrowSchedule = 0x59;
    static constexpr std::string_view kRightHandAttachment = "anim_attachment_RH";

private:
    IHost& host_;
    IOriginalTossProvider* toss_provider_=nullptr;
    std::optional<int> registered_event_id_;
    std::string throw_name_;
    // The original Jessica spawn initializes these temporary amounts to -1.
    std::int32_t primary_ammo_ = -1;
    std::int32_t secondary_ammo_ = -1;
};
} // namespace sin_compat::jessica
