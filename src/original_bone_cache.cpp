#include "sin_compat/original_bone_cache.hpp"
#include <cmath>
#include <cstring>
#include <limits>

namespace sin_compat::jessica {
namespace {
bool finite(const WorldMatrix3x4& m) noexcept {
    for(float value:m.row_major)if(!std::isfinite(value))return false;
    return true;
}
std::uint16_t read_u16(const std::vector<std::uint8_t>& data,
                       std::size_t off) noexcept {
    return std::uint16_t(data[off]) |
           (std::uint16_t(data[off+1])<<8);
}
float read_f32(const std::vector<std::uint8_t>& data,
               std::size_t off) noexcept {
    const std::uint32_t bits=std::uint32_t(data[off])|
        (std::uint32_t(data[off+1])<<8)|
        (std::uint32_t(data[off+2])<<16)|
        (std::uint32_t(data[off+3])<<24);
    float value=0;
    static_assert(sizeof(float)==4,"expected IEEE-754 single precision");
    std::memcpy(&value,&bits,sizeof(float));
    return value;
}
}
OriginalBoneCacheReadResult read_original_bone_cache_matrix(
    const std::vector<std::uint8_t>& cache,std::uint32_t index) noexcept {
    using Code=OriginalBoneCacheReadCode;
    constexpr std::size_t kHeader=0x14;
    constexpr std::size_t kMatrixBytes=48;
    if(cache.size()<kHeader+2)return {Code::InvalidBuffer,{}};
    // Avoid overflow even when index comes from a malformed host callback.
    if(index>(cache.size()-kHeader-2)/2)
        return {Code::InvalidBone,{}};
    const std::size_t mapping_offset=kHeader+std::size_t(index)*2;
    const std::uint16_t raw_remap=read_u16(cache,mapping_offset);
    const std::int32_t mapped=raw_remap<0x8000u
        ?std::int32_t(raw_remap):std::int32_t(raw_remap)-0x10000;
    if(mapped<0)return {Code::NotCached,{}};
    const std::size_t relative=read_u16(cache,0x0E);
    const std::size_t byte_start=kHeader+relative+std::size_t(mapped)*kMatrixBytes;
    if(byte_start>cache.size()||cache.size()-byte_start<kMatrixBytes)
        return {Code::InvalidBuffer,{}};
    WorldMatrix3x4 matrix{};
    for(std::size_t i=0;i<12;++i)
        matrix.row_major[i]=read_f32(cache,byte_start+i*4);
    if(!finite(matrix))return {Code::InvalidMatrix,{}};
    return {Code::Found,matrix};
}
OriginalBoneTransformResult resolve_original_bone_transform(
    IOriginalBoneCacheHost& host,WorldEntityId entity,
    std::uint32_t bone) noexcept {
    using Code=OriginalBoneTransformCode;
    try {
        const auto count=host.studio_bone_count(entity);
        if(!count)return {Code::MissingStudio,{}};
        if(bone>=*count)return {Code::InvalidBone,{}};
        const auto cache=host.prepared_bone_cache(entity);
        if(!cache)return {Code::InvalidCache,{}};
        const auto result=read_original_bone_cache_matrix(*cache,bone);
        if(result.code==OriginalBoneCacheReadCode::Found)
            return {Code::Cached,result.matrix};
        if(result.code!=OriginalBoneCacheReadCode::NotCached)
            return {Code::InvalidCache,{}};
        const auto world=host.entity_world_matrix(entity);
        if(!world)return {Code::MissingWorldMatrix,{}};
        if(!finite(*world))return {Code::InvalidMatrix,{}};
        return {Code::EntityWorldFallback,*world};
    }catch(...) {return {Code::HostRejected,{}};}
}
} // namespace sin_compat::jessica
