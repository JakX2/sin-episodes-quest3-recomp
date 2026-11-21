#pragma once
// Original-free bounded C++17 reference to SiN native 0x220EB1C0.
// It models a registration-record selection contract, not a game DLL ABI.
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <vector>

namespace sin_compat::phase585 {
struct Entry {
    int32_t numeric_key=0;  // native entry +0x10
    int32_t signed_weight=0; // native entry +0x14
    uint8_t flags=0;         // native entry +0x0D (bit 0x10)
};
struct Registry { std::vector<Entry> records; };
struct Hooks {
    std::function<bool()> revision_is_current;
    std::function<void()> refresh_registration;
    std::function<void(int)> refresh_entry_at_index;
    std::function<int(int,int)> random_inclusive;
};
struct LookupResult {
    bool supported=true;
    int returned_index=-1;
};
// Mirrors the confirmed order of the native function when records/rng are valid.
// Native negative-int minimum, signed-overflow and malformed-record behavior
// are intentionally refused rather than guessed.
inline LookupResult Select(const Registry* reg, int32_t requested_key,
                           int32_t state_code, Hooks& host) {
    if (!reg) return {true,0}; // real 0x220EB1CA early return, not -1
    if (!host.revision_is_current()) host.refresh_registration();
    int32_t total=0;
    int result=-1;
    const int n=static_cast<int>(reg->records.size());
    for(int i=0;i<n;++i){
        const Entry& e=reg->records[static_cast<size_t>(i)];
        if(!(e.flags&0x10)) host.refresh_entry_at_index(i);
        if(e.numeric_key!=requested_key) continue;
        if(state_code==0 && e.signed_weight<0) return {true,i};
        if(e.signed_weight==std::numeric_limits<int32_t>::min()) return {false,-1};
        const int32_t w=e.signed_weight<0?-e.signed_weight:e.signed_weight;
        if(w>std::numeric_limits<int32_t>::max()-total) return {false,-1};
        total+=w;
        // Native calls the random interface even if running sum is 0.
        const int draw=host.random_inclusive(0,total-1);
        if(total==0 || draw<w) result=i;
    }
    return {true,result};
}
}