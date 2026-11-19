#pragma once
// Original SiN 2006 CClientShadowMgr::Init (VA 0x24118880).
// This is an independently owned, callback-driven orchestration model.
// Real original helper bodies and virtual function implementations remain
// unknown. Never pass a retail game object or pointer into this component.
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace sin_independent::client_shadow {
enum class Action {
    InitSimpleShadowTexture,
    SetShadowDirection,
    SetShadowDistance,
    SetShadowBlobbyCutoffArea,
    InitRenderToTextureShadows,
    AddMaterialRestoreFunc
};
struct Operation {
    Action action;
    std::uint32_t callsite_va;
    std::uint32_t original_slot;
    const char* name;
    const char* category;
    const float* vector_arg;
    float scalar_arg;
    std::uint32_t original_callback_va;
};
inline constexpr float kOriginalVector[3]={0.1f,0.1f,-1.0f};
inline constexpr std::size_t kOriginalTextureSubobjectOffset=0x14;
inline constexpr std::size_t kOriginalFlagOffset=0xcd;
inline constexpr std::size_t kOriginalModeOffset=0xd8;
inline constexpr std::uint32_t kOriginalEngineCallbackVA=0x24115e70u;
inline constexpr char kDecal[]="decals/simpleshadow";
inline constexpr char kTextureCategory[]="Decal textures";
using ActionSink=void (*)(void*,const Operation&) noexcept;
inline constexpr Operation kPrecache{
    Action::InitSimpleShadowTexture,0x2411889au,0,
    kDecal,kTextureCategory,nullptr,0.0f,0};
inline constexpr Operation kVector{
    Action::SetShadowDirection,0x241188c0u,24,
    nullptr,nullptr,kOriginalVector,0.0f,0};
inline constexpr Operation kRadius{
    Action::SetShadowDistance,0x241188ccu,27,
    nullptr,nullptr,nullptr,50.0f,0};
inline constexpr Operation kBias{
    Action::SetShadowBlobbyCutoffArea,0x241188d8u,28,
    nullptr,nullptr,nullptr,0.005f,0};
inline constexpr Operation kRefresh{
    Action::InitRenderToTextureShadows,0x241188f3u,0,
    nullptr,nullptr,nullptr,0.0f,0};
inline constexpr Operation kRegisterCallback{
    Action::AddMaterialRestoreFunc,0x24118905u,116,
    nullptr,nullptr,nullptr,0.0f,kOriginalEngineCallbackVA};
// This host expects callbacks implementing the missing original helper
// and virtual-method behavior. It does not silently fake that work.
// Native always returns true, regardless of those methods' results.
inline bool Init(void* owner,std::size_t ownerBytes,bool refreshEnabled,
                 void* context,ActionSink sink) noexcept {
    if(!owner || ownerBytes<kOriginalModeOffset+sizeof(std::uint32_t) ||
       !context || !sink)return false;
    auto* dst=static_cast<unsigned char*>(owner);
    dst[kOriginalFlagOffset]=0;
    sink(context,kPrecache);
    sink(context,kVector);
    sink(context,kRadius);
    sink(context,kBias);
    constexpr std::uint32_t mode=8;
    std::memcpy(dst+kOriginalModeOffset,&mode,sizeof(mode));
    if(refreshEnabled)sink(context,kRefresh);
    sink(context,kRegisterCallback);
    return true;
}
} // namespace sin_independent::client_shadow
