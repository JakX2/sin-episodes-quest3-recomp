#include "sin_compat/original_bone_preparation.hpp"
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace sin_compat::jessica {
namespace {
constexpr std::size_t header_bytes=0x14;
constexpr std::size_t matrix_bytes=48;
constexpr std::uint32_t mask=0x300;
std::uint16_t u16(const std::vector<std::uint8_t>& b,std::size_t at) noexcept {
    return std::uint16_t(b[at])|std::uint16_t(std::uint16_t(b[at+1])<<8);
}
std::uint32_t u32(const std::vector<std::uint8_t>& b,std::size_t at) noexcept {
    return std::uint32_t(b[at])|(std::uint32_t(b[at+1])<<8)|
           (std::uint32_t(b[at+2])<<16)|(std::uint32_t(b[at+3])<<24);
}
float f32(const std::vector<std::uint8_t>& b,std::size_t at) noexcept {
    std::uint32_t bits=u32(b,at);float x;
    static_assert(sizeof(float)==4,"requires 32-bit float");
    std::memcpy(&x,&bits,4);return x;
}
void store_f32(std::vector<std::uint8_t>& b,std::size_t at,float f) noexcept {
    std::uint32_t bits;std::memcpy(&bits,&f,4);
    for(std::size_t j=0;j<4;++j)
        b[at+j]=static_cast<std::uint8_t>(bits>>(j*8));
}
bool finite(const WorldMatrix3x4& m) noexcept {
    for(float value:m.row_major)if(!std::isfinite(value))return false;
    return true;
}
}

bool original_bone_cache_is_fresh(float now,float cached_at) noexcept {
    if(!std::isfinite(now)||!std::isfinite(cached_at))return false;
    return double(now)-double(cached_at)<=double(0.1f);
}

OriginalBoneUpdateResult update_original_compact_bone_cache(
    const std::vector<std::uint8_t>& old,
    const std::vector<WorldMatrix3x4>& poses,float now) noexcept {
    using Code=OriginalBoneUpdateCode;
    if(!std::isfinite(now))return {Code::InvalidTime,{}};
    if(old.size()<header_bytes)return {Code::InvalidBuffer,{}};
    const std::size_t n=u16(old,0x0c);
    const std::size_t matrix_start=header_bytes+u16(old,0x0e);
    const std::size_t reverse_start=header_bytes+u16(old,0x10);
    // Validate full source/destination span before any memory access.
    if(reverse_start>old.size()||n>(old.size()-reverse_start)/2 ||
       matrix_start>old.size()||n>(old.size()-matrix_start)/matrix_bytes)
        return {Code::InvalidBuffer,{}};
    try {
    auto result=old; // transactional validation: a failed update publishes none
    for(std::size_t i=0;i<n;++i) {
        const std::uint16_t raw=u16(old,reverse_start+2*i);
        const std::int32_t bone=raw<0x8000u?
            std::int32_t(raw):std::int32_t(raw)-0x10000;
        if(bone<0 || std::size_t(bone)>=poses.size())
            return {Code::InvalidSourceIndex,{}};
        const auto& m=poses[std::size_t(bone)];
        if(!finite(m))return {Code::InvalidMatrix,{}};
        const std::size_t at=matrix_start+i*matrix_bytes;
        for(std::size_t j=0;j<12;++j)store_f32(result,at+j*4,m.row_major[j]);
    }
    store_f32(result,0,now); // original 0x22100450 updates +0 time
    return {Code::Updated,std::move(result)};
    }catch(...) {return {Code::InvalidBuffer,{}};}
}

OriginalBonePrepareResult prepare_original_bone_cache(
    IOriginalBonePreparationHost& host,WorldEntityId id) noexcept {
    using Code=OriginalBonePrepareCode;
    try {
        const auto state=host.state(id);
        if(!state)return {Code::MissingState,{},{}};
        if(!std::isfinite(state->current_time))return {Code::InvalidTime,{},{}};
        const auto now=state->current_time;
        auto current=state->stored_handle;
        std::optional<std::vector<std::uint8_t>> previous;
        if(current)previous=host.resolve_cache(*current);
        // Original compares the timestamp before checking cached type 0x300.
        // Even a mismatched tag returns immediately if still fresh.
        if(previous) {
            if(previous->size()<8)return {Code::InvalidCache,{},{}};
            const float cached_at=f32(*previous,0);
            if(!std::isfinite(cached_at))return {Code::InvalidTime,{},{}};
            if(original_bone_cache_is_fresh(now,cached_at))
                return {Code::Fresh,current,*previous};
            if(u32(*previous,4)!=mask) {
                if(!host.release_cache(*current)||
                   !host.set_entity_cache_handle(id,std::nullopt))
                    return {Code::HostRejected,{},{}};
                current.reset();previous.reset();
            }
        }
        // The 2006 caller dereferences studio header for in-place update;
        // this portable path fails safely instead of dereferencing null.
        if(!state->studio_bone_count)return {Code::MissingStudio,{},{}};
        // This callback represents original virtual +0x2B4. Its detailed
        // skeletal solver and game-owned MDL state are NOT reimplemented.
        const auto pose=host.calculate_bones(id,mask);
        if(!pose)return {Code::InvalidPose,{},{}};
        if(previous && current) {
            const auto update=update_original_compact_bone_cache(*previous,*pose,now);
            if(!update.successful())return {Code::InvalidCache,{},{}};
            if(!host.store_cache_bytes(*current,update.bytes))
                return {Code::HostRejected,{},{}};
            return {Code::Updated,current,update.bytes};
        }
        const auto allocated=host.allocate_cache(id,*state->studio_bone_count,
                                                  *pose,now,mask);
        if(!allocated)return {Code::HostRejected,{},{}};
        if(!host.set_entity_cache_handle(id,*allocated))
            return {Code::HostRejected,{},{}};
        const auto fresh=host.resolve_cache(*allocated);
        if(!fresh||fresh->size()<header_bytes)return {Code::InvalidCache,{},{}};
        return {Code::Allocated,allocated,*fresh};
    }catch(...) {return {Code::HostRejected,{},{}};}
}
} // namespace sin_compat::jessica
