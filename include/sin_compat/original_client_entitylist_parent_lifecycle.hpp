#pragma once
// Phase231 original 2006 SiN client.dll parent entity-list vtable slots 0,1,2.
// Independent host-only observable field/event reference: NO original game
// engine objects, proprietary methods or actual native pointers are run.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_entitylist_handle_dispatch.hpp"

namespace sin_compat::original_client_entitylist_parent_lifecycle {
namespace entitylist=original_client_entitylist_handle_dispatch;
using Bytes=entitylist::Bytes;

inline constexpr std::uint32_t kParentVtable=0x242973B8u;
inline constexpr std::uint32_t kEmbeddedVtable=0x24297390u;
inline constexpr std::uint32_t kParentObjectVA=0x2433FDC8u;
inline constexpr std::uint32_t kParentVptrConstructorStore=0x241081A2u;
inline constexpr std::uint32_t kIndexMask=0xFFFu;
inline constexpr std::uint32_t kMirrorGuardExclusive=0x800u;
inline constexpr std::uint32_t kMirrorStride=8u;
inline constexpr std::uint32_t kMirrorPointerField=0x10028u;
inline constexpr std::uint32_t kMirrorAuxField=0x1002Cu;
inline constexpr std::uint32_t kLiveCountField=0x10018u;
inline constexpr std::uint32_t kSecondaryFlagCountField=0x10020u;
inline constexpr std::uint32_t kHighestSlotField=0x10024u;
inline constexpr std::uint32_t kAuxStorageOffset=0x18028u;
inline constexpr std::uint32_t kSecondaryFlagBit=0x400u;
inline constexpr std::uint16_t kInvalidAuxIndex=0xFFFFu;
inline constexpr std::size_t kNewFullyReviewedMethods=3u;
inline constexpr std::size_t kNewOriginalNativeBytes=348u;
struct Method {std::uint32_t slot,va,size,stack_pop;};
inline constexpr Method kMethods[]={
 {0u,0x241089D0u,157u,8u},
 {1u,0x24108A70u,161u,8u},
 {2u,0x241082A0u,30u,4u},
};
inline std::size_t checked_offset(std::size_t start,std::uint64_t relative) {
    if(relative>std::numeric_limits<std::size_t>::max() ||
       start>std::numeric_limits<std::size_t>::max()-static_cast<std::size_t>(relative))
        throw std::overflow_error("host-only original parent offset overflow");
    return start+static_cast<std::size_t>(relative);
}
inline void ensure_dword(const Bytes& bytes,std::size_t pos) {
    if(pos>bytes.size() || bytes.size()-pos<4u)
        throw std::out_of_range("host-only original entity-list DWORD outside object");
}
inline void ensure_word(const Bytes& bytes,std::size_t pos) {
    if(pos>bytes.size() || bytes.size()-pos<2u)
        throw std::out_of_range("host-only original entity-list WORD outside object");
}
inline std::uint32_t read_u32(const Bytes& b,std::size_t pos){
    return entitylist::load_u32(b,pos);
}
inline void write_u32(Bytes& b,std::size_t pos,std::uint32_t x){
    entitylist::write_u32(b,pos,x);
}
inline std::uint16_t read_u16(const Bytes& b,std::size_t pos){
    ensure_word(b,pos);
    return static_cast<std::uint16_t>(std::uint16_t(b[pos])|
                                     (std::uint16_t(b[pos+1u])<<8));
}
inline void write_u16(Bytes& b,std::size_t pos,std::uint16_t value){
    ensure_word(b,pos);
    b[pos]=static_cast<std::uint8_t>(value&255u);
    b[pos+1u]=static_cast<std::uint8_t>(value>>8);
}
inline std::int64_t i386_signed(std::uint32_t x) noexcept {
    return x<=0x7FFFFFFFu?static_cast<std::int64_t>(x):
          static_cast<std::int64_t>(x)-0x100000000LL;
}
inline std::size_t mirror_at(std::size_t parent,std::uint32_t index,
                             std::uint32_t field) {
    return checked_offset(parent,std::uint64_t(field)+
                          std::uint64_t(index)*kMirrorStride);
}
// These injected methods stand for real native helpers at 0x24108880,
// 0x24108910, 0x24108680, 0x24106C20 and entity vtable methods.
// NONE of their original C++ object side effects are fabricated.
struct IHost {
    virtual ~IHost()=default;
    virtual std::uint32_t entity_virtual(std::uint32_t entity_token,
                                         std::uint32_t vtable_byte_offset)=0;
    virtual void registry_add_24108880(std::uint32_t entity_token)=0;
    virtual void registry_remove_24108910(std::uint32_t entity_token)=0;
    virtual std::uint16_t aux_acquire_24108680(std::uint32_t secondary_token)=0;
    virtual void aux_release_24106C20(std::uint16_t index)=0;
    virtual void call_destructor_24107430(std::uint32_t parent_token)=0;
    virtual void free_object_2416FD60(std::uint32_t parent_token)=0;
};
// Native parent slot0, MOV/virtual/CALL order preserved. Raw handle is
// masked to 12 bits; only slots <2048 alter the mirror pointer/count,
// but auxiliary indices may be read/written for all 4096 mask values.
inline void observe_add(Bytes& bytes,std::size_t parent,
        std::uint32_t entity_token,std::uint32_t raw_handle,IHost& host) {
    if(entity_token==0u)
        throw std::invalid_argument("host-only null entity not executable");
    const std::uint32_t index=raw_handle&kIndexMask;
    const auto aux=mirror_at(parent,index,kMirrorAuxField);
    ensure_word(bytes,aux);
    if(index<kMirrorGuardExclusive) {
        const auto ptr=mirror_at(parent,index,kMirrorPointerField);
        const auto count=checked_offset(parent,kLiveCountField);
        const auto highest=checked_offset(parent,kHighestSlotField);
        ensure_dword(bytes,ptr);ensure_dword(bytes,count);ensure_dword(bytes,highest);
        write_u32(bytes,count,read_u32(bytes,count)+1u);
        if(static_cast<std::int64_t>(index)>i386_signed(read_u32(bytes,highest)))
            write_u32(bytes,highest,index);
        const auto pointer=host.entity_virtual(entity_token,0x10u);
        write_u32(bytes,ptr,pointer);
    }
    host.registry_add_24108880(entity_token);
    const auto secondary=host.entity_virtual(entity_token,0x1Cu);
    if(secondary!=0u) {
        const auto aux_id=host.aux_acquire_24108680(secondary);
        write_u16(bytes,aux,aux_id);
        const auto flags=host.entity_virtual(secondary,0xC4u);
        // The native TEST AH,0x4 checks bit TEN of the 32-bit flags.
        if((flags&kSecondaryFlagBit)!=0u) {
            const auto count=checked_offset(parent,kSecondaryFlagCountField);
            ensure_dword(bytes,count);
            write_u32(bytes,count,read_u32(bytes,count)+1u);
        }
    } else write_u16(bytes,aux,kInvalidAuxIndex);
}
// Native parent slot1. If removed index >= old highest, original
// 0x24105B20 scans the 16-byte PRIMARY slot table BEFORE helper
// 0x24108910 removes the entity. Do not reorder these two operations.
inline void observe_remove(Bytes& bytes,std::size_t parent,
        std::uint32_t entity_token,std::uint32_t raw_handle,IHost& host) {
    if(entity_token==0u)
        throw std::invalid_argument("host-only null entity not executable");
    const std::uint32_t index=raw_handle&kIndexMask;
    const auto aux=mirror_at(parent,index,kMirrorAuxField);
    ensure_word(bytes,aux);
    if(index<kMirrorGuardExclusive) {
        const auto ptr=mirror_at(parent,index,kMirrorPointerField);
        const auto count=checked_offset(parent,kLiveCountField);
        const auto highest=checked_offset(parent,kHighestSlotField);
        ensure_dword(bytes,ptr);ensure_dword(bytes,count);ensure_dword(bytes,highest);
        const bool scan=static_cast<std::int64_t>(index)>=
                        i386_signed(read_u32(bytes,highest));
        if(scan)ensure_dword(bytes,checked_offset(parent,4u+0x7FFu*16u));
        write_u32(bytes,ptr,0u);
        write_u32(bytes,count,read_u32(bytes,count)-1u);
        if(scan)entitylist::parent_recompute_highest_live_slot(bytes,parent);
    }
    host.registry_remove_24108910(entity_token);
    const auto secondary=host.entity_virtual(entity_token,0x1Cu);
    if(secondary!=0u) {
        const auto flags=host.entity_virtual(secondary,0xC4u);
        if((flags&kSecondaryFlagBit)!=0u) {
            const auto count=checked_offset(parent,kSecondaryFlagCountField);
            ensure_dword(bytes,count);
            write_u32(bytes,count,read_u32(bytes,count)-1u);
        }
    }
    const auto old_aux=read_u16(bytes,aux);
    if(old_aux!=kInvalidAuxIndex)
        host.aux_release_24106C20(old_aux);
    write_u16(bytes,aux,kInvalidAuxIndex);
}
// Native parent slot2 is a scalar deleting-destructor wrapper. It
// invokes original destructor 0x24107430, then optionally free routine
// 0x2416FD60 only if caller flag LOW BIT is set. Returns this token.
inline std::uint32_t observe_deleting_destructor(std::uint32_t parent_token,
        std::uint32_t flags,IHost& host) {
    if(parent_token==0u)
        throw std::invalid_argument("host-only null parent cannot be destroyed");
    host.call_destructor_24107430(parent_token);
    if((flags&1u)!=0u)host.free_object_2416FD60(parent_token);
    return parent_token;
}
} // namespace sin_compat::original_client_entitylist_parent_lifecycle
