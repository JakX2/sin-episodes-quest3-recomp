// Native MSVC Win32 tests for three exact trivial and one complete
// detail-object original Init methods, including secondary interface.
#include "client_known_constructed_gamesystem_init.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
namespace known=sin_independent::constructed_gamesystem;
namespace games=sin_independent::auto_gamesystem;
struct Provider {
    unsigned char padding[0x28]{};
    std::uint32_t field=0;
};
int main(){
    static_assert(sizeof(void*)==4,"original PE32 only");
    known::LeafComposite leaf;
    if(reinterpret_cast<std::uintptr_t>(&leaf.game_system)-
       reinterpret_cast<std::uintptr_t>(&leaf)!=4)return 1;
    known::StringPoolSystem strings;
    known::EffectPrecacheSystem effects;
    if(!strings.Init() || !leaf.game_system.Init() || !effects.Init())return 2;
    games::Registry r;
    r.DirectRegister(&strings);
    r.Construct(&effects);
    r.DirectRegister(&leaf.game_system);
    auto outcome=r.Initialize();
    if(!outcome.succeeded() || outcome.initialized_count!=3 ||
       r.registered_size()!=3)return 3;
    // Real source CGameStringPool is first in the original direct
    // startup registry. Its original Init is a no-op true, NOT
    // permission to discard strings interned before registration.
    char name[]="SinTEK_Enemy";
    const char* interned=strings.Intern(name);
    if(!interned || interned==name ||
       strings.Intern("sintek_enemy")!=interned ||
       strings.Find("SINTEK_ENEMY")!=interned ||
       strings.Size()!=1u || !strings.Init() ||
       strings.Find("sintek_enemy")!=interned)return 10;
    name[0]='X';
    if(std::strcmp(interned,"SinTEK_Enemy"))return 11;
    strings.ClearOnOriginalSlot5();
    if(strings.Size()!=0u || strings.Find("sintek_enemy") ||
       strings.Intern("") || strings.Intern(nullptr))return 12;
    const char* again=strings.Intern("sintek_enemy");
    if(!again || std::strcmp(again,"sintek_enemy") ||
       strings.Find("SINTEK_ENEMY")!=again)return 13;
    Provider first{},second{};
    first.field=0x12345678;second.field=0xABCDEF90;
    constexpr std::size_t kObjectBytes=0x9000;
    void* object=VirtualAlloc(nullptr,kObjectBytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!object)return 4;
    std::memset(object,0xC5,kObjectBytes);
    if(known::InitDetailObject(nullptr,&first,&second) ||
       known::InitDetailObject(object,nullptr,&second) ||
       known::InitDetailObject(object,&first,nullptr))return 5;
    if(!known::InitDetailObject(object,&first,&second))return 6;
    auto* p=static_cast<unsigned char*>(object);
    std::uint32_t a=0,b=0;
    std::memcpy(&a,p+0x806c,sizeof(a));
    std::memcpy(&b,p+0x8070,sizeof(b));
    if(a!=first.field || b!=second.field ||
       p[0x806b]!=0xC5 || p[0x8074]!=0xC5)return 7;
    DWORD previous=0;
    if(!VirtualProtect(object,kObjectBytes,PAGE_READONLY,&previous))return 8;
    if(known::InitDetailObject(object,&first,&second))return 9;
    DWORD unused=0;VirtualProtect(object,kObjectBytes,previous,&unused);
    VirtualFree(object,0,MEM_RELEASE);
    std::puts("PASS original three true Init methods, secondary leaf +4 and complete CDetailObjectSystem Init");
    return 0;
}
