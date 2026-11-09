#include "sin_compat/entity_io_runtime.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace sin_compat;

class Host final : public IEntityOutputHost {
public:
    struct Delivery {
        EntityRef target;
        std::string input;
        std::string parameter;
        EntityRef activator;
        EntityRef caller;
    };

    EntityRef add(std::uint32_t index, std::uint32_t generation) {
        const EntityRef entity{index, generation};
        generations[index] = generation;
        return entity;
    }

    bool is_live(EntityRef entity) const override {
        if (entity.index == 0 || entity.generation == 0) return false;
        const auto found = generations.find(entity.index);
        return found != generations.end() && found->second == entity.generation;
    }

    std::vector<EntityRef> resolve_targets(const std::string& target,
                                           EntityRef, EntityRef) override {
        if (target == "target" && is_live(target_entity)) return {target_entity};
        return {};
    }

    void accept_input(EntityRef target, const std::string& input,
                      const std::string& parameter, EntityRef activator,
                      EntityRef caller) override {
        deliveries.push_back(
            Delivery{target, input, parameter, activator, caller});
    }

    EntityRef target_entity{};
    std::map<std::uint32_t, std::uint32_t> generations;
    std::vector<Delivery> deliveries;
};

class IdentityMap final : public IEntityIoPersistenceMap {
public:
    void bind(EntityRef entity, std::uint64_t id) {
        by_entity[{entity.index, entity.generation}] = id;
        by_id[id] = entity;
    }

    void override_reverse(std::uint64_t id, EntityRef entity) {
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

static EntityIoSnapshot pending_snapshot(std::uint64_t caller_id,
                                         std::uint64_t activator_id) {
    EntityIoSnapshot snapshot;
    snapshot.next_ordinal = 1;
    snapshot.pending_events.push_back(EntityIoSavedEvent{
        caller_id, activator_id, "target", "Use", "saved", 2.0, 0});
    return snapshot;
}

int main() {
    // A collision involving only weak event handles must be rejected too.
    // Checking output-source IDs alone is insufficient: the restored weak
    // activator could otherwise resolve to a different live entity.
    Host alias_host;
    const EntityRef source = alias_host.add(1, 1);
    const EntityRef activator_a = alias_host.add(2, 1);
    const EntityRef activator_b = alias_host.add(3, 1);
    alias_host.target_entity = alias_host.add(4, 1);

    IdentityMap alias_ids;
    alias_ids.bind(source, 100);
    alias_ids.bind(activator_a, 200);
    alias_ids.bind(activator_b, 200); // Reverse lookup now points at B.

    EntityIoRuntime alias_runtime(alias_host);
    assert(alias_runtime.connect(
        source, "OnPulse", {"target", "Use", "", 5.0, -1}));
    assert(alias_runtime.fire(source, "OnPulse", activator_a) == 1);
    assert(alias_runtime.fire(source, "OnPulse", activator_b) == 1);

    EntityIoSnapshot unchanged;
    unchanged.next_ordinal = 77;
    assert(!alias_runtime.save_state(unchanged, alias_ids));
    assert(unchanged.next_ordinal == 77);
    assert(unchanged.pending_events.empty());

    // Required output owners cannot be restored through a one-way or
    // inconsistent stable-ID mapping. The destination runtime is unchanged.
    Host required_host;
    const EntityRef required_source = required_host.add(10, 1);
    required_host.target_entity = required_host.add(11, 1);
    IdentityMap required_ids;
    required_ids.bind(required_source, 900);
    required_ids.override_reverse(100, required_source);

    EntityIoSnapshot required_snapshot;
    EntityIoSavedOutput output;
    output.source_id = 100;
    output.output = "OnRestore";
    output.connections.push_back(
        EntityIoSavedConnection{"target", "Use", "", 1.0, -1, -1});
    required_snapshot.outputs.push_back(output);

    EntityIoRuntime required_runtime(required_host);
    assert(required_runtime.connect(
        required_source, "Keep", {"target", "Keep", "", 5.0, -1}));
    assert(required_runtime.fire(required_source, "Keep", required_source) == 1);
    const auto old_pending = required_runtime.pending_events();
    const auto old_time = required_runtime.now();
    assert(!required_runtime.restore_state(
        required_snapshot, required_ids, 50.0));
    assert(required_runtime.pending_events() == old_pending);
    assert(required_runtime.now() == old_time);
    assert(required_runtime.fire(required_source, "Keep", required_source) == 1);

    // A weak reference whose reverse mapping is inconsistent degrades to
    // null; it must not attach the pending event to an unrelated entity.
    Host weak_host;
    const EntityRef weak_caller = weak_host.add(20, 1);
    const EntityRef wrong_activator = weak_host.add(21, 1);
    weak_host.target_entity = weak_host.add(22, 1);

    IdentityMap weak_ids;
    weak_ids.bind(weak_caller, 100);
    weak_ids.bind(wrong_activator, 999);
    weak_ids.override_reverse(200, wrong_activator);

    EntityIoRuntime weak_runtime(weak_host);
    const auto weak_snapshot = pending_snapshot(100, 200);
    assert(weak_runtime.restore_state(weak_snapshot, weak_ids, 10.0));
    assert(weak_runtime.pending_events() == 1);
    const auto delivered = weak_runtime.advance_to(12.0);
    assert(delivered.events_processed == 1);
    assert(delivered.inputs_delivered == 1);
    assert(weak_host.deliveries.size() == 1);
    assert(weak_host.deliveries[0].activator == EntityRef{});
    assert(weak_host.deliveries[0].caller == weak_caller);
    assert(weak_host.deliveries[0].target == weak_host.target_entity);
}
