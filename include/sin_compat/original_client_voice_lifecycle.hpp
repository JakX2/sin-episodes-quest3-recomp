#pragma once
// Phase552: small, original-free reference for authenticated CVoiceStatus
// lifecycle decisions. Not a Source ABI, GUI implementation or game client.
#include <cstdint>
#include <string_view>

namespace sin_compat::original_client_voice_lifecycle {
constexpr std::uint32_t kNativeVoiceStatusObjectBytes=0x2834;
constexpr std::uint32_t kNativeHashTableOffset=0x28;
constexpr std::uint32_t kNativeHashTableBytes=0x2800;
constexpr std::uint32_t kNativeTextureHandleOffset=0x2828;
constexpr std::uint32_t kNativeHashSeededFlagOffset=0x282C;
constexpr std::uint32_t kNativeVtable=0x242A9EF8;
constexpr std::uint32_t kNativeGlobalSingleton=0x24378658;
constexpr std::uint32_t kNativeActiveInstance=0x2437865C;
constexpr std::string_view kVoiceMaskMessage="VoiceMask";
constexpr std::string_view kRequestStateMessage="RequestState";
constexpr std::string_view kVoiceTexture="voice/icntlk_pl";
static_assert(kNativeHashTableOffset+kNativeHashTableBytes==kNativeTextureHandleOffset);
static_assert(kNativeHashSeededFlagOffset+8==kNativeVoiceStatusObjectBytes);

struct ConstructedFields {
    // Descriptive native constructor state, *not* full 10,292-byte model.
    std::uint32_t this_plus_0x04=0;
    std::uint32_t this_plus_0x08=0xffffffffU;
    std::uint32_t this_plus_0x0c=0;
    std::uint32_t this_plus_0x10=0;
    std::uint32_t this_plus_0x14=0;
    std::uint32_t this_plus_0x18=0;
    std::uint32_t this_plus_0x1c=0;
    std::uint32_t this_plus_0x2828=0;
    bool this_plus_0x282c=false;
    std::uint32_t this_plus_0x2830=0;
};

// Original 0x241E5D00 uses x87 current global +0x0C minus this+0x04
// and compares with exact float 1.0f. NaN fails the ordinary threshold.
inline bool should_unforced_refresh(float current_time, float previous_time) noexcept {
    return current_time - previous_time >= 1.0f;
}

struct IVoiceEvents {
    virtual ~IVoiceEvents()=default;
    // Evidence-preserving names; implementations are entirely synthetic.
    virtual void seed_voice_key_table()=0;
    virtual void acquire_vgui_texture(std::string_view texture)=0;
    virtual void publish_active_instance()=0;
    virtual void initialize_vgui_texture()=0;
    virtual void register_message(std::string_view name)=0;
    virtual void update_server_state(bool force)=0;
};

// Original vtable +4 method (0x241E5C60): bounded *ordering* reference.
// Original code does not check a null result from the VGUI texture factory
// before it invokes texture virtual +0x30; this model has no pointer derefs.
inline void describe_original_init_sequence(IVoiceEvents& host,
                                           bool original_engine_virtual_0x98_nonnull) {
    if (original_engine_virtual_0x98_nonnull) host.seed_voice_key_table();
    host.acquire_vgui_texture(kVoiceTexture);
    host.publish_active_instance();
    host.initialize_vgui_texture();
    host.register_message(kVoiceMaskMessage);
    host.register_message(kRequestStateMessage);
}

// Original registered 0x241E5C30 callback tests the *active* global pointer
// and then calls UpdateServerState with forced=1. It need not be the same as
// the lazily allocated singleton global at 0x24378658.
inline void registered_request_state(IVoiceEvents& host,bool has_active_instance) {
    if (has_active_instance) host.update_server_state(true);
}

// Native 0x241E5D30 returns early when VGUI +0xB0 says "already handled";
// only when false does it dispatch virtual +0x30 for a true requested flag
// and +0x34 for a false flag. Names of these virtuals are not recovered.
enum class VguiDispatch { AlreadyHandled, Virtual0x30, Virtual0x34 };
constexpr VguiDispatch choose_vgui_dispatch(bool vgui_virtual_0xb0, bool request_flag) noexcept {
    return vgui_virtual_0xb0 ? VguiDispatch::AlreadyHandled
        : request_flag ? VguiDispatch::Virtual0x30 : VguiDispatch::Virtual0x34;
}
}