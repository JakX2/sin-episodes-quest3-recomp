#pragma once
// Phase1726, authenticated owner original SiN Episodes (2006) PE32 client.dll.
// CWorld constructor 0x240CCE30 calls the 9-byte helper at 0x24195260,
// with ECX = owner + 0xA4. Source helper is:
//   mov eax,ecx; mov dword ptr [eax],0; ret
// This is an independently portable, bounded implementation ONLY of
// that 4-byte embedded-word write. It does NOT run the CWorld constructor,
// install original x86 vtables, initialize embedded engines, or publish
// an entity/networkable pointer to any retail engine interface.
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_client_world_embedded_word_phase1726 {
inline constexpr std::size_t kOwnerAllocationBytes=0x4B0u;
inline constexpr std::size_t kEmbeddedWordOffset=0xA4u;
inline constexpr std::size_t kEmbeddedWordBytes=sizeof(std::uint32_t);
inline constexpr std::uint32_t kSourceConstructorVA=0x240CCE30u;
inline constexpr std::uint32_t kSourceEmbeddedWordHelperVA=0x24195260u;
inline constexpr std::uint32_t kSourceEmbeddedWordHelperCallVA=0x240CCE86u;
inline constexpr std::size_t kSourceConstructorBytes=573u;
inline constexpr std::size_t kSourceEmbeddedWordHelperBytes=9u;

enum class ResetStatus : std::uint8_t {
    Applied,
    NullOwner,
    InsufficientOwnerSpan
};
struct ResetResult final {
    ResetStatus status;
    // Matches the original helper's return value, ECX, on success.
    // The returned pointer is an interior pointer, NOT a valid CWorld.
    std::uint8_t* embeddedWord;
};
inline ResetResult ResetVerifiedEmbeddedWord(
    std::uint8_t* owner, std::size_t availableBytes) noexcept {
    if(owner==nullptr) return {ResetStatus::NullOwner,nullptr};
    // A validated, byte-addressable span must cover the entire 4B write.
    // A caller can pass a bounded subspan, but NEVER publish it as CWorld.
    if(availableBytes<kEmbeddedWordOffset+kEmbeddedWordBytes)
        return {ResetStatus::InsufficientOwnerSpan,nullptr};
    auto* const word=owner+kEmbeddedWordOffset;
    for(std::size_t i=0;i<kEmbeddedWordBytes;++i) word[i]=0u;
    return {ResetStatus::Applied,word};
}
inline constexpr bool CanPublishRetailWorld() noexcept {return false;}
static_assert(kEmbeddedWordOffset+kEmbeddedWordBytes<=kOwnerAllocationBytes);
static_assert(kSourceConstructorBytes==573u);
static_assert(!CanPublishRetailWorld());
} // namespace sin_compat::original_client_world_embedded_word_phase1726
