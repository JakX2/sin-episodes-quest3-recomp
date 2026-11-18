// Exercise the original Connect ABI with independently owned interfaces.
// This test does not load the engine or either commercial game DLL.
#include "sound_emitter_connect_core.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

using namespace sin_independent;
static unsigned engineCalls=0, appCalls=0;
static int engineToken=1, appToken=2;
static void* __cdecl Engine(const char* name,int* status) {
    ++engineCalls;
    if (!name || std::strcmp(name,"synthetic-service") || status) return nullptr;
    return &engineToken;
}
static void* __cdecl App(const char* name,int* status) {
    ++appCalls;
    if (!name || std::strcmp(name,"synthetic-service") || status) return nullptr;
    return &appToken;
}
class Sound final {
public:
    virtual bool __thiscall Connect(EngineFactory factory) noexcept {
        ++calls;
        seen=factory;
        service=factory("synthetic-service",nullptr);
        return accept && service!=nullptr;
    }
    virtual bool __thiscall Init() noexcept { ++unexpected; return true; }
    virtual void __thiscall Shutdown() noexcept { ++unexpected; }
    EngineFactory seen=nullptr;
    void* service=nullptr;
    unsigned calls=0, unexpected=0;
    bool accept=true;
};
static int fail(int code,const char* message) {
    std::fprintf(stderr,"FAIL sound Connect %d: %s\n",code,message);
    return code;
}
int main() {
    Sound sound;
    if (TryConnectSoundEmitter(nullptr,&Engine)!=SoundConnectResult::MissingInterface ||
        TryConnectSoundEmitter(&sound,nullptr)!=SoundConnectResult::MissingFactory ||
        sound.calls || engineCalls || appCalls)
        return fail(1,"missing interface or factory");
    void* emptyTable[1]={};
    void** emptyObject=emptyTable;
    if(TryConnectSoundEmitter(&emptyObject,&Engine)!=SoundConnectResult::MissingCallback ||
       engineCalls) return fail(2,"missing slot");

    if(TryConnectSoundEmitter(&sound,&Engine)!=SoundConnectResult::Connected ||
       sound.seen!=&Engine || sound.service!=&engineToken ||
       sound.calls!=1 || engineCalls!=1 || appCalls || sound.unexpected)
        return fail(3,"server Connect receives exact factory");
    if(TryConnectSoundEmitter(&sound,&App)!=SoundConnectResult::Connected ||
       sound.seen!=&App || sound.service!=&appToken ||
       sound.calls!=2 || engineCalls!=1 || appCalls!=1 || sound.unexpected)
        return fail(4,"client Connect receives exact factory");
    sound.accept=false;
    if(TryConnectSoundEmitter(&sound,&App)!=SoundConnectResult::Rejected ||
       sound.calls!=3 || appCalls!=2 || sound.unexpected)
        return fail(5,"engine rejection propagated");

    // Independent direct x86 ABI check: receiver ECX, one cdecl factory
    // pointer argument, bool in AL and callee stack cleanup of 4 bytes.
    auto** slots=*reinterpret_cast<void***>(&sound);
    void* target=slots[0];
    void* object=&sound;
    EngineFactory callback=&Engine;
    std::uintptr_t before=0,after=0;
    unsigned returned=99;
    sound.accept=true;
    __asm {
        mov eax, esp
        mov before, eax
        push callback
        mov ecx, object
        mov eax, target
        call eax
        movzx eax, al
        mov returned, eax
        mov eax, esp
        mov after, eax
    }
    if(before!=after || returned!=1 || sound.calls!=4 || engineCalls!=2 ||
       sound.unexpected) return fail(6,"native x86 slot-zero Connect ABI");
    std::puts("PASS sound emitter Connect: both factory roles, missing inputs, bool rejection, native x86 stack, no Init/Shutdown");
    return 0;
}
