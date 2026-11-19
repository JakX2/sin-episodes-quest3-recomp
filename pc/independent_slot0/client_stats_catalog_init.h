#pragma once
// Original 2006 SiN C_StatsSystem owner helper 0x241FC5A0 initial
// 66-record catalog setup, excluding original resource-enumeration ABI.
// Records are independently owned and cannot be passed to the game DLL.
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
namespace sin_independent::stats_catalog {
inline constexpr std::size_t kOriginalCatalogSlots=66;
inline constexpr std::size_t kOriginalSubobjectOffset=0x84;
struct Entry {
    std::uint32_t first=0;
    std::uint32_t second=0;
    std::int32_t identifier=-1;
};
using AllowAllocation=bool (*)(void*,std::size_t) noexcept;
class Catalog final {
public:
    Catalog()=default;
    Catalog(const Catalog&)=delete;
    Catalog& operator=(const Catalog&)=delete;
    Catalog(Catalog&&)=delete;
    Catalog& operator=(Catalog&&)=delete;
    void Rebuild(void* context=nullptr,AllowAllocation allow=nullptr) noexcept {
        for(auto& slot:entries_)slot.reset();
        count_=0;
        for(std::size_t index=0;index<kOriginalCatalogSlots;++index){
            Entry* record=(!allow || allow(context,index))?
                new(std::nothrow) Entry:nullptr;
            if(record){
                record->first=0;
                record->second=0;
                record->identifier=-1;
            }
            entries_[index].reset(record);
            ++count_; // native vector appends even a null allocation
        }
    }
    // Atomic ownership publication between the pending stage-3 catalog
    // and the last known-good catalog. No allocation or raw-pointer copy.
    void Swap(Catalog& other) noexcept {
        entries_.swap(other.entries_);
        std::swap(count_,other.count_);
    }
    std::size_t size() const noexcept{return count_;}
    const Entry* at(std::size_t index) const noexcept {
        return index<count_?entries_[index].get():nullptr;
    }
    Entry* mutableAt(std::size_t index) noexcept {
        return index<count_?entries_[index].get():nullptr;
    }
private:
    std::array<std::unique_ptr<Entry>,kOriginalCatalogSlots> entries_{};
    std::size_t count_=0;
};
// The remainder of original 0x241FC5A0 enumerates resources and updates
// the catalog. Its true ABI and parser are NOT claimed here. The injected
// continuation keeps this boundary explicit and fail-closed.
using ContinueResourceLoad=bool (*)(void*,Catalog&) noexcept;
inline bool BeginOwnerSetup(Catalog& catalog,void* host,
                            ContinueResourceLoad continuation,
                            AllowAllocation allow=nullptr) noexcept {
    if(!host || !continuation)return false;
    catalog.Rebuild(host,allow);
    return continuation(host,catalog);
}
} // namespace sin_independent::stats_catalog
