// MSVC x86 native fixture for three complete 2006 AutoGameSystem
// Init methods, with original virtual slot indices and state offsets.
#include "client_known_gamesystem_init.h"
#include "client_auto_gamesystem_init_core.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <array>
#include <vector>
namespace game=sin_independent::known_game_system;
namespace auto_sys=sin_independent::auto_gamesystem;
struct SoundProvider {
    int initCalls=0;
    bool result=true;
    virtual bool Guard() noexcept {std::abort();return false;}
    virtual bool Init() noexcept {++initCalls;return result;}
};
struct ViewportProvider {
    int lastSlot=0;
    virtual void S0() noexcept {std::abort();}
    virtual void S1() noexcept {std::abort();}
    virtual void S2() noexcept {std::abort();}
    virtual void S3() noexcept {std::abort();}
    virtual void S4() noexcept {std::abort();}
    virtual void S5() noexcept {std::abort();}
    virtual void S6() noexcept {std::abort();}
    virtual void S7() noexcept {lastSlot=7;}
};
struct SoundSystem: auto_sys::System {
    SoundProvider* provider=nullptr;
    bool Init() noexcept override {
        return game::InitSoundEmitter(provider)==game::CallResult::CalledTrue;
    }
};
struct ViewportSystem: auto_sys::System {
    ViewportProvider* provider=nullptr;
    bool Init() noexcept override {
        return game::InitViewport(provider)==game::CallResult::CalledTrue;
    }
};
struct ThinkSystem: auto_sys::System {
    // With 32-bit C++ System having vptr + next = 8 bytes, this
    // synthetic padding puts the original fields at +0x48 and +0x4C.
    unsigned char padding[0x40]{};
    std::uint32_t counter=0xAABBCCDDu;
    unsigned char pending=0xEEu;
    bool Init() noexcept override {return game::InitClientThinkList(this);}
};
int main(){
    static_assert(sizeof(void*)==4,"original is PE32");
    SoundProvider sound;
    ViewportProvider viewport;
    using game::CallResult;
    if(game::InitSoundEmitter(nullptr)!=CallResult::InvalidObject ||
       game::InitViewport(nullptr)!=CallResult::InvalidObject ||
       game::InitClientThinkList(nullptr))return 1;
    if(game::InitSoundEmitter(&sound)!=CallResult::CalledTrue ||
       sound.initCalls!=1)return 2;
    sound.result=false;
    if(game::InitSoundEmitter(&sound)!=CallResult::CalledFalse ||
       sound.initCalls!=2)return 3;
    if(game::InitViewport(&viewport)!=CallResult::CalledTrue ||
       viewport.lastSlot!=7)return 4;
    struct Invalid {void* vptr;};
    Invalid bad{nullptr};
    if(game::InitSoundEmitter(&bad)!=CallResult::InvalidVtable ||
       game::InitViewport(&bad)!=CallResult::InvalidVtable)return 5;
    ThinkSystem think;
    if(reinterpret_cast<std::uintptr_t>(&think.counter)-
       reinterpret_cast<std::uintptr_t>(&think)!=0x48 ||
       reinterpret_cast<std::uintptr_t>(&think.pending)-
       reinterpret_cast<std::uintptr_t>(&think)!=0x4c)return 6;
    if(!think.Init() || think.counter || think.pending)return 7;
    // Probe all three adapters through the reconstructed real
    // AutoGameSystem order/failure semantics, not just standalone calls.
    sound.result=true;
    SoundSystem ownedSound;ownedSound.provider=&sound;
    ViewportSystem ownedViewport;ownedViewport.provider=&viewport;
    auto_sys::Registry systems;
    systems.DirectRegister(&ownedSound);
    systems.Construct(&ownedViewport);
    systems.Construct(&think);
    auto outcome=systems.Initialize();
    if(!outcome.succeeded() || outcome.initialized_count!=3 ||
       sound.initCalls!=3)return 8;
    sound.result=false;
    auto_sys::Registry failed;
    failed.DirectRegister(&ownedSound);
    failed.DirectRegister(&ownedViewport);
    outcome=failed.Initialize();
    if(outcome.outcome!=auto_sys::Outcome::InitRefused ||
       outcome.initialized_count!=0 ||
       viewport.lastSlot!=7)return 9;
    // No emulation of the still-unrecovered benchmark system.
    std::puts("PASS real SiN Init ABI adapters: CSoundEmitterSystem, CViewportClientSystem, CClientThinkList");
    return 0;
}
