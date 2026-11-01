#pragma once
// Independent, host-neutral NPC-template placement and destination scheduling.
// Guided by the original SiN Episodes SDK FGD, NOT a Source engine or AI API.
#include "sin_compat/npc_maker_runtime.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace sin_compat {
struct SpawnVector3 { double x = 0, y = 0, z = 0; };
enum class SpawnVisibility { Visible = 0, Hidden = 1, Any = 2 };
enum class SpawnDistance { Nearest = 0, Farthest = 1, Any = 2 };
enum class SpawnPlacementMode { Automatic, Radius, Line };
struct NpcSpawnPlacementConfig {
    std::string template_name; // FGD TemplateName
    std::string destination_group; // FGD DestinationGroup
    double radius = 256.0; // FGD Radius; actual location solving is host-owned
    SpawnVisibility criterion_visibility = SpawnVisibility::Any;
    SpawnDistance criterion_distance = SpawnDistance::Any;
    bool force_radius = false; // FGD spawnflag 256
    bool dont_spawn_while_visible = false; // FGD spawnflag 128
};
struct NpcSpawnDestination {
    EntityRef destination;
    std::string group;
    SpawnVector3 origin;
    bool visible_to_player = false;
    bool has_room = true;
    double reuse_delay_seconds = 1.0; // FGD ReuseDelay
    std::string rename_npc; // FGD RenameNPC
};
struct NpcSpawnPlacement {
    SpawnPlacementMode mode = SpawnPlacementMode::Radius;
    EntityRef destination{};
    SpawnVector3 origin;
    std::string rename_npc;
};
enum class SpawnPlacementResult {
    Spawned, InvalidMaker, InvalidConfig, MissingTemplate, MakerVisible,
    NoDestination, NoSpawnPoint, FactoryRejected
};
struct NpcSpawnAttempt {
    SpawnPlacementResult result = SpawnPlacementResult::InvalidMaker;
    EntityRef child{};
    NpcSpawnPlacement placement;
    explicit operator bool() const noexcept { return result == SpawnPlacementResult::Spawned; }
};
class INpcSpawnPlacementWorld {
public:
    virtual ~INpcSpawnPlacementWorld() = default;
    virtual bool is_live(EntityRef entity) const = 0;
    virtual bool template_exists(const std::string& name) const = 0;
    virtual bool is_maker_visible(EntityRef maker) const = 0;
    virtual bool player_origin(SpawnVector3& origin) const = 0;
    virtual std::vector<NpcSpawnDestination> destinations(const std::string& group) const = 0;
    // Host implements occupancy, collision traces, terrain and navigation.
    virtual bool radius_point(EntityRef maker, double radius, SpawnVector3& result) = 0;
    virtual bool line_point(EntityRef maker, SpawnVector3& result) = 0;
    // Host clones original template data, assigns target name, and returns live child.
    virtual EntityRef create_template_child(EntityRef maker, const std::string& template_name,
                                             const NpcSpawnPlacement& where) = 0;
    virtual void destination_spawned(EntityRef destination, EntityRef child) = 0;
};
class NpcSpawnPlacementPlanner final {
public:
    explicit NpcSpawnPlacementPlanner(INpcSpawnPlacementWorld& world, double start = 0.0);
    bool advance_to(double now); // reject nonfinite or backward clock
    NpcSpawnAttempt spawn(EntityRef maker, const NpcSpawnPlacementConfig& config,
                          SpawnPlacementMode override_mode = SpawnPlacementMode::Automatic);
    double now() const noexcept { return now_; }
private:
    struct RefLess {
        bool operator()(EntityRef a, EntityRef b) const noexcept;
    };
    INpcSpawnPlacementWorld& world_;
    std::map<EntityRef, double, RefLess> blocked_until_;
    double now_;
};
// Adapter supplies the existing Phase85 NpcMakerRuntime with real placement
// requests without changing that runtime's stable host contract.
class NpcMakerPlacementHostAdapter final : public INpcMakerHost {
public:
    explicit NpcMakerPlacementHostAdapter(INpcSpawnPlacementWorld& world, double start = 0.0)
        : world_(world), planner_(world, start) {}
    bool bind(EntityRef maker, NpcSpawnPlacementConfig config);
    void unbind(EntityRef maker) { configs_.erase(key(maker)); }
    bool change_destination_group(EntityRef maker, const std::string& group);
    bool set_time(double now) { return planner_.advance_to(now); }
    // Enforces the Phase85 maker quota, enabled-state, and live-child rules.
    NpcMakerAction spawn_with_mode(NpcMakerRuntime& runtime, EntityRef maker,
                                   SpawnPlacementMode mode);
    const NpcSpawnAttempt& last_attempt() const noexcept { return last_; }
    bool is_live(EntityRef entity) const override { return world_.is_live(entity); }
    EntityRef create_child(EntityRef maker) override;
private:
    INpcSpawnPlacementWorld& world_;
    NpcSpawnPlacementPlanner planner_;
    std::map<std::uint64_t, NpcSpawnPlacementConfig> configs_;
    std::map<std::uint64_t, SpawnPlacementMode> temporary_modes_;
    NpcSpawnAttempt last_;
    static std::uint64_t key(EntityRef r) {
        return (std::uint64_t(r.index) << 32) | r.generation;
    }
};
} // namespace sin_compat
