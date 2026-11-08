#include "sin_compat/original_skeletal_setup.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
using namespace sin_compat::jessica;
void eq(float a,float b){assert(std::abs(a-b)<0.02f);}
WorldMatrix3x4 at(float x){
    WorldMatrix3x4 m{};m.row_major={1,0,0,x,0,1,0,20,0,0,1,30};return m;
}
void u16(std::vector<std::uint8_t>& b,int at,std::uint16_t v){
    b.at(at)=std::uint8_t(v);b.at(at+1)=std::uint8_t(v>>8);
}
void u32(std::vector<std::uint8_t>& b,int at,std::uint32_t v){
    for(int i=0;i<4;i++)b.at(at+i)=std::uint8_t(v>>(i*8));
}
void f32(std::vector<std::uint8_t>& b,int at,float x){
    std::uint32_t bits;std::memcpy(&bits,&x,4);u32(b,at,bits);
}
struct Cache final:IOriginalBonePreparationHost {
    float now=1.f;
    std::vector<std::uint8_t> bytes=std::vector<std::uint8_t>(0x14+8+48,0);
    std::optional<OriginalBoneCacheToken> handle=7;
    int stores=0;
    Cache(){
        u32(bytes,4,0x300); // cache type
        u16(bytes,0xc,1);   // compact matrices count
        u16(bytes,0xe,8);   // matrix data at cache+0x14+8
        u16(bytes,0x10,4);  // reverse source-bone array at cache+0x14+4
        u16(bytes,0x14,0);  // bone -> compact matrix index
        u16(bytes,0x18,0);  // compact matrix -> bone 0
        f32(bytes,0,0.f);   // stale
    }
    std::optional<OriginalBonePreparationSnapshot> state(WorldEntityId) override {
        return OriginalBonePreparationSnapshot{now,1,handle};
    }
    std::optional<std::vector<std::uint8_t>> resolve_cache(
        OriginalBoneCacheToken t) override {
        if(t!=7)return std::nullopt;
        return bytes;
    }
    bool release_cache(OriginalBoneCacheToken) override{return false;}
    bool set_entity_cache_handle(WorldEntityId,
         std::optional<OriginalBoneCacheToken>) override{return false;}
    std::optional<std::vector<WorldMatrix3x4>> calculate_bones(
         WorldEntityId,std::uint32_t) override {assert(false);return std::nullopt;}
    bool store_cache_bytes(OriginalBoneCacheToken t,
        const std::vector<std::uint8_t>& b) override {
        assert(t==7);bytes=b;++stores;return true;
    }
    std::optional<OriginalBoneCacheToken> allocate_cache(
        WorldEntityId,std::uint32_t,
        const std::vector<WorldMatrix3x4>&,float,
        std::uint32_t) override {return std::nullopt;}
};
struct Setup final:IOriginalSkeletalSetupHost {
    int builds=0,locks=0,unlocks=0;
    float current_x=10;
    std::optional<OriginalSkeletalSetupSnapshot> snapshot(WorldEntityId) override {
        OriginalSkeletalSetupSnapshot s{};s.valid_studio=true;
        s.world_origin={1,2,3};return s;
    }
    bool lock_model_cache(WorldEntityId) override {++locks;return true;}
    bool unlock_model_cache(WorldEntityId) override {++unlocks;return true;}
    bool refresh_world_transform(WorldEntityId) override {return true;}
    bool increment_pose_driver_counter(WorldEntityId) override{return true;}
    bool seed_pose_driver(WorldEntityId,const TossVec3&,
                          const TossVec3&,std::uint32_t) override{return true;}
    bool evaluate_virtual_base_pose(WorldEntityId,std::uint32_t m) override {
        assert(m==0x300);return true;
    }
    bool blend_pose_driver(WorldEntityId,std::uint32_t) override {return true;}
    bool invoke_virtual_pose_hook(WorldEntityId,std::uint32_t) override {return true;}
    bool finalize_pose_driver(WorldEntityId,std::uint32_t) override {return true;}
    bool parent_is_animating(WorldEntityId,WorldEntityId) override {
        assert(false);return false;
    }
    bool prepare_parent_bone_cache(WorldEntityId) override {
        assert(false);return false;
    }
    bool compose_parented_bones(WorldEntityId,WorldEntityId,std::uint32_t) override {
        assert(false);return false;
    }
    bool compose_unparented_bones(WorldEntityId,std::uint32_t) override {
        ++builds;return true;
    }
    std::optional<std::vector<WorldMatrix3x4>> resulting_bone_matrices(
        WorldEntityId) override {
        return std::vector<WorldMatrix3x4>{at(current_x)};
    }
};
struct Underlying final:IOriginalBoneCacheHost {
    std::optional<std::uint32_t> studio_bone_count(WorldEntityId) override {
        return 1;
    }
    std::optional<std::vector<std::uint8_t>> prepared_bone_cache(
        WorldEntityId) override {assert(false);return std::nullopt;}
    std::optional<WorldMatrix3x4> entity_world_matrix(WorldEntityId) override {
        return at(1000);
    }
};
struct Studio final:IOriginalStudioAttachmentHost {
    std::optional<std::uint32_t> attachment_count(WorldEntityId) override {
        return 1;
    }
    std::optional<OriginalStudioAttachmentRecord> attachment_record(
        WorldEntityId,std::uint32_t index) override {
        if(index)return std::nullopt;
        return OriginalStudioAttachmentRecord{0,0,at(4)};
    }
    std::optional<WorldMatrix3x4> bone_world_transform(
        WorldEntityId,std::uint32_t) override {assert(false);return std::nullopt;}
};
int main(){
    Cache cache;Setup setup;Underlying underlying;Studio studio;
    OriginalSkeletalPreparationAdapter skeletal(cache,setup);
    OriginalPreparedBoneCacheAdapter prepared(underlying,skeletal);
    OriginalBoneCacheStudioAdapter bone(studio,prepared);
    auto a=resolve_original_studio_attachment(bone,1,1);
    assert(a.successful());eq(a.matrix.row_major[3],14);
    assert(setup.builds==1 && setup.locks==1 && setup.unlocks==1);
    assert(cache.stores==1);
    cache.now=1.05f;setup.current_x=30;
    a=resolve_original_studio_attachment(bone,1,1);
    assert(a.successful());eq(a.matrix.row_major[3],14);
    assert(setup.builds==1 && cache.stores==1); // original freshness reuse
    cache.now=1.3f;
    a=resolve_original_studio_attachment(bone,1,1);
    assert(a.successful());eq(a.matrix.row_major[3],34);
    assert(setup.builds==2 && cache.stores==2);
    assert(setup.locks==setup.unlocks);
    std::cout<<"SiN skeletal dispatch -> compact cache -> studio attachment PASS\n";
}
