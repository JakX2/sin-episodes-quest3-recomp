#pragma once
// Original 2006 engine.dll edict-side FreeContainingEntity contract.
// Authenticated direct routine VA 0x20110E50 and indexed route 0x20110E70.
// Source server.dll owns the cleanup callback; engine owns this 0x14-byte
// edict array, flag, serial and trailing free-link writes.
//
// Independent safety: does not call original engine, dispatch foreign
// vtables, or dereference unowned pointers. A trusted host must attest that
// the full 0x14-byte edict remains physically stable AFTER the callback.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sin_independent {
constexpr std::size_t kEngineOriginalEdictStride=0x14;
constexpr std::size_t kEngineOriginalEdictFlags=0x00;
constexpr std::size_t kEngineOriginalEdictSerial=0x04;
constexpr std::size_t kEngineOriginalEdictUnknown=0x0c;
constexpr std::size_t kEngineOriginalEdictNext=0x10;
constexpr std::uint32_t kEngineOriginalEdictFreeBit=2u;
constexpr std::uint32_t kEngineOriginalEdictRetiredSerial=0xffffffffu;
static_assert(kEngineOriginalEdictNext+4==kEngineOriginalEdictStride,
              "owner original edict element includes trailing free-link");

struct EngineEdictRetireHooks {
    void* context=nullptr;
    // Trust-bearing authorizer; must check owner generation AND physical
    // extent WITHOUT dereferencing an untrusted address.
    bool (*authorized)(void*,const void*,std::size_t) noexcept=nullptr;
    // Adapts the original engine-owned call to ServerGameEnts001 slot +0x0C.
    // Returns false if the independent diagnostic vtable refuses the work;
    // unlike original retail void dispatch, refusal is visible to this core.
    bool (*free_containing_entity)(void*,void*) noexcept=nullptr;
    bool Complete() const noexcept {return authorized&&free_containing_entity;}
};

enum class EngineEdictRetireResult : unsigned char {
    InvalidInput, Unauthorized, Inactive, CallbackRefused, Retired
};

inline std::uint32_t ReadEngineEdictDword(
    const unsigned char* data,std::size_t offset) noexcept {
    std::uint32_t result=0;
    std::memcpy(&result,data+offset,sizeof(result));
    return result;
}
inline void WriteEngineEdictDword(
    unsigned char* data,std::size_t offset,std::uint32_t v) noexcept {
    std::memcpy(data+offset,&v,sizeof(v));
}

// Direct engine route: clear state flag bit 1 BEFORE server-side release;
// only AFTER the callback writes edict+0x10=0, edict+0x04=-1.
// The server callback may additionally clear flags and unknown +0x0C.
// An explicit callback refusal is not transactional: the flag is already
// cleared; the serial/free-link remain unchanged to expose incomplete work.
inline EngineEdictRetireResult RetireEngineEdict(
    void* edict,std::size_t bytes,const EngineEdictRetireHooks& h) noexcept {
    if(!edict||bytes<kEngineOriginalEdictStride||!h.Complete())
        return EngineEdictRetireResult::InvalidInput;
    if(!h.authorized(h.context,edict,kEngineOriginalEdictStride))
        return EngineEdictRetireResult::Unauthorized;
    auto* raw=static_cast<unsigned char*>(edict);
    const auto flags=ReadEngineEdictDword(raw,kEngineOriginalEdictFlags);
    WriteEngineEdictDword(raw,kEngineOriginalEdictFlags,
                         flags&~kEngineOriginalEdictFreeBit);
    if(!h.free_containing_entity(h.context,edict))
        return EngineEdictRetireResult::CallbackRefused;
    WriteEngineEdictDword(raw,kEngineOriginalEdictNext,0u);
    WriteEngineEdictDword(raw,kEngineOriginalEdictSerial,
                         kEngineOriginalEdictRetiredSerial);
    return EngineEdictRetireResult::Retired;
}

// Indexed route: the original computes base + index * 20 bytes, checks
// state bit 1 and skips the free call if it was not present. The caller
// supplies a bounded owner-authenticated edict-array extent, not the raw
// original global length or an unchecked arbitrary index.
inline EngineEdictRetireResult RetireEngineEdictAt(
    void* array,std::size_t bytes,int index,
    const EngineEdictRetireHooks& h) noexcept {
    if(!array||index<0||!h.Complete())
        return EngineEdictRetireResult::InvalidInput;
    const auto position=static_cast<std::size_t>(index);
    if(position>=bytes/kEngineOriginalEdictStride)
        return EngineEdictRetireResult::InvalidInput;
    auto* edict=static_cast<unsigned char*>(array)+
                position*kEngineOriginalEdictStride;
    if(!h.authorized(h.context,edict,kEngineOriginalEdictStride))
        return EngineEdictRetireResult::Unauthorized;
    if((ReadEngineEdictDword(edict,kEngineOriginalEdictFlags)&
        kEngineOriginalEdictFreeBit)==0)
        return EngineEdictRetireResult::Inactive;
    return RetireEngineEdict(edict,kEngineOriginalEdictStride,h);
}
} // namespace sin_independent
