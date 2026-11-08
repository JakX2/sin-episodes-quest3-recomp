#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace sin_compat::vehicle {

enum class InputKind : std::uint8_t { Void, Integer, String };

struct InputSpec {
    std::string_view name;
    InputKind kind;
    std::uint32_t original_x86_handler_va;
};

const std::array<InputSpec, 25>& original_input_specs() noexcept;
const InputSpec* find_input_spec(std::string_view name) noexcept;

// Generation-aware identity for host-owned Source entities. No 2006 x86
// EHANDLE bytes or object offsets cross this portable boundary.
struct EntityRef {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    bool valid() const noexcept { return index != 0; }
};

struct ViewRecord {
    // The original 100-byte view record exposes these two fields at +0x55
    // and +0x60. Remaining seat/camera fields are intentionally not guessed.
    bool allow_weapons = false;
    float damage_modifier = 1.0f;
};

struct VehicleState {
    // Optional means the value has not yet been supplied by map keyvalues,
    // SaveRestore, or a corresponding original input. No guessed constructor
    // defaults are presented as original behavior.
    std::optional<bool> enable_use;
    std::optional<bool> views_locked;
    std::optional<bool> infinite_ammo;
    std::optional<bool> invulnerable;
    std::optional<bool> driver_door_locked;
    std::optional<float> damage_tolerance;
    std::optional<bool> monitor_active;
    std::string monitor_camera;
    std::optional<bool> hints_enabled;
    std::optional<bool> headlights_on;
    std::optional<bool> wipers_on;
};

enum class InputValueKind : std::uint8_t { Void, Integer, String };
using InputPayload = std::variant<std::monostate, int, std::string_view>;

struct InputValue {
    InputPayload payload;
    std::optional<EntityRef> activator;

    InputValueKind kind() const noexcept {
        if (std::holds_alternative<int>(payload)) return InputValueKind::Integer;
        if (std::holds_alternative<std::string_view>(payload)) return InputValueKind::String;
        return InputValueKind::Void;
    }
};

enum class Result : std::uint8_t {
    Applied,
    Unchanged,
    RejectedInput,
    MissingActivator,
    MissingLinkedEntity,
    WrongEntityType,
    OutOfRange,
    HostRejected,
    UninitializedState
};

class IOriginalVehicleHost {
public:
    virtual ~IOriginalVehicleHost() = default;

    // Door/window collections are engine-owned. Bulk inputs visit every
    // entry even if one host operation fails, matching the original fan-out.
    virtual std::size_t door_count() const noexcept = 0;
    virtual std::size_t window_count() const noexcept = 0;
    virtual bool set_door_open(std::size_t index, bool open) = 0;
    virtual bool set_window_open(std::size_t index, bool open) = 0;

    // SetView resolves the input activator to a live player-like entity. The
    // original writes both player fields before the remaining view dispatch.
    virtual std::optional<EntityRef> resolve_player(EntityRef activator) = 0;
    virtual void set_player_view_index(EntityRef player, int index) = 0;
    virtual void set_player_allow_weapons(EntityRef player, bool allow) = 0;
    virtual bool apply_vehicle_view(EntityRef player, int index) = 0;

    // Headlight state has a confirmed client receive property named
    // m_bHeadlightIsOn. The engine adapter owns actual light effects and netvar
    // publication; this runtime never writes a client object by raw offset.
    virtual bool set_vehicle_headlights(bool enabled) = 0;
    virtual void publish_headlight_state(bool enabled) = 0;

    // The original monitor entity is addressed by the literal target name
    // "monitor" with "open" / "close" inputs.
    virtual void fire_named_input(std::string_view target,
                                  std::string_view input,
                                  EntityRef caller) = 0;

    virtual bool is_live(EntityRef entity) = 0;
    virtual std::optional<EntityRef> resolve_named_entity(std::string_view name) = 0;
    virtual bool is_point_camera(EntityRef entity) = 0;
    virtual bool link_monitor_camera(EntityRef camera) = 0;
    virtual void unlink_monitor_camera(EntityRef camera) = 0;

    // Radio is a linked entity, not a vehicle-local on/off Boolean.
    virtual std::optional<EntityRef> resolve_radio() = 0;
    virtual bool set_radio_power(EntityRef radio, bool enabled) = 0;
    virtual bool set_radio_station(EntityRef radio, int station) = 0;

    // Enter/Exit are deliberately delegated to the real vehicle/passenger API.
    virtual bool enter_vehicle(std::optional<EntityRef> activator, int argument) = 0;
    virtual bool exit_vehicle(std::optional<EntityRef> activator, int argument) = 0;
};

class OriginalVehicleRuntime {
public:
    explicit OriginalVehicleRuntime(EntityRef self,
                                    std::vector<ViewRecord> views = {},
                                    VehicleState initial_state = {});

    const VehicleState& state() const noexcept { return state_; }
    const std::vector<ViewRecord>& views() const noexcept { return views_; }
    std::optional<EntityRef> camera_link() const noexcept { return camera_link_; }

    // Applies only the independently reconstructed extension behavior. The
    // Source base vehicle physics, input dispatch, save/restore and network
    // object registration remain responsibilities of the eventual host.
    Result handle_input(std::string_view name,
                       const InputValue& value,
                       IOriginalVehicleHost& host);

    // Called from the host's vehicle update. SetCamera only stores the name
    // and unlinks the old handle; the original resolves it later during update.
    Result refresh_monitor_camera(IOriginalVehicleHost& host);

private:
    Result clear_camera_link(IOriginalVehicleHost& host);
    Result set_one_door(int index, bool open, IOriginalVehicleHost& host);
    Result set_one_window(int index, bool open, IOriginalVehicleHost& host);
    Result set_all_doors(bool open, IOriginalVehicleHost& host);
    Result set_all_windows(bool open, IOriginalVehicleHost& host);

    EntityRef self_;
    VehicleState state_;
    std::vector<ViewRecord> views_;
    std::optional<EntityRef> camera_link_;
};

} // namespace sin_compat::vehicle
