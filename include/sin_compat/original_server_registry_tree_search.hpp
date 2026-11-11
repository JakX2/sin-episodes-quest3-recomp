#pragma once
// Phase174: original 2006 SiN Episodes server.dll native registry search,
// symbol-tree search, record registration and guarded indexed storage cleanup.
// Branch/callback model only; not the MSVC x86/Source engine runtime ABI.
#include <cstdint>

namespace sin_compat::original_server_registry_tree_search {
inline constexpr std::uint16_t kInvalidIndex = 0xffffu;
inline constexpr std::uint32_t kOriginalInsertSearchVA = 0x2228A280u;
inline constexpr std::uint32_t kOriginalSymbolSearchVA = 0x2228EB40u;
inline constexpr std::uint32_t kOriginalSymbolRecordInsertVA = 0x2228F710u;
inline constexpr std::uint32_t kOriginalStorageReleaseVA = 0x2228A380u;
inline constexpr std::uint32_t kInsertSentinelVA = 0x2258DF88u;
inline constexpr std::uint32_t kSymbolSentinelVA = 0x2259012Cu;

// The original uses two DIFFERENT per-table global lazy-initialization guards.
// Native initialization sets the guard before writing the four sentinel WORDs.
struct LazySentinel {
    bool initialized = false;
};
template<class Host>
inline void initialize_once(LazySentinel& s, Host& host) {
    if (!s.initialized) {
        s.initialized = true;   // original guard bit0 before the sentinel stores
        host.initialize_sentinel_words(kInvalidIndex,kInvalidIndex,
                                       kInvalidIndex,1u);
    }
}

struct InsertSearchResult {
    std::uint16_t last_examined = kInvalidIndex;
    bool last_direction = false;
};
// Original 0x2228A280, 255 bytes, called by native 0x2228B0A0.
// Initializes both output parameters before checking root +0x10. If nonempty,
// writes current 16-bit index, invokes callback from object+0 on record+8,
// then writes a direction byte and follows record WORD+0 if true or WORD+2
// if false. This routine never allocates a record itself.
// Host supplies a finite, valid native table; cycles are invalid input.
template<class Host>
InsertSearchResult original_prepare_insertion(std::uint16_t root,
                                             LazySentinel& sentinel,
                                             Host& host) {
    InsertSearchResult out{};
    for (std::uint16_t idx=root; idx!=kInvalidIndex;) {
        out.last_examined=idx;
        const bool branch=host.compare_insertion_candidate(idx);
        out.last_direction=branch;
        initialize_once(sentinel,host);
        idx=host.read_insertion_child(idx,branch ? 0u : 2u);
    }
    return out;
}

// Original 0x2228EB40, 266 bytes, called by the symbol interner.
// Its first comparator selects the left WORD+0 child. If false, it calls
// the second comparator with a different argument order: second FALSE means
// the current index matches and is returned; second TRUE follows WORD+2.
// This is NOT equivalent to the insertion-preparation routine above.
template<class Host>
std::uint16_t original_find_symbol(std::uint16_t root,
                                   LazySentinel& sentinel,Host& host) {
    for (std::uint16_t idx=root;idx!=kInvalidIndex;) {
        const bool first=host.compare_symbol_first(idx);
        if (first) {
            initialize_once(sentinel,host);
            idx=host.read_symbol_child(idx,0u);
            continue;
        }
        const bool second=host.compare_symbol_second(idx);
        if (!second) return idx;
        initialize_once(sentinel,host);
        idx=host.read_symbol_child(idx,2u);
    }
    return kInvalidIndex;
}

// Original 0x2228F710 (94 bytes): prepares record insertion with native
// 0x2228E470, obtains slot via 0x2228E620, links via 0x2228F460,
// increments 16-bit used count at +0x12, then writes the first argument's
// raw DWORD to [record + 12*slot + 8]. The callbacks preserve native order.
struct SymbolRecords { std::uint16_t registered_count=0; };
template<class Host>
std::uint16_t original_insert_symbol_record(SymbolRecords& state,
                                             std::uint32_t original_word,
                                             Host& host) {
    host.prepare_record_inputs(original_word);  // 0x2228E470
    const std::uint16_t slot=host.allocate_symbol_slot(); // 0x2228E620
    host.link_symbol_slot(slot); // 0x2228F460
    state.registered_count=static_cast<std::uint16_t>(state.registered_count+1u);
    host.write_record_word(slot,original_word,state.registered_count);
    return slot;
}

// Original 0x2228A380 (39 bytes): if DWORD+8 equals -1 return untouched;
// otherwise release nonnull DWORD+0 through 0x2226E3E0, clear +0 only if
// release occurred, then always clear DWORD+4. It leaves DWORD+8 unchanged.
struct OwnedIndexedStorage {
    std::uint32_t allocation=0;
    std::uint32_t auxiliary=0;
    std::uint32_t guard=0xffffffffu;
};
template<class Host>
void original_release_indexed_storage(OwnedIndexedStorage& s,Host& host) {
    if(s.guard==0xffffffffu)return;
    if(s.allocation!=0u){
        host.release_allocation(s.allocation);
        s.allocation=0u;
    }
    s.auxiliary=0u;
}
} // namespace sin_compat::original_server_registry_tree_search