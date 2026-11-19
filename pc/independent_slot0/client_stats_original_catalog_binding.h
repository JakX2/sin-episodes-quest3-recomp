#pragma once
// Clean-room source-stat text projection into the owner-allocated 66-record
// catalog. The original MSVC fields are 32-bit strdup pointers; do NOT
// write independent host pointers into those ABI fields. We own strings
// separately while retaining the original percentage target ID in +8.
// All IDs are recovered directly from SHA-pinned 2006 client.dll data.
#include "client_stats_original_id_map.h"
#include "client_stats_catalog_init.h"
#include "client_stats_script_parser.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace sin_independent::stats_binding {
struct Text {
    std::string short_text;
    std::string long_text;
    std::int32_t percentage_id=-1;
};
class Projection final {
public:
    Projection()=default;
    Projection(const Projection&)=delete;
    Projection& operator=(const Projection&)=delete;
    Projection(Projection&&)=delete;
    Projection& operator=(Projection&&)=delete;
    const Text* at(std::size_t id) const noexcept {
        return id<entries_.size()?entries_[id].get():nullptr;
    }
    std::size_t count() const noexcept {return count_;}
    void Swap(Projection& other) noexcept {
        entries_.swap(other.entries_);
        std::swap(count_,other.count_);
    }
private:
    friend bool Bind(stats_catalog::Catalog&,Projection&,
                     const std::vector<stats_scripts::Definition>&,
                     std::string&) noexcept;
    std::array<std::unique_ptr<Text>,stat_name_ids::kCount> entries_{};
    std::size_t count_=0;
};
// Fails without modifying existing projection or any Catalog entry.
// A complete valid source document maps only 62 of the original 66 IDs.
// Missing target catalog slots are refused rather than dereferenced.
// Exact host matching is intentional; the original private case-folding
// semantics are not inferred by this function.
inline bool Bind(stats_catalog::Catalog& catalog,Projection& target,
                 const std::vector<stats_scripts::Definition>& definitions,
                 std::string& error) noexcept {
    auto refuse=[&error](const char* reason) noexcept -> bool {
        try {error=reason;} catch(...) {}
        return false;
    };
    if(catalog.size()!=stat_name_ids::kCount)
        return refuse("catalog not initialized to 66 slots");
    if(definitions.size()>stat_name_ids::kCount)
        return refuse("too many definitions for original IDs");
    try {
        Projection pending;
        std::array<bool,stat_name_ids::kCount> seen{};
        for(const auto& definition:definitions) {
            const int idx=stat_name_ids::Lookup(definition.name);
            if(idx<0)return refuse("unrecognized stat identifier");
            const auto index=static_cast<std::size_t>(idx);
            if(seen[index])return refuse("duplicate stat identifier");
            if(!catalog.at(index))return refuse("unallocated catalog record");
            seen[index]=true;
            auto item=std::make_unique<Text>();
            item->short_text=definition.short_text;
            item->long_text=definition.long_text;
            if(!definition.percentage_of.empty()) {
                const int reference=stat_name_ids::Lookup(definition.percentage_of);
                if(reference<0)return refuse("invalid percentage target ID");
                item->percentage_id=reference;
            }
            pending.entries_[index]=std::move(item);
            ++pending.count_;
        }
        // Do not allow a reference to a compiled-but-unscripted name.
        for(std::size_t i=0;i<stat_name_ids::kCount;++i) {
            const Text* entry=pending.at(i);
            if(entry && entry->percentage_id>=0 &&
               !seen[static_cast<std::size_t>(entry->percentage_id)])
                return refuse("unbound percentage target");
        }
        // All allocations, string copies and validation completed.
        // The native +8 integer is the only catalog field modified.
        for(std::size_t i=0;i<stat_name_ids::kCount;++i) {
            if(auto* record=catalog.mutableAt(i))
                record->identifier=pending.at(i)?
                    pending.at(i)->percentage_id:-1;
        }
        target.Swap(pending);
        error.clear();
        return true;
    } catch(...) {
        return refuse("binding allocation or copy failed");
    }
}
} // namespace sin_independent::stats_binding
