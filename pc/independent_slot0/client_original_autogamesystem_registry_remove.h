#pragma once
// SHA-authenticated 2006 SiN AutoGameSystem base destructor:
//   0x24156820, 26 bytes, full SHA256
//   cecc59ef0571ba365a6ce38669ea8c6a229ce3669a8f9f00bc27d06b3e892b8a
// and original registry first-match removal:
//   0x241565E0, 90 bytes, full SHA256
//   f71baaca312397a0a87e72799b2b20bb69214751462124cd73e3b4e19b1f0807
//
// The base destructor writes 0x24293B1C into this+0 and removes
// this object from original global list at 0x242FBFA8 using a pointer
// to this. The list scan tests pointer equality, removes FIRST match,
// memmoves later DWORDs down, decrements +0x0C count, and does not
// clear the unused trailing slot. This independent model never
// dereferences, registers or frees actual original game-system objects.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent::original_gamesystem_registry {
inline constexpr std::uint32_t kOriginalBaseDestructorVA=0x24156820u;
inline constexpr std::uint32_t kOriginalRemoveVA=0x241565E0u;
inline constexpr std::uint32_t kOriginalRegistryVA=0x242FBFA8u;
inline constexpr std::uint32_t kOriginalRegistryCountVA=0x242FBFB4u;
inline constexpr std::uint32_t kOriginalBaseVtableVA=0x24293B1Cu;
inline constexpr std::uint32_t kOriginalSoundscapeVtableVA=0x24293C34u;
inline constexpr std::uint32_t kOriginalMemmoveVA=0x24248B50u;
inline constexpr std::size_t kOwnedRegistryMax=1024u; // owned policy
enum class RemoveResult {Removed,NotFound,Empty};
enum class SeedResult {Accepted,TooLarge};
struct RemoveReport final {
    RemoveResult result=RemoveResult::Empty;
    std::size_t index=0u;
    std::size_t elements_shifted=0u;
    std::size_t count_before=0u;
    std::size_t count_after=0u;
};
class OwnedRegistry final {
public:
    std::size_t size() const noexcept{return count_;}
    std::uint64_t revision() const noexcept{return revision_;}
    std::uint32_t At(std::size_t pos) const noexcept {
        return pos<count_?data_[pos]:0u;
    }
    // Source trailing slot remains in allocated backing store after
    // decrement. Exposed solely to test against false blanket clears.
    std::uint32_t Raw(std::size_t pos) const noexcept {
        return pos<data_.size()?data_[pos]:0u;
    }
    SeedResult Seed(const std::uint32_t* values,
                    std::size_t count) noexcept {
        if(count>data_.size()||(count&&!values))
            return SeedResult::TooLarge;
        for(std::size_t i=0;i<count;++i)data_[i]=values[i];
        count_=count;
        ++revision_;
        return SeedResult::Accepted;
    }
    RemoveReport RemoveFirst(std::uint32_t identity) noexcept {
        RemoveReport out{};
        out.count_before=count_;
        out.count_after=count_;
        if(count_==0u)return out; // original count<=0 exits
        out.result=RemoveResult::NotFound;
        std::size_t i=0;
        for(;i<count_&&data_[i]!=identity;++i){}
        if(i==count_)return out;
        out.result=RemoveResult::Removed;
        out.index=i;
        out.elements_shifted=count_-i-1u;
        for(std::size_t j=i+1u;j<count_;++j)
            data_[j-1u]=data_[j];
        // Original source only decrements count +0x0C,
        // leaving the allocated trailing DWORD untouched.
        --count_;
        out.count_after=count_;
        ++revision_;
        return out;
    }
private:
    std::array<std::uint32_t,kOwnedRegistryMax> data_{};
    std::size_t count_=0u;
    std::uint64_t revision_=0u;
};
struct BaseObject final {
    std::uint32_t owned_identity=0u;
    std::uint32_t original_vtable_metadata=kOriginalSoundscapeVtableVA;
    bool independently_destroyed=false;
};
enum class DestructorResult {Applied,AlreadyDestroyed};
struct DestructorReport final {
    DestructorResult result=DestructorResult::Applied;
    RemoveReport registry{};
    bool original_base_vtable_written=false;
    bool originally_registered=false;
};
inline DestructorReport DestroyBaseOwned(OwnedRegistry& registry,
                                         BaseObject& object) noexcept {
    DestructorReport output{};
    if(object.independently_destroyed) {
        output.result=DestructorResult::AlreadyDestroyed;
        return output;
    }
    // The actual source writes its base vtable first, and passes
    // a POINTER TO this into the original global-registry remove.
    // We use an arbitrary owned identity, never a real address.
    object.original_vtable_metadata=kOriginalBaseVtableVA;
    output.original_base_vtable_written=true;
    output.registry=registry.RemoveFirst(object.owned_identity);
    output.originally_registered=
        output.registry.result==RemoveResult::Removed;
    object.independently_destroyed=true;
    return output;
}
static_assert(kOriginalRegistryCountVA-kOriginalRegistryVA==0xCu);
} // namespace sin_independent::original_gamesystem_registry
