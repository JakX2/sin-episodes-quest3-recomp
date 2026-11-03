#pragma once
// SiN Episodes 2006 x86-derived host-only reference. No native engine ABI.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
namespace sin_compat::phase94 {
inline constexpr std::size_t loadingdisc_allocation_bytes = 0xdc;
inline constexpr std::uint32_t loadingdisc_x86_vtable_va=0x242ac8b0u;
inline constexpr std::array<std::uint32_t,4> loadingdisc_methods{{
    0x241ee470u,0x241ee480u,0x241ee4d0u,0x241ee530u}};
struct LoadingDiscFields {
    void* first=nullptr;     // original this+4
    void* second=nullptr;    // original this+8
    void* context=nullptr;   // original this+0x0c, separately assigned
};
// Models original 0x241EE4D0 and 0x241EE530. The original secondary
// path dereferences a newly-created pointer without explicit null check;
// our host-only mock returns false safely instead of emulating a crash.
template<class Create,class Attach,class Configure,class Visible>
bool toggle_loading_disc(LoadingDiscFields& s,bool second,bool enabled,
                         Create&& create,Attach&& attach,
                         Configure&& configure,Visible&& visible) {
    void*& slot=second?s.second:s.first;
    if(enabled && !slot) {
        void* candidate=create(loadingdisc_allocation_bytes,s.context);
        attach(candidate); // original helper 0x240049D0 on both paths
        slot=candidate;
        if(!slot)return false;  // guard added only by safe host mock
        if(second)configure(slot,0x28c,0x242ac8c0u);
    }
    if(slot)visible(slot,enabled,0x78);
    return true;
}
inline constexpr std::size_t original_event_stride_bytes=0x50;
inline constexpr std::uint32_t unresolved_model_sound_token=0xffffffffu;
inline constexpr std::array<std::uint32_t,6> original_event_ids{{14,1004,1008,5004,6004,6005}};
inline constexpr std::array<std::string_view,4> footstep_suffixes{{
    ".FootstepRight", ".FootstepLeft", ".RunFootstepRight", ".RunFootstepLeft"}};
struct ParsedEvent {
    std::uint32_t id=0;
    std::uint8_t flags=0;
    std::string name;
};
// Inputs are synthetic *already-parsed* event descriptors, not raw .cache
// or model bytes. This models only verified event-id/flag filters.
template<class Emit,class EmptyWarning>
void examine_model_sound_events(const std::vector<ParsedEvent>& events,
                                std::string_view fallback_base,
                                Emit&& emit,EmptyWarning&& warn) {
    const std::string base=fallback_base.empty()?"NPC_grunt":std::string(fallback_base);
    for(const auto& event:events) {
        if(event.id==6004u || event.id==6005u) {
            for(auto suffix:footstep_suffixes)
                emit(base+std::string(suffix));
            continue;
        }
        if(event.id==14u) {
            if((event.flags & 0x10u)==0)continue;
            if(event.name.empty()){warn(event.id);continue;}
            emit(event.name);
        } else if(event.id==1004u || event.id==1008u || event.id==5004u) {
            emit(event.name);
        }
    }
}
// Original 0x221456E0 resolves strings to unsigned-32-bit tokens and
// stores each valid token at most once in its per-object vector.
template<class Resolve>
void insert_unique_sound_token(std::vector<std::uint32_t>& tokens,
                               std::string_view name,Resolve&& resolve) {
    const std::uint32_t token=resolve(name);
    if(token==unresolved_model_sound_token)return;
    if(std::find(tokens.begin(),tokens.end(),token)==tokens.end())
        tokens.push_back(token);
}
} // namespace sin_compat::phase94
