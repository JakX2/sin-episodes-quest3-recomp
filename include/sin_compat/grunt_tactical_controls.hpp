#pragma once
// Independently authored, Source ABI-free tactical *input state* for SiN grunts.
// FGD/DLL verified names only; this is NOT the original AI decision scheduler.
#include "sin_compat/weapon_entity_io.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace sin_compat {
constexpr std::uint32_t kGruntStartLookOff = 65536u;
constexpr std::uint32_t kGruntDontSecondaryFire = 16777216u;
constexpr int kGruntUnlimitedGrenades = 999999;

enum class GruntAlertState { Idle, Alert, Combat };
enum class GruntSightPreference { OwnEyes, SquadEyes };
enum class GruntCommandResult {
    Applied, NoChange, InvalidGrunt, InvalidParameter, UnknownInput, NoGrenades
};

struct GruntDefinition {
    std::string classname;
    std::string equipment;      // empty: use SDK FGD default
    std::uint32_t spawnflags = 0;
    int num_grenades = 5;       // original BaseGrunt FGD default
    double grenade_height = 0; // original FGD: advisory height threshold
};

struct GruntTacticalSnapshot {
    bool valid = false;
    GruntSightPreference sight = GruntSightPreference::OwnEyes;
    bool patrol_requested = false; // only in Idle / Alert states
    bool aggressive_grenades_requested = false;
    bool secondary_fire_allowed = true;
    bool unlimited_grenades = false;
    bool grenades_available = false;
    int grenades_remaining = 0; // 999999 sentinel if unlimited
    double grenade_height = 0.0;
    std::string equipment;
};

// These values are SDK FGD editor choices, not a recovered game AI table.
std::string grunt_default_equipment(const std::string& classname);
bool grunt_equipment_allowed(const std::string& classname, const std::string& equipment);

class IGruntTacticalHost {
public:
    virtual ~IGruntTacticalHost() = default;
    virtual bool is_live(EntityRef grunt) const = 0;
};

class GruntTacticalControls final {
public:
    explicit GruntTacticalControls(IGruntTacticalHost& host) : host_(host) {}
    bool register_grunt(EntityRef grunt, GruntDefinition definition);
    void remove_grunt(EntityRef grunt);

    // FGD names: LookOn, LookOff, StartPatrolling, StopPatrolling,
    // ThrowGrenades(bool). No actual AI scheduler or engine input bindings.
    GruntCommandResult apply_input(EntityRef grunt, const std::string& name,
                                   const std::string& argument = {});
    GruntTacticalSnapshot snapshot(EntityRef grunt, GruntAlertState state) const;
    // A host must report a *successful* grenade throw to decrement inventory.
    GruntCommandResult report_grenade_thrown(EntityRef grunt);
    std::size_t grunt_count() const noexcept { return states_.size(); }
private:
    struct RefLess {
        bool operator()(EntityRef a, EntityRef b) const noexcept {
            return a.index != b.index ? a.index < b.index : a.generation < b.generation;
        }
    };
    struct State {
        GruntDefinition definition;
        bool look_off = false;
        bool patrol_enabled = false;
        bool aggressive_grenades = false;
        int remaining = 0;
    };
    IGruntTacticalHost& host_;
    std::map<EntityRef, State, RefLess> states_;
};
} // namespace sin_compat
