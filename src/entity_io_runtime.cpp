#include "sin_compat/entity_io_runtime.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace sin_compat {

bool EntityIoRuntime::SourceKey::operator<(const SourceKey& rhs) const {
    if (source.index != rhs.source.index) return source.index < rhs.source.index;
    if (source.generation != rhs.source.generation)
        return source.generation < rhs.source.generation;
    return output < rhs.output;
}

bool EntityIoRuntime::connect(EntityRef source, const std::string& output,
                              EntityOutputConnection cfg) {
    if (!host_.is_live(source) || output.empty() || cfg.target.empty() ||
        cfg.input.empty() || !std::isfinite(cfg.delay_seconds) ||
        cfg.delay_seconds < 0.0 || cfg.max_fires < -1) return false;
    auto& dest = outputs_[SourceKey{source, output}];
    const int remaining = cfg.max_fires;
    dest.push_back(ConnectionState{std::move(cfg), remaining});
    return true;
}

void EntityIoRuntime::enqueue(Pending pending) {
    auto pos = std::upper_bound(pending_.begin(), pending_.end(), pending,
        [](const Pending& key, const Pending& value) {
            if (key.due != value.due) return key.due < value.due;
            return key.ordinal < value.ordinal;
        });
    pending_.insert(pos, std::move(pending));
}

std::size_t EntityIoRuntime::fire(EntityRef source, const std::string& output,
                                  EntityRef activator) {
    if (!host_.is_live(source)) return 0;
    auto it = outputs_.find(SourceKey{source, output});
    if (it == outputs_.end()) return 0;
    std::size_t scheduled = 0;
    for (auto& link : it->second) {
        if (link.remaining == 0) continue;
        if (next_ordinal_ == std::numeric_limits<std::uint64_t>::max()) break;
        const double due = now_ + link.config.delay_seconds;
        if (!std::isfinite(due)) continue;
        if (link.remaining > 0) --link.remaining;
        enqueue(Pending{source, activator, link.config, due, next_ordinal_++});
        ++scheduled;
    }
    return scheduled;
}

void EntityIoRuntime::remove_source(EntityRef source) {
    for (auto it = outputs_.begin(); it != outputs_.end();) {
        if (it->first.source == source) it = outputs_.erase(it);
        else ++it;
    }
}

std::size_t EntityIoRuntime::cancel_events_from_caller(EntityRef caller) {
    const auto before = pending_.size();
    pending_.erase(std::remove_if(pending_.begin(), pending_.end(),
        [caller](const Pending& pending) { return pending.caller == caller; }),
        pending_.end());
    return before - pending_.size();
}

bool EntityIoRuntime::save_state(
    EntityIoSnapshot& snapshot,
    const IEntityIoPersistenceMap& identities) const {
    if (draining_) return false;

    EntityIoSnapshot saved;
    saved.next_ordinal = next_ordinal_;
    // Validate the persistence map as a bijection for every live entity that
    // enters the snapshot, not only output sources. Otherwise two different
    // weak caller/activator handles could serialize to one ID and restore as
    // the wrong entity.
    std::map<std::uint64_t, EntityRef> stable_id_entities;
    const auto stable_id_or_null = [&](EntityRef entity,
                                       std::uint64_t& stable_id) {
        if (!host_.is_live(entity)) {
            stable_id = 0;
            return true;
        }
        if (!identities.stable_id_for(entity, stable_id) || stable_id == 0)
            return false;
        const EntityRef reverse = identities.entity_for_stable_id(stable_id);
        if (!host_.is_live(reverse) || reverse != entity) return false;
        const auto known = stable_id_entities.find(stable_id);
        if (known != stable_id_entities.end() && known->second != entity)
            return false;
        stable_id_entities.emplace(stable_id, entity);
        return true;
    };

    for (const auto& entry : outputs_) {
        if (!host_.is_live(entry.first.source)) continue;
        std::uint64_t source_id = 0;
        if (!stable_id_or_null(entry.first.source, source_id)) return false;
        EntityIoSavedOutput output;
        output.source_id = source_id;
        output.output = entry.first.output;
        output.connections.reserve(entry.second.size());
        for (const auto& link : entry.second) {
            output.connections.push_back(EntityIoSavedConnection{
                link.config.target, link.config.input, link.config.parameter,
                link.config.delay_seconds, link.config.max_fires, link.remaining});
        }
        saved.outputs.push_back(std::move(output));
    }

    saved.pending_events.reserve(pending_.size());
    for (const auto& event : pending_) {
        std::uint64_t caller_id = 0;
        std::uint64_t activator_id = 0;
        if (!stable_id_or_null(event.caller, caller_id) ||
            !stable_id_or_null(event.activator, activator_id)) return false;
        const double remaining = event.due > now_ ? event.due - now_ : 0.0;
        if (!std::isfinite(remaining)) return false;
        saved.pending_events.push_back(EntityIoSavedEvent{
            caller_id, activator_id, event.config.target, event.config.input,
            event.config.parameter, remaining, event.ordinal});
    }

    if (!validate_entity_io_snapshot(saved)) return false;
    snapshot = std::move(saved);
    return true;
}

bool EntityIoRuntime::restore_state(
    const EntityIoSnapshot& snapshot,
    const IEntityIoPersistenceMap& identities,
    double restored_time) {
    if (draining_ || !std::isfinite(restored_time) || restored_time < 0.0 ||
        !validate_entity_io_snapshot(snapshot)) return false;

    // A stable ID is accepted only when both directions agree. Required output
    // owners fail restore on any mismatch; weak caller/activator references
    // degrade to null so the queued event itself is retained.
    const auto resolve_live_identity = [&](std::uint64_t stable_id) {
        if (stable_id == 0) return EntityRef{};
        const EntityRef entity = identities.entity_for_stable_id(stable_id);
        if (!host_.is_live(entity)) return EntityRef{};
        std::uint64_t reverse_id = 0;
        if (!identities.stable_id_for(entity, reverse_id) ||
            reverse_id != stable_id) return EntityRef{};
        return entity;
    };

    std::map<SourceKey, std::vector<ConnectionState>> restored_outputs;
    for (const auto& saved_output : snapshot.outputs) {
        const EntityRef source = resolve_live_identity(saved_output.source_id);
        if (!host_.is_live(source)) return false;

        std::vector<ConnectionState> connections;
        connections.reserve(saved_output.connections.size());
        for (const auto& saved_link : saved_output.connections) {
            EntityOutputConnection config{
                saved_link.target, saved_link.input, saved_link.parameter,
                saved_link.delay_seconds, saved_link.max_fires};
            connections.push_back(ConnectionState{std::move(config),
                                                  saved_link.remaining});
        }
        const auto inserted = restored_outputs.emplace(
            SourceKey{source, saved_output.output}, std::move(connections));
        if (!inserted.second) return false;
    }

    const auto weak_entity = [&](std::uint64_t stable_id) {
        return resolve_live_identity(stable_id);
    };

    std::vector<Pending> restored_pending;
    restored_pending.reserve(snapshot.pending_events.size());
    for (const auto& saved_event : snapshot.pending_events) {
        const double due = restored_time + saved_event.remaining_seconds;
        if (!std::isfinite(due)) return false;
        EntityOutputConnection config{
            saved_event.target, saved_event.input, saved_event.parameter, 0.0, -1};
        restored_pending.push_back(Pending{
            weak_entity(saved_event.caller_id),
            weak_entity(saved_event.activator_id),
            std::move(config), due, saved_event.ordinal});
    }

    std::sort(restored_pending.begin(), restored_pending.end(),
        [](const Pending& left, const Pending& right) {
            if (left.due != right.due) return left.due < right.due;
            return left.ordinal < right.ordinal;
        });

    outputs_.swap(restored_outputs);
    pending_.swap(restored_pending);
    now_ = restored_time;
    next_ordinal_ = snapshot.next_ordinal;
    return true;
}

EntityIoDrain EntityIoRuntime::advance_to(double seconds, std::size_t budget) {
    EntityIoDrain result;
    if (!std::isfinite(seconds) || seconds < now_) {
        result.invalid_time = true;
        return result;
    }
    if (draining_) {
        result.reentrant = true;
        return result;
    }
    now_ = seconds;
    draining_ = true;
    struct ResetFlag {
        bool& flag;
        ~ResetFlag() { flag = false; }
    } reset{draining_};

    while (!pending_.empty() && pending_.front().due <= now_ &&
           result.events_processed < budget) {
        Pending event = std::move(pending_.front());
        pending_.erase(pending_.begin());
        ++result.events_processed;
        const EntityRef activator = host_.is_live(event.activator)
            ? event.activator : EntityRef{};
        const EntityRef caller = host_.is_live(event.caller)
            ? event.caller : EntityRef{};
        auto targets = host_.resolve_targets(event.config.target, activator, caller);
        for (EntityRef target : targets) {
            if (!host_.is_live(target)) continue;
            const EntityRef current_activator = host_.is_live(event.activator)
                ? event.activator : EntityRef{};
            const EntityRef current_caller = host_.is_live(event.caller)
                ? event.caller : EntityRef{};
            host_.accept_input(target, event.config.input, event.config.parameter,
                               current_activator, current_caller);
            ++result.inputs_delivered;
        }
    }
    result.budget_exhausted = !pending_.empty() &&
        pending_.front().due <= now_ && result.events_processed >= budget;
    return result;
}

} // namespace sin_compat
