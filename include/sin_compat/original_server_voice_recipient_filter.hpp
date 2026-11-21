#pragma once
// Bounded 2006 original server IRecipientFilter/CRecipientFilter reference.
// Only the proven *normal* recipient add / player resolution branches.
// The special native CRecipientFilter +0x1C gating case is not reproduced.
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sin_compat::original_server_voice_recipient_filter {

struct PlayerResolveWitness {
    std::int32_t one_based_index;
    std::int32_t world_player_count;
    bool edict_present;
    std::uint32_t edict_flags;
    bool edict_owner_present;
    bool owner_virtual_resolves;
};

// Native helper 0x22286C70: invalid entries return null. The final
// vtable+0x14 dispatch can return null too; it is represented by a witness,
// NOT by invoking any original engine object.
inline bool can_resolve_player(const PlayerResolveWitness& p) {
    return p.one_based_index > 0 &&
           p.one_based_index <= p.world_player_count &&
           p.edict_present &&
           (p.edict_flags & 2U) == 0U &&
           p.edict_owner_present &&
           p.owner_virtual_resolves;
}

// Original filter normal path: caller has already obtained a valid 1-based
// index using original engine virtual +0x48. Reject duplicate indices.
class RecipientListNormalPath {
public:
    bool append_unique(std::int32_t one_based_index) {
        if (one_based_index <= 0) return false; // host guard only
        for (auto x : recipients_) if (x==one_based_index) return false;
        recipients_.push_back(one_based_index);
        return true;
    }
    std::size_t size() const noexcept {return recipients_.size();}
    std::int32_t recipient(std::size_t index) const {return recipients_.at(index);}
    void clear() noexcept {recipients_.clear();}
private:
    std::vector<std::int32_t> recipients_;
};

} // namespace sin_compat::original_server_voice_recipient_filter