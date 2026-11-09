#pragma once

// Independently authored, portable Source-style entity output scheduling model.
// No Source SDK ABI, original SiN code, or engine integration is implied.

#include "sin_compat/weapon_entity_io.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace sin_compat {

struct EntityOutputConnection {
    std::string target;
    std::string input;
    std::string parameter;
    double delay_seconds = 0.0;
    int max_fires = -1;
};

class IEntityOutputHost {
public:
    virtual ~IEntityOutputHost() = default;
    virtual bool is_live(EntityRef entity) const = 0;
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

constexpr std::uint32_t kEntityIoSnapshotVersion = 1;

struct EntityIoSavedConnection {
    std::string target;
    std::string input;
    std::string parameter;
    double delay_seconds = 0.0;
    int max_fires = -1;
    int remaining = -1;
};

struct EntityIoSavedOutput {
    std::uint64_t source_id = 0;
    std::string output;
    std::vector<EntityIoSavedConnection> connections;
};

struct EntityIoSavedEvent {
    std::uint64_t caller_id = 0;
    std::uint64_t activator_id = 0;
    std::string target;
    std::string input;
    std::string parameter;
    double remaining_seconds = 0.0;
    std::uint64_t ordinal = 0;
};

struct EntityIoSnapshot {
    std::uint32_t version = kEntityIoSnapshotVersion;
    std::uint64_t next_ordinal = 0;
    std::vector<EntityIoSavedOutput> outputs;
    std::vector<EntityIoSavedEvent> pending_events;
};

// Stable IDs must be unique per live entity and must not be reused for a
// different entity while a save may still reference the old one.
class IEntityIoPersistenceMap {
public:
    virtual ~IEntityIoPersistenceMap() = default;
    virtual bool stable_id_for(EntityRef live_entity,
                               std::uint64_t& stable_id) const = 0;
    virtual EntityRef entity_for_stable_id(std::uint64_t stable_id) const = 0;
};

bool validate_entity_io_snapshot(const EntityIoSnapshot& snapshot);
bool encode_entity_io_snapshot(const EntityIoSnapshot& snapshot,
                               std::vector<std::uint8_t>& bytes);
bool decode_entity_io_snapshot(const std::uint8_t* bytes, std::size_t size,
                               EntityIoSnapshot& snapshot);

class EntityIoRuntime final {
public:
    explicit EntityIoRuntime(IEntityOutputHost& host) : host_(host) {}

    bool connect(EntityRef source, const std::string& output,
                 EntityOutputConnection connection);
    std::size_t fire(EntityRef source, const std::string& output,
                     EntityRef activator);
    void remove_source(EntityRef source);
    std::size_t cancel_events_from_caller(EntityRef caller);

    // Captures definitions, fire counts and pending events using host-owned IDs.
    // Fails while an input callback is being serviced.
    bool save_state(EntityIoSnapshot& snapshot,
                    const IEntityIoPersistenceMap& identities) const;

    // Call only after the host has recreated its entity set. Required output
    // sources must resolve; expired weak event handles become null. The new
    // clock origin preserves remaining game-time delays across process pause.
    // Failed restore leaves the existing runtime untouched.
    bool restore_state(const EntityIoSnapshot& snapshot,
                       const IEntityIoPersistenceMap& identities,
                       double restored_time = 0.0);

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
        EntityRef caller;
        EntityRef activator;
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
