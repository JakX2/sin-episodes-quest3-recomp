#pragma once
// Independently authored, owner-bounded reconstruction of 2006 engine.dll
// edict reuse at 0x20110FB0..0x20111008. NOT an installed retail hook.
// This transition is distinct from edict selection and from entity creation.
#include "server_gameents_engine_edict_retire_core.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace sin_independent {

struct EngineEdictReuseEvent {
    std::size_t index=0;
    std::uint32_t prior_serial=0;
    std::uint32_t next_serial=0;
    std::uint32_t time_bits=0;
};

struct EngineEdictReuseHooks {
    void* context=nullptr;
    // These callbacks authenticate allocation extents/generations without
    // reading an untrusted pointer. The owner pins physical backing for the
    // whole transition: individual callback checks do not pin it by themselves.
    bool (*authorized_array)(void*,const void*,std::size_t) noexcept=nullptr;
    bool (*authorized_edict)(void*,const void*,std::size_t) noexcept=nullptr;
    // Original GameEnts +0x0C is called on the inactive, non-reserved edict.
    // A successful no-entity cleanup can be reported as true by the adapter.
    bool (*free_containing_entity)(void*,void*) noexcept=nullptr;
    // Trusted host reserves a NEW, globally monotonic uint64 owner epoch.
    // This is NOT the original uint32 serial. No entity is enrolled here.
    bool (*reserve_epoch)(void*,const EngineEdictReuseEvent&,
                          std::uint64_t*) noexcept=nullptr;
    bool Complete() const noexcept {
        return authorized_array&&authorized_edict&&
               free_containing_entity&&reserve_epoch;
    }
};

enum class EngineEdictReuseStatus : unsigned char {
    InvalidInput, Unauthorized, AlreadyActive, ReservedIndex,
    CallbackRefused, PostCallbackUnauthorized, SerialChanged,
    EpochRefused, Reused
};
struct EngineEdictReuseResult {
    EngineEdictReuseStatus status=EngineEdictReuseStatus::InvalidInput;
    std::uint64_t owner_epoch=0;
    std::uint32_t new_serial=0;
};

// Engine VA 0x20110FF7 first multiplies a float-rounded signed integer tick
// by a float scale, then at 0x20111004 stores its rounded float result.
// Double is sufficient for the exact product of two IEEE binary32 inputs;
// it avoids a second intermediate binary32 rounding on non-x87 hosts.
inline bool EngineEdictReuseTimeBits(std::int32_t ticks,float scale,
                                     std::uint32_t* bits) noexcept {
    static_assert(sizeof(float)==sizeof(std::uint32_t),
                  "original engine reuse timestamp is IEEE binary32");
    if(!bits||!std::isfinite(scale)||scale<0.0f)return false;
    const float rounded_ticks=static_cast<float>(ticks);
    const double product=static_cast<double>(rounded_ticks)*
                         static_cast<double>(scale);
    if(!std::isfinite(product)||
       product>static_cast<double>((std::numeric_limits<float>::max)())||
       product<-static_cast<double>((std::numeric_limits<float>::max)()))
        return false;
    const float stamped=static_cast<float>(product);
    std::memcpy(bits,&stamped,sizeof(*bits));
    return true;
}

// Independently safe subset of original VA 0x20110FB0:
// 1. validate whole array and indexed 0x14-byte record before reading,
// 2. if active (flag & 2), no-op; if index in [1,reserved_last], no-op,
// 3. dispatch original GameEnts +0x0C free callback,
// 4. revalidate address and unchanged serial before the engine's own writes,
// 5. reserve a separate host owner epoch, then set bit 2, increment serial,
//    and stamp reuse-time float bits. No original DLL code is executed.
// This safety order is intentionally stricter than retail after step 3.
inline EngineEdictReuseResult ReuseEngineEdictAt(
    void* array,std::size_t bytes,int index,std::size_t reserved_last,
    std::int32_t ticks,float seconds_per_tick,
    const EngineEdictReuseHooks& hooks) noexcept {
    using S=EngineEdictReuseStatus;
    if(!array||index<0||!hooks.Complete()||
       bytes<kEngineOriginalEdictStride||
       bytes%kEngineOriginalEdictStride!=0)return {S::InvalidInput};
    // Authenticate before constructing any pointer to the indexed record.
    if(!hooks.authorized_array(hooks.context,array,bytes))
        return {S::Unauthorized};
    const auto at=static_cast<std::size_t>(index);
    if(at>=bytes/kEngineOriginalEdictStride)return {S::InvalidInput};
    auto* raw=static_cast<unsigned char*>(array)+
              at*kEngineOriginalEdictStride;
    if(!hooks.authorized_edict(hooks.context,raw,kEngineOriginalEdictStride))
        return {S::Unauthorized};
    if(ReadEngineEdictDword(raw,kEngineOriginalEdictFlags)&
       kEngineOriginalEdictFreeBit)return {S::AlreadyActive};
    // Original compares index >= 1 and index <= [0x2079DA64].
    if(at>=1&&at<=reserved_last)return {S::ReservedIndex};
    const auto old_serial=ReadEngineEdictDword(raw,kEngineOriginalEdictSerial);
    std::uint32_t time_bits=0;
    if(!EngineEdictReuseTimeBits(ticks,seconds_per_tick,&time_bits))
        return {S::InvalidInput};
    if(!hooks.free_containing_entity(hooks.context,raw))
        return {S::CallbackRefused};
    if(!hooks.authorized_array(hooks.context,array,bytes)||
       !hooks.authorized_edict(hooks.context,raw,kEngineOriginalEdictStride))
        return {S::PostCallbackUnauthorized};
    if(ReadEngineEdictDword(raw,kEngineOriginalEdictSerial)!=old_serial||
       (ReadEngineEdictDword(raw,kEngineOriginalEdictFlags)&
        kEngineOriginalEdictFreeBit))return {S::SerialChanged};
    const std::uint32_t next_serial=old_serial+1u; // retail wraps at 32 bits
    const EngineEdictReuseEvent event{at,old_serial,next_serial,time_bits};
    std::uint64_t epoch=0;
    if(!hooks.reserve_epoch(hooks.context,event,&epoch)||epoch==0)
        return {S::EpochRefused};
    // A misbehaving host epoch callback must not silently swap the backing.
    if(!hooks.authorized_array(hooks.context,array,bytes)||
       !hooks.authorized_edict(hooks.context,raw,kEngineOriginalEdictStride))
        return {S::PostCallbackUnauthorized};
    if(ReadEngineEdictDword(raw,kEngineOriginalEdictSerial)!=old_serial||
       (ReadEngineEdictDword(raw,kEngineOriginalEdictFlags)&
        kEngineOriginalEdictFreeBit))return {S::SerialChanged};
    WriteEngineEdictDword(raw,kEngineOriginalEdictFlags,
                         ReadEngineEdictDword(raw,kEngineOriginalEdictFlags)|
                         kEngineOriginalEdictFreeBit);
    WriteEngineEdictDword(raw,kEngineOriginalEdictSerial,next_serial);
    WriteEngineEdictDword(raw,kEngineOriginalEdictReuseTime,time_bits);
    return {S::Reused,epoch,next_serial};
}
} // namespace sin_independent
