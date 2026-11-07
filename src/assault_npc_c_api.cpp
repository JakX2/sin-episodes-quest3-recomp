#define SIN_ASSAULT_NPC_C_EXPORT
#include "sin_compat/assault_npc_c_api.h"
#include "sin_compat/mobgrunt_rally_rank.hpp"
#include "sin_compat/original_assault_cue_wait.hpp"
#include "sin_compat/original_assault_setparameters_outcomes.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <new>
#include <unordered_set>
#include <vector>

struct sin_assault_session_v1 {
    sin_assault_host_v1 host;
    void *userdata;
    sin_assault_ref_v1 actor;
    uint32_t provoked;
    uint32_t hear_combat;
    sin_compat::OriginalAssaultCueGate cue{};
    uint32_t rally_id = 0;
    bool in_api = false;
};

namespace {
using namespace sin_compat;
static bool valid(sin_assault_ref_v1 r) noexcept {
    return r.index != 0 && r.generation != 0;
}
static bool empty(sin_assault_ref_v1 r) noexcept {
    return r.index == 0 && r.generation == 0;
}
static bool complete(const sin_assault_host_v1& h) noexcept {
    return h.version == SIN_ASSAULT_ABI_VERSION_1 &&
        h.struct_size == sizeof(h) && h.is_actor_live &&
        h.try_lock_rally && h.resolve_rally && h.unlock_rally &&
        h.initialize_behavior && h.reset_begin_assault_fields &&
        h.set_npc_condition && h.has_npc_condition;
}
class Guard final {
public:
    explicit Guard(sin_assault_session_v1* s) noexcept : s_(s) {
        if (s && !s->in_api) { s->in_api = true; ok_ = true; }
    }
    ~Guard() noexcept { if (ok_) s_->in_api = false; }
    explicit operator bool() const noexcept { return ok_; }
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
private:
    sin_assault_session_v1* s_;
    bool ok_ = false;
};
static bool trit(int32_t value) noexcept { return value == 0 || value == 1; }
static bool valid_origin(const float* p) noexcept {
    return p && std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]);
}
static int32_t unlock_after_error(sin_assault_session_v1* s, uint32_t point) {
    // Even if unlock fails, return a fatal host failure rather than continue
    // with a stale lease. The host retains cleanup responsibility.
    const int32_t released = s->host.unlock_rally(s->userdata, point, s->actor);
    if (released == 1) return SIN_ASSAULT_UNRESOLVED_RALLY;
    s->rally_id = point; // preserve lease for explicit finish retry
    return SIN_ASSAULT_HOST_FAILED;
}
} // namespace

extern "C" {
uint32_t sin_assault_abi_version_v1(void) { return SIN_ASSAULT_ABI_VERSION_1; }

sin_assault_session_v1* sin_assault_create_v1(
    const sin_assault_host_v1* host, void* user, sin_assault_ref_v1 actor,
    uint32_t provoked, uint32_t combat) {
    if (!host || !complete(*host) || !valid(actor) ||
        provoked == UINT32_MAX || combat == UINT32_MAX || provoked == combat)
        return nullptr;
    try { return new (std::nothrow) sin_assault_session_v1{*host, user, actor, provoked, combat}; }
    catch (...) { return nullptr; }
}
int32_t sin_assault_destroy_v1(sin_assault_session_v1* s) {
    if (!s) return SIN_ASSAULT_INVALID_ARGUMENT;
    if (s->in_api) return SIN_ASSAULT_REENTRANT;
    if (s->rally_id != 0) return SIN_ASSAULT_ALREADY_BOUND;
    delete s;
    return SIN_ASSAULT_OK;
}
int32_t sin_assault_bind_v1(sin_assault_session_v1* s,
    const float npc_origin[3], const sin_assault_candidate_v1* candidates,
    uint32_t count, int32_t mode, uint32_t* out_id) {
    if (!s) return SIN_ASSAULT_INVALID_ARGUMENT;
    Guard guard(s);
    if (!guard) return SIN_ASSAULT_REENTRANT;
    if (out_id) *out_id = 0;
    if (s->rally_id != 0) return SIN_ASSAULT_ALREADY_BOUND;
    if (!out_id || !valid_origin(npc_origin) ||
        count > SIN_ASSAULT_MAX_CANDIDATES || (count && !candidates) ||
        mode < 1 || mode > 3) return SIN_ASSAULT_INVALID_ARGUMENT;
    try {
        std::unordered_set<uint32_t> ids;
        std::vector<RallyPointCandidate> ranks;
        ranks.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            const auto& c = candidates[i];
            if (!c.rally_id || !ids.insert(c.rally_id).second ||
                (!empty(c.snapshot_owner) && !valid(c.snapshot_owner)) ||
                c.eligible > 1) return SIN_ASSAULT_INVALID_ARGUMENT;
            // Fail closed if a live owner can't be resolved by the host.
            bool eligible = c.eligible != 0;
            if (eligible && valid(c.snapshot_owner)) {
                const int32_t live = s->host.is_actor_live(s->userdata, c.snapshot_owner);
                if (!trit(live)) return SIN_ASSAULT_HOST_FAILED;
                if (live) eligible = false;
            }
            ranks.push_back({c.priority, {{c.origin[0], c.origin[1], c.origin[2]}}, eligible});
        }
        const std::array<float, 3> origin{{npc_origin[0], npc_origin[1], npc_origin[2]}};
        const auto chosen = choose_mobgrunt_rally_point(origin, ranks);
        if (!chosen) return SIN_ASSAULT_NO_CANDIDATE;
        const auto id = candidates[chosen->index].rally_id;
        const int32_t locked = s->host.try_lock_rally(s->userdata, id, s->actor);
        if (!trit(locked)) return SIN_ASSAULT_HOST_FAILED;
        if (!locked) return SIN_ASSAULT_LOCK_REJECTED;
        const int32_t resolved = s->host.resolve_rally(s->userdata, id);
        if (!trit(resolved)) {
            (void)unlock_after_error(s, id);
            return SIN_ASSAULT_HOST_FAILED;
        }
        if (!resolved) {
            const int32_t status = unlock_after_error(s, id);
            if (status == SIN_ASSAULT_UNRESOLVED_RALLY)
                apply_original_setparameters_cue_effects(
                    s->cue, mode, OriginalAssaultParameterOutcome::UnresolvedRally);
            return status;
        }
        const int32_t initialized = s->host.initialize_behavior(s->userdata, s->actor, id);
        if (initialized != 1) {
            (void)unlock_after_error(s, id);
            return SIN_ASSAULT_HOST_FAILED;
        }
        apply_original_setparameters_cue_effects(
            s->cue, mode, OriginalAssaultParameterOutcome::ResolvedRally);
        s->rally_id = id;
        *out_id = id;
        return SIN_ASSAULT_OK;
    } catch (...) { return SIN_ASSAULT_HOST_FAILED; }
}
int32_t sin_assault_begin_v1(sin_assault_session_v1* s) {
    if (!s) return SIN_ASSAULT_INVALID_ARGUMENT;
    Guard guard(s);
    if (!guard) return SIN_ASSAULT_REENTRANT;
    if (!s->rally_id) return SIN_ASSAULT_NOT_BOUND;
    try {
        if (s->host.reset_begin_assault_fields(s->userdata, s->actor) != 1)
            return SIN_ASSAULT_HOST_FAILED;
        const auto old = s->cue.received_cue;
        s->cue.received_cue = 1;
        if (s->host.set_npc_condition(s->userdata, s->actor, s->provoked) != 1) {
            s->cue.received_cue = old;
            return SIN_ASSAULT_HOST_FAILED;
        }
        return SIN_ASSAULT_OK;
    } catch (...) { return SIN_ASSAULT_HOST_FAILED; }
}
int32_t sin_assault_check_cue_v1(sin_assault_session_v1* s, uint32_t* out_ready) {
    if (!s) return SIN_ASSAULT_INVALID_ARGUMENT;
    Guard guard(s);
    if (!guard) return SIN_ASSAULT_REENTRANT;
    if (!out_ready) return SIN_ASSAULT_INVALID_ARGUMENT;
    *out_ready = 0;
    if (!s->rally_id) return SIN_ASSAULT_NOT_BOUND;
    try {
        bool combat = false;
        if (s->cue.received_cue != 4 && s->cue.configured_mode == 2) {
            const int32_t heard = s->host.has_npc_condition(s->userdata, s->actor, s->hear_combat);
            if (!trit(heard)) return SIN_ASSAULT_HOST_FAILED;
            combat = heard != 0;
        }
        *out_ready = s->cue.check(combat) ? 1u : 0u;
        return SIN_ASSAULT_OK;
    } catch (...) { return SIN_ASSAULT_HOST_FAILED; }
}
int32_t sin_assault_finish_v1(sin_assault_session_v1* s) {
    if (!s) return SIN_ASSAULT_INVALID_ARGUMENT;
    Guard guard(s);
    if (!guard) return SIN_ASSAULT_REENTRANT;
    if (!s->rally_id) return SIN_ASSAULT_NOT_BOUND;
    try {
        if (s->host.unlock_rally(s->userdata, s->rally_id, s->actor) != 1)
            return SIN_ASSAULT_HOST_FAILED;
        s->rally_id = 0;
        s->cue.configured_mode = 0; // original chain-terminal state clear
        return SIN_ASSAULT_OK;
    } catch (...) { return SIN_ASSAULT_HOST_FAILED; }
}
int32_t sin_assault_get_state_v1(sin_assault_session_v1* s, sin_assault_state_v1* out) {
    if (!s) return SIN_ASSAULT_INVALID_ARGUMENT;
    Guard guard(s);
    if (!guard) return SIN_ASSAULT_REENTRANT;
    if (!out) return SIN_ASSAULT_INVALID_ARGUMENT;
    *out = {s->cue.configured_mode, s->cue.received_cue, s->rally_id};
    return SIN_ASSAULT_OK;
}
} // extern "C"
