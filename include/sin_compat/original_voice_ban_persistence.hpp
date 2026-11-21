#pragma once
// Source-bounded C++17 reference for the original 2006 CVoiceStatus
// voice_ban.dt V1 disk layout. This is NOT the retail game's source.
// Strict input validation below is additional host safety, not an assertion
// about the original x86 reader's behavior on malformed files.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sin_compat::original_voice_ban {
constexpr std::uint32_t kDiskVersion = 1;
constexpr std::size_t kDiskHeaderBytes = 4;
constexpr std::size_t kKeyBytes = 32;
constexpr std::size_t kBucketCount = 256;
constexpr std::size_t kNativeBucketStride = 40;

struct Key32 {
    std::array<std::uint8_t, kKeyBytes> data{};
    bool operator==(const Key32& rhs) const noexcept { return data == rhs.data; }
};

inline std::uint8_t bucket_for(const Key32& key) noexcept {
    std::uint8_t accumulator = 0;
    for (std::uint8_t byte : key.data)
        accumulator = static_cast<std::uint8_t>(accumulator + byte);
    return accumulator;
}

// Models original 256-bucket iteration and 32-byte record placement.
// The C++ vectors are deliberately not representations of the game's native
// 40-byte pointer-linked nodes or its allocator semantics.
class KeyTable {
public:
    void clear() noexcept {
        for (auto& bucket : buckets_) bucket.clear();
    }
    bool insert_unique(const Key32& key) {
        auto& bucket = buckets_[bucket_for(key)];
        for (const Key32& candidate : bucket)
            if (candidate == key) return false;
        bucket.push_back(key);
        return true;
    }
    bool contains(const Key32& key) const noexcept {
        for (const Key32& candidate : buckets_[bucket_for(key)])
            if (candidate == key) return true;
        return false;
    }
    std::size_t count() const noexcept {
        std::size_t n = 0;
        for (const auto& bucket : buckets_) n += bucket.size();
        return n;
    }
    template <typename Visit>
    void visit_native_bucket_order(Visit&& visitor) const {
        for (const auto& bucket : buckets_)
            for (const Key32& key : bucket) visitor(key);
    }
private:
    std::array<std::vector<Key32>, kBucketCount> buckets_{};
};

inline std::vector<std::uint8_t> save_v1(const KeyTable& table) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(kDiskHeaderBytes + table.count() * kKeyBytes);
    bytes.push_back(1); // native little-endian write of uint32 version 1
    bytes.push_back(0);
    bytes.push_back(0);
    bytes.push_back(0);
    table.visit_native_bucket_order([&](const Key32& key) {
        bytes.insert(bytes.end(), key.data.begin(), key.data.end());
    });
    return bytes;
}

// Independently added defensive behavior: reject truncated version, wrong
// version, non-multiple-of-32 records, and an explicitly limited input size.
// The original routine does not validate all these cases and returns true
// for some missing/invalid files; do not claim parity on malformed data.
inline bool load_v1_strict(const std::vector<std::uint8_t>& data,
                           KeyTable& destination,
                           std::size_t max_records = 1u << 20) {
    destination.clear(); // the native reader clears *before* file open
    if (data.size() < kDiskHeaderBytes) return false;
    const std::uint32_t version =
        std::uint32_t(data[0]) |
        (std::uint32_t(data[1]) << 8) |
        (std::uint32_t(data[2]) << 16) |
        (std::uint32_t(data[3]) << 24);
    if (version != kDiskVersion) return false;
    const std::size_t payload = data.size() - kDiskHeaderBytes;
    if ((payload % kKeyBytes) != 0 || (payload / kKeyBytes) > max_records)
        return false;
    const std::size_t records = payload / kKeyBytes;
    for (std::size_t n = 0; n < records; ++n) {
        Key32 key;
        const std::size_t start = kDiskHeaderBytes + n * kKeyBytes;
        for (std::size_t j = 0; j < kKeyBytes; ++j)
            key.data[j] = data[start + j];
        destination.insert_unique(key);
    }
    return true;
}
} // namespace sin_compat::original_voice_ban