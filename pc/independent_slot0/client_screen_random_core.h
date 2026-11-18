#pragma once
// Original-guided client screen and gaussian-stream startup boundary.
// The engine owns IVEngineClient and IUniformRandomStream. The
// independently allocated Gaussian storage uses the SDK2004 public
// 12-byte x86 layout and the existing vstdlib constructor/Attach ABI.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Win32 original ABI only
#endif
#include <cstddef>
#include <array>
#include <cstdint>
namespace sin_independent {
using ScreenSizeSlot=void (__thiscall *)(void*,int&,int&);
struct ScreenState {
    int width=0,height=0;
    bool queried=false;
    bool Query(void* engine) noexcept {
        if(!engine || queried)return false;
        auto** vt=*reinterpret_cast<void***>(engine);
        if(!vt || !vt[5])return false;
        auto get=reinterpret_cast<ScreenSizeSlot>(vt[5]);
        // Original screen globals are initially zero-filled. Preserve
        // the actual values, even zero: do not invent a resolution.
        get(engine,width,height);
        queried=true;
        return true;
    }
};
struct GaussianStorage {
    void* uniform=nullptr;
    bool haveValue=false;
    float cached=0;
};
static_assert(sizeof(GaussianStorage)==12 && offsetof(GaussianStorage,uniform)==0 &&
              offsetof(GaussianStorage,haveValue)==4 && offsetof(GaussianStorage,cached)==8,
              "SDK 2004 Win32 Gaussian storage");
using GaussianCtor=void* (__thiscall *)(GaussianStorage*,void*);
using GaussianAttach=void (__thiscall *)(GaussianStorage*,void*);
struct ClientScreenRandomState {
    ScreenState screen{};
    GaussianStorage gaussian{};
    // Extra owned guard storage detects unexpected provider writes past
    // the SDK layout. This is diagnostic detection, not memory isolation.
    std::array<std::uint32_t,16> guard{};
    bool attached=false;
    bool attempted=false;
    bool Attach(void* uniform,GaussianCtor ctor,GaussianAttach attach) noexcept {
        if(!screen.queried || attempted || !uniform || !ctor || !attach)return false;
        attempted=true;
        guard.fill(0x53494E47u);
        const auto intact=[this]() noexcept {
            for(auto word:guard)if(word!=0x53494E47u)return false;
            return true;
        };
        // Original static initializer constructed with null. The live
        // Init path attaches the acquired VEngineRandom001 interface.
        // Construct on owned storage, not the original DLL's globals.
        if(ctor(&gaussian,nullptr)!=&gaussian || !intact())return false;
        attach(&gaussian,uniform);
        attached=intact() && gaussian.uniform==uniform && !gaussian.haveValue;
        return attached;
    }
};
} // namespace sin_independent
