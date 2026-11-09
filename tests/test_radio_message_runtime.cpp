#include "sin_compat/radio_message_runtime.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

using namespace sin_compat;

class FakeRadioHost final : public IRadioMessageHost {
public:
    struct Delivery {
        EntityRef target;
        std::string input;
        std::string parameter;
        EntityRef activator;
        EntityRef caller;
    };

    std::map<std::uint32_t, std::uint32_t> generations;
    std::map<std::string, std::vector<EntityRef>> named;
    std::optional<EntityRef> local_player;
    std::vector<Delivery> deliveries;
    bool remove_allowed = true;
    bool remove_requested = false;
    EntityRef removed_entity{};

    EntityRef add(std::uint32_t index, std::uint32_t generation,
                  const std::string& targetname = {}) {
        const EntityRef ref{index, generation};
        generations[index] = generation;
        if (!targetname.empty()) named[targetname].push_back(ref);
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
        const auto it = named.find(target);
        return it == named.end() ? std::vector<EntityRef>{} : it->second;
    }

    void accept_input(EntityRef target, const std::string& input,
                      const std::string& parameter, EntityRef activator,
                      EntityRef caller) override {
        deliveries.push_back({target, input, parameter, activator, caller});
    }

    std::optional<EntityRef> resolve_local_player() override {
        return local_player;
    }

    bool request_remove(EntityRef entity) override {
        remove_requested = true;
        removed_entity = entity;
        return remove_allowed;
    }
};

int main() {
    FakeRadioHost host;
    const auto message_entity = host.add(10, 4, "radio_message");
    const auto player = host.add(2, 7, "player");
    const auto sink = host.add(20, 1, "sink");
    host.local_player = player;

    EntityIoRuntime io(host);
    OriginalRadioMessageRuntime message(message_entity, host, io);

    // The three datamap-backed values are retained without inventing audio
    // playback, priority arbitration, or localization behavior.
    assert(message.set_message("Radio.Dispatch_01") == RadioMessageResult::Applied);
    assert(message.set_speaker("Jessica") == RadioMessageResult::Applied);
    assert(message.set_priority(17) == RadioMessageResult::Applied);
    assert(message.state().message == "Radio.Dispatch_01");
    assert(message.state().speaker == "Jessica");
    assert(message.state().priority == 17);
    assert(message.set_message("Radio.Dispatch_01") == RadioMessageResult::Unchanged);
    assert(message.set_speaker("Jessica") == RadioMessageResult::Unchanged);
    assert(message.set_priority(17) == RadioMessageResult::Unchanged);

    // No clamping is introduced for a raw integer priority field.
    assert(message.set_priority(-3) == RadioMessageResult::Applied);
    assert(message.state().priority == -3);

    // A real CRadioMessage Dispatch invokes its OnDispatch output only after
    // resolving a live local player. The generic scheduler then handles links.
    assert(io.connect(message_entity, "OnDispatch",
        {"!activator", "OnRadioDispatched", "from_message", 0.0, -1}));
    assert(io.connect(message_entity, "OnDispatch",
        {"!caller", "OnMessageEntityDispatched", "", 0.0, -1}));

    const auto dispatched = message.dispatch();
    assert(dispatched.result == RadioMessageResult::OutputInvoked);
    assert(dispatched.connections_scheduled == 2);
    const auto drained = io.advance_to(0.0);
    assert(drained.events_processed == 2);
    assert(drained.inputs_delivered == 2);
    assert(host.deliveries.size() == 2);

    // Source-convention mapping is isolated in dispatch(): local player is
    // activator and the output-owning entity is caller.
    assert(host.deliveries[0].target == player);
    assert(host.deliveries[0].input == "OnRadioDispatched");
    assert(host.deliveries[0].parameter == "from_message");
    assert(host.deliveries[0].activator == player);
    assert(host.deliveries[0].caller == message_entity);
    assert(host.deliveries[1].target == message_entity);
    assert(host.deliveries[1].input == "OnMessageEntityDispatched");

    // Missing or stale local player suppresses the dispatch output.
    host.local_player.reset();
    const auto missing = message.dispatch();
    assert(missing.result == RadioMessageResult::MissingLocalPlayer);
    assert(missing.connections_scheduled == 0);
    assert(io.pending_events() == 0);

    host.local_player = player;
    host.generations[player.index] = player.generation + 1;
    const auto stale_player = message.dispatch();
    assert(stale_player.result == RadioMessageResult::MissingLocalPlayer);
    assert(stale_player.connections_scheduled == 0);

    // Remove is a host request; actual entity destruction and scheduler
    // cleanup remain the engine adapter's responsibility.
    host.generations[player.index] = player.generation;
    assert(message.remove() == RadioMessageResult::RemoveRequested);
    assert(host.remove_requested);
    assert(host.removed_entity == message_entity);

    host.remove_allowed = false;
    assert(message.remove() == RadioMessageResult::HostRejected);

    // Recycled source handles are rejected before any state or output action.
    host.generations[message_entity.index] = message_entity.generation + 1;
    assert(message.set_message("ignored") == RadioMessageResult::StaleEntity);
    assert(message.set_speaker("ignored") == RadioMessageResult::StaleEntity);
    assert(message.set_priority(9) == RadioMessageResult::StaleEntity);
    assert(message.dispatch().result == RadioMessageResult::StaleEntity);
    assert(message.remove() == RadioMessageResult::StaleEntity);

    (void)sink;
}
