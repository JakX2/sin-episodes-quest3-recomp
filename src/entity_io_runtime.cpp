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
    // Append equal-time events after older events for deterministic FIFO order.
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
        enqueue(Pending{source, activator, link.config, due,
                        next_ordinal_++});
        ++scheduled;
    }
    return scheduled;
}

void EntityIoRuntime::remove_source(EntityRef source) {
    // Entity-owned output definitions disappear with their source. The global
    // event queue is separate: pending events retain weak caller handles and
    // are still serviced with a null caller if that entity is later removed.
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
        // Remove before calling the host: callbacks may fire further outputs.
        Pending event = std::move(pending_.front());
        pending_.erase(pending_.begin());
        ++result.events_processed;
        // EHANDLE-like references become null when their generation is no
        // longer live; they do not invalidate the queued event itself.
        const EntityRef activator = host_.is_live(event.activator)
            ? event.activator : EntityRef{};
        const EntityRef caller = host_.is_live(event.caller)
            ? event.caller : EntityRef{};
        auto targets = host_.resolve_targets(event.config.target,
                                             activator, caller);
        for (EntityRef target : targets) {
            if (!host_.is_live(target)) continue;
            // Revalidate weak context handles for each delivery because an
            // earlier target input may remove/recycle the activator or caller.
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
