// Phase1726: one fully reconstructed 2006 original CWorld constructor leaf.
// Tests the exact observable 4-byte write/returned interior pointer,
// including refusal of invalid spans. No original x86 code is executed.
#include "sin_compat/original_client_world_embedded_word_phase1726.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>

namespace word=sin_compat::original_client_world_embedded_word_phase1726;
int main() {
    static_assert(!word::CanPublishRetailWorld());
    constexpr std::size_t n=word::kOwnerAllocationBytes;
    std::array<std::uint8_t,n> owner{};
    owner.fill(0xA5u);
    const auto ok=word::ResetVerifiedEmbeddedWord(owner.data(),owner.size());
    assert(ok.status==word::ResetStatus::Applied);
    assert(ok.embeddedWord==owner.data()+word::kEmbeddedWordOffset);
    for(std::size_t i=0;i<owner.size();++i)
        assert(owner[i]==((i>=word::kEmbeddedWordOffset &&
                          i<word::kEmbeddedWordOffset+word::kEmbeddedWordBytes)
                             ? 0u : 0xA5u));
    // Uninitialized/invalid owner pointers cannot be published.
    const auto noOwner=word::ResetVerifiedEmbeddedWord(nullptr,n);
    assert(noOwner.status==word::ResetStatus::NullOwner);
    assert(noOwner.embeddedWord==nullptr);
    owner.fill(0x5Au);
    for(std::size_t bytes : {std::size_t{0u},
                              word::kEmbeddedWordOffset,
                              word::kEmbeddedWordOffset+word::kEmbeddedWordBytes-1u}) {
        const auto shortResult=word::ResetVerifiedEmbeddedWord(owner.data(),bytes);
        assert(shortResult.status==word::ResetStatus::InsufficientOwnerSpan);
        assert(shortResult.embeddedWord==nullptr);
        for(const auto b:owner) assert(b==0x5Au);
    }
    const auto exact=word::ResetVerifiedEmbeddedWord(
        owner.data(),word::kEmbeddedWordOffset+word::kEmbeddedWordBytes);
    assert(exact.status==word::ResetStatus::Applied);
    assert(exact.embeddedWord==owner.data()+word::kEmbeddedWordOffset);
    for(std::size_t i=0;i<owner.size();++i)
        assert(owner[i]==((i>=word::kEmbeddedWordOffset &&
                          i<word::kEmbeddedWordOffset+word::kEmbeddedWordBytes)
                             ? 0u : 0x5Au));
    assert(!word::CanPublishRetailWorld());
    std::puts("PASS Phase1726 CWorld +0xA4 original 9B leaf: exact 4B "
              "write, returned subobject pointer, bounded refusal; no retail callbacks");
}
