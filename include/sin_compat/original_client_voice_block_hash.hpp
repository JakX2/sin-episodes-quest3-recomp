#pragma once
// Independently reconstructed reference for the *observed* original 2006
// SiN client.dll CVoiceStatus hashed-key membership control flow.
// This is NOT a Source engine ABI, complete voice manager, or a port.
#include <array>
#include <cstddef>
#include <cstdint>
#include <list>

namespace sin_compat::original_client_voice_block_hash {

constexpr std::size_t kIdentityBytes = 32;
constexpr std::size_t kBucketCount = 256;
constexpr std::size_t kNativeBucketStride = 0x28;
constexpr std::uint32_t kNativeBucketCollectionBytes = 0x2800;
constexpr std::uint32_t kNativeVoiceObjectBucketOffset = 0x28;

// Original 32-bit native record: first 32 bytes key, then two 4-byte links.
// The real x86 records contain native addresses. Here these fields are merely
// fixed-width surrogates, never to be dereferenced as pointers.
struct NativeKeyRecord32 {
    std::array<std::uint8_t, kIdentityBytes> key{};
    std::uint32_t previous_address = 0;
    std::uint32_t next_address = 0;
};
static_assert(sizeof(NativeKeyRecord32) == 0x28);
static_assert(offsetof(NativeKeyRecord32, previous_address) == 0x20);
static_assert(offsetof(NativeKeyRecord32, next_address) == 0x24);
static_assert(kBucketCount * kNativeBucketStride == kNativeBucketCollectionBytes);

using Key32 = std::array<std::uint8_t, kIdentityBytes>;

// Faithful logical key/byte hash order from x86 0x241E4DD0/0x241E4E30.
// No claims about the native heap allocator or linked-node address values.
constexpr std::uint8_t bucket_for(const Key32& key) noexcept {
    std::uint8_t total = 0;
    for (const auto byte : key)
        total = static_cast<std::uint8_t>(total + byte);
    return total;
}

class KeyMembership {
public:
    bool contains(const Key32& key) const {
        for (const auto& item : buckets_[bucket_for(key)])
            if (item == key) return true;
        return false;
    }
    bool set_present(const Key32& key, bool present) {
        auto& chain = buckets_[bucket_for(key)];
        for (auto it = chain.begin(); it != chain.end(); ++it) {
            if (*it != key) continue;
            if (!present) {
                chain.erase(it);
                return true;
            }
            return false;
        }
        if (present) {
            chain.push_back(key);
            return true;
        }
        return false;
    }
    void clear() noexcept {
        for (auto& bucket : buckets_) bucket.clear();
    }
    std::size_t size() const noexcept {
        std::size_t count = 0;
        for (const auto& bucket : buckets_) count += bucket.size();
        return count;
    }
private:
    std::array<std::list<Key32>, kBucketCount> buckets_{};
};

// The actual 0x241E5B20 method acquires a player data object first, then
// checks original this+0x14 mask, mutates this+0x28 hash-key table, and calls
// UpdateServerState(0) only if both gates pass. Its *second* ret-8 parameter
// has no direct read in the authenticated function. Host guards for invalid
// player numbers are deliberately safer than original unchecked SAR indexing.
struct IVoiceHost {
    virtual ~IVoiceHost() = default;
    virtual bool get_identity(int player_one_based, Key32& identity_out) = 0;
    virtual void update_server_state(bool force) = 0;
};

enum class ToggleOutcome { InvalidPlayer, PlayerInfoUnavailable, MaskNotSet, Added, Removed };

inline ToggleOutcome toggle_key_if_masked(
    KeyMembership& table, IVoiceHost& host,
    int player_one_based, bool /*ignored_original_second_parameter*/,
    std::uint32_t native_mask_at_this_plus_0x14)
{
    if (player_one_based < 1 || player_one_based > 32)
        return ToggleOutcome::InvalidPlayer;
    Key32 identity{};
    if (!host.get_identity(player_one_based, identity))
        return ToggleOutcome::PlayerInfoUnavailable;
    const auto bit = std::uint32_t{1} << (static_cast<unsigned>(player_one_based - 1) & 31U);
    if ((native_mask_at_this_plus_0x14 & bit) == 0)
        return ToggleOutcome::MaskNotSet;
    const bool was_present = table.contains(identity);
    table.set_present(identity, !was_present);
    host.update_server_state(false);
    return was_present ? ToggleOutcome::Removed : ToggleOutcome::Added;
}
} // namespace sin_compat::original_client_voice_block_hash