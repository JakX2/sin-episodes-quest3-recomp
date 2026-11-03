#pragma once
// SiN Episodes: Emergence 2006 x86 behavioral reference, not an Android DLL.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace sin_compat::phase93 {
struct HudCtor {
    std::string_view original_class;
    std::uint32_t original_x86_va;
    std::uint32_t original_vtable;
};
inline constexpr std::array<HudCtor,8> hud_ctors{{
    {"CDebugOverlayPanel",0x241ECA50u,0x242ABF0Cu},
    {"CNetGraphPanelInterface",0x241EF9E0u,0x242ACC34u},
    {"CFPS",0x241ED190u,0x242AC1F0u},
    {"CMessageChars",0x241EEA20u,0x242ACB38u},
    {"CLoadingDisc",0x241EE460u,0x242AC8B0u},
    {"CCenterPrint",0x241EC5E0u,0x242ABC4Cu},
    {"CTextMessage",0x241F2310u,0x242AD484u},
    {"CStatsPanelInterface",0x24207660u,0x242AF988u},
}};
// Original x86 constructor initializes ONLY vtable at +0 and a null
// member at +4. The later teardown of CLoadingDisc also reads +8,
// which must be initialized elsewhere; do not write it in this model.
struct OriginalHudX86Words {
    std::uint32_t vtable=0;
    std::uint32_t member_4=0;
    std::uint32_t member_8=0;
};
static_assert(sizeof(OriginalHudX86Words)==12);
inline void apply_verified_constructor(OriginalHudX86Words& obj,
                                        const HudCtor& ctor) noexcept {
    obj.vtable=ctor.original_vtable;
    obj.member_4=0;
}

inline constexpr std::uint32_t unresolved_token=0xffffffffu;
// CSceneCache slot 2 invokes a candidate classifier: type==5,
// extra predicate returns zero; extraction succeeds, token resolved.
template<class Append>
bool scene_candidate_gate(bool candidate_present,int kind,bool reject_state,
                          bool extracted,std::uint32_t token,Append&& append) {
    if(!candidate_present || kind!=5 || reject_state || !extracted ||
       token==unresolved_token)return false;
    append(token);
    return true;
}
// Model sounds: unresolved token / no object / wrong type / no child skip.
template<class Handle>
bool model_sound_candidate_gate(std::uint32_t token,bool entity_present,
                                int type,bool child_present,Handle&& handle) {
    if(token==unresolved_token || !entity_present || type!=3 || !child_present)
        return false;
    handle();
    return true;
}
} // namespace sin_compat::phase93
