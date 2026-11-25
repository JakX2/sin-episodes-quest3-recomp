#pragma once
// Owner-only optional differential probe. Does not LoadLibrary the retail
// module, run its initializers, resolve imports or ship any original bytes.
#include "native_server_flex_spline.h"
#include "native_server_animation_scene.h"
#include "../../include/sin_compat/phase100_sha256.hpp"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <vector>
namespace sin_port::native_weapon_tables::test {
class OriginalFlexSplineImage {
    unsigned char* base_=nullptr;
public:
    OriginalFlexSplineImage()=default;
    OriginalFlexSplineImage(const OriginalFlexSplineImage&)=delete;
    OriginalFlexSplineImage& operator=(const OriginalFlexSplineImage&)=delete;
    ~OriginalFlexSplineImage(){if(base_)VirtualFree(base_,0,MEM_RELEASE);}
    bool open(const char* filename){
        static_assert(sizeof(void*)==4,"original spline probe requires Win32");
        constexpr sin_compat::phase100::Digest256 expected{{
            0x56,0x8a,0x00,0x86,0xfd,0xe6,0x07,0x34,0x5a,0xdf,0x31,0xd7,0xcf,0xb2,0xa5,0xbf,
            0x4e,0x4e,0xfe,0x9a,0x86,0xf4,0x16,0xfa,0xa9,0x70,0x76,0xdd,0xf9,0xd9,0x76,0x50}};
        std::ifstream in(filename,std::ios::binary|std::ios::ate);
        if(!in||in.tellg()!=6049792){std::fprintf(stderr,"ORIGINAL SPLINE: wrong file length\n");return false;}
        std::vector<unsigned char> bytes(6049792);in.seekg(0);
        if(!in.read(reinterpret_cast<char*>(bytes.data()),bytes.size())||
           sin_compat::phase100::sha256(bytes.data(),bytes.size())!=expected){
            std::fprintf(stderr,"ORIGINAL SPLINE: original server SHA256 mismatch\n");return false;
        }
        // Section addresses/sizes are valid only for that exact authenticated
        // hash. Copy .text/.rdata/.data; zero-fill virtual .data tail. No
        // relocation, imports, TLS, DllMain, or arbitrary entrypoint.
        auto* requested=reinterpret_cast<void*>(0x22000000u);
        base_=static_cast<unsigned char*>(VirtualAlloc(requested,0x628000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        if(base_!=requested){std::fprintf(stderr,"ORIGINAL SPLINE: preferred base unavailable\n");return false;}
        std::memcpy(base_+0x1000,bytes.data()+0x1000,0x3d011a);
        std::memcpy(base_+0x3d2000,bytes.data()+0x3d2000,0x103626);
        std::memcpy(base_+0x4d6000,bytes.data()+0x4d6000,0x68000);
        DWORD old=0;
        if(!VirtualProtect(base_+0x1000,0x3d1000,PAGE_EXECUTE_READ,&old)||
           !VirtualProtect(base_+0x3d2000,0x104000,PAGE_READONLY,&old)||
           !FlushInstructionCache(GetCurrentProcess(),base_+0x1000,0x3d1000)){
            std::fprintf(stderr,"ORIGINAL SPLINE: image protection failed\n");return false;
        }
        return true;
    }
    // Fixed authenticated leaves only. The original selector can call the
    // engine allocator if fewer than two ordinary free rows exist. The
    // probe below verifies this precondition before invoking it.
    int layer_select(void* actor,int priority,float now) const noexcept{
        alignas(4) unsigned char globals[0x30]{};std::memcpy(globals+0xc,&now,4);
        void* old=nullptr;void* current=globals;
        std::memcpy(&old,base_+0x572190,4);std::memcpy(base_+0x572190,&current,4);
        using Fn=int (__thiscall *)(void*,int);
        const int result=reinterpret_cast<Fn>(base_+0xf4e20)(actor,priority);
        std::memcpy(base_+0x572190,&old,4);return result;
    }
    void layer_remove(void* actor,int layer,float now) const noexcept{
        alignas(4) unsigned char globals[0x30]{};std::memcpy(globals+0xc,&now,4);
        void* old=nullptr;void* current=globals;
        std::memcpy(&old,base_+0x572190,4);std::memcpy(base_+0x572190,&current,4);
        using Fn=void (__thiscall *)(void*,int);
        reinterpret_cast<Fn>(base_+0xf4060)(actor,layer);
        std::memcpy(base_+0x572190,&old,4);
    }
    void layer_blend(void* actor,int layer,float duration,float delay) const noexcept{
        using Fn=void (__thiscall *)(void*,int,float,float);
        reinterpret_cast<Fn>(base_+0xf3ff0)(actor,layer,duration,delay);
    }
    float sample(const ServerFlexSample* rows,unsigned n,float low,float high,float duration,float time,unsigned bank) const noexcept{
        alignas(4) unsigned char event[0x3c0]{},track[0x50]{};
        auto put=[](void* p,unsigned off,auto v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));};
        put(event,0x188,duration);put(track,0xc,low);put(track,0x10,high);
        put(track,0x4c,event+0);
        put(track,0x14+bank*0x14,rows);put(track,0x20+bank*0x14,n);
        // 2210d970 thiscall: ECX track, stack relative time then bank.
        // Its only call chain is internal 2210d890, 2226b720, 2226b210;
        // no imported engine service or math callback is invoked.
        using Original=float (__thiscall *)(void*,float,int);
        const auto fn=reinterpret_cast<Original>(base_+0x10d970);
        return fn(track,time,static_cast<int>(bank));
    }
};
inline bool audit_original_flex_spline(const char* filename,const ServerFlexSplineApi& native){
    OriginalFlexSplineImage original;
    if(!original.open(filename))return false;
    const ServerFlexSample uneven[]={{.2f,.2f,0},{.8f,.6f,123},{.2f,.8f,0}};
    const ServerFlexSample duplicate[]={{1,0,0},{.2f,.5f,0},{.9f,.5f,0}};
    const ServerFlexSample overshoot[]={{1.4f,.15f,0},{-.4f,.7f,0},{.6f,1,0}};
    struct Case {const ServerFlexSample* rows;unsigned n;float lo,hi;};
    const Case cases[]={{uneven,3,0,1},{uneven,3,-2,3},{uneven,3,3,-2},
        {duplicate,3,0,1},{overshoot,3,0,1},{nullptr,0,1,2},{nullptr,0,2,2}};
    unsigned compared=0,failures=0;float max_error=0;
    for(unsigned c=0;c<sizeof(cases)/sizeof(*cases);++c)for(unsigned bank=0;bank<2;++bank){
        const auto& q=cases[c];
        for(unsigned i=0;i<=40;++i){
            const float time=static_cast<float>(i)/40;
            const float expected=original.sample(q.rows,q.n,q.lo,q.hi,1,time,bank);
            float actual=0;
            if(!native.sample(q.rows,q.n,q.lo,q.hi,1,time,bank,&actual)||!std::isfinite(actual)||!std::isfinite(expected)){
                ++failures;continue;
            }
            ++compared;
            const float error=std::fabs(actual-expected);
            if(error>max_error)max_error=error;
            if(error>0.00002f){
                if(failures<20)std::fprintf(stderr,"ORIGINAL SPLINE mismatch case=%u bank=%u time=%.9g original=%.9g native=%.9g\n",c,bank,time,expected,actual);
                ++failures;
            }
        }
    }
    std::printf("ORIGINAL SPLINE AUDIT compared=%u failures=%u max_error=%.9g\n",compared,failures,max_error);
    return !failures&&compared==574;
}

inline bool audit_original_animation_layers(const char* filename,const ServerAnimationLayerOwnerApi& native){
    OriginalFlexSplineImage original;if(!original.open(filename))return false;
    struct Fixture{
        alignas(4) unsigned char actor[0x4a8]{},rows[6*0x40]{};
        unsigned char* row(int i){return rows+i*0x40;}
        static std::uint32_t word(const void* p,unsigned off){std::uint32_t w;std::memcpy(&w,static_cast<const unsigned char*>(p)+off,4);return w;}
        static void put(void* p,unsigned off,std::uint32_t v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,4);}
        static void fl(void* p,unsigned off,float v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,4);}
        void init(int variant){
            std::memset(actor,0,sizeof(actor));std::memset(rows,0,sizeof(rows));
            const auto address=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(rows));
            put(actor,0x494,address);put(actor,0x498,6);put(actor,0x4a0,6);put(actor,0x4a4,address);
            for(int i=0;i<6;++i){put(row(i),0x34,15);fl(row(i),0x14,1.f);fl(row(i),0x24,100.f);}
            for(int i=0;i<3;++i){
                put(row(i),0,1);put(row(i),8,7+i);put(row(i),0x30,static_cast<std::uint32_t>(i*2-2));
                put(row(i),0x34,i);fl(row(i),0x18,.25f*(i+1));row(i)[5]=static_cast<unsigned char>(i&1);
                put(row(i),0x38,0x12340000u+i);
            }
            if(variant&1)put(row(3),0,0x20);
            if(variant&2){put(row(1),0,0);put(row(1),0x34,1);}
        }
        unsigned free_rows(){
            unsigned count=0;for(int i=0;i<6;++i)if(!(word(row(i),0)&0x21))++count;return count;
        }
    } old,copy;
    ChoreographyVectorBindings b{};b.context=&copy;
    b.storage=[](void* p,std::uint32_t w,void** out,unsigned* extent) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);
        if(w!=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(f.rows)))return 0;
        *out=f.rows;*extent=sizeof(f.rows);return 1;};
    b.resize=[](void*,std::uint32_t,unsigned,void**,unsigned*) noexcept -> int{return 0;};
    unsigned compared=0;
    for(int variant=0;variant<4;++variant)for(int layer=-1;layer<=6;++layer){
        for(float duration:{0.f,.125f,.5f,1.f,-1.f}){
            old.init(variant);copy.init(variant);
            original.layer_blend(old.actor,layer,duration,.25f);
            if(!native.blend_out(copy.actor,sizeof(copy.actor),layer,duration,.25f,&b)||
               std::memcmp(old.rows,copy.rows,sizeof(old.rows)))return false;
            ++compared;
        }
        old.init(variant);copy.init(variant);
        original.layer_remove(old.actor,layer,17.f);
        if(!native.remove(copy.actor,sizeof(copy.actor),layer,17.f,&b)||
           std::memcmp(old.rows,copy.rows,sizeof(old.rows)))return false;
        ++compared;
    }
    for(int variant=0;variant<4;++variant)for(int priority=-3;priority<=4;++priority){
        old.init(variant);copy.init(variant);
        // No path may reach 220f41c0 (the original raw allocator).
        if(old.free_rows()<2||copy.free_rows()<2)return false;
        const int selected=original.layer_select(old.actor,priority,19.f);int actual=-1;
        if(!native.allocate(copy.actor,sizeof(copy.actor),12,priority,19.f,0,&b,&actual)||
           actual!=selected||selected<0||selected>=6||Fixture::word(copy.actor,0x4a0)!=6)return false;
        for(int i=0;i<6;++i){
            if(i==selected){
                // Original 220f4e20 ends before 220f5120 initialization.
                for(unsigned off:{0u,0x30u,0x34u,0x3cu})
                    if(Fixture::word(old.row(i),off)!=Fixture::word(copy.row(i),off))return false;
            }else if(std::memcmp(old.row(i),copy.row(i),0x40))return false;
        }
        ++compared;
    }
    std::printf("ORIGINAL ANIMATION LAYER AUDIT compared=%u failures=0\n",compared);
    return compared==224;
}

}
