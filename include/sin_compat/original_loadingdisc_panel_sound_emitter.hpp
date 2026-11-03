#pragma once
// SiN Episodes (2006): verified x86 object and interface choreography.
// Host-only behavioral contract. It does NOT call original 32-bit addresses.
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::phase95 {
inline constexpr std::size_t loading_panel_allocation_bytes=0xdc;
inline constexpr std::size_t loading_label_allocation_bytes=0xf0;
inline constexpr std::uint32_t original_loading_panel_vtable=0x242ac5c4u;
inline constexpr std::uint32_t original_vgui_label_vtable=0x242d4d64u;
inline constexpr std::size_t loading_panel_label_pointer_offset=0xd8;
inline constexpr std::string_view loading_panel_class="CLoadingDiscPanel";
inline constexpr std::string_view loading_label_class="vgui::Label";
inline constexpr std::string_view loading_label_name="LoadingLabel";
inline constexpr std::string_view loading_client_scheme="ClientScheme";
inline constexpr std::string_view loading_panel_resource="resource/LoadingDiscPanel.res";

struct LoadingPanelHostState {
    void* panel=nullptr;
    void* label=nullptr;
    bool label_initially_hidden=false;
};
// Partial original constructor operation-order model. The real constructor
// has extra panel geometry/helpers, retained in the pinned original x86 audit.
// A null allocation returns false safely; original x86 may dereference null.
template<class Allocate,class ConstructPanel,class ConstructLabel,class Attach,
         class HideLabel,class LoadResources>
bool construct_loading_panel(LoadingPanelHostState& state,void* context,
                             Allocate&& allocate,ConstructPanel&& construct_panel,
                             ConstructLabel&& construct_label,Attach&& attach,
                             HideLabel&& hide_label,LoadResources&& load_resources) {
    state.panel=allocate(loading_panel_allocation_bytes);
    if(!state.panel)return false;
    construct_panel(state.panel,context,loading_panel_class,loading_client_scheme);
    state.label=allocate(loading_label_allocation_bytes);
    if(!state.label)return false;
    construct_label(state.label,state.panel,loading_label_name);
    attach(state.label);
    hide_label(state.label);
    state.label_initially_hidden=true;
    load_resources(state.panel,loading_panel_resource);
    return true;
}

inline constexpr std::string_view sound_emitter_version="VSoundEmitter001";
inline constexpr std::uint32_t original_sound_emitter_global_va=0x225721d0u;
inline constexpr std::uint8_t emitter_init_virtual_byte_offset=0;
inline constexpr std::uint8_t name_to_token_virtual_byte_offset=0x0c;
inline constexpr std::uint8_t token_to_name_virtual_byte_offset=0x18;
inline constexpr std::uint32_t missing_sound_token=0xffffffffu;
struct EmitterCalls {
    bool initialized=false;
    std::size_t lookup_calls=0;
    std::size_t reverse_calls=0;
};
// Engine interface adapter concept: preserves call ordering, sentinels and
// ignores x86 vtable addresses. Engine signatures are NOT independently proven.
template<class Init,class Lookup,class Reverse>
class SoundEmitterReference {
public:
    SoundEmitterReference(Init init,Lookup lookup,Reverse reverse)
        :init_(init),lookup_(lookup),reverse_(reverse) {}
    bool initialize(){state.initialized=init_();return state.initialized;}
    std::uint32_t resolve_name(std::string_view name) {
        if(!state.initialized)return missing_sound_token;
        ++state.lookup_calls;
        return lookup_(name);
    }
    std::string_view resolve_token(std::uint32_t token) {
        if(!state.initialized || token==missing_sound_token)return {};
        ++state.reverse_calls;
        return reverse_(token);
    }
    EmitterCalls state{};
private:
    Init init_;
    Lookup lookup_;
    Reverse reverse_;
};
} // namespace sin_compat::phase95
