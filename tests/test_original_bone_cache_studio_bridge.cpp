#include "sin_compat/original_bone_cache.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
using namespace sin_compat::jessica;
void eq(float a,float b){assert(std::abs(a-b)<0.01f);}
WorldMatrix3x4 pose(TossVec3 xyz){
    return *make_local_angle_matrix({0,0,0},xyz);
}
struct Studio final:IOriginalStudioAttachmentHost {
    std::optional<std::uint32_t> attachment_count(WorldEntityId) override {return 1;}
    std::optional<OriginalStudioAttachmentRecord> attachment_record(
        WorldEntityId,std::uint32_t index) override {
        if(index>0)return std::nullopt;
        return OriginalStudioAttachmentRecord{0,1,pose({4,5,6})};
    }
    std::optional<WorldMatrix3x4> bone_world_transform(
        WorldEntityId,std::uint32_t) override {assert(false);return {};}
};
struct BoneHost final:IOriginalBoneCacheHost {
    bool use_cached=true;
    std::optional<std::uint32_t> studio_bone_count(WorldEntityId) override {
        return 2;
    }
    std::optional<std::vector<std::uint8_t>> prepared_bone_cache(WorldEntityId) override {
        std::vector<std::uint8_t> bytes(0x14+8+48,0);
        bytes[0x0e]=8;
        bytes[0x16]=use_cached?0:0xff;
        bytes[0x17]=use_cached?0:0xff;
        auto m=pose({10,20,30});
        for(std::size_t i=0;i<12;i++) {
            std::uint32_t bits=0;std::memcpy(&bits,&m.row_major[i],4);
            for(int j=0;j<4;j++)bytes[0x1c+i*4+j]=std::uint8_t(bits>>(8*j));
        }
        return bytes;
    }
    std::optional<WorldMatrix3x4> entity_world_matrix(WorldEntityId) override {
        return pose({100,200,300});
    }
};
int main(){
    Studio s;BoneHost b;OriginalBoneCacheStudioAdapter bridge(s,b);
    auto r=resolve_original_studio_attachment(bridge,1,1);
    assert(r.successful());
    eq(r.matrix.row_major[3],14);eq(r.matrix.row_major[7],25);
    eq(r.matrix.row_major[11],36);
    b.use_cached=false;
    r=resolve_original_studio_attachment(bridge,1,1);
    assert(r.successful());
    eq(r.matrix.row_major[3],104);eq(r.matrix.row_major[7],205);
    eq(r.matrix.row_major[11],306);
    std::cout<<"Original SiN bone cache -> studio attachment bridge PASS\n";
}
