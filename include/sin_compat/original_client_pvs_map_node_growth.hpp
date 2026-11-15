#pragma once
// Phase238 independent C++17 host reference for original 2006 SiN client
// PVS map node address resolution and map-specific 16-byte storage growth.
// Native entries 0x24105F40 and 0x24106140. Not engine-owned classes.
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include "sin_compat/original_client_pvs_map_search_index.hpp"
#include "sin_compat/original_client_pvs_storage_growth.hpp"

namespace sin_compat::original_client_pvs_map_node_growth {

namespace map = sin_compat::original_client_pvs_map_frontends;
namespace index = sin_compat::original_client_pvs_map_search_index;
namespace growth = sin_compat::original_client_pvs_storage_growth;

inline constexpr std::uint32_t kNativeResolverVA=0x24105F40u;
inline constexpr std::uint32_t kNativeGrowthVA=0x24106140u;
inline constexpr std::size_t kOriginalReviewedMethods=2u;
inline constexpr std::size_t kOriginalReviewedCodeBytes=191u;
inline constexpr std::uint32_t kNativeSentinelVA=0x2433FDBCu;
inline constexpr std::uint32_t kNativeInitFlagVA=0x2433FDC4u;
inline constexpr std::size_t kNativePointerDwordMapOffset=0x04u;
inline constexpr std::size_t kNativeCapacityDwordMapOffset=0x08u;
inline constexpr std::size_t kNativeGrowPolicyDwordMapOffset=0x0Cu;
inline constexpr std::size_t kNativeRecordStride=16u;
inline constexpr std::uint16_t kMissingIndex=0xFFFFu;
inline constexpr std::uint32_t kOriginalExistingStorageHelper=0x2416F960u;
inline constexpr std::uint32_t kOriginalNullStorageHelper=0x2416F8F0u;

// Native 0x24105F40 checks a process-global bit, and on first call writes
// WORDs FFFF, FFFF, FFFF, 0001 into the global sentinel header. Model only
// these original proven first 8 bytes; the native global's later bytes are
// not inferred. This state is host-injected, NOT a reconstructed CRT global.
struct ResolverState {
    bool initialized{};
    std::array<std::uint16_t,4u> sentinel_header{};
    std::size_t initializations{};
};
struct NodeView {
    bool is_sentinel{};
    const map::Record* record{}; // null for original global nil sentinel
    std::array<std::uint16_t,4u> header{};
    std::uint16_t free_next() const noexcept { return header[1u]; }
};

inline NodeView resolve(const map::Map& manager,std::uint16_t record_index,
                        ResolverState& state) {
    if(!state.initialized) {
        state.initialized=true;
        state.sentinel_header={{kMissingIndex,kMissingIndex,kMissingIndex,1u}};
        ++state.initializations;
    }
    if(record_index==kMissingIndex)
        return {true,nullptr,state.sentinel_header};
    // Native thiscall computes pointer = storage DWORD+4 + index*16,
    // with no bounds check. The host explicitly refuses bad indices.
    const map::Record& record=map::checked(manager,record_index);
    const auto opaque=record.opaque_header;
    return {false,&record,{{
        record.left,record.right,
        static_cast<std::uint16_t>(opaque&0xFFFFu),
        static_cast<std::uint16_t>(opaque>>16)
    }}};
}

// Phase235 already modelled the same *semantics* for 16-byte list growth,
// but this original map-specific function has its own 110-byte SHA256 body
// and is called with ECX=map+4, not the PVS-list manager. Reuse the verified
// policy model without conflating the actual original native function VAs.
struct NativeIndexCallbacks final:index::IndexCallbacks {
    growth::PvsSixteen storage{};
    growth::IMemoryBackend& memory;
    ResolverState& resolver;
    std::size_t recycle_resolutions{};

    NativeIndexCallbacks(const map::Map& manager,
                         growth::IMemoryBackend& backend,
                         ResolverState& resolver_state,
                         std::int32_t grow_policy=0):
        memory(backend),resolver(resolver_state) {
        if(manager.nodes.size()>=kMissingIndex)
            throw std::out_of_range("map record count exceeds 16-bit native index space");
        storage.capacity=static_cast<std::int32_t>(manager.nodes.size());
        storage.storage.resize(manager.nodes.size());
        storage.has_storage=!manager.nodes.empty();
        storage.grow_policy=grow_policy;
    }

    growth::MemoryAction grow_records(map::Map& manager,
                                     std::int32_t additional) {
        if(storage.capacity<0 ||
           static_cast<std::size_t>(storage.capacity)!=manager.nodes.size())
            throw std::logic_error("original map capacity host models are out of sync");
        // Fail closed BEFORE native-like memory callbacks when the next
        // normal increment/doubling could outrun the WORD index domain.
        if(additional>0 && storage.grow_policy!=-1) {
            const std::int64_t cap=storage.capacity;
            const std::int64_t policy=storage.grow_policy;
            const std::int64_t target=cap+additional;
            if(target>=kMissingIndex)
                throw std::out_of_range("native 16-bit map index domain exhausted");
            std::int64_t predicted=cap;
            if(policy < 0)
                throw std::out_of_range("invalid synthetic grow policy");
            while(predicted<target) {
                if(predicted==0)predicted=2;
                else if(policy==0)predicted*=2;
                else predicted+=policy;
                if(predicted>=kMissingIndex)
                    throw std::out_of_range("native map growth exceeds WORD index range");
            }
        }
        const auto action=storage.grow(additional,memory);
        if(storage.capacity<0 ||
           static_cast<std::uint64_t>(storage.capacity)>=kMissingIndex)
            throw std::out_of_range("invalid native map capacity");
        manager.nodes.resize(static_cast<std::size_t>(storage.capacity));
        return action;
    }

    void grow_one(map::Map& manager) override {
        (void)grow_records(manager,1);
    }

    std::uint16_t recycled_next(map::Map& manager,
                                std::uint16_t record_index) override {
        ++recycle_resolutions;
        return resolve(manager,record_index,resolver).free_next();
    }
};

} // namespace sin_compat::original_client_pvs_map_node_growth
