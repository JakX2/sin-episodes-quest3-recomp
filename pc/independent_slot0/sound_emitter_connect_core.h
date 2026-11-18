#pragma once
// First post-factory initialization operation shared by both original
// game DLLs: ISoundEmitterSystemBase::Connect(CreateInterfaceFn).
// The original SDK 2004 interface has Connect at slot zero, no leading
// virtual destructor. Both retail startup methods were observed to
// call this slot and branch to their failure return if bool is false.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Native Windows x86 only
#endif

namespace sin_independent {
using EngineFactory = void* (__cdecl *)(const char*, int*);
using SoundConnect = bool (__thiscall *)(void*, EngineFactory);
enum class SoundConnectResult {
    MissingInterface, MissingFactory, MissingCallback, Rejected, Connected
};
inline SoundConnectResult TryConnectSoundEmitter(
    void* sound, EngineFactory factory) noexcept {
    if (!sound) return SoundConnectResult::MissingInterface;
    if (!factory) return SoundConnectResult::MissingFactory;
    auto* table = *reinterpret_cast<void***>(sound);
    if (!table || !table[0]) return SoundConnectResult::MissingCallback;
    auto connect = reinterpret_cast<SoundConnect>(table[0]);
    return connect(sound, factory) ? SoundConnectResult::Connected :
        SoundConnectResult::Rejected;
}
} // namespace sin_independent
