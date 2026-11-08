#include "sin_compat/original_vehicle_runtime.hpp"

#include <utility>

namespace sin_compat::vehicle {
namespace {
constexpr std::array<InputSpec, 25> kInputs{{
    {"EnterVehicle", InputKind::Integer, 0x2232dd00u},
    {"ExitVehicle", InputKind::Integer, 0x2232dd40u},
    {"EnableUse", InputKind::Integer, 0x2232dd90u},
    {"SetView", InputKind::Integer, 0x2232fa70u},
    {"LockView", InputKind::Integer, 0x2232ddc0u},
    {"OpenDoor", InputKind::Integer, 0x22330800u},
    {"OpenWindow", InputKind::Integer, 0x22330870u},
    {"CloseDoor", InputKind::Integer, 0x223308d0u},
    {"CloseWindow", InputKind::Integer, 0x22330940u},
    {"OpenDoors", InputKind::Void, 0x223309a0u},
    {"CloseDoors", InputKind::Void, 0x223309b0u},
    {"OpenWindows", InputKind::Void, 0x223309c0u},
    {"CloseWindows", InputKind::Void, 0x223309d0u},
    {"HeadlightOn", InputKind::Void, 0x223309e0u},
    {"HeadlightOff", InputKind::Void, 0x22330a20u},
    {"WipersOn", InputKind::Void, 0x2232ddf0u},
    {"WipersOff", InputKind::Void, 0x2232de00u},
    {"MonitorOn", InputKind::Void, 0x22330a60u},
    {"MonitorOff", InputKind::Void, 0x22330a90u},
    {"SetCamera", InputKind::String, 0x2232fb20u},
    {"EnableHints", InputKind::Integer, 0x2232de10u},
    {"EnableAmmo", InputKind::Integer, 0x2232de40u},
    {"RadioOn", InputKind::Void, 0x2232f500u},
    {"RadioOff", InputKind::Void, 0x2232f540u},
    {"SetRadioStation", InputKind::Integer, 0x2232f580u},
}};

InputValueKind to_value_kind(InputKind kind) noexcept {
    switch (kind) {
    case InputKind::Void: return InputValueKind::Void;
    case InputKind::Integer: return InputValueKind::Integer;
    case InputKind::String: return InputValueKind::String;
    }
    return InputValueKind::Void;
}
} // namespace

const std::array<InputSpec, 25>& original_input_specs() noexcept { return kInputs; }

const InputSpec* find_input_spec(std::string_view name) noexcept {
    for (const auto& spec : kInputs) if (spec.name == name) return &spec;
    return nullptr;
}

OriginalVehicleRuntime::OriginalVehicleRuntime(EntityRef self,
                                               std::vector<ViewRecord> views,
                                               VehicleState initial_state)
    : self_(self), state_(std::move(initial_state)), views_(std::move(views)) {}

DamageViewResult OriginalVehicleRuntime::apply_view_damage_modifier(
    float damage_after_pre_view_scaling,
    bool entity_is_player,
    std::optional<int> selected_view_index) const noexcept {
    if (!entity_is_player)
        return {damage_after_pre_view_scaling, DamageViewPath::NonPlayer};

    // The original x86 routine does not show an equivalent bounds check here.
    // Preserve the already-scaled damage instead of indexing an invalid view.
    if (!selected_view_index || *selected_view_index < 0 ||
        static_cast<std::size_t>(*selected_view_index) >= views_.size()) {
        return {damage_after_pre_view_scaling, DamageViewPath::InvalidViewFallback};
    }

    const auto& view = views_[static_cast<std::size_t>(*selected_view_index)];
    return {damage_after_pre_view_scaling * view.damage_modifier,
            DamageViewPath::PlayerViewModifier};
}

Result OriginalVehicleRuntime::clear_camera_link(IOriginalVehicleHost& host) {
    if (!camera_link_) return Result::Unchanged;
    const auto old = *camera_link_;
    if (host.is_live(old)) host.unlink_monitor_camera(old);
    camera_link_.reset();
    return Result::Applied;
}

Result OriginalVehicleRuntime::set_one_door(int index, bool open,
                                            IOriginalVehicleHost& host) {
    if (index < 0 || static_cast<std::size_t>(index) >= host.door_count())
        return Result::OutOfRange;
    return host.set_door_open(static_cast<std::size_t>(index), open)
        ? Result::Applied : Result::HostRejected;
}

Result OriginalVehicleRuntime::set_one_window(int index, bool open,
                                              IOriginalVehicleHost& host) {
    if (index < 0 || static_cast<std::size_t>(index) >= host.window_count())
        return Result::OutOfRange;
    return host.set_window_open(static_cast<std::size_t>(index), open)
        ? Result::Applied : Result::HostRejected;
}

Result OriginalVehicleRuntime::set_all_doors(bool open, IOriginalVehicleHost& host) {
    bool all_applied = true;
    const auto count = host.door_count();
    for (std::size_t i = 0; i < count; ++i)
        if (!host.set_door_open(i, open)) all_applied = false;
    return all_applied ? Result::Applied : Result::HostRejected;
}

Result OriginalVehicleRuntime::set_all_windows(bool open, IOriginalVehicleHost& host) {
    bool all_applied = true;
    const auto count = host.window_count();
    for (std::size_t i = 0; i < count; ++i)
        if (!host.set_window_open(i, open)) all_applied = false;
    return all_applied ? Result::Applied : Result::HostRejected;
}

Result OriginalVehicleRuntime::handle_input(std::string_view name,
                                            const InputValue& value,
                                            IOriginalVehicleHost& host) {
    const auto* spec = find_input_spec(name);
    if (!spec || value.kind() != to_value_kind(spec->kind)) return Result::RejectedInput;
    const int n = std::holds_alternative<int>(value.payload) ? std::get<int>(value.payload) : 0;

    if (name == "WipersOn" || name == "WipersOff") {
        state_.wipers_on = name == "WipersOn";
        return Result::Applied;
    }
    if (name == "HeadlightOn" || name == "HeadlightOff") {
        const bool enabled = name == "HeadlightOn";
        if (!host.set_vehicle_headlights(enabled)) return Result::HostRejected;
        state_.headlights_on = enabled;
        host.publish_headlight_state(enabled);
        return Result::Applied;
    }
    if (name == "OpenDoor") return set_one_door(n, true, host);
    if (name == "CloseDoor") return set_one_door(n, false, host);
    if (name == "OpenWindow") return set_one_window(n, true, host);
    if (name == "CloseWindow") return set_one_window(n, false, host);
    if (name == "OpenDoors") return set_all_doors(true, host);
    if (name == "CloseDoors") return set_all_doors(false, host);
    if (name == "OpenWindows") return set_all_windows(true, host);
    if (name == "CloseWindows") return set_all_windows(false, host);

    if (name == "EnableUse") { state_.enable_use = n != 0; return Result::Applied; }
    if (name == "LockView") { state_.views_locked = n != 0; return Result::Applied; }
    if (name == "EnableAmmo") { state_.infinite_ammo = n != 0; return Result::Applied; }
    if (name == "EnableHints") { state_.hints_enabled = n != 0; return Result::Applied; }
    if (name == "EnterVehicle")
        return host.enter_vehicle(value.activator, n) ? Result::Applied : Result::HostRejected;
    if (name == "ExitVehicle")
        return host.exit_vehicle(value.activator, n) ? Result::Applied : Result::HostRejected;

    if (name == "SetView") {
        if (!value.activator || !value.activator->valid()) return Result::MissingActivator;
        if (n < 0 || static_cast<std::size_t>(n) >= views_.size()) return Result::OutOfRange;
        const auto player = host.resolve_player(*value.activator);
        if (!player || !player->valid()) return Result::MissingActivator;
        host.set_player_view_index(*player, n);
        host.set_player_allow_weapons(*player, views_[static_cast<std::size_t>(n)].allow_weapons);
        return host.apply_vehicle_view(*player, n) ? Result::Applied : Result::HostRejected;
    }

    if (name == "SetCamera") {
        const auto camera_name = std::get<std::string_view>(value.payload);
        state_.monitor_camera = std::string(camera_name.begin(), camera_name.end());
        clear_camera_link(host);
        return Result::Applied;
    }
    if (name == "MonitorOn") {
        if (!state_.monitor_active) return Result::UninitializedState;
        if (*state_.monitor_active) return Result::Unchanged;
        host.fire_named_input("monitor", "open", self_);
        state_.monitor_active = true;
        return Result::Applied;
    }
    if (name == "MonitorOff") {
        if (!state_.monitor_active) return Result::UninitializedState;
        if (!*state_.monitor_active) return Result::Unchanged;
        host.fire_named_input("monitor", "close", self_);
        state_.monitor_active = false;
        clear_camera_link(host);
        return Result::Applied;
    }
    if (name == "RadioOn" || name == "RadioOff" || name == "SetRadioStation") {
        const auto radio = host.resolve_radio();
        if (!radio || !radio->valid() || !host.is_live(*radio)) return Result::MissingLinkedEntity;
        if (name == "SetRadioStation")
            return host.set_radio_station(*radio, n) ? Result::Applied : Result::HostRejected;
        return host.set_radio_power(*radio, name == "RadioOn") ? Result::Applied : Result::HostRejected;
    }
    return Result::RejectedInput;
}

Result OriginalVehicleRuntime::refresh_monitor_camera(IOriginalVehicleHost& host) {
    if (!state_.monitor_active) return Result::UninitializedState;
    if (!*state_.monitor_active) return Result::Unchanged;
    if (!state_.monitor_camera) return Result::UninitializedState;
    if (state_.monitor_camera->empty()) return Result::Unchanged;
    if (camera_link_ && host.is_live(*camera_link_)) return Result::Unchanged;
    if (camera_link_) camera_link_.reset();

    const auto candidate = host.resolve_named_entity(*state_.monitor_camera);
    if (!candidate || !candidate->valid() || !host.is_live(*candidate)) {
        state_.monitor_camera = std::string{};
        return Result::MissingLinkedEntity;
    }
    if (!host.is_point_camera(*candidate)) {
        state_.monitor_camera = std::string{};
        return Result::WrongEntityType;
    }
    if (!host.link_monitor_camera(*candidate)) {
        state_.monitor_camera = std::string{};
        return Result::HostRejected;
    }
    camera_link_ = *candidate;
    return Result::Applied;
}

} // namespace sin_compat::vehicle
