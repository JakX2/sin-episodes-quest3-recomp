#include "sin_compat/entity_io_runtime.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace sin_compat;

class TestHost final : public IEntityOutputHost {
public:
    struct Delivery {
        EntityRef target;
        std::string input;
        std::string parameter;
        EntityRef activator;
        EntityRef caller;
    };

    EntityRef add(std::uint32_t index, std::uint32_t generation,
                  bool door = false) {
        const EntityRef entity{index, generation};
        generations[index] = generation;
        if (door) doors.push_back(entity);
        return entity;
    }

    void recycle(EntityRef entity, std::uint32_t generation) {
        generations[entity.index] = generation;
    }

    bool is_live(EntityRef entity) const override {
        const auto found = generations.find(entity.index);
        return found != generations.end() &&
               found->second == entity.generation;
    }

    std::vector<EntityRef> resolve_targets(const std::string& target,
                                            EntityRef, EntityRef) override {
        if (target == "door") return doors;
        return {};
    }

    void accept_input(EntityRef target, const std::string& input,
                      const std::string& parameter, EntityRef activator,
                      EntityRef caller) override {
        deliveries.push_back(Delivery{target, input, parameter, activator, caller});
    }

    std::map<std::uint32_t, std::uint32_t> generations;
    std::vector<EntityRef> doors;
    std::vector<Delivery> deliveries;
};

class TestIdentityMap final : public IEntityIoPersistenceMap {
public:
    void bind(EntityRef entity, std::uint64_t id) {
        by_entity[{entity.index, entity.generation}] = id;
        by_id[id] = entity;
    }

    bool stable_id_for(EntityRef entity,
                       std::uint64_t& stable_id) const override {
        const auto found = by_entity.find({entity.index, entity.generation});
        if (found == by_entity.end()) return false;
        stable_id = found->second;
        return true;
    }

    EntityRef entity_for_stable_id(std::uint64_t stable_id) const override {
        const auto found = by_id.find(stable_id);
        return found == by_id.end() ? EntityRef{} : found->second;
    }

private:
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint64_t> by_entity;
    std::map<std::uint64_t, EntityRef> by_id;
};

int main() {
    TestHost old_host;
    const EntityRef old_source = old_host.add(1, 1);
    const EntityRef old_activator = old_host.add(2, 4);
    old_host.add(3, 2, true);
    old_host.add(4, 9, true);

    TestIdentityMap old_ids;
    old_ids.bind(old_source, 1001);
    old_ids.bind(old_activator, 2002);

    EntityIoRuntime old_runtime(old_host);
    assert(old_runtime.advance_to(10.0).events_processed == 0);
    assert(old_runtime.connect(old_source, "OnPulse",
        {"door", "Open", "first", 5.0, 3}));
    assert(old_runtime.connect(old_source, "OnPulse",
        {"door", "Set", "second", 5.0, -1}));
    assert(old_runtime.fire(old_source, "OnPulse", old_activator) == 2);

    EntityIoSnapshot snapshot;
    assert(old_runtime.save_state(snapshot, old_ids));
    assert(snapshot.outputs.size() == 1);
    assert(snapshot.outputs[0].connections.size() == 2);
    assert(snapshot.outputs[0].connections[0].remaining == 2);
    assert(snapshot.outputs[0].connections[1].remaining == -1);
    assert(snapshot.pending_events.size() == 2);
    assert(snapshot.pending_events[0].remaining_seconds == 5.0);
    assert(snapshot.pending_events[1].ordinal > snapshot.pending_events[0].ordinal);

    std::vector<std::uint8_t> encoded{0xAA, 0xBB};
    assert(encode_entity_io_snapshot(snapshot, encoded));
    assert(encoded.size() > 24);
    EntityIoSnapshot decoded;
    assert(decode_entity_io_snapshot(encoded.data(), encoded.size(), decoded));
    assert(validate_entity_io_snapshot(decoded));
    assert(decoded.outputs.size() == snapshot.outputs.size());
    assert(decoded.pending_events.size() == snapshot.pending_events.size());

    EntityIoSnapshot unchanged;
    unchanged.next_ordinal = 77;
    assert(!decode_entity_io_snapshot(encoded.data(), encoded.size() - 1, unchanged));
    assert(unchanged.next_ordinal == 77);
    auto trailing = encoded;
    trailing.push_back(0);
    assert(!decode_entity_io_snapshot(trailing.data(), trailing.size(), unchanged));
    assert(unchanged.next_ordinal == 77);
    assert(!decode_entity_io_snapshot(nullptr, 0, unchanged));
    auto unsupported = encoded;
    unsupported[4] = 2;
    assert(!decode_entity_io_snapshot(unsupported.data(), unsupported.size(), unchanged));
    assert(unchanged.next_ordinal == 77);

    TestHost new_host;
    const EntityRef new_source = new_host.add(41, 7);
    const EntityRef new_activator = new_host.add(42, 8);
    const EntityRef new_door_a = new_host.add(43, 5, true);
    const EntityRef new_door_b = new_host.add(44, 6, true);
    TestIdentityMap new_ids;
    new_ids.bind(new_source, 1001);
    new_ids.bind(new_activator, 2002);
    new_ids.bind(new_door_a, 3003);
    new_ids.bind(new_door_b, 4004);

    EntityIoRuntime restored(new_host);
    assert(restored.restore_state(decoded, new_ids, 100.0));
    assert(restored.now() == 100.0);
    assert(restored.pending_events() == 2);
    assert(restored.advance_to(104.999).events_processed == 0);
    const auto resumed = restored.advance_to(105.0);
    assert(resumed.events_processed == 2);
    assert(resumed.inputs_delivered == 4);
    assert(new_host.deliveries[0].target == new_door_a);
    assert(new_host.deliveries[0].input == "Open");
    assert(new_host.deliveries[0].parameter == "first");
    assert(new_host.deliveries[0].activator == new_activator);
    assert(new_host.deliveries[0].caller == new_source);
    assert(new_host.deliveries[1].target == new_door_b);
    assert(new_host.deliveries[2].input == "Set");
    assert(new_host.deliveries[2].parameter == "second");
    assert(new_host.deliveries[3].target == new_door_b);

    assert(restored.fire(new_source, "OnPulse", new_activator) == 2);
    assert(restored.fire(new_source, "OnPulse", new_activator) == 2);
    assert(restored.fire(new_source, "OnPulse", new_activator) == 1);
    assert(restored.fire(new_source, "OnPulse", new_activator) == 1);

    // An unresolved weak activator becomes null, but the queued event survives.
    TestHost weak_host;
    const EntityRef weak_source = weak_host.add(51, 1);
    weak_host.add(52, 1, true);
    TestIdentityMap weak_ids;
    weak_ids.bind(weak_source, 1001);
    EntityIoRuntime weak_runtime(weak_host);
    assert(weak_runtime.restore_state(decoded, weak_ids, 20.0));
    assert(weak_runtime.advance_to(25.0).inputs_delivered == 2);
    assert(weak_host.deliveries[0].activator == EntityRef{});
    assert(weak_host.deliveries[0].caller == weak_source);

    // Stale weak handles serialize as null; stale output sources are not revived.
    TestHost stale_host;
    const EntityRef stale_source = stale_host.add(61, 1);
    const EntityRef stale_activator = stale_host.add(62, 1);
    stale_host.add(63, 1, true);
    TestIdentityMap stale_ids;
    stale_ids.bind(stale_source, 6001);
    stale_ids.bind(stale_activator, 6002);
    EntityIoRuntime stale_runtime(stale_host);
    assert(stale_runtime.connect(stale_source, "OnGone",
        {"door", "Use", "stale", 2.0, 1}));
    assert(stale_runtime.fire(stale_source, "OnGone", stale_activator) == 1);
    stale_host.recycle(stale_activator, 2);
    stale_host.recycle(stale_source, 2);
    EntityIoSnapshot stale_snapshot;
    assert(stale_runtime.save_state(stale_snapshot, stale_ids));
    assert(stale_snapshot.outputs.empty());
    assert(stale_snapshot.pending_events.size() == 1);
    assert(stale_snapshot.pending_events[0].caller_id == 0);
    assert(stale_snapshot.pending_events[0].activator_id == 0);

    // Missing required output source rejects restore without mutating the target.
    TestHost atomic_host;
    const EntityRef atomic_source = atomic_host.add(71, 1);
    atomic_host.add(72, 1, true);
    EntityIoRuntime atomic_runtime(atomic_host);
    assert(atomic_runtime.connect(atomic_source, "Keep",
        {"door", "Keep", "", 3.0, -1}));
    assert(atomic_runtime.fire(atomic_source, "Keep", atomic_source) == 1);
    assert(atomic_runtime.advance_to(2.0).events_processed == 0);
    const auto old_pending = atomic_runtime.pending_events();
    const auto old_now = atomic_runtime.now();
    TestIdentityMap missing_source_map;
    assert(!atomic_runtime.restore_state(decoded, missing_source_map, 50.0));
    assert(atomic_runtime.pending_events() == old_pending);
    assert(atomic_runtime.now() == old_now);
    assert(atomic_runtime.fire(atomic_source, "Keep", atomic_source) == 1);

    // A host must not alias two distinct live source entities to one stable ID.
    TestHost collision_host;
    const EntityRef collision_a = collision_host.add(81, 1);
    const EntityRef collision_b = collision_host.add(82, 1);
    TestIdentityMap collision_ids;
    collision_ids.bind(collision_a, 8100);
    collision_ids.bind(collision_b, 8100);
    EntityIoRuntime collision_runtime(collision_host);
    assert(collision_runtime.connect(collision_a, "OnA",
        {"door", "Use", "a", 0.0, -1}));
    assert(collision_runtime.connect(collision_b, "OnB",
        {"door", "Use", "b", 0.0, -1}));
    EntityIoSnapshot collision_snapshot;
    collision_snapshot.next_ordinal = 99;
    assert(!collision_runtime.save_state(collision_snapshot, collision_ids));
    assert(collision_snapshot.next_ordinal == 99);

    auto invalid = snapshot;
    invalid.version = 2;
    std::vector<std::uint8_t> preserved{0x11, 0x22};
    assert(!encode_entity_io_snapshot(invalid, preserved));
    assert((preserved == std::vector<std::uint8_t>{0x11, 0x22}));
    invalid = snapshot;
    invalid.pending_events[1].ordinal = invalid.pending_events[0].ordinal;
    assert(!validate_entity_io_snapshot(invalid));
    assert(!encode_entity_io_snapshot(invalid, preserved));
    assert((preserved == std::vector<std::uint8_t>{0x11, 0x22}));
    invalid = snapshot;
    invalid.outputs[0].connections[0].remaining = 99;
    assert(!validate_entity_io_snapshot(invalid));
    assert(!restored.restore_state(snapshot, new_ids, NAN));
}
