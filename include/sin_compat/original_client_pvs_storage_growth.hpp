#pragma once
// Phase235 independent C++17 host reference for the original SiN 2006
// client.dll 8/16-byte linked-list storage growth methods.
// Native x86: 0x24106010 (8-byte) and 0x24106090 (16-byte).
// This is NOT a Source-engine allocator, C++ object, or gameplay DLL.
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace sin_compat::original_client_pvs_storage_growth {

inline constexpr std::uint32_t kAuxGrowthVA=0x24106010u;
inline constexpr std::uint32_t kPvsGrowthVA=0x24106090u;
inline constexpr std::uint32_t kOriginalNativeBytes=226u;
inline constexpr std::size_t kManagerStoragePointerDword=0x00u;
inline constexpr std::size_t kManagerCapacityDword=0x04u;
inline constexpr std::size_t kManagerGrowPolicyDword=0x08u;
inline constexpr std::uint32_t kOpaqueOriginalExistingStorageHelper=0x2416F960u;
inline constexpr std::uint32_t kOpaqueOriginalNullStorageHelper=0x2416F8F0u;

enum class MemoryAction { unchanged, allocate, reallocate };
struct IMemoryBackend {
    virtual ~IMemoryBackend() = default;
    // The original uses two opaque native helpers with byte-count arguments.
    // A host may reject a request; rejection is a safe divergence from x86.
    virtual bool request(MemoryAction operation, std::size_t bytes) = 0;
};

// Original layout differs by stride and zero-capacity seed, with otherwise
// near-identical native control flow. Storage is a synthetic byte buffer.
template<std::size_t Stride, std::int32_t InitialCapacity>
struct Manager {
    static_assert((Stride == 8u && InitialCapacity == 4) ||
                  (Stride == 16u && InitialCapacity == 2),
                  "only original verified eight/sixteen-byte specializations");
    using Record=std::array<std::uint8_t, Stride>;
    std::vector<Record> storage{};
    std::int32_t capacity{};
    std::int32_t grow_policy{}; // native this+8; -1 = do not grow
    bool has_storage{};         // models original this+0 pointer null check

    // Native thiscall + RET4 with stack additional-count parameter.
    // Original checks signed capacity >= capacity+additional and grows in
    // a signed loop. This host rejects invalid or overflowing values rather
    // than entering undefined native loops or allowing byte-size overflow.
    MemoryAction grow(std::int32_t additional, IMemoryBackend& backend) {
        if(grow_policy == -1) return MemoryAction::unchanged;
        if(additional < 0 || capacity < 0)
            throw std::out_of_range("negative native PVS capacity/request");
        const auto max=static_cast<std::int64_t>(
            std::numeric_limits<std::int32_t>::max());
        const auto target=static_cast<std::int64_t>(capacity)+additional;
        if(target > max)
            throw std::overflow_error("PVS native signed capacity overflow");
        // Native JGE skips ONLY the capacity-growth loop. It still invokes
        // the memory helper afterward, even for a zero additional request.
        std::int64_t next=capacity;
        if(target > capacity) {
            if(grow_policy < 0)
                throw std::out_of_range("nonprogressing native growth policy");
            while(next < target) {
                if(next == 0) next=InitialCapacity;
                else if(grow_policy != 0) next+=grow_policy;
                else next*=2;
                if(next > max || next <= 0)
                    throw std::overflow_error("native PVS capacity growth overflow");
            }
        }
        if(static_cast<std::uint64_t>(next) >
           std::numeric_limits<std::size_t>::max()/Stride)
            throw std::overflow_error("native PVS allocation byte overflow");
        const auto action=has_storage
            ? MemoryAction::reallocate : MemoryAction::allocate;
        const auto bytes=static_cast<std::size_t>(next)*Stride;
        if(!backend.request(action,bytes))
            throw std::runtime_error("synthetic PVS backend declined allocation");
        // Synthetic vector-owned storage; native allocation contents may be
        // uninitialized and the real memory helpers are not reconstructed.
        storage.resize(static_cast<std::size_t>(next));
        capacity=static_cast<std::int32_t>(next);
        has_storage=true;
        return action;
    }
};

using AuxEight=Manager<8u,4>;
using PvsSixteen=Manager<16u,2>;
} // namespace sin_compat::original_client_pvs_storage_growth
