#pragma once
// Original 2006 engine.dll negative-index edict selection at
// 0x20110ECC..0x20110FA7, with independent fail-closed authorization.
// This does not operate a retail owner allocator or call its vtables.
#include "server_gameents_engine_edict_reuse_core.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sin_independent {
constexpr float kEngineOriginalReuseEarlyThreshold=2.0f;
constexpr double kEngineOriginalReuseMinimumElapsed=1.0;

enum class EngineEdictSelectStatus : unsigned char {
    InvalidInput, Unauthorized, NoEligibleRecord,
    CallbackRefused, PostCallbackUnauthorized,
    SelectedExisting, SelectedNew
};
struct EngineEdictSelection {
    EngineEdictSelectStatus status=EngineEdictSelectStatus::InvalidInput;
    std::size_t index=0;
    std::size_t new_count=0;
};

// The caller supplies owner-authenticated current_count and array_capacity;
// the original 0x2079DAFC count, 0x2079DB00 capacity and 0x2079DA64
// protected last index are distinct fields. 'array_bytes' is *capacity*.
// Authorizer must pin the whole array and reject concurrently recycled bytes.
inline EngineEdictSelection SelectEngineEdictForReuse(
    void* array,std::size_t array_bytes,std::size_t current_count,
    std::size_t protected_last,std::int32_t ticks,float seconds_per_tick,
    const EngineEdictRetireHooks& hooks) noexcept {
    using S=EngineEdictSelectStatus;
    if(!array||!hooks.Complete()||!hooks.authorized_array||
       array_bytes<kEngineOriginalEdictStride||
       array_bytes%kEngineOriginalEdictStride!=0||
       current_count>array_bytes/kEngineOriginalEdictStride||
       protected_last>=current_count)return {S::InvalidInput};
    if(!hooks.authorized_array(hooks.context,array,array_bytes))
        return {S::Unauthorized};
    std::uint32_t now_bits=0;
    if(!EngineEdictReuseTimeBits(ticks,seconds_per_tick,&now_bits))
        return {S::InvalidInput};
    // The scan's x87 fmul/fsub/fcomp uses the unrounded intermediate
    // product, unlike the final per-record fstp binary32 timestamp.
    const double now=static_cast<double>(static_cast<float>(ticks))*
                     static_cast<double>(seconds_per_tick);
    auto* raw=static_cast<unsigned char*>(array);
    // In the original scan, candidates start at protected_last+1.
    std::size_t index=current_count;
    bool existing=false;
    for(std::size_t i=protected_last+1;i<current_count;++i) {
        auto* record=raw+i*kEngineOriginalEdictStride;
        if(!hooks.authorized(hooks.context,record,kEngineOriginalEdictStride))
            return {S::Unauthorized};
        const auto flags=ReadEngineEdictDword(record,kEngineOriginalEdictFlags);
        if((flags&kEngineOriginalEdictFreeBit)==0)continue;
        const auto bits=ReadEngineEdictDword(record,kEngineOriginalEdictReuseTime);
        float stored=0;std::memcpy(&stored,&bits,sizeof(stored));
        // x87 fcomp 2.0, and otherwise fcomp 1.0 after now-stored.
        // NaN/Inf age inputs are rejected rather than relying on x87 parity.
        if(!std::isfinite(stored))continue;
        if(stored<kEngineOriginalReuseEarlyThreshold||
           now-static_cast<double>(stored)>=
           kEngineOriginalReuseMinimumElapsed) {
            index=i;existing=true;break;
        }
    }
    if(!existing&&index==array_bytes/kEngineOriginalEdictStride)
        return {S::NoEligibleRecord};
    if(index>=array_bytes/kEngineOriginalEdictStride)
        return {S::NoEligibleRecord};
    auto* record=raw+index*kEngineOriginalEdictStride;
    if(!hooks.authorized(hooks.context,record,kEngineOriginalEdictStride))
        return {S::Unauthorized};
    const auto release=RetireEngineEdict(record,kEngineOriginalEdictStride,hooks);
    if(release==EngineEdictRetireResult::Unauthorized)
        return {S::Unauthorized};
    if(release==EngineEdictRetireResult::CallbackRefused)
        return {S::CallbackRefused};
    if(release==EngineEdictRetireResult::PostCallbackUnauthorized)
        return {S::PostCallbackUnauthorized};
    if(release!=EngineEdictRetireResult::Retired)
        return {S::InvalidInput};
    return {existing?S::SelectedExisting:S::SelectedNew,
            index,current_count+(existing?0u:1u)};
}
} // namespace sin_independent
