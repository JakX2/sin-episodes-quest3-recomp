#include "sin_compat/difficulty_checkpoint_runtime.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
namespace sin_compat {
bool DifficultyCheckpointRuntime::KeyLess::operator()(EntityRef a, EntityRef b) const noexcept {
    return a.index != b.index ? a.index < b.index : a.generation < b.generation;
}
DifficultyCheckpointRuntime::DifficultyCheckpointRuntime(IDifficultyCheckpointHost& host,
                                                         double map_start)
    : host_(host), last_time_(map_start) {
    if (!std::isfinite(map_start) || map_start < 0.0)
        throw std::invalid_argument("invalid synthetic map start time");
}
bool DifficultyCheckpointRuntime::register_checkpoint(EntityRef entity,
                                                       DifficultyCheckpointConfig cfg) {
    if (!host_.is_live(entity) || !std::isfinite(cfg.estimated_time_seconds) ||
        cfg.estimated_time_seconds < 0.0 || cfg.estimated_difficulty < 0 ||
        cfg.estimated_difficulty > 4) return false;
    return states_.emplace(entity, CheckpointState{cfg, false}).second;
}
void DifficultyCheckpointRuntime::remove_checkpoint(EntityRef entity) {
    states_.erase(entity);
}
CheckpointTouch DifficultyCheckpointRuntime::touch(EntityRef entity, double at_seconds) {
    if (!std::isfinite(at_seconds) || at_seconds < last_time_)
        return {CheckpointResult::InvalidTime, {}};
    const auto it = states_.find(entity);
    if (it == states_.end() || !host_.is_live(entity))
        return {CheckpointResult::UnknownOrStaleEntity, {}};
    if (it->second.triggered && !it->second.config.repeatable)
        return {CheckpointResult::AlreadyTriggered, {}};
    auto& state = it->second;
    const auto& cfg = state.config;
    DifficultyCheckpointObservation event{entity, at_seconds, at_seconds - last_time_,
                                          cfg.estimated_time_seconds,
                                          cfg.estimated_difficulty, cfg.end_of_game};
    state.triggered = true;
    last_time_ = at_seconds;
    return {CheckpointResult::Accepted, event};
}
SpawnCountDecision adjust_spawn_count(int requested_count, double modifier,
                                      double random_roll) noexcept {
    if (requested_count < 0 || !std::isfinite(modifier) ||
        modifier < -1.0 || modifier > 1.0 || !std::isfinite(random_roll) ||
        random_roll < 0.0 || random_roll >= 1.0)
        return {requested_count, SpawnChange::InvalidInput};
    if (modifier == 0.0 || random_roll > std::abs(modifier))
        return {requested_count, SpawnChange::Unchanged};
    if (modifier > 0.0 && requested_count < std::numeric_limits<int>::max())
        return {requested_count + 1, SpawnChange::AddedOne};
    if (modifier < 0.0 && requested_count > 0)
        return {requested_count - 1, SpawnChange::RemovedOne};
    return {requested_count, SpawnChange::Unchanged};
}
} // namespace sin_compat
