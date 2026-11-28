#pragma once
// Phase1719: exact original 2006 PE32 40-byte zero-initializing
// allocation wrapper at 0x240C4FF0, used by 142 source entity creators.
// Full original executable helper SHA256:
// e9428fee2e9eccc2fb344b2d86b9e238e56d5ad2b12cc128db314f7416513e9d
// Original source behavior (allocation SUCCESS path only):
// PUSH ESI; MOV ESI,[ESP+8]; PUSH EDI; PUSH ESI;
// CALL 0x2416F8F0 (original native allocation service);
// EDX=EAX; ECX=size/4; EAX=0; EDI=EDX; REP STOS DWORD;
// ECX=size&3; REP STOS BYTE; return EDX. No NULL check exists.
// Four analogous original 40B functions at 0x240C4FF0,0x240C5020,
// 0x240C5050,0x240C5080 call the same original allocation service.
// These original executable x86 methods are NOT shipped, executed,
// or substituted into the replacement Windows/Android ClientClass.
//
// Safe host model only: accept a known source 142 entity creator tuple,
// produce a newly-owned ZEROED byte buffer of the verified original
// requested size, without vtables/constructors/native pointers.
// This is NOT a native entity instance or a usable Source allocator.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>
#include "sin_compat/original_client_entity_allocation_prologues.hpp"

namespace sin_compat::original_client_source_zero_init_allocator {
namespace prologues=sin_compat::original_client_entity_allocation_prologues;

inline constexpr std::uint32_t kOriginalWrapperVA=0x240C4FF0u;
inline constexpr std::uint32_t kNativeAllocationServiceVA=0x2416F8F0u;
inline constexpr std::size_t kNativeWrapperSize=40u;
inline constexpr std::size_t kOriginalWrapperPadding=8u;
inline constexpr std::size_t kOriginalSharedWrappers=4u;
inline constexpr std::size_t kOriginalCommonSourceCreators=142u;

enum class Status : std::uint8_t {
    PreparedOwnedZeroBytes, UnrecognizedSource, RefusedBadOwnedMemory,
    RefusedHostAllocationFailure
};

// Explicit host-owner supplied vector. Source "REP STOSD then REP STOSB"
// is simulated without interpreting the original pointer. The caller
// must provide a region at least count bytes; if too short, NO writes.
// This can test the original x86 tail behavior even though the 142
// recovered size requests are all 4-byte aligned.
inline Status ZeroOriginalSizeIntoOwnedBuffer(
    std::size_t requestedBytes,std::vector<std::uint8_t>& storage) noexcept {
    if(requestedBytes>storage.size())
        return Status::RefusedBadOwnedMemory;
    const auto dwords=requestedBytes/4u;
    const auto tail=requestedBytes%4u;
    for(std::size_t i=0u;i<dwords;++i) {
        const auto start=i*4u;
        storage[start]=0u;
        storage[start+1u]=0u;
        storage[start+2u]=0u;
        storage[start+3u]=0u;
    }
    for(std::size_t i=0u;i<tail;++i)
        storage[dwords*4u+i]=0u;
    return Status::PreparedOwnedZeroBytes;
}

// Atomic synthetic owner memory preparation: rejects a wrong original
// creator ordinal/VA before any mutation; only source-proven sizes
// (142 common allocation routes, 69 distinct sizes) are accepted.
// The source allocator 0x2416F8F0 may have other allocation/lifetime
// side effects, which are deliberately NOT claimed here.
inline Status PrepareSyntheticOriginalEntityZeroedBytes(
    std::size_t sourceClassOrdinal,std::uint32_t originalCreatorVA,
    std::vector<std::uint8_t>& outOwnedStorage) {
    const auto* record=prologues::SourceAllocationForClass(
        sourceClassOrdinal,originalCreatorVA);
    if(record==nullptr)return Status::UnrecognizedSource;
    try {
        std::vector<std::uint8_t> allocated(
            static_cast<std::size_t>(record->sourceObjectByteCount),0u);
        outOwnedStorage.swap(allocated);
    }catch(const std::bad_alloc&) {
        return Status::RefusedHostAllocationFailure;
    }
    return Status::PreparedOwnedZeroBytes;
}
inline constexpr bool CanPublishRetailCreatorPointers() noexcept {return false;}
inline constexpr bool CanCreateRetailEntity() noexcept {return false;}
static_assert(kOriginalWrapperVA==prologues::kOriginalAllocatorVA);
static_assert(kOriginalCommonSourceCreators==
              prologues::kSharedAllocatorPrologueCount);
static_assert(prologues::SourceAllocationInventoryConsistent());
static_assert(!CanPublishRetailCreatorPointers());
static_assert(!CanCreateRetailEntity());
} // namespace sin_compat::original_client_source_zero_init_allocator
