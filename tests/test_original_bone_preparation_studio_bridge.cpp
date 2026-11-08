#include "sin_compat/original_bone_preparation.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
using namespace sin_compat::jessica;
WorldMatrix3x4 translate(float x){
    return *make_local_angle_matrix({0,0,0},{x,0,0});
}
void u16(std::vector<std::uint8_t>& bytes,std::size_t off,std::uint16_t x){
    bytes.at(off)=std::uint8_t(x);bytes.at(off+1)=std::uint8_t(x>>8);
}
void u32(std::vector<std::uint8_t>& bytes,std::size_t off,std::uint32_t x){
    for(int i=0;i<4;++i)bytes.at(off+i)=std::uint8_t(x>>(8*i));
}
std::vector<std::uint8_t> fixture(){
    std::vector<std::uint8_t> bytes(0x14+0x10+48);
    u16(bytes,0x0c,1);u16(bytes,0x0e,0x10);u16(bytes,0x10,0x8);
    u16(bytes,0x14,0); // bone 0 -> compact index 0
    u16(bytes,0x1c,0); // compact index 0 -> source bone 0
    u32(bytes,4,0x300);
    return bytes;
}
struct PrepHost final:IOriginalBonePreparationHost {
    OriginalBonePreparationSnapshot s{10,1,OriginalBoneCacheToken{4}};
    std::map<OriginalBoneCacheToken,std::vector<std::uint8_t>> caches{{4,fixture()}};
    int calculations=0;
    std::optional<OriginalBonePreparationSnapshot> state(WorldEntityId) override {return s;}
    std::optional<std::vector<std::uint8_t>> resolve_cache(OriginalBoneCacheToken h) override {
        auto it=caches.find(h);if(it==caches.end())return std::nullopt;
        return it->second;
    }
    bool release_cache(OriginalBoneCacheToken) override {assert(false);return false;}
    bool set_entity_cache_handle(WorldEntityId,std::optional<OriginalBoneCacheToken>) override {
        assert(false);return false;
    }
    std::optional<std::vector<WorldMatrix3x4>> calculate_bones(WorldEntityId,
        std::uint32_t mask) override {
        assert(mask==0x300);++calculations;
        return std::vector<WorldMatrix3x4>{translate(float(10*calculations))};
    }
    bool store_cache_bytes(OriginalBoneCacheToken h,
        const std::vector<std::uint8_t>& bytes) override {caches[h]=bytes;return true;}
    std::optional<OriginalBoneCacheToken> allocate_cache(WorldEntityId,
        std::uint32_t,const std::vector<WorldMatrix3x4>&,float,
        std::uint32_t) override {assert(false);return std::nullopt;}
};
struct Bones final:IOriginalBoneCacheHost {
    std::optional<std::uint32_t> studio_bone_count(WorldEntityId) override {return 1;}
    std::optional<std::vector<std::uint8_t>> prepared_bone_cache(WorldEntityId) override {
        assert(false);return std::nullopt;
    }
    std::optional<WorldMatrix3x4> entity_world_matrix(WorldEntityId) override {
        return translate(200);
    }
};
struct Studio final:IOriginalStudioAttachmentHost {
    std::optional<std::uint32_t> attachment_count(WorldEntityId) override {return 1;}
    std::optional<OriginalStudioAttachmentRecord> attachment_record(WorldEntityId,
        std::uint32_t idx) override {
        if(idx!=0)return std::nullopt;
        return OriginalStudioAttachmentRecord{0,0,translate(3)};
    }
    std::optional<WorldMatrix3x4> bone_world_transform(WorldEntityId,
        std::uint32_t) override {assert(false);return std::nullopt;}
};
int main(){
    PrepHost prep;Bones b;Studio s;
    OriginalPreparedBoneCacheAdapter supplied(b,prep);
    OriginalBoneCacheStudioAdapter attachment(s,supplied);
    auto a=resolve_original_studio_attachment(attachment,7,1);
    assert(a.successful());assert(std::abs(a.matrix.row_major[3]-13)<0.001f);
    assert(prep.calculations==1);
    // Fresh cache bypasses animation recomputation.
    prep.s.current_time=10.05f;
    a=resolve_original_studio_attachment(attachment,7,1);
    assert(a.successful());assert(std::abs(a.matrix.row_major[3]-13)<0.001f);
    assert(prep.calculations==1);
    // Expired cache refreshes in place and yields a different bone pose.
    prep.s.current_time=11.0f;
    a=resolve_original_studio_attachment(attachment,7,1);
    assert(a.successful());assert(std::abs(a.matrix.row_major[3]-23)<0.001f);
    assert(prep.calculations==2);
    std::cout<<"SiN original bone preparation -> cache getter -> studio attachment PASS\n";
}
