#pragma once

// Verified-original SiN assault rallypoint ownership protocol, engine-neutral.
// Original server.dll: CAssaultRallyPoint lock 0x220CE430, unlock 0x220CE4A0,
// m_hLockedBy at x86 class +0x3BC. This is NOT a Source entity or game server.
// A native Source adapter must resolve live entity handles incl. generation.

#include <cstdint>
#include <optional>

namespace sin_compat {

class RallyPointLease {
public:
    using OwnerToken = std::uint64_t;

    // Source server refuses acquisition when the current handle points to a
    // living entity, including when the requester is already the owner.
    // Empty or expired ownership allows a new owner (or nullptr to clear).
    template <typename IsLive>
    bool acquire(std::optional<OwnerToken> requester, IsLive&& is_live) {
        if (owner_ && is_live(*owner_)) {
            return false;
        }
        owner_ = requester;
        return true;
    }

    // Source refuses release only when a live different entity currently owns
    // the point. It accepts same owner, already-unlocked and stale handles.
    template <typename IsLive>
    bool release(std::optional<OwnerToken> requester, IsLive&& is_live) {
        if (owner_ && is_live(*owner_) && owner_ != requester) {
            return false;
        }
        owner_.reset();
        return true;
    }

    std::optional<OwnerToken> current_owner() const { return owner_; }

private:
    // Opaque actor token, not a raw Source pointer or original 32-bit EHANDLE.
    // The native engine supplies generation-aware liveness predicates.
    std::optional<OwnerToken> owner_;
};

} // namespace sin_compat
