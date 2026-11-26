// Owner-only direct original ammo comparison with synthetic services.
// No retail initializer, external PE import, original vtable or original bytes in repo.
#include "native_server_npc_state_reconsider.h"
#include "native_server_weapon_script_view.h"
#include "../../include/sin_compat/phase100_sha256.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
using namespace sin_port::native_weapon_tables;
namespace {
constexpr sin_compat::phase100::Digest256 expected{{
    0x56,0x8a,0x00,0x86,0xfd,0xe6,0x07,0x34,0x5a,0xdf,0x31,0xd7,0xcf,0xb2,0xa5,0xbf,
    0x4e,0x4e,0xfe,0x9a,0x86,0xf4,0x16,0xfa,0xa9,0x70,0x76,0xdd,0xf9,0xd9,0x76,0x50}};
constexpr std::uintptr_t base=0x22000000u;
template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
struct Slice {unsigned offset,size;};
constexpr Slice code[]={
    {0x1e6370,0x55}, // real ammo-definition max-carry method
    {0x2aa770,0x37}, // owner count
    {0x2abfd0,0x13},{0x2ac010,0x13}, // primary/secondary uses-clip methods
    {0x2ad690,0x79},{0x2ad710,0x6c} // primary/secondary availability
};
struct Original {
    unsigned char* image=nullptr;
    ~Original(){if(image)VirtualFree(image,0,MEM_RELEASE);}
    bool open(const char* path,void* definitions,void* registry){
        static_assert(sizeof(void*)==4,"original ammo probe needs Win32");
        std::ifstream in(path,std::ios::binary|std::ios::ate);
        if(!in||in.tellg()!=6049792)return false;
        std::vector<unsigned char> bytes(6049792);in.seekg(0);
        if(!in.read(reinterpret_cast<char*>(bytes.data()),bytes.size())||
            sin_compat::phase100::sha256(bytes.data(),bytes.size())!=expected)return false;
        auto requested=reinterpret_cast<void*>(base);
        image=static_cast<unsigned char*>(VirtualAlloc(requested,0x628000,MEM_RESERVE,PAGE_NOACCESS));
        if(image!=requested)return false;
        constexpr unsigned pages[]={0x1e6000,0x2aa000,0x2ab000,0x2ac000,0x2ad000,0x2d5000,0x4f5000};
        for(unsigned page:pages)if(VirtualAlloc(image+page,0x1000,MEM_COMMIT,PAGE_READWRITE)!=image+page)return false;
        for(const auto& s:code)std::memcpy(image+s.offset,bytes.data()+s.offset,s.size);
        // Replace only lazy singleton accessor 222d54b0: cdecl, no stack pop.
        // Its caller's pushed ammo index is consumed by the genuine 221e6370.
        image[0x2d54b0]=0xb8;put(image,0x2d54b1,reinterpret_cast<std::uintptr_t>(definitions));image[0x2d54b5]=0xc3;
        // Original serial-resolved registry: first entity pointer at global+4.
        put(image,0x4f5620,reinterpret_cast<std::uintptr_t>(registry));
        for(unsigned i=0;i<7;++i){DWORD old=0;
            if(!VirtualProtect(image+pages[i],0x1000,i<6?PAGE_EXECUTE_READ:PAGE_READONLY,&old))return false;
            if(i<6&&!FlushInstructionCache(GetCurrentProcess(),image+pages[i],0x1000))return false;
        }
        return true;
    }
    bool query(void* w,bool primary) const {
        using Fn=bool (__thiscall *)(void*);
        return reinterpret_cast<Fn>(image+(primary?0x2ad690:0x2ad710))(w);
    }
};
struct Fixture {
    alignas(4) unsigned char original_weapon[0x520]{},native_weapon[0x520]{},owner[0x700]{},adjusted[0x700]{};
    alignas(4) unsigned char script[0x168]{},definitions[0x34+32*60]{},registry[32]{};
    std::uintptr_t weapon_vt[0x440/4]{},owner_vt[0x100/4]{};
    sin_independent::EntityHandleEntry entries[2]{};
    void* combat=nullptr;
    int cap=0,primary_max=0,secondary_max=0;
};
Fixture* current=nullptr;
int __fastcall primary_clip(void*,void*) noexcept{return current->primary_max;}
int __fastcall secondary_clip(void*,void*) noexcept{return current->secondary_max;}
void* __fastcall owner_cast(void*,void*) noexcept{return current->combat;}
int __cdecl extent(void* p,void* e,unsigned* n) noexcept{
    auto& f=*static_cast<Fixture*>(p);
    if(e==f.native_weapon)*n=sizeof(f.native_weapon);
    else if(e==f.owner)*n=sizeof(f.owner);
    else if(e==f.adjusted)*n=sizeof(f.adjusted);
    else return 0;
    return 1;
}
int __cdecl combat_owner(void* p,void* e,void** out) noexcept{
    auto& f=*static_cast<Fixture*>(p);if(e!=f.owner)return 0;*out=f.combat;return 1;
}
int __cdecl max_carry(void* p,int type,int* out) noexcept{
    if(type<1||type>2)return 0;*out=static_cast<Fixture*>(p)->cap;return 1;
}
bool run_matrix(const ServerNpcWeaponAmmoApi& api,Fixture& f,const Original* original){
    ServerWeaponScriptView script_view{};script_view.fallback=f.script;script_view.fallback_bytes=sizeof(f.script);
    ServerNpcWeaponAmmoBindings ammo{};ammo.scripts=&script_view;ammo.context=&f;ammo.combat_owner=combat_owner;ammo.max_carry=max_carry;
    ServerNpcGruntAssaultAdmissionBindings ab{};ab.context=&f;ab.entries=f.entries;ab.count=2;ab.extent=extent;
    f.weapon_vt[0x404/4]=reinterpret_cast<std::uintptr_t>(&primary_clip);
    f.weapon_vt[0x408/4]=reinterpret_cast<std::uintptr_t>(&secondary_clip);
    f.owner_vt[0xfc/4]=reinterpret_cast<std::uintptr_t>(&owner_cast);
    if(original){
        f.weapon_vt[0x438/4]=base+0x2abfd0;f.weapon_vt[0x43c/4]=base+0x2ac010;
    }
    put(f.owner,0,reinterpret_cast<std::uintptr_t>(f.owner_vt));put(f.definitions,4,32);
    f.entries[1].serial=7;put(f.registry,24,7u);
    unsigned cases=0;
    // Same fixture drives both implementations; expected outcome is independent
    // of the DLLs. No negative array indexes other than original sentinel -1.
    for(int mode=0;mode<6;++mode)for(int maximum:{-1,0,8})
    for(int clip:{-1,0,1,8})for(int type:{-1,1,2})
    for(int reserve:{-1,0,3})for(int carry:{0,80,-2})for(int own_reserve:{0,4}){
        const bool live=mode>=3;
        f.entries[1].entity=mode==2?nullptr:reinterpret_cast<CBaseEntity*>(f.owner);
        put(f.registry,20,reinterpret_cast<std::uintptr_t>(f.entries[1].entity));
        f.combat=mode==4?static_cast<void*>(f.owner):mode==5?static_cast<void*>(f.adjusted):nullptr;
        const unsigned handle=mode==0?0xffffffffu:mode==1?0x8001u:0x7001u;
        f.cap=carry;f.primary_max=f.secondary_max=maximum;
        put(f.script,0x160,maximum);put(f.script,0x164,maximum);
        put(f.definitions,0x34+60,carry);put(f.definitions,0x34+120,carry);
        for(auto* a:{f.owner,f.adjusted}){put(a,0x650,reserve);put(a,0x654,reserve);}
        for(auto* w:{f.original_weapon,f.native_weapon}){
            put(w,0x494,handle);put(w,0x4e4,type);put(w,0x4e8,type);
            put(w,0x4ec,clip);put(w,0x4f0,clip);put(w,0x4cc,own_reserve);
            put(w,0x51c,static_cast<std::uint16_t>(0xffff));
        }
        put(f.original_weapon,0,reinterpret_cast<std::uintptr_t>(f.weapon_vt));
        put(f.native_weapon,0,0x2247875cu);
        for(bool primary:{true,false}){
            const bool expected_available=(maximum!=-1&&clip>0)||
                (live&&f.combat?(type!=-1&&(carry==-2||reserve>0)):(primary&&own_reserve>0));
            ServerNpcWeaponAmmoResult result{};
            if(!api.query(f.native_weapon,primary?0x330:0x334,&ab,&ammo,&result)||!result.completed||
                (result.available!=0)!=expected_available||
                (original&&original->query(f.original_weapon,primary)!=expected_available)){
                std::fprintf(stderr,"ammo mismatch case=%u primary=%d mode=%d max=%d clip=%d type=%d reserve=%d carry=%d own=%d native=%u expected=%d\n",
                    cases,primary,mode,maximum,clip,type,reserve,carry,own_reserve,result.available,expected_available);
                return false;
            }
            ++cases;
        }
    }
    std::printf("ammo matrix %u/%u passed (%s)\n",cases,cases,original?"native + original PE32":"native only");
    return true;
}
}
int main(int argc,char** argv){
    if(argc!=3){std::fputs("Usage: sin_server_npc_weapon_ammo_audit --self-test <native server.dll> OR <native server.dll> <original server.dll>\n",stderr);return 2;}
    const bool self=!std::strcmp(argv[1],"--self-test");
    Fixture f;current=&f;Original original;
    if(!self&&!original.open(argv[2],f.definitions,f.registry)){std::fputs("Original hash/preferred mapping failed\n",stderr);return 2;}
    HMODULE dll=LoadLibraryA(self?argv[2]:argv[1]);if(!dll)return 2;
    auto getter=reinterpret_cast<GetServerNpcWeaponAmmoApi>(GetProcAddress(dll,server_npc_weapon_ammo_export));
    const auto* api=getter?getter(1,sizeof(ServerNpcWeaponAmmoApi)):nullptr;
    const bool ok=api&&run_matrix(*api,f,self?nullptr:&original);FreeLibrary(dll);
    return ok?0:1;
}
