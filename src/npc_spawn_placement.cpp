#include "sin_compat/npc_spawn_placement.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace sin_compat {
namespace {
bool finite(SpawnVector3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
bool valid(const NpcSpawnPlacementConfig& c) {
    return !c.template_name.empty() && std::isfinite(c.radius) && c.radius >= 0.0 &&
        (c.criterion_visibility == SpawnVisibility::Visible ||
         c.criterion_visibility == SpawnVisibility::Hidden ||
         c.criterion_visibility == SpawnVisibility::Any) &&
        (c.criterion_distance == SpawnDistance::Nearest ||
         c.criterion_distance == SpawnDistance::Farthest ||
         c.criterion_distance == SpawnDistance::Any);
}
bool ref_less(EntityRef a, EntityRef b) {
    return a.index == b.index ? a.generation < b.generation : a.index < b.index;
}
// hypot avoids overflow and avoids comparing squared distances at large maps.
double distance(SpawnVector3 a, SpawnVector3 b) {
    return std::hypot(std::hypot(a.x-b.x, a.y-b.y), a.z-b.z);
}
}
bool NpcSpawnPlacementPlanner::RefLess::operator()(EntityRef a, EntityRef b) const noexcept {
    return ref_less(a,b);
}
NpcSpawnPlacementPlanner::NpcSpawnPlacementPlanner(INpcSpawnPlacementWorld& world, double start)
    : world_(world), now_(start) {
    if (!std::isfinite(start) || start < 0.0)
        throw std::invalid_argument("spawn placement requires nonnegative finite time");
}
bool NpcSpawnPlacementPlanner::advance_to(double now) {
    if (!std::isfinite(now) || now < now_) return false;
    now_ = now;
    return true;
}
NpcSpawnAttempt NpcSpawnPlacementPlanner::spawn(EntityRef maker,
    const NpcSpawnPlacementConfig& c, SpawnPlacementMode mode) {
    if (!world_.is_live(maker)) return {SpawnPlacementResult::InvalidMaker, {}, {}};
    if (!valid(c)) return {SpawnPlacementResult::InvalidConfig, {}, {}};
    if (!world_.template_exists(c.template_name))
        return {SpawnPlacementResult::MissingTemplate, {}, {}};
    if (c.dont_spawn_while_visible && world_.is_maker_visible(maker))
        return {SpawnPlacementResult::MakerVisible, {}, {}};
    NpcSpawnPlacement place;
    NpcSpawnDestination chosen;
    const bool named = mode == SpawnPlacementMode::Automatic &&
        !c.force_radius && !c.destination_group.empty();
    if (named) {
        SpawnVector3 player{};
        if (c.criterion_distance != SpawnDistance::Any &&
            (!world_.player_origin(player) || !finite(player)))
            return {SpawnPlacementResult::NoDestination, {}, {}};
        std::vector<NpcSpawnDestination> eligible;
        for (const auto& d : world_.destinations(c.destination_group)) {
            if (!world_.is_live(d.destination) || d.group != c.destination_group ||
                !d.has_room || !finite(d.origin) ||
                !std::isfinite(d.reuse_delay_seconds) || d.reuse_delay_seconds < 0.0)
                continue;
            if (c.criterion_visibility == SpawnVisibility::Visible && !d.visible_to_player)
                continue;
            if (c.criterion_visibility == SpawnVisibility::Hidden && d.visible_to_player)
                continue;
            auto until = blocked_until_.find(d.destination);
            if (until != blocked_until_.end() && now_ < until->second)
                continue;
            eligible.push_back(d);
        }
        if (eligible.empty()) return {SpawnPlacementResult::NoDestination, {}, {}};
        std::sort(eligible.begin(), eligible.end(), [&](const auto& a, const auto& b) {
            if (c.criterion_distance != SpawnDistance::Any) {
                const double da=distance(a.origin,player), db=distance(b.origin,player);
                if (da != db) return c.criterion_distance == SpawnDistance::Nearest ? da < db : da > db;
            }
            return ref_less(a.destination,b.destination);
        });
        chosen=eligible.front();
        place.mode=SpawnPlacementMode::Automatic;
        place.destination=chosen.destination;
        place.origin=chosen.origin;
        place.rename_npc=chosen.rename_npc;
    } else {
        // Forced-radius flag only applies to automatic mode; the line input
        // remains an explicit request handled by the host's line geometry.
        place.mode=mode == SpawnPlacementMode::Line ? SpawnPlacementMode::Line : SpawnPlacementMode::Radius;
        SpawnVector3 target;
        const bool success=place.mode == SpawnPlacementMode::Line ?
            world_.line_point(maker,target) : world_.radius_point(maker,c.radius,target);
        if (!success || !finite(target))
            return {SpawnPlacementResult::NoSpawnPoint, {}, {}};
        place.origin=target;
    }
    const EntityRef child=world_.create_template_child(maker,c.template_name,place);
    if (!world_.is_live(child) || child == maker ||
        (named && child == chosen.destination))
        return {SpawnPlacementResult::FactoryRejected, {}, {}};
    if (named) {
        const double deadline=now_+chosen.reuse_delay_seconds;
        blocked_until_[chosen.destination]=std::isfinite(deadline) ? deadline :
            std::numeric_limits<double>::infinity();
        world_.destination_spawned(chosen.destination,child);
    }
    return {SpawnPlacementResult::Spawned, child, std::move(place)};
}
bool NpcMakerPlacementHostAdapter::bind(EntityRef maker, NpcSpawnPlacementConfig config) {
    if (!world_.is_live(maker) || !valid(config)) return false;
    return configs_.emplace(key(maker),std::move(config)).second;
}
bool NpcMakerPlacementHostAdapter::change_destination_group(EntityRef maker,
                                                            const std::string& group) {
    const auto it = configs_.find(key(maker));
    if (it == configs_.end() || !world_.is_live(maker)) return false;
    it->second.destination_group = group;
    return true;
}
NpcMakerAction NpcMakerPlacementHostAdapter::spawn_with_mode(
    NpcMakerRuntime& runtime, EntityRef maker, SpawnPlacementMode mode) {
    // No callback may reenter this adapter during synchronous creation.
    const auto id = key(maker);
    if (temporary_modes_.count(id)) return {NpcMakerResult::HostCreationFailed, {}};
    temporary_modes_[id] = mode;
    struct Reset {
        std::map<std::uint64_t, SpawnPlacementMode>& modes;
        std::uint64_t id;
        ~Reset() { modes.erase(id); }
    } reset{temporary_modes_, id};
    return runtime.spawn_now(maker);
}
EntityRef NpcMakerPlacementHostAdapter::create_child(EntityRef maker) {
    const auto it = configs_.find(key(maker));
    if (it == configs_.end()) {
        last_ = {SpawnPlacementResult::InvalidConfig, {}, {}};
        return {};
    }
    const auto mode = temporary_modes_.find(key(maker));
    last_ = planner_.spawn(maker, it->second,
                           mode == temporary_modes_.end() ? SpawnPlacementMode::Automatic : mode->second);
    return last_.child;
}
} // namespace sin_compat
