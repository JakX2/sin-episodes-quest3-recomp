#pragma once
// Phase164: real 2006 SiN Episodes client.dll panel-animation
// registry owner lookup and singleton. This C++17 model preserves
// original branch order, but native symbol map/allocator remain callbacks.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace sin_compat::original_client_panel_registry {

inline constexpr std::uint32_t kRegistrySingletonVA = 0x2438D938u;
inline constexpr std::uint32_t kRegistryOnceFlagVA = 0x2438D9C0u;
inline constexpr std::uint32_t kOwnerSize = 28u;
inline constexpr std::uint32_t kRecordSize = 24u;
inline constexpr std::uint32_t kOwnerAtRecordOffset = 0x14u;
inline constexpr std::uint32_t kRecordStorageOffset = 0x34u;
inline constexpr std::uint32_t kSymbolMapOffset = 0x30u;

// The native code calls Q_strnicmp(name,"vgui::",6).
// ONLY the prefix is compared case-insensitively; subsequent symbol
// lookup case rules are handled by an external original symbol table.
constexpr char ascii_fold(char c) noexcept {
    return c>='A' && c<='Z' ? static_cast<char>(c+('a'-'A')) : c;
}
constexpr bool has_original_vgui_prefix(std::string_view name) noexcept {
    constexpr std::string_view prefix="vgui::";
    if(name.size()<prefix.size())return false;
    for(std::size_t i=0;i<prefix.size();++i)
        if(ascii_fold(name[i])!=prefix[i])return false;
    return true;
}
struct ClassName {
    std::string_view original_symbol_key;
    bool prefix_removed=false;
};
constexpr ClassName normalize_original_class_name(
    std::string_view name) noexcept {
    const bool strip=has_original_vgui_prefix(name);
    return {strip?name.substr(6):name,strip};
}

// Original 0x24011D30 marks bit 0 BEFORE the singleton constructor
// and shutdown registrar. Reentrant reads see the same global address.
struct SingletonState {
    std::uint32_t initialization_bits=0;
};
template<class Init>
std::uint32_t original_lazy_registry(
    SingletonState& state,Init&& initialize) {
    if((state.initialization_bits&1u)==0) {
        state.initialization_bits|=1u;
        initialize(kRegistrySingletonVA);
    }
    return kRegistrySingletonVA;
}

// On successful 0x24170580 allocation, 0x24011BF0 explicitly clears
// seven DWORDs (28 bytes) in the new class owner. This differs from
// the six-DWORD (24-byte) property registry records from Phase163.
using EmptyOwnerWords = std::array<std::uint32_t,7>;
inline constexpr EmptyOwnerWords kEmptyOwnerWords{{0,0,0,0,0,0,0}};
struct Resolution {
    std::string_view canonical_key;
    std::uint32_t owner_handle=0;
    bool found_existing=false;
    bool insertion_attempted=false;
    bool allocation_succeeded=false;
};

// The original existing-owner search calls Q_strnicmp -> native
// symbol map -> tree lookup and returns +0x14 from a 24-byte slot.
// Missing classes call 0x240093B0, allocate a 28-byte owner via
// 0x24170580, zero its 7 words if nonnull, then register key/owner
// using 0x240124B0. Original insertion is attempted EVEN if null.
// Callbacks represent these engine operations, NOT fabricated classes.
template<class Find,class Setup,class Allocate,class Insert>
Resolution original_find_or_create(
    std::string_view name,Find&& find,Setup&& setup,
    Allocate&& allocate,Insert&& insert) {
    const auto normalized=normalize_original_class_name(name);
    const std::uint32_t existing=static_cast<std::uint32_t>(
        find(normalized.original_symbol_key));
    if(existing) {
        return {normalized.original_symbol_key,existing,true,false,true};
    }
    setup();
    const std::uint32_t owner=static_cast<std::uint32_t>(allocate(kOwnerSize));
    // The insertion callback can observe the seven ZERO initial words;
    // on allocation failure it must NOT dereference owner == 0.
    insert(normalized.original_symbol_key,owner,kEmptyOwnerWords);
    return {normalized.original_symbol_key,owner,false,true,owner!=0};
}

// Original 0x24011DC0 resolves singleton FIRST, then calls the
// original find-or-create method with the caller's class-name argument.
template<class Init,class Find,class Setup,class Allocate,class Insert>
Resolution original_class_registry_entry(
    SingletonState& state,std::string_view name,
    Init&& initialize,Find&& find,Setup&& setup,
    Allocate&& allocate,Insert&& insert) {
    (void) original_lazy_registry(state,std::forward<Init>(initialize));
    return original_find_or_create(
        name,std::forward<Find>(find),std::forward<Setup>(setup),
        std::forward<Allocate>(allocate),std::forward<Insert>(insert));
}
} // namespace sin_compat::original_client_panel_registry
