#include "sin_compat/difficulty_checkpoint_runtime.hpp"
#include "sin_compat/entity_io_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
using namespace sin_compat;
struct EncounterHost final : IEntityOutputHost, IDifficultyCheckpointHost {
    std::map<std::uint32_t, std::uint32_t> generation;
    std::map<std::string, EntityRef> names;
    EntityIoRuntime* io = nullptr;
    DifficultyCheckpointRuntime* checkpoints = nullptr;
    EntityRef relay{}, checkpoint{}, maker{}, player{};
    int spawned = 0, enabled = 0, checkpoint_signals = 0;
    std::vector<DifficultyCheckpointObservation> observations;
    EntityRef add(std::uint32_t id, const std::string& name) {
        generation[id] = 1;
        EntityRef out{id, 1};
        names[name] = out;
        return out;
    }
    bool is_live(EntityRef e) const override {
        auto it = generation.find(e.index);
        return it != generation.end() && it->second == e.generation;
    }
    std::vector<EntityRef> resolve_targets(const std::string& name,
                                            EntityRef activator, EntityRef caller) override {
        if (name == "!activator") return {activator};
        if (name == "!caller") return {caller};
        auto it = names.find(name);
        return it == names.end() ? std::vector<EntityRef>{} : std::vector<EntityRef>{it->second};
    }
    void accept_input(EntityRef target, const std::string& input,
                      const std::string&, EntityRef activator, EntityRef) override {
        if (target == relay && input == "Trigger") {
            assert(io->fire(relay, "OnTrigger", activator) == 1);
        } else if (target == checkpoint && input == "ReachCheckpoint") {
            const auto visit = checkpoints->touch(checkpoint, io->now());
            assert(visit && visit.observation.upcoming_difficulty == 3);
            observations.push_back(visit.observation);
            ++checkpoint_signals;
            assert(io->fire(checkpoint, "SyntheticReached", activator) == 1);
        } else if (target == maker && input == "Enable") {
            ++enabled;
            const auto count = adjust_spawn_count(2, 0.4, 0.15);
            assert(count && count.change == SpawnChange::AddedOne);
            spawned += count.count; // Fake host only, not original SinTEK NPC logic.
        } else {
            assert(false && "unexpected synthetic map input");
        }
    }
};
int main() {
    EncounterHost host;
    auto trigger = host.add(10, "trigger");
    host.relay = host.add(11, "relay");
    host.checkpoint = host.add(12, "difficulty");
    host.maker = host.add(13, "maker");
    host.player = host.add(14, "player");
    EntityIoRuntime io(host);
    DifficultyCheckpointRuntime checkpoints(host);
    host.io = &io;
    host.checkpoints = &checkpoints;
    assert(checkpoints.register_checkpoint(host.checkpoint, {150,3,false,false}));
    // Synthetic event key names: no claim that original SiN links are identical.
    assert(io.connect(trigger, "OnStartTouch", {"relay", "Trigger", "", 0, 1}));
    assert(io.connect(host.relay, "OnTrigger",
                      {"difficulty", "ReachCheckpoint", "", 0.25, 1}));
    assert(io.connect(host.checkpoint, "SyntheticReached",
                      {"maker", "Enable", "", 0.1, 1}));
    assert(io.fire(trigger, "OnStartTouch", host.player) == 1);
    assert(io.advance_to(0).inputs_delivered == 1);
    assert(io.advance_to(0.249).inputs_delivered == 0);
    assert(io.advance_to(0.25).inputs_delivered == 1);
    assert(host.observations.size() == 1);
    assert(host.observations.front().actual_segment_seconds == 0.25);
    assert(host.observations.front().expected_segment_seconds == 150);
    assert(io.advance_to(0.35).inputs_delivered == 1);
    assert(host.spawned == 3 && host.enabled == 1 && host.checkpoint_signals == 1);
    assert(io.pending_events() == 0);
    assert(io.fire(trigger, "OnStartTouch", host.player) == 0);
    host.generation[12] = 2;
    assert(checkpoints.touch(host.checkpoint, 1.0).result ==
           CheckpointResult::UnknownOrStaleEntity);
    return 0;
}
