#pragma once
// Independently reconstructed normal-path behavior of original SiN (2006)
// server.dll 0x220ED1B0 and compact bone cache lookup 0x221004C0.
// Native model-cache, CBaseAnimating and SetupBones remain host-owned.
#include "sin_compat/original_studio_attachment.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace sin_compat::jessica {
enum class OriginalBoneCacheReadCode : std::uint8_t {
    Found, NotCached, InvalidBuffer, InvalidBone, InvalidMatrix
};
struct OriginalBoneCacheReadResult {
    OriginalBoneCacheReadCode code=OriginalBoneCacheReadCode::InvalidBuffer;
    WorldMatrix3x4 matrix{};
};
// Original cache format's selected fields:
// +0x0E uint16 matrix block relative offset from cache+0x14
// +0x14 int16 bone-to-cached-matrix remap array, -1 for unmapped bones
// cache+0x14+matrixRelativeOffset+48*remap[bone] -> world 3x4 matrix.
// This parser adds bounds and floating-point checks absent from helper.
OriginalBoneCacheReadResult read_original_bone_cache_matrix(
    const std::vector<std::uint8_t>& cache,
    std::uint32_t bone_index) noexcept;

class IOriginalBoneCacheHost {
public:
    virtual ~IOriginalBoneCacheHost()=default;
    // Original 0x220ED1B0 verifies mdldata type 3 and index < hdr+0x9C.
    // Nullopt means no valid studio resource; no native pointers leak here.
    virtual std::optional<std::uint32_t> studio_bone_count(WorldEntityId)=0;
    // The original 0x220EB8D0 can compute/setup cached animation matrices.
    // Return an owned immutable snapshot; nullopt means host failure.
    virtual std::optional<std::vector<std::uint8_t>> prepared_bone_cache(
        WorldEntityId)=0;
    // Fallback original 0x220EBE70 refreshes world transform if dirty,
    // then returns entity+0x268 rather than a bogus bone matrix.
    virtual std::optional<WorldMatrix3x4> entity_world_matrix(WorldEntityId)=0;
};
enum class OriginalBoneTransformCode : std::uint8_t {
    Cached, EntityWorldFallback, MissingStudio, InvalidBone,
    InvalidCache, MissingWorldMatrix, InvalidMatrix, HostRejected
};
struct OriginalBoneTransformResult {
    OriginalBoneTransformCode code=OriginalBoneTransformCode::HostRejected;
    WorldMatrix3x4 matrix{};
    bool successful() const noexcept {
        return code==OriginalBoneTransformCode::Cached ||
               code==OriginalBoneTransformCode::EntityWorldFallback;
    }
};
OriginalBoneTransformResult resolve_original_bone_transform(
    IOriginalBoneCacheHost&,WorldEntityId entity,std::uint32_t bone) noexcept;

// Supplies Phase 110 studio attachment getter with the bone-world result;
// studio metadata is delegated to the separate owner-controlled host.
class OriginalBoneCacheStudioAdapter final:public IOriginalStudioAttachmentHost {
public:
    OriginalBoneCacheStudioAdapter(IOriginalStudioAttachmentHost& studio,
                                   IOriginalBoneCacheHost& bone)
        :studio_(studio),bone_(bone){}
    std::optional<std::uint32_t> attachment_count(WorldEntityId id) override {
        return studio_.attachment_count(id);
    }
    std::optional<OriginalStudioAttachmentRecord> attachment_record(
        WorldEntityId id,std::uint32_t index) override {
        return studio_.attachment_record(id,index);
    }
    std::optional<WorldMatrix3x4> bone_world_transform(
        WorldEntityId id,std::uint32_t bone) override {
        auto r=resolve_original_bone_transform(bone_,id,bone);
        if(!r.successful())return std::nullopt;
        return r.matrix;
    }
private:
    IOriginalStudioAttachmentHost& studio_;
    IOriginalBoneCacheHost& bone_;
};
} // namespace sin_compat::jessica
