#include "sin_compat/original_vehicle_runtime.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace sin_compat::vehicle;

namespace {
struct Host final : IOriginalVehicleHost {
    std::vector<bool> doors{false, false, false};
    std::vector<bool> windows{false, false};
    std::vector<std::string> events;
    std::optional<EntityRef> player{EntityRef{10, 2}};
    std::optional<EntityRef> radio{EntityRef{20, 3}};
    std::optional<EntityRef> named_camera{EntityRef{30, 4}};
    bool camera_is_point = true;
    bool reject_headlights = false;
    bool reject_door_index_one = false;
    bool reject_link = false;
    bool live_camera = true;
    bool live_player = true;
    int selected_view = -1;
    std::uint8_t allow_weapons = 0;
    bool headlights = false;
    bool published_headlights = false;
    bool radio_power = false;
    int radio_station = -1;
    std::optional<EntityRef> linked_camera;
    std::optional<EntityRef> last_activator;
    int last_vehicle_argument = -1;

    std::size_t door_count() const noexcept override { return doors.size(); }
    std::size_t window_count() const noexcept override { return windows.size(); }
    bool set_door_open(std::size_t index, bool open) override {
        events.push_back(std::string("door:") + std::to_string(index) + (open ? ":open" : ":close"));
        if (index == 1 && reject_door_index_one) return false;
        doors.at(index) = open;
        return true;
    }
    bool set_window_open(std::size_t index, bool open) override {
        events.push_back(std::string("window:") + std::to_string(index) + (open ? ":open" : ":close"));
        windows.at(index) = open;
        return true;
    }
    std::optional<EntityRef> resolve_player(EntityRef activator) override {
        if (!player || activator.index != 99) return std::nullopt;
        return player;
    }
    void set_player_view_index(EntityRef, int index) override {
        selected_view = index;
        events.emplace_back("player-view-index");
    }
    void set_player_allow_weapons(EntityRef, std::uint8_t allow) override {
        allow_weapons = allow;
        events.emplace_back("player-view-weapons");
    }
    bool apply_vehicle_view(EntityRef, int index) override {
        events.push_back(std::string("apply-view:") + std::to_string(index));
        return true;
    }
    bool set_vehicle_headlights(bool enabled) override {
        if (reject_headlights) return false;
        headlights = enabled;
        events.emplace_back(enabled ? "headlights:on" : "headlights:off");
        return true;
    }
    void publish_headlight_state(bool enabled) override {
        published_headlights = enabled;
        events.emplace_back(enabled ? "net-headlights:on" : "net-headlights:off");
    }
    void fire_named_input(std::string_view target, std::string_view input, EntityRef) override {
        events.push_back(std::string(target) + ":" + std::string(input));
    }
    bool is_live(EntityRef ref) override {
        if (ref.index == 10) return live_player && ref.generation == 2;
        if (ref.index == 30) return live_camera && ref.generation == 4;
        if (ref.index == 20) return ref.generation == 3;
        return ref.valid();
    }
    std::optional<EntityRef> resolve_named_entity(std::string_view name) override {
        events.push_back(std::string("find-camera:") + std::string(name));
        return named_camera;
    }
    bool is_point_camera(EntityRef) override { return camera_is_point; }
    bool link_monitor_camera(EntityRef camera) override {
        if (reject_link) return false;
        linked_camera = camera;
        events.emplace_back("link-camera");
        return true;
    }
    void unlink_monitor_camera(EntityRef camera) override {
        assert(linked_camera && linked_camera->index == camera.index);
        linked_camera.reset();
        events.emplace_back("unlink-camera");
    }
    std::optional<EntityRef> resolve_radio() override { return radio; }
    bool set_radio_power(EntityRef, bool enabled) override {
        radio_power = enabled;
        events.emplace_back(enabled ? "radio:on" : "radio:off");
        return true;
    }
    bool set_radio_station(EntityRef, int station) override {
        radio_station = station;
        events.emplace_back("radio:station");
        return true;
    }
    bool enter_vehicle(std::optional<EntityRef> activator, int argument) override {
        last_activator = activator;
        last_vehicle_argument = argument;
        events.emplace_back("enter-vehicle");
        return true;
    }
    bool exit_vehicle(std::optional<EntityRef> activator, int argument) override {
        last_activator = activator;
        last_vehicle_argument = argument;
        events.emplace_back("exit-vehicle");
        return true;
    }
};

InputValue void_value() { return {std::monostate{}, std::nullopt}; }
InputValue int_value(int n, std::optional<EntityRef> activator = std::nullopt) { return {n, activator}; }
InputValue string_value(std::string_view s) { return {s, std::nullopt}; }

void test_automatic_view_transitions_update_view_then_permission_byte() {
    Host host;
    OriginalVehicleRuntime vehicle({1, 1}, {{0x00, 1.0f}, {0xA5, 0.75f}});

    assert(vehicle.apply_automatic_view_transition(
        EntityRef{10, 2}, 1, AutomaticViewPermission::CopyViewRecordByte, host)
        == Result::Applied);
    assert(host.selected_view == 1);
    assert(host.allow_weapons == 0xA5);
    assert((host.events == std::vector<std::string>{
        "player-view-index", "player-view-weapons"}));

    host.events.clear();
    assert(vehicle.apply_automatic_view_transition(
        EntityRef{10, 2}, 0, AutomaticViewPermission::ForceZero, host)
        == Result::Applied);
    assert(host.selected_view == 0);
    assert(host.allow_weapons == 0);
    assert((host.events == std::vector<std::string>{
        "player-view-index", "player-view-weapons"}));
}

void test_automatic_view_transition_rejects_stale_players_and_bad_indices() {
    Host host;
    OriginalVehicleRuntime vehicle({1, 1}, {{0xFF, 1.0f}});

    assert(vehicle.apply_automatic_view_transition(
        EntityRef{10, 2}, 0, AutomaticViewPermission::CopyViewRecordByte, host)
        == Result::Applied);

    host.events.clear();
    host.live_player = false;
    assert(vehicle.apply_automatic_view_transition(
        EntityRef{10, 2}, 0, AutomaticViewPermission::CopyViewRecordByte, host)
        == Result::MissingActivator);
    assert(host.events.empty());

    host.live_player = true;
    assert(vehicle.apply_automatic_view_transition(
        EntityRef{10, 2}, -1, AutomaticViewPermission::CopyViewRecordByte, host)
        == Result::OutOfRange);
    assert(vehicle.apply_automatic_view_transition(
        EntityRef{10, 2}, 1, AutomaticViewPermission::CopyViewRecordByte, host)
        == Result::OutOfRange);
    assert(host.events.empty());
}

void test_view_damage_stage_is_player_only_and_uses_selected_record() {
    std::vector<ViewRecord> views{{false, 0.5f}, {true, 1.25f}};
    OriginalVehicleRuntime vehicle({1, 1}, views);

    const auto player = vehicle.apply_view_damage_modifier(80.0f, true, 1);
    assert(player.path == DamageViewPath::PlayerViewModifier);
    assert(player.damage == 100.0f);

    const auto non_player = vehicle.apply_view_damage_modifier(80.0f, false, 1);
    assert(non_player.path == DamageViewPath::NonPlayer);
    assert(non_player.damage == 80.0f);
}

void test_view_damage_stage_default_and_invalid_view_fallback() {
    OriginalVehicleRuntime vehicle({1, 1}, {ViewRecord{}});
    assert(vehicle.views().size() == 1);
    assert(vehicle.views()[0].damage_modifier == 1.0f);

    const auto missing = vehicle.apply_view_damage_modifier(42.0f, true, std::nullopt);
    assert(missing.path == DamageViewPath::InvalidViewFallback);
    assert(missing.damage == 42.0f);

    const auto negative = vehicle.apply_view_damage_modifier(42.0f, true, -1);
    assert(negative.path == DamageViewPath::InvalidViewFallback);
    assert(negative.damage == 42.0f);

    const auto too_large = vehicle.apply_view_damage_modifier(42.0f, true, 1);
    assert(too_large.path == DamageViewPath::InvalidViewFallback);
    assert(too_large.damage == 42.0f);
}

void test_view_damage_modifier_is_not_clamped() {
    std::vector<ViewRecord> views{
        {false, 0.0f},
        {false, 1.5f},
        {false, -0.5f}
    };
    OriginalVehicleRuntime vehicle({1, 1}, views);

    assert(vehicle.apply_view_damage_modifier(10.0f, true, 0).damage == 0.0f);
    assert(vehicle.apply_view_damage_modifier(10.0f, true, 1).damage == 15.0f);
    assert(vehicle.apply_view_damage_modifier(10.0f, true, 2).damage == -5.0f);
}


void write_little_endian_float(std::vector<std::uint8_t>& bytes,
                               std::size_t offset, float value) {
    std::uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value), "32-bit float expected");
    std::memcpy(&bits, &value, sizeof(bits));
    bytes.at(offset + 0) = static_cast<std::uint8_t>(bits & 0xffu);
    bytes.at(offset + 1) = static_cast<std::uint8_t>((bits >> 8u) & 0xffu);
    bytes.at(offset + 2) = static_cast<std::uint8_t>((bits >> 16u) & 0xffu);
    bytes.at(offset + 3) = static_cast<std::uint8_t>((bits >> 24u) & 0xffu);
}

void test_original_view_record_decoder_reads_only_verified_member() {
    std::vector<std::uint8_t> raw(200, 0xCD);
    raw[0x55] = 0x00;
    raw[100 + 0x55] = 0x7F;
    write_little_endian_float(raw, 0x60, 0.75f);
    write_little_endian_float(raw, 100 + 0x60, -1.25f);
    std::vector<ViewRecord> decoded{{1, 99.0f}};

    assert(decode_original_view_records(raw.data(), raw.size(), 2, decoded));
    assert(decoded.size() == 2);
    assert(decoded[0].allow_weapons == 0x00);
    assert(decoded[1].allow_weapons == 0x7F);
    assert(decoded[0].damage_modifier == 0.75f);
    assert(decoded[1].damage_modifier == -1.25f);
}

void test_original_view_record_decoder_is_bounded_and_transactional() {
    std::vector<std::uint8_t> raw(199, 0);
    std::vector<ViewRecord> decoded{{true, 2.0f}};
    assert(!decode_original_view_records(raw.data(), raw.size(), 2, decoded));
    assert(decoded.size() == 1);
    assert(decoded[0].allow_weapons && decoded[0].damage_modifier == 2.0f);
    assert(!decode_original_view_records(nullptr, 100, 1, decoded));
    assert(decoded.size() == 1);
    assert(decode_original_view_records(nullptr, 0, 0, decoded));
    assert(decoded.empty());
}

void test_original_view_record_decoder_preserves_ieee_payload() {
    std::vector<std::uint8_t> raw(100, 0);
    write_little_endian_float(raw, 0x60, std::numeric_limits<float>::quiet_NaN());
    std::vector<ViewRecord> decoded;
    assert(decode_original_view_records(raw.data(), raw.size(), 1, decoded));
    assert(decoded.size() == 1 && std::isnan(decoded[0].damage_modifier));
}


void test_decoded_view_permission_byte_reaches_set_view_unchanged() {
    std::vector<std::uint8_t> raw(100, 0);
    raw[0x55] = 0xA5;
    write_little_endian_float(raw, 0x60, 1.0f);

    std::vector<ViewRecord> decoded;
    assert(decode_original_view_records(raw.data(), raw.size(), 1, decoded));
    assert(decoded.size() == 1 && decoded[0].allow_weapons == 0xA5);

    Host host;
    OriginalVehicleRuntime vehicle({1, 1}, decoded);
    assert(vehicle.handle_input("SetView", int_value(0, EntityRef{99, 1}), host)
           == Result::Applied);
    assert(host.selected_view == 0);
    assert(host.allow_weapons == 0xA5);
}

void test_input_registry_and_types() {
    assert(original_input_specs().size() == 25);
    assert(find_input_spec("SetCamera") != nullptr);
    assert(find_input_spec("SetCamera")->kind == InputKind::String);
    assert(find_input_spec("OpenDoors")->kind == InputKind::Void);
    assert(find_input_spec("SetView")->kind == InputKind::Integer);
    assert(find_input_spec("not_a_vehicle_input") == nullptr);
    assert((EntityRef{1, 0}.valid()));
    assert((!EntityRef{0, 1}.valid()));

    Host host;
    OriginalVehicleRuntime vehicle({1, 1});
    assert(vehicle.handle_input("WipersOn", int_value(1), host) == Result::RejectedInput);
    assert(vehicle.handle_input("NoSuchInput", void_value(), host) == Result::RejectedInput);
    assert(vehicle.handle_input("WipersOn", void_value(), host) == Result::Applied);
    assert(vehicle.state().wipers_on == true);
    assert(vehicle.handle_input("WipersOff", void_value(), host) == Result::Applied);
    assert(vehicle.state().wipers_on == false);
}

void test_door_and_window_fanout() {
    Host host;
    OriginalVehicleRuntime vehicle({1, 1});
    assert(vehicle.handle_input("OpenDoors", void_value(), host) == Result::Applied);
    assert((host.doors == std::vector<bool>{true, true, true}));
    assert(vehicle.handle_input("CloseWindows", void_value(), host) == Result::Applied);
    assert((host.windows == std::vector<bool>{false, false}));
    assert(vehicle.handle_input("OpenWindow", int_value(1), host) == Result::Applied);
    assert(host.windows[1]);
    assert(vehicle.handle_input("CloseDoor", int_value(-1), host) == Result::OutOfRange);
    assert(vehicle.handle_input("OpenDoor", int_value(3), host) == Result::OutOfRange);

    host.reject_door_index_one = true;
    assert(vehicle.handle_input("CloseDoors", void_value(), host) == Result::HostRejected);
    assert(host.events.back() == "door:2:close");
}

void test_set_view_updates_both_player_fields() {
    Host host;
    std::vector<ViewRecord> views{{false, 1.0f}, {true, 0.75f}};
    OriginalVehicleRuntime vehicle({1, 1}, views);
    assert(vehicle.handle_input("SetView", int_value(0), host) == Result::MissingActivator);
    assert(vehicle.handle_input("SetView", int_value(-1, EntityRef{99, 1}), host) == Result::OutOfRange);
    assert(vehicle.handle_input("SetView", int_value(2, EntityRef{99, 1}), host) == Result::OutOfRange);
    assert(vehicle.handle_input("SetView", int_value(1, EntityRef{99, 1}), host) == Result::Applied);
    assert(host.selected_view == 1);
    assert(host.allow_weapons);
    assert(host.events.back() == "apply-view:1");
    host.player.reset();
    assert(vehicle.handle_input("SetView", int_value(0, EntityRef{99, 1}), host) == Result::MissingActivator);
}

void test_headlights_and_known_client_property() {
    Host host;
    OriginalVehicleRuntime vehicle({1, 1});
    assert(vehicle.handle_input("HeadlightOn", void_value(), host) == Result::Applied);
    assert(vehicle.state().headlights_on == true);
    assert(host.headlights && host.published_headlights);
    host.reject_headlights = true;
    assert(vehicle.handle_input("HeadlightOff", void_value(), host) == Result::HostRejected);
    assert(vehicle.state().headlights_on == true);
}

void test_uninitialized_monitor_state_is_not_guessed() {
    Host host;
    OriginalVehicleRuntime vehicle({1, 1});
    assert(vehicle.handle_input("MonitorOn", void_value(), host) == Result::UninitializedState);
    assert(vehicle.refresh_monitor_camera(host) == Result::UninitializedState);
    assert(host.events.empty());
}

void test_monitor_camera_lifecycle() {
    Host host;
    VehicleState initial;
    initial.monitor_active = false;
    OriginalVehicleRuntime vehicle({1, 1}, {}, initial);
    assert(vehicle.handle_input("SetCamera", string_value("camera_a"), host) == Result::Applied);
    assert(vehicle.state().monitor_camera && *vehicle.state().monitor_camera == "camera_a");
    assert(vehicle.refresh_monitor_camera(host) == Result::Unchanged);
    assert(vehicle.handle_input("MonitorOn", void_value(), host) == Result::Applied);
    assert(vehicle.handle_input("MonitorOn", void_value(), host) == Result::Unchanged);
    assert(host.events.back() == "monitor:open");
    assert(vehicle.refresh_monitor_camera(host) == Result::Applied);
    assert(vehicle.camera_link() && vehicle.camera_link()->index == 30);
    assert(vehicle.handle_input("SetCamera", string_value("camera_b"), host) == Result::Applied);
    assert(!vehicle.camera_link());
    assert(!host.linked_camera);
    assert(vehicle.refresh_monitor_camera(host) == Result::Applied);
    assert(vehicle.handle_input("MonitorOff", void_value(), host) == Result::Applied);
    assert(vehicle.handle_input("MonitorOff", void_value(), host) == Result::Unchanged);
    assert(!vehicle.camera_link());
    const auto close_it = std::find(host.events.begin(), host.events.end(), "monitor:close");
    assert(close_it != host.events.end());
    const auto unlink_it = std::find(close_it, host.events.end(), "unlink-camera");
    assert(unlink_it != host.events.end());
}

void test_camera_failure_clears_name_and_stale_handle_is_not_unlinked() {
    Host host;
    VehicleState initial;
    initial.monitor_active = false;
    OriginalVehicleRuntime vehicle({1, 1}, {}, initial);
    vehicle.handle_input("SetCamera", string_value("not_a_point_camera"), host);
    vehicle.handle_input("MonitorOn", void_value(), host);
    host.camera_is_point = false;
    assert(vehicle.refresh_monitor_camera(host) == Result::WrongEntityType);
    assert(vehicle.state().monitor_camera && vehicle.state().monitor_camera->empty());

    host.camera_is_point = true;
    vehicle.handle_input("SetCamera", string_value("camera_c"), host);
    assert(vehicle.refresh_monitor_camera(host) == Result::Applied);
    host.live_camera = false;
    vehicle.handle_input("SetCamera", string_value("camera_d"), host);
    assert(!vehicle.camera_link());
    host.live_camera = true;
    host.reject_link = true;
    assert(vehicle.refresh_monitor_camera(host) == Result::HostRejected);
    assert(vehicle.state().monitor_camera && vehicle.state().monitor_camera->empty());
}

void test_radio_and_vehicle_entry_are_host_owned() {
    Host host;
    OriginalVehicleRuntime vehicle({1, 1});
    host.radio.reset();
    assert(vehicle.handle_input("RadioOn", void_value(), host) == Result::MissingLinkedEntity);
    host.radio = EntityRef{20, 3};
    assert(vehicle.handle_input("RadioOn", void_value(), host) == Result::Applied);
    assert(host.radio_power);
    assert(vehicle.handle_input("SetRadioStation", int_value(7), host) == Result::Applied);
    assert(host.radio_station == 7);
    assert(vehicle.handle_input("EnterVehicle", int_value(2, EntityRef{99, 1}), host) == Result::Applied);
    assert(host.last_vehicle_argument == 2 && host.last_activator.has_value());
    assert(vehicle.handle_input("ExitVehicle", int_value(1), host) == Result::Applied);
    assert(host.last_vehicle_argument == 1 && !host.last_activator.has_value());
}

void test_integer_flags_keep_original_field_meaning() {
    Host host;
    OriginalVehicleRuntime vehicle({1, 1});
    assert(vehicle.handle_input("EnableUse", int_value(0), host) == Result::Applied);
    assert(vehicle.state().enable_use == false);
    assert(vehicle.handle_input("LockView", int_value(1), host) == Result::Applied);
    assert(vehicle.state().views_locked == true);
    assert(vehicle.handle_input("EnableAmmo", int_value(1), host) == Result::Applied);
    assert(vehicle.state().infinite_ammo == true);
    assert(vehicle.handle_input("EnableHints", int_value(1), host) == Result::Applied);
    assert(vehicle.state().hints_enabled == true);
}
} // namespace

int main() {
    test_automatic_view_transitions_update_view_then_permission_byte();
    test_automatic_view_transition_rejects_stale_players_and_bad_indices();
    test_view_damage_stage_is_player_only_and_uses_selected_record();
    test_view_damage_stage_default_and_invalid_view_fallback();
    test_view_damage_modifier_is_not_clamped();
    test_original_view_record_decoder_reads_only_verified_member();
    test_original_view_record_decoder_is_bounded_and_transactional();
    test_original_view_record_decoder_preserves_ieee_payload();
    test_decoded_view_permission_byte_reaches_set_view_unchanged();
    test_input_registry_and_types();
    test_door_and_window_fanout();
    test_set_view_updates_both_player_fields();
    test_headlights_and_known_client_property();
    test_uninitialized_monitor_state_is_not_guessed();
    test_monitor_camera_lifecycle();
    test_camera_failure_clears_name_and_stale_handle_is_not_unlinked();
    test_radio_and_vehicle_entry_are_host_owned();
    test_integer_flags_keep_original_field_meaning();
    std::cout << "PASS: original SiN vehicle runtime inputs, view permissions and damage modifier (mock host only)" << std::endl;
}
