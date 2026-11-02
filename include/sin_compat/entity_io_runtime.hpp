#pragma once

// Independently authored, portable Source-style entity output scheduling model.
// No Source SDK ABI, original SiN code, or engine integration is implied.

#include "sin_compat/weapon_entity_io.hpp" // Generation-safe EntityRef

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace sin_compat {

struct EntityOutputConnection {
    std::string target;    // Host resolves !activator, !caller, wildcards, etc.
    std::string input;
    std::string parameter;
    double delay_seconds = 0.0;
    int max_fires = -1;  // -1: unlimited, 0: disabled; positive: once per firing
};

class IEntityOutputHost {
public:
    virtual ~IEntityOutputHost() = default;
    virtual bool is_live(EntityRef entity) const = 0;
    // Resolved at DELIVERY time. Host must supply actual Source-compatible
    // dynamic targetname semantics. Returned order determines delivery order.
    virtual std::vector<EntityRef> resolve_targets(
        const std::string& target, EntityRef activator, EntityRef caller) = 0;
    virtual void accept_input(EntityRef target, const std::string& input,
                              const std::string& parameter, EntityRef activator,
                              EntityRef caller) = 0;
};

struct EntityIoDrain {
    std::size_t events_processed = 0;
    std::size_t inputs_delivered = 0;
    std::size_t dropped_events = 0;
    bool budget_exhausted = false;
    bool invalid_time = false;
    bool reentrant = false;
};

class EntityIoRuntime final {
public:
    explicit EntityIoRuntime(IEntityOutputHost& host) : host_(host) {}

    // Registers a fixed connection on the supplied, currently-live source.
    // Runtime never takes ownership of the entity. This is NOT an engine entity
    // factory, original map parser, or a Source event queue replacement.
    bool connect(EntityRef source, const std::string& output,
                 EntityOutputConnection connection);

    // One call represents one output firing, not one input per target.
    // Limits decrement at output-fire time; targets are resolved later.
    std::size_t fire(EntityRef source, const std::string& output,
                     EntityRef activator);

    // Remove a source's output definitions and pending events on destruction.
    // Cancel queued outputs without destroying the source's wiring. Used by
    // logic_relay CancelPending; later Trigger calls may schedule anew.
    std::size_t cancel_pending(EntityRef source);

    void remove_source(EntityRef source);

    // Advance monotonic synthetic/host time. A bounded per-call event budget
    // prevents infinite zero-delay relay loops from hanging the main thread.
    // Reentrant advance is rejected; nested fire() from a callback is allowed.
    EntityIoDrain advance_to(double seconds, std::size_t event_budget = 1024);

    double now() const { return now_; }
    std::size_t pending_events() const { return pending_.size(); }

private:
    struct SourceKey {
        EntityRef source;
        std::string output;
        bool operator<(const SourceKey& other) const;
    };
    struct ConnectionState {
        EntityOutputConnection config;
        int remaining;
    };
    struct Pending {
        EntityRef source;
        EntityRef activator;
        EntityRef caller;
        EntityOutputConnection config;
        double due;
        std::uint64_t ordinal;
    };

    void enqueue(Pending pending);
    IEntityOutputHost& host_;
    std::map<SourceKey, std::vector<ConnectionState>> outputs_;
    std::vector<Pending> pending_;
    double now_ = 0.0;
    std::uint64_t next_ordinal_ = 0;
    bool draining_ = false;
};

} // namespace sin_compat
