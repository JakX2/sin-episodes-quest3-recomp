#include "sin_compat/difficulty_checkpoint_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
using namespace sin_compat;
struct Host final : IDifficultyCheckpointHost {
    std::map<std::uint32_t, std::uint32_t> generations;
    EntityRef add(std::uint32_t id, std::uint32_t generation) {
        generations[id] = generation;
        return {id, generation};
    }
    bool is_live(EntityRef e) const override {
        auto it = generations.find(e.index);
        return it != generations.end() && it->second == e.generation;
    }
};
int main() {
    Host host;
    const auto a = host.add(1, 8);
    const auto b = host.add(2, 1);
    const auto c = host.add(3, 1);
    DifficultyCheckpointRuntime runtime(host);
    assert(runtime.register_checkpoint(a, {150.0, 2, false, false}));
    assert(runtime.register_checkpoint(b, {90.0, 3, false, false}));
    assert(runtime.register_checkpoint(c, {60.0, 1, true, true}));
    assert(runtime.registered_count() == 3);
    assert(!runtime.register_checkpoint(a, {})); // Duplicate generation.
    auto first = runtime.touch(a, 160.0);
    assert(first && first.observation.checkpoint == a);
    assert(first.observation.actual_segment_seconds == 160.0);
    assert(first.observation.expected_segment_seconds == 150.0);
    assert(first.observation.upcoming_difficulty == 2);
    assert(!first.observation.end_of_game);
    assert(runtime.touch(a, 170.0).result == CheckpointResult::AlreadyTriggered);
    assert(runtime.last_checkpoint_time() == 160.0);
    assert(runtime.touch(b, 159.9).result == CheckpointResult::InvalidTime);
    assert(runtime.touch(b, INFINITY).result == CheckpointResult::InvalidTime);
    assert(runtime.touch(b, NAN).result == CheckpointResult::InvalidTime);
    assert(runtime.touch(b, -1).result == CheckpointResult::InvalidTime);
    auto second = runtime.touch(b, 220.0);
    assert(second && second.observation.actual_segment_seconds == 60.0);
    assert(second.observation.upcoming_difficulty == 3);
    auto end = runtime.touch(c, 280.0);
    assert(end && end.observation.end_of_game);
    assert(end.observation.expected_segment_seconds == 60.0);
    assert(end.observation.actual_segment_seconds == 60.0);
    assert(runtime.touch(c, 310.0)); // Explicit synthetic repeatable option, not FGD parity.
    assert(runtime.last_checkpoint_time() == 310.0);
    assert(runtime.touch({9, 8}, 311).result == CheckpointResult::UnknownOrStaleEntity);
    host.generations[2] = 2;
    assert(runtime.touch(b, 320.0).result == CheckpointResult::UnknownOrStaleEntity);
    assert(!runtime.register_checkpoint(b, {90,2,false,false}));
    const auto b2 = EntityRef{2,2};
    assert(runtime.register_checkpoint(b2, {45, 4, false, false}));
    assert(runtime.touch(b2, 325.0));
    runtime.remove_checkpoint(b);
    assert(runtime.registered_count() == 3);
    runtime.remove_checkpoint(b2);
    assert(runtime.registered_count() == 2);
    assert(runtime.touch(b2, 330.0).result == CheckpointResult::UnknownOrStaleEntity);
    DifficultyCheckpointRuntime offset(host, 42.0);
    assert(offset.register_checkpoint(a, {}));
    assert(offset.touch(a, 50.0).observation.actual_segment_seconds == 8.0);
    bool bad = false;
    try { DifficultyCheckpointRuntime invalid(host, INFINITY); }
    catch (const std::invalid_argument&) { bad = true; }
    assert(bad);
    bad = false;
    try { DifficultyCheckpointRuntime invalid(host, -0.5); }
    catch (const std::invalid_argument&) { bad = true; }
    assert(bad);
    assert(!runtime.register_checkpoint(a, {NAN,2,false,false}));
    assert(!runtime.register_checkpoint(a, {-1,2,false,false}));
    assert(!runtime.register_checkpoint(a, {0,-1,false,false}));
    assert(!runtime.register_checkpoint(a, {0,5,false,false}));
    assert(!runtime.register_checkpoint(EntityRef{}, {}));
    assert(adjust_spawn_count(3, 0.4, 0.0).count == 4);
    assert(adjust_spawn_count(3, 0.4, 0.399).change == SpawnChange::AddedOne);
    assert(adjust_spawn_count(3, 0.4, 0.4).change == SpawnChange::AddedOne);
    assert(adjust_spawn_count(3, 0.4, 0.4000001).change == SpawnChange::Unchanged);
    assert(adjust_spawn_count(3, -0.1, 0.099).count == 2);
    assert(adjust_spawn_count(3, -0.1, 0.1).count == 2);
    assert(adjust_spawn_count(3, -0.1, 0.100001).count == 3);
    assert(adjust_spawn_count(0, -1.0, 0.5).count == 0);
    assert(adjust_spawn_count(std::numeric_limits<int>::max(), 1, 0).change == SpawnChange::Unchanged);
    assert(adjust_spawn_count(3, 0.0, 0.1).count == 3);
    assert(adjust_spawn_count(3, 1.0, 0.9999999).count == 4);
    for (auto bad_mod : {double(NAN), double(INFINITY), -double(INFINITY), -1.01, 1.01})
        assert(adjust_spawn_count(3, bad_mod, 0.5).change == SpawnChange::InvalidInput);
    for (auto roll : {double(NAN), double(INFINITY), -0.1, 1.0})
        assert(adjust_spawn_count(3, 0.4, roll).change == SpawnChange::InvalidInput);
    assert(adjust_spawn_count(-1, 0.4, 0).change == SpawnChange::InvalidInput);
    return 0;
}
