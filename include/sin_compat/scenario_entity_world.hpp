#pragma once

// Minimal, independently authored Source-style scenario entity implementation
// for the original SiN Episodes entity I/O contracts. THIS IS NOT an exported
// Source game server factory or a replacement for the original Windows DLL.
//
// Real hosts must map their engine entity handles and collision/movement
// callbacks to this module. Doors are not visually/physically animated here.

#include "sin_compat/entity_io_runtime.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace sin_compat {

struct ScenarioEntitySpec {
    std::string classname;
    std::string targetname;
    // Preserve duplicate On... keys and order from a BSP entity lump.
    std::vector<std::pair<std::string, std::string>> keyvalues;
};

enum class ScenarioEntityKind { LogicAuto, LogicRelay, TriggerOnce, FuncDoor };
enum class ScenarioDoorPhase { Closed, Opening, Open, Closing };

struct ScenarioEntitySnapshot {
    ScenarioEntityKind kind = ScenarioEntityKind::LogicAuto;
    std::string targetname;
    bool disabled = false;
    bool locked = false;
    ScenarioDoorPhase door_phase = ScenarioDoorPhase::Closed;
};

struct ScenarioSpawnResult {
    EntityRef entity{};
    std::string error;
    explicit operator bool() const { return error.empty(); }
};

// Called only to request actual engine movement. The host must call
// door_motion_completed() when a real door reaches its endpoint.
class IScenarioDoorMotionHost {
public:
    virtual ~IScenarioDoorMotionHost() = default;
    virtual void request_door_motion(EntityRef door, bool opening,
                                     EntityRef activator) = 0;
};

class ScenarioEntityWorld final : public IEntityOutputHost {
public:
    explicit ScenarioEntityWorld(IScenarioDoorMotionHost& movement);

    ScenarioSpawnResult spawn(const ScenarioEntitySpec& spec);
    bool remove(EntityRef entity);
    bool snapshot(EntityRef entity, ScenarioEntitySnapshot& out) const;

    // Emulates the event the actual engine emits after map entity spawning.
    // Only first activation dispatches OnMapSpawn. No map loading is performed.
    bool activate_map(EntityRef activator = {});

    // Invoke from native game/physics hooks. Not an automatic trigger detector.
    bool touch_once(EntityRef trigger, EntityRef activator);

    // Run a named input through a registered entity in this portable world.
    bool input(EntityRef target, const std::string& input_name,
               const std::string& parameter = {}, EntityRef activator = {},
               EntityRef caller = {});

    // Only engine-confirmed door movement may complete these transitions.
    bool door_motion_completed(EntityRef door, bool opened,
                               EntityRef activator = {});

    EntityIoDrain advance_to(double seconds, std::size_t event_budget = 1024);
    std::size_t pending_events() const { return io_.pending_events(); }
    double now() const { return io_.now(); }

    // IEntityOutputHost: can later be replaced with a real Source host adapter.
    bool is_live(EntityRef entity) const override;
    std::vector<EntityRef> resolve_targets(const std::string& target,
                                            EntityRef activator,
                                            EntityRef caller) override;
    void accept_input(EntityRef target, const std::string& input_name,
                      const std::string& parameter, EntityRef activator,
                      EntityRef caller) override;

private:
    struct Slot {
        std::uint32_t generation = 1;
        bool alive = false;
        bool trigger_consumed = false;
        ScenarioEntitySnapshot state{};
    };
    Slot* lookup(EntityRef entity);
    const Slot* lookup(EntityRef entity) const;
    bool emit(EntityRef source, const char* output, EntityRef activator);
    bool command_door(EntityRef door, bool opening, EntityRef activator);

    IScenarioDoorMotionHost& movement_;
    EntityIoRuntime io_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_slots_;
    bool map_activated_ = false;
};

} // namespace sin_compat
