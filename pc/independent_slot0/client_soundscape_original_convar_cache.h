#pragma once
// Phase 496: real-call-capable, opt-IN independent reconstruction of
// original 2006 C_SoundscapeSystem virtual slot 3, VA 0x240F3C40.
// Entire 61-byte original body SHA-256:
// 8a2bce6161d8de0973141bfae7d6c151091ee75933291d1c1d61b15d65b82040
//
// Original source caches ICvar::FindVar("snd_soundmixer") and
// ICvar::FindVar("dsp_volume") in globals 0x2433E4AC/+0x2433E4A8
// respectively, only when the corresponding cache is currently null.
// This adapter calls ONLY the typed x86 VEngineCvar002 virtual slot3
// on an explicitly supplied caller-owned object. It never changes
// the actual game globals, changes ConVar values, or invokes Source
// audio. No persistent engine pointer is retained across Reset().
#if !defined(_M_IX86)
#error Exact 2006 Source VEngineCvar002 ABI requires MSVC native x86
#endif
#include "client_known_gamesystem_init.h"
#include "server_single_cvar_probe.h"
#include <cstddef>
#include <cstdint>

namespace sin_independent::soundscape_original_cvars {
inline constexpr std::uint32_t kOriginalMethodVA=0x240F3C40u;
inline constexpr std::uint32_t kOriginalMethodBytes=61u;
inline constexpr std::uint32_t kOriginalVtableVA=0x24293C34u;
inline constexpr std::size_t kOriginalSystemVtableSlot=3u;
inline constexpr std::size_t kOriginalCvarVirtualSlot=3u;
inline constexpr std::uint32_t kOriginalCvarSingletonVA=0x2433F690u;
inline constexpr std::uint32_t kOriginalCachedMixerVA=0x2433E4ACu;
inline constexpr std::uint32_t kOriginalCachedDspVA=0x2433E4A8u;
inline constexpr char kMixerName[]="snd_soundmixer";
inline constexpr char kDspName[]="dsp_volume";
enum class Result {
    BothCached, OneCached, NeitherCached, InvalidCvarInterface,
    CallbackRefused
};
inline const char* Name(Result r) noexcept {
    switch(r) {
        case Result::BothCached:return "original-soundscape-both-vars-cached";
        case Result::OneCached:return "original-soundscape-one-var-cached";
        case Result::NeitherCached:return "original-soundscape-vars-unavailable";
        case Result::InvalidCvarInterface:return "soundscape-cvar-invalid-abi";
        case Result::CallbackRefused:return "soundscape-cvar-callback-refused";
    }
    return "unknown";
}
struct Report final {
    Result result=Result::InvalidCvarInterface;
    unsigned requested=0;
    unsigned found_this_call=0;
    bool mixer_cached=false;
    bool dsp_cached=false;
    bool different_engine_instance=false;
};
class OwnedCache final {
public:
    // Pointer values are kept only to model the original engine
    // references. Never dereferenced, released or modified.
    const void* mixer() const noexcept{return mixer_;}
    const void* dsp() const noexcept{return dsp_;}
    void* associated_engine() const noexcept{return engine_;}
    void Reset() noexcept {
        mixer_=nullptr;
        dsp_=nullptr;
        engine_=nullptr;
    }
    Report Resolve(void* engine) noexcept {
        Report output{};
        // Guard BEFORE any engine call or state change. The same
        // 2004 vtable-slot ownership check is used by the already
        // verified client KnownGameSystem adapters.
        known_game_system::CallResult refused=
            known_game_system::CallResult::InvalidObject;
        void* const method=known_game_system::VerifiedMethod(
            engine,kOriginalCvarVirtualSlot,refused);
        if(!method)return output;
        const bool changed=engine_!=nullptr&&engine_!=engine;
        // A new provider invalidates all references from the old provider,
        // even if the new FindVar callback throws before commit.
        if(changed)Reset();
        const auto mixerAtStart=changed?nullptr:mixer_;
        const auto dspAtStart=changed?nullptr:dsp_;
        const void* newMixer=mixerAtStart;
        const void* newDsp=dspAtStart;
        // The authentic original globals are singletons for the
        // lifetime of one Windows engine. Unlike the retail code,
        // an owned adapter refuses to reuse pointers from a DIFFERENT
        // mock/engine instance across reinitialization.
        using Find=sin_independent::console_abi::FindSlot;
        auto find=reinterpret_cast<Find>(method);
        try {
            if(!newMixer) {
                ++output.requested;
                newMixer=find(engine,kMixerName);
                if(newMixer)++output.found_this_call;
            }
            if(!newDsp) {
                ++output.requested;
                newDsp=find(engine,kDspName);
                if(newDsp)++output.found_this_call;
            }
        }catch(...) {
            output.result=Result::CallbackRefused;
            output.different_engine_instance=changed;
            output.mixer_cached=mixer_!=nullptr;
            output.dsp_cached=dsp_!=nullptr;
            return output; // same-provider cache survives; switched provider is cleared
        }
        mixer_=newMixer;
        dsp_=newDsp;
        engine_=engine;
        output.different_engine_instance=changed;
        output.mixer_cached=mixer_!=nullptr;
        output.dsp_cached=dsp_!=nullptr;
        output.result=output.mixer_cached&&output.dsp_cached?
            Result::BothCached:
            output.mixer_cached||output.dsp_cached?
            Result::OneCached:Result::NeitherCached;
        return output;
    }
private:
    const void* mixer_=nullptr;
    const void* dsp_=nullptr;
    void* engine_=nullptr;
};
static_assert(kOriginalSystemVtableSlot==3u,
              "Source game-system virtual 3 identity");
static_assert(kOriginalCvarVirtualSlot==3u,
              "Source VEngineCvar002 FindVar virtual byte +0x0C");
} // namespace sin_independent::soundscape_original_cvars
