#include "sin_compat/grunt_tactical_controls.hpp"
#include <cmath>
#include <utility>

namespace sin_compat {
namespace {
bool valid_grenade_count(int count) {
    return (count >= 0 && count <= 10) || count == kGruntUnlimitedGrenades;
}
bool parse_fgd_bool(const std::string& text, bool& result) {
    if (text == "1" || text == "true" || text == "True") {
        result = true;
        return true;
    }
    if (text == "0" || text == "false" || text == "False") {
        result = false;
        return true;
    }
    return false;
}
} // namespace

std::string grunt_default_equipment(const std::string& cls) {
    if (cls == "npc_sintek_grunt" || cls == "npc_druglab_grunt_assault_rifle")
        return "weapon_assault_rifle";
    if (cls == "npc_druglab_grunt_pistol") return "weapon_npc_pistol";
    if (cls == "npc_druglab_grunt_scattergun") return "weapon_scattergun";
    if (cls == "npc_druglab_grunt_elite") return "weapon_chaingun";
    return {};
}
bool grunt_equipment_allowed(const std::string& cls, const std::string& weapon) {
    if (grunt_default_equipment(cls).empty()) return false;
    if (cls == "npc_sintek_grunt") {
        return weapon == "weapon_assault_rifle" || weapon == "weapon_scattergun" ||
               weapon == "weapon_npc_pistol";
    }
    return grunt_default_equipment(cls) == weapon;
}

bool GruntTacticalControls::register_grunt(EntityRef grunt, GruntDefinition def) {
    if (!host_.is_live(grunt) || !valid_grenade_count(def.num_grenades) ||
        !std::isfinite(def.grenade_height) || def.grenade_height < 0.0)
        return false;
    if (def.equipment.empty()) def.equipment = grunt_default_equipment(def.classname);
    if (!grunt_equipment_allowed(def.classname, def.equipment)) return false;
    State state;
    state.look_off = (def.spawnflags & kGruntStartLookOff) != 0;
    state.remaining = def.num_grenades;
    state.definition = std::move(def);
    return states_.emplace(grunt, std::move(state)).second;
}
void GruntTacticalControls::remove_grunt(EntityRef grunt) {
    states_.erase(grunt);
}
GruntCommandResult GruntTacticalControls::apply_input(
    EntityRef grunt, const std::string& name, const std::string& argument) {
    auto it = states_.find(grunt);
    if (it == states_.end() || !host_.is_live(grunt)) return GruntCommandResult::InvalidGrunt;
    State& state = it->second;
    bool value = false;
    bool* target = nullptr;
    if (name == "LookOn") {
        if (!argument.empty()) return GruntCommandResult::InvalidParameter;
        target = &state.look_off;
        value = false;
    } else if (name == "LookOff") {
        if (!argument.empty()) return GruntCommandResult::InvalidParameter;
        target = &state.look_off;
        value = true;
    } else if (name == "StartPatrolling") {
        if (!argument.empty()) return GruntCommandResult::InvalidParameter;
        target = &state.patrol_enabled;
        value = true;
    } else if (name == "StopPatrolling") {
        if (!argument.empty()) return GruntCommandResult::InvalidParameter;
        target = &state.patrol_enabled;
        value = false;
    } else if (name == "ThrowGrenades") {
        if (!parse_fgd_bool(argument, value)) return GruntCommandResult::InvalidParameter;
        target = &state.aggressive_grenades;
    } else {
        return GruntCommandResult::UnknownInput;
    }
    if (*target == value) return GruntCommandResult::NoChange;
    *target = value;
    return GruntCommandResult::Applied;
}
GruntTacticalSnapshot GruntTacticalControls::snapshot(EntityRef grunt,
                                                      GruntAlertState alert) const {
    const auto it = states_.find(grunt);
    if (it == states_.end() || !host_.is_live(grunt)) return {};
    const auto& s = it->second;
    GruntTacticalSnapshot out;
    out.valid = true;
    out.sight = s.look_off ? GruntSightPreference::SquadEyes : GruntSightPreference::OwnEyes;
    out.patrol_requested = s.patrol_enabled &&
        (alert == GruntAlertState::Idle || alert == GruntAlertState::Alert);
    out.aggressive_grenades_requested = s.aggressive_grenades;
    out.secondary_fire_allowed = !(s.definition.spawnflags & kGruntDontSecondaryFire);
    out.unlimited_grenades = s.remaining == kGruntUnlimitedGrenades;
    out.grenades_available = out.unlimited_grenades || s.remaining > 0;
    out.grenades_remaining = s.remaining;
    out.grenade_height = s.definition.grenade_height;
    out.equipment = s.definition.equipment;
    return out;
}
GruntCommandResult GruntTacticalControls::report_grenade_thrown(EntityRef grunt) {
    auto it = states_.find(grunt);
    if (it == states_.end() || !host_.is_live(grunt)) return GruntCommandResult::InvalidGrunt;
    if (it->second.remaining == 0) return GruntCommandResult::NoGrenades;
    if (it->second.remaining == kGruntUnlimitedGrenades)
        return GruntCommandResult::Applied;
    --it->second.remaining;
    return GruntCommandResult::Applied;
}
} // namespace sin_compat
