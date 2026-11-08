#pragma once
// Independently authored model of the 2006 SiN server.dll 0x220EB8D0
// cached bone preparation and 0x22100450 reverse-mapped matrix updater.
// Original model-cache, animation evaluation, handles and SetupBones stay
// behind the host boundary. No native x86 ABI is used here.
#include "sin_compat/original_bone_cache.hpp"
#include <cstdint>
#include <optional>
#include <vector>
#include <utility>

namespace sin_compat::jessica {
using OriginalBoneCacheToken = std::uint64_t;

struct OriginalBonePreparationSnapshot {
    float current_time=0.0f;             // original globalcurtime +0xC
    std::optional<std::uint32_t> studio_bone_count{}; // hdr +0x9C
    std::optional<OriginalBoneCacheToken> stored_handle{}; // entity +0x478
};

class IOriginalBonePreparationHost {
public:
    virtual ~IOriginalBonePreparationHost()=default;
    virtual std::optional<OriginalBonePreparationSnapshot> state(WorldEntityId)=0;
    // Original registry resolves the entity's stored handle via 0x22101AE0.
    virtual std::optional<std::vector<std::uint8_t>> resolve_cache(
        OriginalBoneCacheToken)=0;
    virtual bool release_cache(OriginalBoneCacheToken)=0;
    virtual bool set_entity_cache_handle(WorldEntityId,
        std::optional<OriginalBoneCacheToken>)=0;
    // Real virtual +0x2B4/0x220EF510 writes pose/bone matrices, mask=0x300.
    // This host callback is NOT independently reconstructed SetupBones.
    virtual std::optional<std::vector<WorldMatrix3x4>> calculate_bones(
        WorldEntityId,std::uint32_t mask)=0;
    virtual bool store_cache_bytes(OriginalBoneCacheToken,
        const std::vector<std::uint8_t>&)=0;
    // Real 0x22104BC0 allocates and registers a compact model cache. Caller
    // supplies the pose output and studio count; host builds/cache-registers.
    virtual std::optional<OriginalBoneCacheToken> allocate_cache(
        WorldEntityId,std::uint32_t studio_bones,
        const std::vector<WorldMatrix3x4>& pose,float now,
        std::uint32_t mask)=0;
};

enum class OriginalBoneUpdateCode : std::uint8_t {
    Updated, InvalidBuffer, InvalidSourceIndex, InvalidMatrix, InvalidTime
};
struct OriginalBoneUpdateResult {
    OriginalBoneUpdateCode code=OriginalBoneUpdateCode::InvalidBuffer;
    std::vector<std::uint8_t> bytes{};
    bool successful() const noexcept {return code==OriginalBoneUpdateCode::Updated;}
};
// Original cache header: +0x00 timestamp, +0x0C matrix count,
// +0x0E matrix-array relative offset and +0x10 reverse-map offset.
// The reverse map starts at cache+0x14+uint16(+0x10): signed int16 source
// bone index for each cached 48-byte matrix. This bounded/transactional
// implementation validates all referenced matrices before modifying bytes.
OriginalBoneUpdateResult update_original_compact_bone_cache(
    const std::vector<std::uint8_t>& cache,
    const std::vector<WorldMatrix3x4>& source_bones,
    float now) noexcept;

// Original 0x220FC5A0 compares (now - cacheTimestamp) <= 0.1f.
// Deliberately accepts future-dated caches, matching that comparison.
// NaNs are rejected; floating-point intermediates use portable doubles.
bool original_bone_cache_is_fresh(float now,float cached_at) noexcept;

enum class OriginalBonePrepareCode : std::uint8_t {
    Fresh, Updated, Allocated, MissingState, MissingStudio,
    InvalidTime, InvalidCache, InvalidPose, HostRejected
};
struct OriginalBonePrepareResult {
    OriginalBonePrepareCode code=OriginalBonePrepareCode::HostRejected;
    std::optional<OriginalBoneCacheToken> handle{};
    std::vector<std::uint8_t> bytes{};
    bool successful() const noexcept {
        return code==OriginalBonePrepareCode::Fresh ||
               code==OriginalBonePrepareCode::Updated ||
               code==OriginalBonePrepareCode::Allocated;
    }
};
OriginalBonePrepareResult prepare_original_bone_cache(
    IOriginalBonePreparationHost&,WorldEntityId) noexcept;

// Phase 111 adapter: reuse the recovered getter/remap/fallback unchanged.
class OriginalPreparedBoneCacheAdapter final:public IOriginalBoneCacheHost {
public:
    OriginalPreparedBoneCacheAdapter(IOriginalBoneCacheHost& underlying,
                                     IOriginalBonePreparationHost& preparation)
        :underlying_(underlying),preparation_(preparation){}
    std::optional<std::uint32_t> studio_bone_count(WorldEntityId id) override {
        return underlying_.studio_bone_count(id);
    }
    std::optional<std::vector<std::uint8_t>> prepared_bone_cache(
        WorldEntityId id) override {
        auto r=prepare_original_bone_cache(preparation_,id);
        if(!r.successful())return std::nullopt;
        return std::move(r.bytes);
    }
    std::optional<WorldMatrix3x4> entity_world_matrix(WorldEntityId id) override {
        return underlying_.entity_world_matrix(id);
    }
private:
    IOriginalBoneCacheHost& underlying_;
    IOriginalBonePreparationHost& preparation_;
};
} // namespace sin_compat::jessica
