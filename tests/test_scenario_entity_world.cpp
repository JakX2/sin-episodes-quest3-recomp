#include "sin_compat/scenario_entity_world.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

using namespace sin_compat;

struct FakeMotion final : IScenarioDoorMotionHost {
    std::vector<std::tuple<EntityRef, bool, EntityRef>> commands;
    void request_door_motion(EntityRef door, bool opening,
                             EntityRef activator) override {
        commands.emplace_back(door, opening, activator);
    }
};

static ScenarioEntitySpec entity(const char* type, const char* name,
                                 std::vector<std::pair<std::string,std::string>> kv = {}) {
    return {type, name, std::move(kv)};
}

static EntityRef must_spawn(ScenarioEntityWorld& world, const ScenarioEntitySpec& spec) {
    auto result = world.spawn(spec);
    assert(result && result.error.empty());
    assert(world.is_live(result.entity));
    return result.entity;
}

int main() {
    FakeMotion motion;
    ScenarioEntityWorld world(motion);

    // Exact FGD names in original SiN SDK; field-value fixtures are original-free.
    const auto automatic = must_spawn(world, entity("logic_auto", "auto",
        {{"OnMapSpawn", "RELAY,Trigger,,0,-1"}}));
    const auto relay = must_spawn(world, entity("logic_relay", "ReLaY",
        {{"OnTrigger", "door,Open,,0.25,1"},
         {"OnTrigger", "door,Open,,0.25,0"}}));
    const auto door = must_spawn(world, entity("func_door", "Door",
        {{"OnFullyOpen", "second,Trigger,,0.1,-1"},
         {"OnFullyClosed", "second,Trigger,,0.1,-1"}}));
    const auto second = must_spawn(world, entity("logic_relay", "second"));
    const auto touch = must_spawn(world, entity("trigger_once", "one_shot",
        {{"OnStartTouch", "relay,Trigger,,0,-1"}}));
    const auto actor = must_spawn(world, entity("logic_relay", "actor"));

    assert(world.activate_map(actor));
    assert(!world.activate_map(actor));
    assert(world.pending_events() == 1);
    auto fired = world.advance_to(0);
    assert(fired.events_processed == 1 && fired.inputs_delivered == 1);
    assert(world.pending_events() == 1); // delayed door Open
    assert(world.advance_to(0.24).inputs_delivered == 0);
    assert(world.advance_to(0.25).inputs_delivered == 1);
    assert(motion.commands.size() == 1);
    assert(std::get<0>(motion.commands[0]) == door);
    assert(std::get<1>(motion.commands[0]));
    assert(std::get<2>(motion.commands[0]) == actor);
    ScenarioEntitySnapshot snap;
    assert(world.snapshot(door,snap));
    assert(snap.door_phase == ScenarioDoorPhase::Opening);
    // No OnFullyOpen is fired until a physical host confirms arrival.
    assert(world.pending_events() == 0);
    assert(!world.door_motion_completed(door, false, actor));
    assert(world.door_motion_completed(door, true, actor));
    assert(!world.door_motion_completed(door, true, actor));
    assert(world.pending_events() == 1);
    assert(world.advance_to(0.349).inputs_delivered == 0);
    assert(world.advance_to(0.35).inputs_delivered == 1);
    assert(world.snapshot(door,snap) && snap.door_phase == ScenarioDoorPhase::Open);

    // A consumed finite OnTrigger connection does not queue again.
    assert(world.input(relay, "Trigger", {}, actor, automatic));
    assert(world.pending_events() == 0);

    // Touch trigger does not destroy its source before delayed events deliver,
    // and cannot be touched again after the first successful activation.
    assert(world.touch_once(touch, actor));
    assert(!world.touch_once(touch, actor));
    assert(world.advance_to(0.35).inputs_delivered == 1);
    assert(world.pending_events() == 0);
    assert(world.snapshot(touch,snap) && snap.disabled);
    assert(world.input(touch, "Enable"));
    // Enabling a consumed trigger_once must never rearm its one-shot touch.
    assert(!world.touch_once(touch, actor));

    // Native door controls use explicit movement request & acknowledgement.
    assert(world.input(door,"Lock"));
    assert(!world.input(door,"Close",{},actor));
    assert(world.input(door,"Unlock"));
    assert(world.input(door,"Close",{},actor));
    assert(motion.commands.size() == 2 && !std::get<1>(motion.commands[1]));
    assert(world.snapshot(door,snap) && snap.door_phase == ScenarioDoorPhase::Closing);
    assert(world.door_motion_completed(door,false,actor));
    assert(world.advance_to(0.45).inputs_delivered == 1);
    assert(world.snapshot(door,snap) && snap.door_phase == ScenarioDoorPhase::Closed);

    // CancelPending should preserve links: future Trigger still schedules.
    const auto pending_relay = must_spawn(world, entity("logic_relay","pending",
        {{"OnTrigger","door,Open,,3,-1"}}));
    assert(world.input(pending_relay, "Trigger",{},actor));
    assert(world.pending_events() == 1);
    assert(world.input(pending_relay,"CancelPending"));
    assert(world.pending_events() == 0);
    assert(world.input(pending_relay,"Trigger",{},actor));
    assert(world.pending_events() == 1);
    assert(world.input(pending_relay,"Disable"));
    assert(!world.input(pending_relay,"Trigger",{},actor));
    assert(world.input(pending_relay,"Enable"));
    assert(world.remove(pending_relay));
    assert(world.pending_events() == 0);

    // Case-insensitive target names and wildcard expansion are deterministic.
    assert(world.resolve_targets("rElAy",{},{}).size() == 1);
    assert(world.resolve_targets("*",{},{}).size() >= 6);
    assert(world.resolve_targets("!activator",actor,{}).at(0) == actor);
    assert(world.resolve_targets("!caller",{},relay).at(0) == relay);
    assert(world.resolve_targets("!player",actor,{}).empty());

    // Bad output specifications reject the entire spawn, not partial links.
    const auto invalid = world.spawn(entity("logic_relay","invalid",
        {{"OnTrigger","door,Open,,nan,-1"}}));
    assert(!invalid && !world.is_live(invalid.entity));
    assert(!world.spawn(entity("arbitrary_original_entity","what")));
    assert(!world.spawn(entity("logic_relay","bad",{{"StartDisabled","abc"}})));
    assert(!world.spawn(entity("logic_auto","bad",{{"classname","npc_sintek_grunt"}})));

    // Generation protection on actual slot reuse.
    const auto first = must_spawn(world,entity("logic_relay","recycled"));
    assert(world.remove(first));
    assert(!world.is_live(first));
    const auto reused = must_spawn(world,entity("logic_relay","reused"));
    assert(first.index == reused.index && first.generation != reused.generation);
    assert(!world.input(first,"Trigger"));
    assert(world.input(reused,"Disable"));

    // Invalid time should never drain pending or move backwards.
    assert(world.advance_to(NAN).invalid_time);
    assert(world.advance_to(0.01).invalid_time);
    assert(world.now() == 0.45);
    assert(!world.input(EntityRef{},"Open"));
    assert(!world.door_motion_completed(relay,true,actor));
    return 0;
}
