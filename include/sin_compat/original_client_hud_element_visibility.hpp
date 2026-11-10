#pragma once
// Phase160: independent original SiN Episodes: Emergence 2006 client.dll
// CHudElement: constructor, field accessor/setters, virtual no-op,
// shared visibility suppression predicate and negating slot9.
// NOT a 2006 MSVC x86 class layout, Source/VGUI ABI or playable DLL.
#include <cstdint>

namespace sin_compat::original_client_hud_element {
struct State {
    std::uint32_t original_vtable_va = 0; // original this+0x00
    std::uint8_t byte_04 = 0;             // original this+0x04
    std::uint32_t mask_08 = 0;            // original this+0x08
    std::uint32_t identity_0c = 0;        // original this+0x0C
    std::uint8_t byte_10 = 0;             // original this+0x10
    std::uint8_t byte_11 = 0;             // original this+0x11
};
inline constexpr std::uint32_t kOriginalCHudElementVtable = 0x2429B6ECu;

// Exact original constructor 0x24142220, original argument at [esp+4].
inline void initialize(State& state,std::uint32_t identity) noexcept {
    state.original_vtable_va=kOriginalCHudElementVtable;
    state.byte_04=0;
    state.mask_08=0;
    state.identity_0c=identity;
    state.byte_10=0;
    state.byte_11=0;
}
inline void virtual_slot2_noop(State&) noexcept {} // 0x24142240
inline std::uint32_t virtual_slot8_get_identity(const State& s) noexcept {
    return s.identity_0c; // 0x24140770
}
inline std::uint8_t virtual_slot10_get_byte04(const State& s) noexcept {
    return s.byte_04; // 0x24140780
}
inline void virtual_slot11_set_byte04(State& s,std::uint8_t value) noexcept {
    s.byte_04=value; // 0x24141830
}
inline void virtual_slot12_set_mask08(State& s,std::uint32_t mask) noexcept {
    s.mask_08=mask; // 0x24141850
}

// Exact original global HUD gate 0x24142250. Source identifiers of all
// fields are unknown. Original global addresses / method offsets:
// [0x2433F684]->vtable+0x68, [0x24338CF0] local player,
// [0x243623A0]+0x2C override flags, player+0xD8C flags,
// player virtual+0x190 and byte player+0xE15.
struct OriginalGateInputs {
    bool engine_flag_check_enabled=false;
    bool local_player_present=false;
    std::uint32_t override_flags_2c=0;
    std::uint32_t player_flags_d8c=0;
    std::uint8_t player_byte_e15=0;
};

// Host callback is invoked only when the original 0x10-mask branch
// reaches the signed player virtual+0x190 call; <=0 suppresses HUD.
template<class QueryPlayerVirtual190>
bool original_global_hud_suppression(
    const OriginalGateInputs& input,std::uint32_t element_mask,
    QueryPlayerVirtual190&& player_virtual_190) {
    if (!input.engine_flag_check_enabled) return true;
    if (!input.local_player_present) return true;
    const auto active_flags=input.override_flags_2c!=0U
        ? input.override_flags_2c : input.player_flags_d8c;
    if ((active_flags & 0x04U)!=0U) return true;
    if ((element_mask & 0x10U)!=0U && player_virtual_190()<=0)
        return true;
    if ((element_mask & 0x20U)!=0U && input.player_byte_e15==0U)
        return true;
    return (active_flags & element_mask)!=0U;
}
// Exact CHudElement vtable slot9 0x24142A60, returns logical NOT of
// 0x24142250's AL after passing original this+0x08.
template<class QueryPlayerVirtual190>
bool virtual_slot9_should_draw(
    const State& state,const OriginalGateInputs& input,
    QueryPlayerVirtual190&& player_virtual_190) {
    return !original_global_hud_suppression(
        input,state.mask_08,player_virtual_190);
}
} // namespace sin_compat::original_client_hud_element
