// Owner-only direct PE32 melee condition replay with synthetic virtual services.
#include "native_server_npc_state_reconsider.h"
#include "../../include/sin_compat/phase100_sha256.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
#include <initializer_list>
using namespace sin_port::native_weapon_tables;
namespace {
constexpr sin_compat::phase100::Digest256 expected{{
    0x56,0x8a,0x00,0x86,0xfd,0xe6,0x07,0x34,0x5a,0xdf,0x31,0xd7,0xcf,0xb2,0xa5,0xbf,
    0x4e,0x4e,0xfe,0x9a,0x86,0xf4,0x16,0xfa,0xa9,0x70,0x76,0xdd,0xf9,0xd9,0x76,0x50}};
constexpr std::uintptr_t base=0x22000000u;
template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
template<class T>T read(const void* p,unsigned off){T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
struct Fixture {
    alignas(4) unsigned char oa[0x900]{},oe[0x900]{},ow[0x520]{},a[0x900]{},e[0x900]{},w[0x520]{};
    alignas(4) unsigned char registry[48]{};
    std::uintptr_t owner_vt[0x320/4]{},enemy_vt[0x218/4]{},random_vt[2]{},random_object=0;
    sin_independent::EntityHandleEntry entries[3]{};
    float center[3]{},origin[3]{},velocity[3]{},forward[3]{1,0,0},roll=0;
    float future_origin[3]{},future_velocity[3]{};
    bool has_enemy=true,second=false;
    unsigned calls[5]{}; // velocity, random, second, refresh velocity, refresh origin
};
Fixture* current=nullptr;
void* __fastcall owner_self(void* a,void*) noexcept{return a;}
bool __fastcall owner_is_npc(void*,void*) noexcept{return true;}
void* __fastcall owner_enemy(void*,void*) noexcept{return current->has_enemy?current->oe:nullptr;}
void __fastcall target_velocity(void*,void*,float* linear,float* angular) noexcept{
    ++current->calls[0];std::memcpy(linear,current->velocity,12);std::memset(angular,0,12);
}
const float* __fastcall target_center(void*,void*) noexcept{return current->center;}
const float* __fastcall owner_center(void*,void*) noexcept{return current->origin;}
void __fastcall owner_forward(void*,void*,float* out) noexcept{std::memcpy(out,current->forward,12);}
float __fastcall random_float(void*,void*,float,float) noexcept{++current->calls[1];return current->roll;}
bool __fastcall target_second(void*,void*) noexcept{++current->calls[2];return current->second;}
void refresh_velocity(void* e) noexcept{
    ++current->calls[3];put(e,0xb8,read<unsigned>(e,0xb8)&~0x1000u);
    std::memcpy(static_cast<unsigned char*>(e)+0x250,current->future_velocity,12);
}
void refresh_origin(void* e) noexcept{
    ++current->calls[4];put(e,0xb8,read<unsigned>(e,0xb8)&~0x800u);
    if(e==current->oe||e==current->e)std::memcpy(static_cast<unsigned char*>(e)+0x2bc,current->future_origin,12);
    else {float zero[3]{};std::memcpy(static_cast<unsigned char*>(e)+0x2bc,zero,12);}
}
void __fastcall original_refresh_velocity(void* e,void*) noexcept{refresh_velocity(e);}
void __fastcall original_refresh_origin(void* e,void*) noexcept{refresh_origin(e);}
struct Slice{unsigned offset,size;};
constexpr Slice code[]={
    {0x0deb0,0x41},{0x12e50,0x41},{0x27c30,0x5b},{0x27db0,0x41},
    {0x139f60,0x13},{0x2680e0,7},{0x269af0,0x3d},{0x2ad370,0x39},{0x317300,0x248}
};
constexpr Slice data[]={
    {0x3d22f8,4},{0x3d2590,4},{0x3d8f08,8},{0x3d9788,4},{0x3da014,4},{0x40dca8,4},{0x51b1c4,4}
};
struct Original {
    unsigned char* image=nullptr;
    ~Original(){if(image)VirtualFree(image,0,MEM_RELEASE);}
    bool open(const char* path,Fixture& f){
        static_assert(sizeof(void*)==4,"original melee probe needs Win32");
        std::ifstream in(path,std::ios::binary|std::ios::ate);
        if(!in||in.tellg()!=6049792)return false;
        std::vector<unsigned char> bytes(6049792);in.seekg(0);
        if(!in.read(reinterpret_cast<char*>(bytes.data()),bytes.size())||
            sin_compat::phase100::sha256(bytes.data(),bytes.size())!=expected)return false;
        auto requested=reinterpret_cast<void*>(base);
        image=static_cast<unsigned char*>(VirtualAlloc(requested,0x628000,MEM_RESERVE,PAGE_NOACCESS));
        if(image!=requested)return false;
        constexpr unsigned exec_pages[]={0xd000,0x12000,0x27000,0x139000,0x13d000,0x13f000,0x268000,0x269000,0x2ad000,0x317000};
        constexpr unsigned data_pages[]={0x3d2000,0x3d8000,0x3d9000,0x3da000,0x40d000,0x4f5000,0x51b000,0x572000};
        for(unsigned page:exec_pages)if(VirtualAlloc(image+page,0x1000,MEM_COMMIT,PAGE_READWRITE)!=image+page)return false;
        for(unsigned page:data_pages)if(VirtualAlloc(image+page,0x1000,MEM_COMMIT,PAGE_READWRITE)!=image+page)return false;
        for(const auto& s:code)std::memcpy(image+s.offset,bytes.data()+s.offset,s.size);
        for(const auto& s:data)std::memcpy(image+s.offset,bytes.data()+s.offset,s.size);
        if(read<unsigned>(image,0x51b1c4)!=0x222680e0u)return false;
        auto jump=[&](unsigned offset,std::uintptr_t fn){image[offset]=0xb8;put(image,offset+1,fn);image[offset+5]=0xff;image[offset+6]=0xe0;};
        // Only dirty-transform refresh calls are substituted; original owner
        // resolution, vector arithmetic and all condition decisions execute.
        jump(0x13d750,reinterpret_cast<std::uintptr_t>(&original_refresh_velocity));
        jump(0x13fa60,reinterpret_cast<std::uintptr_t>(&original_refresh_origin));
        put(image,0x4f5620,reinterpret_cast<std::uintptr_t>(f.registry));
        put(image,0x5721b0,reinterpret_cast<std::uintptr_t>(&f.random_object));
        for(unsigned page:exec_pages){DWORD old=0;if(!VirtualProtect(image+page,0x1000,PAGE_EXECUTE_READ,&old)||!FlushInstructionCache(GetCurrentProcess(),image+page,0x1000))return false;}
        for(unsigned page:data_pages){DWORD old=0;if(!VirtualProtect(image+page,0x1000,PAGE_READONLY,&old))return false;}
        return true;
    }
    int query(void* w,float dot,float distance)const{
        using Fn=int (__thiscall *)(void*,float,float);
        return reinterpret_cast<Fn>(image+0x317300)(w,dot,distance);
    }
};
struct Precision{
    unsigned prior=0;bool captured=false,ready=false;
    Precision(){captured=_controlfp_s(&prior,0,0)==0;unsigned now=0;ready=captured&&_controlfp_s(&now,_PC_53|_RC_NEAR,_MCW_PC|_MCW_RC)==0&&(now&(_MCW_PC|_MCW_RC))==(_PC_53|_RC_NEAR);}
    ~Precision(){if(captured){unsigned unused=0;_controlfp_s(&unused,prior,_MCW_PC|_MCW_RC);}}
};
int __cdecl extent(void* p,void* entity,unsigned* n) noexcept{
    auto& f=*static_cast<Fixture*>(p);
    if(entity==f.a)*n=sizeof(f.a);else if(entity==f.e)*n=sizeof(f.e);else if(entity==f.w)*n=sizeof(f.w);else return 0;return 1;
}
bool run_matrix(const ServerNpcWeaponMeleeApi& api,Fixture& f,const Original* original){
    f.owner_vt[0xfc/4]=reinterpret_cast<std::uintptr_t>(&owner_self);f.owner_vt[0xf4/4]=reinterpret_cast<std::uintptr_t>(&owner_is_npc);
    f.owner_vt[0x154/4]=reinterpret_cast<std::uintptr_t>(&owner_enemy);f.owner_vt[0x214/4]=reinterpret_cast<std::uintptr_t>(&owner_center);
    f.owner_vt[0x31c/4]=reinterpret_cast<std::uintptr_t>(&owner_forward);
    f.enemy_vt[0x1e8/4]=reinterpret_cast<std::uintptr_t>(&target_velocity);f.enemy_vt[0x214/4]=reinterpret_cast<std::uintptr_t>(&target_center);
    f.enemy_vt[0x120/4]=reinterpret_cast<std::uintptr_t>(&target_second);
    f.random_vt[1]=reinterpret_cast<std::uintptr_t>(&random_float);f.random_object=reinterpret_cast<std::uintptr_t>(f.random_vt);
    put(f.oa,0,reinterpret_cast<std::uintptr_t>(f.owner_vt));put(f.oe,0,reinterpret_cast<std::uintptr_t>(f.enemy_vt));
    f.entries[1].serial=7;f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.a);
    f.entries[2].serial=8;f.entries[2].entity=reinterpret_cast<CBaseEntity*>(f.e);
    put(f.registry,20,reinterpret_cast<std::uintptr_t>(f.oa));put(f.registry,24,7u);
    put(f.registry,36,reinterpret_cast<std::uintptr_t>(f.oe));put(f.registry,40,8u);
    put(f.w,0,0x22478d0cu);put(f.w,0x494,0x7001u);put(f.ow,0x494,0x7001u);
    ServerNpcGruntAssaultAdmissionBindings ab{};ab.context=&f;ab.entries=f.entries;ab.count=3;ab.extent=extent;ab.actor_vtable=0x2249aaa4;
    ServerNpcWeaponMeleeBindings b{};b.context=&f;
    b.velocity_pair=[](void*,void*,float* v,float* a) noexcept -> int{target_velocity(nullptr,nullptr,v,a);return 1;};
    b.center=[](void* p,void* e,float* v) noexcept -> int{auto& f=*static_cast<Fixture*>(p);std::memcpy(v,e==f.a?f.origin:f.center,12);return 1;};
    b.forward=[](void*,void*,float* v) noexcept -> int{owner_forward(nullptr,nullptr,v);return 1;};
    b.random=[](void*,float lo,float hi,float* v) noexcept -> int{*v=random_float(nullptr,nullptr,lo,hi);return 1;};
    b.second_prediction=[](void*,void*,bool* yes) noexcept -> int{*yes=target_second(nullptr,nullptr);return 1;};
    b.refresh_velocity=[](void*,void* e) noexcept -> int{refresh_velocity(e);return 1;};
    b.refresh_origin=[](void*,void* e) noexcept -> int{refresh_origin(e);return 1;};
    unsigned cases=0,conditions[3]{};
    auto one=[&](float dot,float distance,unsigned mode)->bool{
        const unsigned enemy_handle=f.has_enemy?0x8002u:0xffffffffu;
        put(f.a,0x8a0,enemy_handle);put(f.oa,0x8a0,enemy_handle);
        for(auto* e:{f.e,f.oe}){
            put(e,0xb8,mode==2?0x1800u:0u);std::memcpy(e+0x2bc,f.future_origin,12);std::memcpy(e+0x250,f.future_velocity,12);
        }
        for(auto* w:{f.w,f.ow}){put(w,0xb8,mode==2?0x800u:0u);std::memset(w+0x2bc,0,12);}
        f.second=mode!=0;
        std::memset(f.calls,0,sizeof(f.calls));ServerNpcWeaponMeleeResult result{};
        if(!api.query(f.a,sizeof(f.a),f.w,dot,distance,&ab,&b,&result)||!result.completed)return false;
        unsigned native_calls[5];std::memcpy(native_calls,f.calls,sizeof(native_calls));
        if(!f.has_enemy){if(result.condition||native_calls[0]||native_calls[1])return false;}
        else {
            if(result.condition==0x16)++conditions[0];else if(result.condition==0x25)++conditions[1];else if(result.condition==0x26)++conditions[2];else return false;
            if(native_calls[0]!=1||native_calls[1]!=1||native_calls[2]!=(result.second_called?1u:0u))return false;
        }
        if(original){
            std::memset(f.calls,0,sizeof(f.calls));const int expected_condition=original->query(f.ow,dot,distance);
            if(expected_condition!=result.condition||std::memcmp(native_calls,f.calls,sizeof(f.calls))){
                std::fprintf(stderr,"melee mismatch case=%u x=%g z=%g vx=%g roll=%g dot=%g dist=%g mode=%u original=%d native=%d\n",cases,f.center[0],f.center[2],f.velocity[0],f.roll,dot,distance,mode,expected_condition,result.condition);
                return false;
            }
        }
        ++cases;return true;
    };
    f.has_enemy=false;if(!one(1.f,48.f,0))return false;f.has_enemy=true;
    for(float x:{-60.f,-1.f,0.f,.69f,.7f,47.99f,48.f,48.01f,80.f})
    for(float z:{0.f,70.f,70.01f})for(float vx:{-10.f,0.f,10.f})for(float roll:{-.3f,-.1f,0.f,.2f})
    for(float dot:{.69f,.7f,1.f})for(float distance:{47.f,48.f,49.f})for(unsigned mode:{0u,1u,2u}){
        f.center[0]=x;f.center[2]=z;f.velocity[0]=vx;f.roll=roll;
        f.future_origin[0]=x;f.future_velocity[0]=vx;
        if(!one(dot,distance,mode))return false;
    }
    if(!conditions[0]||!conditions[1]||!conditions[2])return false;
    std::printf("melee matrix %u cases ready=%u too_far=%u facing=%u (%s)\n",cases,conditions[0],conditions[1],conditions[2],original?"native + original PE32":"native only");
    return true;
}
}
int main(int argc,char** argv){
    if(argc!=3){std::fputs("Usage: sin_server_npc_weapon_melee_audit --self-test <native server.dll> OR <native server.dll> <original server.dll>\n",stderr);return 2;}
    const bool self=!std::strcmp(argv[1],"--self-test");Precision precision;if(!precision.ready)return 2;
    Fixture f;current=&f;Original original;
    if(!self&&!original.open(argv[2],f)){std::fputs("Original hash/preferred mapping failed\n",stderr);return 2;}
    HMODULE dll=LoadLibraryA(self?argv[2]:argv[1]);if(!dll)return 2;
    auto getter=reinterpret_cast<GetServerNpcWeaponMeleeApi>(GetProcAddress(dll,server_npc_weapon_melee_export));
    const auto* api=getter?getter(1,sizeof(ServerNpcWeaponMeleeApi)):nullptr;
    const bool ok=api&&run_matrix(*api,f,self?nullptr:&original);FreeLibrary(dll);return ok?0:1;
}
