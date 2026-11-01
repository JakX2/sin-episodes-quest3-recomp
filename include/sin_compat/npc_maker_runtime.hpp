#pragma once
// Self-authored host-neutral NPC template-maker reference, informed by the
// original owner's SiN Episodes SDK FGD. Not original Ritual code or Source ABI.
#include "sin_compat/weapon_entity_io.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace sin_compat {
struct NpcMakerConfig {
    int max_npcs = 1;                  // FGD MaxNPCCount (total children)
    int max_live_children = 5;        // -1 means no simultaneous-live limit
    double spawn_frequency = 5.0;     // -1 means replace when last child dies
    bool start_disabled = false;
    bool infinite_children = false;   // FGD flag 32
};

enum class NpcMakerEventType { Spawned, AllSpawned, AllSpawnedDead };
struct NpcMakerEvent {
    NpcMakerEventType type = NpcMakerEventType::Spawned;
    EntityRef maker{};
    EntityRef child{}; // Valid for Spawned; otherwise unassigned
};
enum class NpcMakerResult {
    Applied, InvalidMaker, Disabled, Exhausted, LiveLimitReached,
    HostCreationFailed, NoChange
};
struct NpcMakerAction {
    NpcMakerResult result = NpcMakerResult::InvalidMaker;
    std::vector<NpcMakerEvent> events;
};
struct NpcMakerAdvance {
    bool invalid_time = false;
    bool budget_exhausted = false;
    std::size_t attempts = 0;
    std::size_t children_created = 0;
    std::vector<NpcMakerEvent> events;
};

class INpcMakerHost {
public:
    virtual ~INpcMakerHost() = default;
    virtual bool is_live(EntityRef entity) const = 0;
    // Must return a newly created *live* generation-safe reference or {}.
    // Host must not reenter NpcMakerRuntime from this synchronous callback.
    virtual EntityRef create_child(EntityRef maker) = 0;
};

class NpcMakerRuntime final {
public:
    explicit NpcMakerRuntime(INpcMakerHost& host, double map_start = 0.0);
    bool register_maker(EntityRef maker, NpcMakerConfig config);
    void remove_maker(EntityRef maker);
    NpcMakerResult enable(EntityRef maker);
    NpcMakerResult disable(EntityRef maker);
    NpcMakerResult toggle(EntityRef maker);
    NpcMakerAction spawn_now(EntityRef maker);
    NpcMakerAdvance advance_to(double time, std::size_t budget = 1024);
    // Notify when the original child reference dies / is removed. Must be
    // called even if the entity slot is recycled to a newer generation.
    NpcMakerAction notify_child_removed(EntityRef maker, EntityRef child);
    double now() const noexcept { return now_; }
    std::size_t maker_count() const noexcept { return states_.size(); }
    std::size_t live_children(EntityRef maker) const noexcept;
    std::size_t total_spawned(EntityRef maker) const noexcept;
private:
    struct RefLess {
        bool operator()(EntityRef a, EntityRef b) const noexcept;
    };
    struct State {
        NpcMakerConfig cfg;
        bool enabled = false;
        bool all_spawned_announced = false;
        bool all_dead_announced = false;
        int total_spawned = 0;
        double next_due = 0.0;
        std::set<EntityRef, RefLess> children;
    };
    bool exhausted(const State& state) const noexcept;
    bool live_limit(const State& state) const noexcept;
    NpcMakerAction spawn_impl(EntityRef maker, State& state);
    INpcMakerHost& host_;
    std::map<EntityRef, State, RefLess> states_;
    double now_;
};
} // namespace sin_compat
