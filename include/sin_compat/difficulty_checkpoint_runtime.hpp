#pragma once
// Portable, independently authored encounter checkpoint and spawn-count
// primitives. Original FGD/map values are evidence, NOT verified Ritual code.
#include "sin_compat/weapon_entity_io.hpp"
#include <cstddef>
#include <map>
namespace sin_compat {
struct DifficultyCheckpointConfig {
    double estimated_time_seconds = 0.0;
    int estimated_difficulty = 2;  // SDK FGD choices: 0..4.
    bool end_of_game = false;
    bool repeatable = false;       // Synthetic host option; NOT known to equal FGD 1024.
};
struct DifficultyCheckpointObservation {
    EntityRef checkpoint{};
    double at_seconds = 0.0;
    double actual_segment_seconds = 0.0;
    double expected_segment_seconds = 0.0;
    int upcoming_difficulty = 2;
    bool end_of_game = false;
};
enum class CheckpointResult {
    Accepted, InvalidTime, UnknownOrStaleEntity, AlreadyTriggered,
};
struct CheckpointTouch {
    CheckpointResult result = CheckpointResult::UnknownOrStaleEntity;
    DifficultyCheckpointObservation observation{};
    explicit operator bool() const noexcept { return result == CheckpointResult::Accepted; }
};
class IDifficultyCheckpointHost {
public:
    virtual ~IDifficultyCheckpointHost() = default;
    virtual bool is_live(EntityRef checkpoint) const = 0;
};
class DifficultyCheckpointRuntime final {
public:
    // A new instance represents a new map/session; the host owns all entities.
    explicit DifficultyCheckpointRuntime(IDifficultyCheckpointHost& host,
                                         double map_start = 0.0);
    bool register_checkpoint(EntityRef entity, DifficultyCheckpointConfig cfg);
    void remove_checkpoint(EntityRef entity);
    CheckpointTouch touch(EntityRef entity, double at_seconds);
    std::size_t registered_count() const noexcept { return states_.size(); }
    double last_checkpoint_time() const noexcept { return last_time_; }
private:
    struct KeyLess { bool operator()(EntityRef a, EntityRef b) const noexcept; };
    struct CheckpointState {
        DifficultyCheckpointConfig config;
        bool triggered = false;
    };
    IDifficultyCheckpointHost& host_;
    double last_time_;
    std::map<EntityRef, CheckpointState, KeyLess> states_;
};
enum class SpawnChange { Unchanged, AddedOne, RemovedOne, InvalidInput };
struct SpawnCountDecision {
    int count = 0;
    SpawnChange change = SpawnChange::InvalidInput;
    explicit operator bool() const noexcept { return change != SpawnChange::InvalidInput; }
};
// Testable interpretation of the historical CSE1Rules +/-one probability
// evidence. Host supplies a random roll [0, 1); this does not implement
// challenge adaptation, the real engine RNG, or an original NPC maker.
SpawnCountDecision adjust_spawn_count(int requested_count, double modifier,
                                      double random_roll) noexcept;
} // namespace sin_compat
