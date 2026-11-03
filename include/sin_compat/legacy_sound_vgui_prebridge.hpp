#pragma once
// SiN Episodes 2006 -> SourceVR 0.5.5: safe, UNBOUND pre-bridge.
// No native function pointers, game assets or original PE bytes here.
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <utility>

namespace sin_compat::phase99 {
inline constexpr std::uint32_t missing_token = 0xffffffffu;
inline constexpr std::uint32_t max_native_token = 0xfffeu;
inline constexpr std::size_t original_key_count = 256u;
inline constexpr std::size_t original_key_nonzero_count = 99u;
inline constexpr std::size_t native_name_to_token_slot = 7u;
inline constexpr std::size_t native_token_to_name_slot = 10u;
inline constexpr std::size_t old_name_to_token_slot = 3u;
inline constexpr std::size_t old_token_to_name_slot = 6u;

// The original VGUI_System010[24] initializes a BSS key table on its first
// use and returns table[key] for 0<=key<256, otherwise zero.  This adapter
// accepts a privately extracted, SHA-checked original table; it never calls
// the unrelated native SourceVR VGUI_System010[24].
class LegacyKeyTranslator {
public:
    bool install(const std::array<std::uint32_t, original_key_count>& map,
                 bool original_binary_and_table_sha_verified) noexcept {
        ready_ = false;
        if (!original_binary_and_table_sha_verified) return false;
        std::size_t nonzero = 0;
        std::array<bool, 104u> seen{};
        for (std::uint32_t value : map) {
            if (value == 0) continue;
            if (value > 103u || seen[value]) return false;
            seen[value] = true;
            ++nonzero;
        }
        if (nonzero != original_key_nonzero_count) return false;
        mapping_ = map;
        ready_ = true;
        return true;
    }
    std::uint32_t translate(std::int32_t legacy_key) const noexcept {
        if (!ready_ || legacy_key < 0 || legacy_key >= 256) return 0;
        return mapping_[static_cast<std::size_t>(legacy_key)];
    }
    bool ready() const noexcept { return ready_; }
    void reset() noexcept { ready_ = false; mapping_.fill(0); }
private:
    std::array<std::uint32_t, original_key_count> mapping_{};
    bool ready_ = false;
};

enum class CrosswalkResult : std::uint8_t {
    Added, AlreadyPresent, LegacyIndexInvalid, NativeIndexInvalid,
    NativeValidationFailed, NameMismatch, MappingConflict
};

// The selected VSoundEmitter002 slot 7 returns a 16-bit index (or -1),
// whereas SiN's original VSoundEmitter001 works in a separate 32-bit token
// namespace. Keep a deliberately EXPLICIT mapping, never truncate an old ID.
// The caller must obtain native candidates through a separately verified ABI;
// this class NEVER invokes native vtables and is NOT that ABI adapter.
class VerifiedSoundTokenCrosswalk {
public:
    CrosswalkResult bind(std::int32_t old_token, std::int32_t old_count,
                         std::int64_t native_candidate, bool native_validated,
                         std::string_view old_name,
                         std::string_view native_roundtrip_name) {
        if (old_token < 0 || old_count < 0 || old_token >= old_count)
            return CrosswalkResult::LegacyIndexInvalid;
        if (native_candidate < 0 ||
            static_cast<std::uint64_t>(native_candidate) > max_native_token)
            return CrosswalkResult::NativeIndexInvalid;
        if (!native_validated) return CrosswalkResult::NativeValidationFailed;
        if (old_name.empty() || native_roundtrip_name != old_name)
            return CrosswalkResult::NameMismatch;
        auto native = static_cast<std::uint16_t>(native_candidate);
        for (const auto& entry: entries_) {
            if (entry.old_token == old_token || entry.native_token == native ||
                entry.name == old_name) {
                if (entry.old_token == old_token && entry.native_token == native &&
                    entry.name == old_name) return CrosswalkResult::AlreadyPresent;
                return CrosswalkResult::MappingConflict;
            }
        }
        entries_.push_back({old_token, native, std::string(old_name)});
        return CrosswalkResult::Added;
    }
    // Owns the copied name; stable until this crosswalk is destroyed or reset.
    // Do not free/cast it as a native sound-emitter's internal string pointer.
    const char* name_from_old_token(std::int32_t old_token) const noexcept {
        for (const auto& entry:entries_)
            if (entry.old_token == old_token) return entry.name.c_str();
        return "";
    }
    std::uint32_t native_from_old_token(std::int32_t old_token) const noexcept {
        for (const auto& entry:entries_)
            if (entry.old_token == old_token) return entry.native_token;
        return missing_token;
    }
    std::uint32_t old_from_native_token(std::uint16_t native) const noexcept {
        for (const auto& entry:entries_)
            if (entry.native_token == native)
                return static_cast<std::uint32_t>(entry.old_token);
        return missing_token;
    }
    std::size_t size() const noexcept { return entries_.size(); }
    void reset() noexcept { entries_.clear(); }
private:
    struct Entry {
        std::int32_t old_token;
        std::uint16_t native_token;
        std::string name;
    };
    // std::deque preserves references/pointers to existing elements across
    // push_back. No mutation of an entry's string after insertion.
    std::deque<Entry> entries_;
};
} // namespace sin_compat::phase99
