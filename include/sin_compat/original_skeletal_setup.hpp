#pragma once
// Independently authored behavioral dispatch of original SiN (2006)
// server.dll CBaseAnimating virtual +0x2B4 / 0x220EF510. The real
// model-cache lock, virtual pose solvers, bone merge and animation state
// are owned by the caller's native host, NOT implemented by this module.
#include "sin_compat/original_bone_preparation.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace sin_compat::jessica {
struct OriginalSkeletalSetupSnapshot {
    bool valid_studio=false;
    bool world_transform_dirty=false; // original entity +0xB8 bit 0x800
    bool has_pose_driver=false;        // original +0x3D8 pointer != nullptr
    TossVec3 world_origin{};           // original entity +0x2BC
    TossVec3 world_angles{};           // original entity +0x354
    float vertical_offset=0.f;        // original entity +0x3D4, added to Z
    std::optional<WorldEntityId> parent{}; // entity +0xF0 EHANDLE
};
class IOriginalSkeletalSetupHost {
public:
    virtual ~IOriginalSkeletalSetupHost()=default;
    // Original model cache lock encloses evaluation of both paths.
    virtual bool lock_model_cache(WorldEntityId)=0;
    virtual bool unlock_model_cache(WorldEntityId)=0;
    virtual std::optional<OriginalSkeletalSetupSnapshot> snapshot(WorldEntityId)=0;
    virtual bool refresh_world_transform(WorldEntityId)=0;
    // Only with +0x3D8 pose driver: increment +0x3DC and run helper.
    virtual bool increment_pose_driver_counter(WorldEntityId)=0;
    virtual bool seed_pose_driver(WorldEntityId,const TossVec3& origin,
                                   const TossVec3& angles,std::uint32_t mask)=0;
    // Virtual +0x2AC is ALWAYS called; its target varies by entity class.
    virtual bool evaluate_virtual_base_pose(WorldEntityId,std::uint32_t mask)=0;
    // Additional steps when original +0x3D8 != 0, all host-owned.
    virtual bool blend_pose_driver(WorldEntityId,std::uint32_t mask)=0;
    virtual bool invoke_virtual_pose_hook(WorldEntityId,std::uint32_t mask)=0;
    virtual bool finalize_pose_driver(WorldEntityId,std::uint32_t mask)=0;
    // Original uses generation-checked EHANDLE and CBaseAnimating dynamic cast.
    virtual bool parent_is_animating(WorldEntityId,WorldEntityId)=0;
    // Original calls parent 0x220EB8D0. A null cache triggers fallback.
    virtual bool prepare_parent_bone_cache(WorldEntityId)=0;
    // Original 0x220ED290 merges with parent cache; 0x220FE0C0
    // constructs bones without that cache. Both remain native-host work.
    virtual bool compose_parented_bones(WorldEntityId,WorldEntityId,
                                         std::uint32_t mask)=0;
    virtual bool compose_unparented_bones(WorldEntityId,std::uint32_t mask)=0;
    virtual std::optional<std::vector<WorldMatrix3x4>> resulting_bone_matrices(
        WorldEntityId)=0;
};

enum class OriginalSkeletalSetupCode : std::uint8_t {
    ParentMerged, Unparented, MissingEntity, MissingStudio, InvalidState,
    ModelCacheRejected, SetupRejected, MissingMatrices, HostRejected
};
struct OriginalSkeletalSetupResult {
    OriginalSkeletalSetupCode code=OriginalSkeletalSetupCode::HostRejected;
    std::vector<WorldMatrix3x4> matrices{};
    bool successful() const noexcept {
        return code==OriginalSkeletalSetupCode::ParentMerged ||
               code==OriginalSkeletalSetupCode::Unparented;
    }
};
OriginalSkeletalSetupResult run_original_skeletal_setup(
    IOriginalSkeletalSetupHost&,WorldEntityId,std::uint32_t mask) noexcept;

// Bridge Phase 113's binary-traced dispatch into Phase 112's
// IOriginalBonePreparationHost::calculate_bones callback. No native vtable
// or original x86 DLL is invoked.
class OriginalSkeletalPreparationAdapter final:public IOriginalBonePreparationHost {
public:
    OriginalSkeletalPreparationAdapter(IOriginalBonePreparationHost& cache,
                                        IOriginalSkeletalSetupHost& setup)
        :cache_(cache),setup_(setup){}
    std::optional<OriginalBonePreparationSnapshot> state(WorldEntityId id) override {
        return cache_.state(id);
    }
    std::optional<std::vector<std::uint8_t>> resolve_cache(
        OriginalBoneCacheToken t) override {return cache_.resolve_cache(t);}
    bool release_cache(OriginalBoneCacheToken t) override {
        return cache_.release_cache(t);
    }
    bool set_entity_cache_handle(WorldEntityId id,
        std::optional<OriginalBoneCacheToken> t) override {
        return cache_.set_entity_cache_handle(id,t);
    }
    std::optional<std::vector<WorldMatrix3x4>> calculate_bones(
        WorldEntityId id,std::uint32_t mask) override {
        auto r=run_original_skeletal_setup(setup_,id,mask);
        if(!r.successful())return std::nullopt;
        return std::move(r.matrices);
    }
    bool store_cache_bytes(OriginalBoneCacheToken t,
        const std::vector<std::uint8_t>& b) override {
        return cache_.store_cache_bytes(t,b);
    }
    std::optional<OriginalBoneCacheToken> allocate_cache(
        WorldEntityId id,std::uint32_t n,
        const std::vector<WorldMatrix3x4>& b,float now,
        std::uint32_t mask) override {
        return cache_.allocate_cache(id,n,b,now,mask);
    }
private:
    IOriginalBonePreparationHost& cache_;
    IOriginalSkeletalSetupHost& setup_;
};
} // namespace sin_compat::jessica
