#include "sin_compat/entity_io_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace sin_compat;

class FakeMap final : public IEntityOutputHost {
public:
    std::map<std::uint32_t, std::uint32_t> generations;
    std::map<std::string, std::vector<EntityRef>> named;
    std::vector<std::string> deliveries;
    std::vector<std::pair<EntityRef, EntityRef>> contexts;
    std::function<void(EntityRef, const std::string&)> on_input;
    EntityIoRuntime* scheduler = nullptr;

    EntityRef add(std::uint32_t index, std::uint32_t generation,
                  const std::string& name = {}) {
        EntityRef ref{index, generation};
        generations[index] = generation;
        if (!name.empty()) named[name].push_back(ref);
        return ref;
    }
    bool is_live(EntityRef entity) const override {
        const auto it = generations.find(entity.index);
        return it != generations.end() && it->second == entity.generation;
    }
    std::vector<EntityRef> resolve_targets(const std::string& target,
                                            EntityRef activator,
                                            EntityRef caller) override {
        if (target == "!activator") return {activator};
        if (target == "!caller") return {caller};
        auto it = named.find(target);
        return it == named.end() ? std::vector<EntityRef>{} : it->second;
    }
    void accept_input(EntityRef target, const std::string& input,
                      const std::string& param, EntityRef activator,
                      EntityRef caller) override {
        deliveries.push_back(std::to_string(target.index) + ":" + input +
                             ":" + param);
        contexts.emplace_back(activator, caller);
        if (on_input) on_input(target, input);
    }
};

static EntityOutputConnection link(std::string target, std::string input,
                                   double delay = 0.0, int max = -1,
                                   std::string parameter = "") {
    return {std::move(target), std::move(input), std::move(parameter), delay, max};
}

int main() {
    // Synthetic logic_auto -> logic_relay -> func_door. This is neither a
    // shipped SiN map nor a Source engine in-game scripted encounter.
    FakeMap host;
    auto auto_ent = host.add(1, 1, "start");
    auto relay = host.add(2, 2, "relay");
    host.add(3, 3, "door");
    auto player = host.add(4, 4, "player");
    EntityIoRuntime io(host);
    host.scheduler = &io;
    assert(io.connect(auto_ent, "OnMapSpawn", link("relay", "Trigger", 0.0, 1)));
    assert(io.connect(relay, "OnTrigger", link("door", "Open", 0.25, 1, "synthetic")));
    host.on_input = [&](EntityRef e, const std::string& input) {
        if (e == relay && input == "Trigger")
            assert(io.fire(relay, "OnTrigger", player) == 1);
    };
    assert(io.fire(auto_ent, "OnMapSpawn", player) == 1);
    assert(io.fire(auto_ent, "OnMapSpawn", player) == 0); // maxfire once
    auto first = io.advance_to(0);
    assert(first.events_processed == 1 && first.inputs_delivered == 1);
    assert((host.deliveries == std::vector<std::string>{"2:Trigger:"}));
    assert(host.contexts[0].first == player && host.contexts[0].second == auto_ent);
    assert(io.advance_to(0.24).events_processed == 0);
    auto second = io.advance_to(0.25);
    assert(second.inputs_delivered == 1 && !second.budget_exhausted);
    assert(host.deliveries[1] == "3:Open:synthetic");
    assert(host.contexts[1].first == player && host.contexts[1].second == relay);
    assert(io.pending_events() == 0);
    assert(io.fire(relay, "OnTrigger", player) == 0);

    // Same-time ordering and multiple targets: host picks deterministic order.
    auto buzzer = host.add(5, 5, "buzzer");
    auto buzzer2 = host.add(6, 1, "buzzer");
    assert(io.connect(auto_ent, "OnBuzz", link("buzzer", "Ring", 0, -1, "first")));
    assert(io.connect(auto_ent, "OnBuzz", link("door", "Close", 0, -1, "second")));
    assert(io.fire(auto_ent, "OnBuzz", player) == 2);
    auto multi = io.advance_to(0.25);
    assert(multi.events_processed == 2 && multi.inputs_delivered == 3);
    assert(host.deliveries[2] == "5:Ring:first");
    assert(host.deliveries[3] == "6:Ring:first");
    assert(host.deliveries[4] == "3:Close:second");
    (void)buzzer;
    (void)buzzer2;

    // Target name resolution happens when delayed event is due, not at emit.
    assert(io.connect(auto_ent, "SpawnThenSignal", link("late", "Activate", 0.5)));
    assert(io.fire(auto_ent, "SpawnThenSignal", player) == 1);
    host.add(7, 1, "late");
    assert(io.advance_to(0.75).inputs_delivered == 1);
    assert(host.deliveries.back() == "7:Activate:");

    // !activator and !caller are host-resolved rather than invented engine
    // semantics in this library.
    assert(io.connect(auto_ent, "OnUsed", link("!activator", "Ping")));
    assert(io.connect(auto_ent, "OnUsed", link("!caller", "Echo")));
    assert(io.fire(auto_ent, "OnUsed", player) == 2);
    assert(io.advance_to(0.75).inputs_delivered == 2);
    assert(host.deliveries[6] == "4:Ping:");
    assert(host.deliveries[7] == "1:Echo:");

    // Stale generation must not deliver queued event or resolve a recycled ID.
    assert(io.connect(auto_ent, "OnDestroy", link("door", "Delete", 1)));
    assert(io.fire(auto_ent, "OnDestroy", player) == 1);
    host.generations[1] = 2;
    auto drop = io.advance_to(1.75);
    assert(drop.dropped_events == 1 && drop.inputs_delivered == 0);
    assert(io.fire(auto_ent, "OnBuzz", player) == 0);
    host.generations[1] = 1;
    // Target itself can be stale (index reuse of a named target).
    assert(io.fire(auto_ent, "OnBuzz", player) == 2);
    host.generations[5] = 6;
    auto stale_target = io.advance_to(1.75);
    assert(stale_target.inputs_delivered == 2); // buzzer2 and door, not buzzer1
    assert(host.deliveries[8] == "6:Ring:first");

    // Explicit source destruction cancels all its pending events.
    assert(io.connect(auto_ent, "DestroyMe", link("door", "Open", 3)));
    assert(io.fire(auto_ent, "DestroyMe", player) == 1);
    io.remove_source(auto_ent);
    assert(io.pending_events() == 0);
    assert(io.fire(auto_ent, "OnBuzz", player) == 0);

    // Zero/negative/non-finite delay and non-monotonic time are not accepted.
    assert(!io.connect(relay, "Bad", link("door", "Open", -0.1)));
    assert(!io.connect(relay, "Bad", link("door", "Open", INFINITY)));
    assert(!io.connect(relay, "Bad", link("door", "Open", NAN)));
    assert(!io.connect(relay, "Bad", link("door", "Open", 0.0, -2)));
    assert(!io.connect(relay, "Bad", link("", "Open")));
    assert(!io.connect(relay, "Bad", link("door", "")));
    assert(!io.connect(relay, "", link("door", "Open")));
    assert(!io.connect(EntityRef{}, "Bad", link("door", "Open")));
    assert(io.advance_to(1.0).invalid_time);
    assert(io.advance_to(NAN).invalid_time);
    assert(io.advance_to(INFINITY).invalid_time);
    assert(io.now() == 1.75);

    // A looping zero-delay relay must not freeze the host frame.
    FakeMap cycles;
    auto loop = cycles.add(8, 9, "loop");
    EntityIoRuntime bounded(cycles);
    assert(bounded.connect(loop, "Loop", link("loop", "Trigger")));
    cycles.on_input = [&](EntityRef target, const std::string&) {
        assert(target == loop);
        assert(bounded.fire(loop, "Loop", loop) == 1);
        const auto nested = bounded.advance_to(0);
        assert(nested.reentrant && nested.events_processed == 0);
    };
    assert(bounded.fire(loop, "Loop", loop) == 1);
    auto limited = bounded.advance_to(0, 5);
    assert(limited.events_processed == 5 && limited.inputs_delivered == 5);
    assert(limited.budget_exhausted && bounded.pending_events() == 1);
    bounded.remove_source(loop);
    assert(bounded.pending_events() == 0);

    // Zero budget must not lose pending events, and invalid times keep queue.
    FakeMap budget_host;
    auto src = budget_host.add(11, 1, "source");
    budget_host.add(12, 1, "target");
    EntityIoRuntime limited_budget(budget_host);
    assert(limited_budget.connect(src, "Once", link("target", "Use", 0, 0)));
    assert(limited_budget.fire(src, "Once", src) == 0);
    assert(limited_budget.connect(src, "Later", link("target", "Use", 0.5, 1)));
    assert(limited_budget.fire(src, "Later", src) == 1);
    assert(limited_budget.advance_to(0.5, 0).budget_exhausted);
    assert(limited_budget.pending_events() == 1);
    auto delivered = limited_budget.advance_to(0.5, 1);
    assert(delivered.inputs_delivered == 1 && !delivered.budget_exhausted);
    assert(limited_budget.fire(src, "Later", src) == 0);
}
