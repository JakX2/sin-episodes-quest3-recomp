#include "sin_compat/npc_maker_runtime.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace sin_compat {
namespace {
constexpr double kNever = std::numeric_limits<double>::infinity();
}
bool NpcMakerRuntime::RefLess::operator()(EntityRef a, EntityRef b) const noexcept {
    return a.index == b.index ? a.generation < b.generation : a.index < b.index;
}
NpcMakerRuntime::NpcMakerRuntime(INpcMakerHost& host, double map_start)
    : host_(host), now_(map_start) {
    if (!std::isfinite(map_start) || map_start < 0.0)
        throw std::invalid_argument("NPC maker requires a finite, nonnegative map clock");
}
bool NpcMakerRuntime::register_maker(EntityRef maker, NpcMakerConfig config) {
    if (!host_.is_live(maker) || config.max_npcs < 0 ||
        (config.max_live_children != -1 && config.max_live_children < 1) ||
        (!std::isfinite(config.spawn_frequency)) ||
        (config.spawn_frequency != -1.0 && config.spawn_frequency <= 0.0)) return false;
    State state;
    state.cfg = config;
    state.enabled = !config.start_disabled;
    state.next_due = state.enabled ? now_ : kNever;
    return states_.emplace(maker, std::move(state)).second;
}
void NpcMakerRuntime::remove_maker(EntityRef maker) { states_.erase(maker); }
bool NpcMakerRuntime::exhausted(const State& s) const noexcept {
    return !s.cfg.infinite_children && s.total_spawned >= s.cfg.max_npcs;
}
bool NpcMakerRuntime::live_limit(const State& s) const noexcept {
    return s.cfg.max_live_children != -1 &&
           s.children.size() >= static_cast<std::size_t>(s.cfg.max_live_children);
}
NpcMakerResult NpcMakerRuntime::enable(EntityRef maker) {
    const auto it = states_.find(maker);
    if (it == states_.end() || !host_.is_live(maker)) return NpcMakerResult::InvalidMaker;
    if (it->second.enabled) return NpcMakerResult::NoChange;
    it->second.enabled = true;
    it->second.next_due = exhausted(it->second) || live_limit(it->second) ? kNever : now_;
    return NpcMakerResult::Applied;
}
NpcMakerResult NpcMakerRuntime::disable(EntityRef maker) {
    const auto it = states_.find(maker);
    if (it == states_.end() || !host_.is_live(maker)) return NpcMakerResult::InvalidMaker;
    if (!it->second.enabled) return NpcMakerResult::NoChange;
    it->second.enabled = false;
    it->second.next_due = kNever;
    return NpcMakerResult::Applied;
}
NpcMakerResult NpcMakerRuntime::toggle(EntityRef maker) {
    const auto it = states_.find(maker);
    if (it == states_.end() || !host_.is_live(maker)) return NpcMakerResult::InvalidMaker;
    return it->second.enabled ? disable(maker) : enable(maker);
}
NpcMakerAction NpcMakerRuntime::spawn_impl(EntityRef maker, State& s) {
    if (!s.enabled) return {NpcMakerResult::Disabled, {}};
    if (exhausted(s)) return {NpcMakerResult::Exhausted, {}};
    if (live_limit(s)) return {NpcMakerResult::LiveLimitReached, {}};
    const EntityRef child = host_.create_child(maker);
    if (!host_.is_live(child) || child == maker || s.children.count(child))
        return {NpcMakerResult::HostCreationFailed, {}};
    s.children.insert(child);
    ++s.total_spawned;
    NpcMakerAction result{NpcMakerResult::Applied, {{NpcMakerEventType::Spawned, maker, child}}};
    if (exhausted(s) && !s.all_spawned_announced) {
        s.all_spawned_announced = true;
        result.events.push_back({NpcMakerEventType::AllSpawned, maker, {}});
    }
    return result;
}
NpcMakerAction NpcMakerRuntime::spawn_now(EntityRef maker) {
    const auto it = states_.find(maker);
    if (it == states_.end() || !host_.is_live(maker))
        return {NpcMakerResult::InvalidMaker, {}};
    auto result = spawn_impl(maker, it->second);
    if (result.result == NpcMakerResult::Applied) {
        // Explicit spawn resets the artificial timer; this is a model choice.
        if (exhausted(it->second) || live_limit(it->second) || it->second.cfg.spawn_frequency == -1)
            it->second.next_due = kNever;
        else {
            const double due = now_ + it->second.cfg.spawn_frequency;
            it->second.next_due = std::isfinite(due) ? due : kNever;
        }
    }
    return result;
}
NpcMakerAdvance NpcMakerRuntime::advance_to(double time, std::size_t budget) {
    NpcMakerAdvance result;
    if (!std::isfinite(time) || time < now_) { result.invalid_time = true; return result; }
    now_ = time;
    // Scan entities by stable ID then generation. If multiple makers are due
    // a single host tick is capped by budget, not a wall-time feedback loop.
    for (auto& item : states_) {
        const EntityRef maker = item.first;
        State& s = item.second;
        if (!host_.is_live(maker) || !s.enabled) continue;
        while (s.next_due <= now_ && !exhausted(s) && !live_limit(s)) {
            if (result.attempts >= budget) { result.budget_exhausted = true; break; }
            const double due = s.next_due;
            ++result.attempts;
            auto action = spawn_impl(maker, s);
            if (action.result == NpcMakerResult::Applied) ++result.children_created;
            result.events.insert(result.events.end(), action.events.begin(), action.events.end());
            if (s.cfg.spawn_frequency == -1 || exhausted(s) || live_limit(s)) {
                s.next_due = kNever;
                break;
            }
            const double next = due + s.cfg.spawn_frequency;
            s.next_due = std::isfinite(next) && next > due ? next : kNever;
            if (action.result != NpcMakerResult::Applied) break;
        }
        if (result.budget_exhausted) break;
    }
    return result;
}
NpcMakerAction NpcMakerRuntime::notify_child_removed(EntityRef maker, EntityRef child) {
    const auto it = states_.find(maker);
    if (it == states_.end()) return {NpcMakerResult::InvalidMaker, {}};
    State& s = it->second;
    if (!s.children.erase(child)) return {NpcMakerResult::NoChange, {}};
    NpcMakerAction result{NpcMakerResult::Applied, {}};
    if (exhausted(s) && s.children.empty() && !s.all_dead_announced) {
        s.all_dead_announced = true;
        result.events.push_back({NpcMakerEventType::AllSpawnedDead, maker, {}});
    }
    if (s.enabled && !exhausted(s) && !live_limit(s)) {
        // Zero live children resumes the death-driven (-1) mode. Regular
        // timers resume from the host's current time once capacity opens.
        if (s.cfg.spawn_frequency != -1.0 || s.children.empty()) s.next_due = now_;
    }
    return result;
}
std::size_t NpcMakerRuntime::live_children(EntityRef maker) const noexcept {
    const auto it = states_.find(maker);
    return it == states_.end() ? 0 : it->second.children.size();
}
std::size_t NpcMakerRuntime::total_spawned(EntityRef maker) const noexcept {
    const auto it = states_.find(maker);
    return it == states_.end() ? 0 : static_cast<std::size_t>(it->second.total_spawned);
}
} // namespace sin_compat
