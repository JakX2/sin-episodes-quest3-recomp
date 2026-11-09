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

struct EntityRef {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;
    bool valid() const noexcept { return index != 0; }
};

struct ViewRecord {
    bool allow_weapons = false;
    // The original constructor initializes view_damage_mod to 1.0f.
    float damage_modifier = 1.0f;
};

// Decodes only the original binary-backed view_damage_mod member from the
// 100-byte x86 view records. Other fields remain at conservative defaults.
// The output is replaced only after the complete requested range is valid.
bool decode_original_view_records(const std::uint8_t* bytes,
                                  std::size_t byte_count,
                                  std::size_t view_count,
                                  std::vector<ViewRecord>& output);

enum class DamageViewPath : std::uint8_t {
    NonPlayer,
    PlayerViewModifier,
    InvalidViewFallback
};

struct DamageViewResult {
    float damage = 0.0f;
    DamageViewPath path = DamageViewPath::NonPlayer;
};

struct VehicleState {
    std::optional<bool> enable_use;
    std::optional<bool> views_locked;
    std::optional<bool> infinite_ammo;
    std::optional<bool> invulnerable;
    std::optional<bool> driver_door_locked;
    std::optional<float> damage_tolerance;
    std::optional<bool> monitor_active;
    std::optional<std::string> monitor_camera;
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
    virtual std::size_t door_count() const noexcept = 0;
    virtual std::size_t window_count() const noexcept = 0;
    virtual bool set_door_open(std::size_t index, bool open) = 0;
    virtual bool set_window_open(std::size_t index, bool open) = 0;
    virtual std::optional<EntityRef> resolve_player(EntityRef activator) = 0;
    virtual void set_player_view_index(EntityRef player, int index) = 0;
    virtual void set_player_allow_weapons(EntityRef player, bool allow) = 0;
    virtual bool apply_vehicle_view(EntityRef player, int index) = 0;
    virtual bool set_vehicle_headlights(bool enabled) = 0;
    virtual void publish_headlight_state(bool enabled) = 0;
    virtual void fire_named_input(std::string_view target,
                                  std::string_view input,
                                  EntityRef caller) = 0;
    virtual bool is_live(EntityRef entity) = 0;
    virtual std::optional<EntityRef> resolve_named_entity(std::string_view name) = 0;
    virtual bool is_point_camera(EntityRef entity) = 0;
    virtual bool link_monitor_camera(EntityRef camera) = 0;
    virtual void unlink_monitor_camera(EntityRef camera) = 0;
    virtual std::optional<EntityRef> resolve_radio() = 0;
    virtual bool set_radio_power(EntityRef radio, bool enabled) = 0;
    virtual bool set_radio_station(EntityRef radio, int station) = 0;
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

    Result handle_input(std::string_view name,
                       const InputValue& value,
                       IOriginalVehicleHost& host);
    Result refresh_monitor_camera(IOriginalVehicleHost& host);

    // Applies only the final player-specific view_damage_mod stage. The caller
    // supplies damage after earlier damage-type/global scaling and the
    // engine-derived player classification/current view index.
    DamageViewResult apply_view_damage_modifier(
        float damage_after_pre_view_scaling,
        bool entity_is_player,
        std::optional<int> selected_view_index) const noexcept;

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
