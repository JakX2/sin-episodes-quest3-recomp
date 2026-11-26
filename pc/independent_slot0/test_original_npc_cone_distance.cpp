// Owner-only optional original PE32 differential; no retail initializers, engine
// imports, relocations or DLL entrypoints. Only five authenticated code slices.
#include "native_server_npc_cone_distance_math.h"
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
using sin_port::cone_distance_math::cone_line_distance;
namespace {
constexpr sin_compat::phase100::Digest256 expected{{
    0x56,0x8a,0x00,0x86,0xfd,0xe6,0x07,0x34,0x5a,0xdf,0x31,0xd7,0xcf,0xb2,0xa5,0xbf,
    0x4e,0x4e,0xfe,0x9a,0x86,0xf4,0x16,0xfa,0xa9,0x70,0x76,0xdd,0xf9,0xd9,0x76,0x50}};
constexpr unsigned base_address=0x22000000u;
struct Slice {unsigned offset,size;};
constexpr Slice code[]={{0x2680e0,7},{0x26bd50,0x88},{0x26be40,0x8e},{0x26bfc0,0x62},{0x26c080,0x7c}};
constexpr Slice data[]={{0x3d2590,4},{0x449d3c,4},{0x51b1c4,4}};
struct Original {
    unsigned char* image=nullptr;
    ~Original(){if(image)VirtualFree(image,0,MEM_RELEASE);}
    bool open(const char* path){
        static_assert(sizeof(void*)==4,"original cone probe needs Win32");
        std::ifstream in(path,std::ios::binary|std::ios::ate);
        if(!in||in.tellg()!=6049792)return false;
        std::vector<unsigned char> bytes(6049792);in.seekg(0);
        if(!in.read(reinterpret_cast<char*>(bytes.data()),bytes.size())||
            sin_compat::phase100::sha256(bytes.data(),bytes.size())!=expected)return false;
        // Exact known PE: all of the selected RVAs equal their raw file offsets.
        auto requested=reinterpret_cast<void*>(base_address);
        image=static_cast<unsigned char*>(VirtualAlloc(requested,0x628000,MEM_RESERVE,PAGE_NOACCESS));
        if(image!=requested)return false;
        constexpr unsigned pages[]={0x268000,0x26b000,0x26c000,0x3d2000,0x449000,0x51b000};
        for(unsigned page:pages)if(VirtualAlloc(image+page,0x1000,MEM_COMMIT,PAGE_READWRITE)!=image+page)return false;
        for(const auto& s:code)std::memcpy(image+s.offset,bytes.data()+s.offset,s.size);
        for(const auto& s:data)std::memcpy(image+s.offset,bytes.data()+s.offset,s.size);
        // This is the original on-disk default sqrt pointer, not an import.
        unsigned target=0;std::memcpy(&target,image+0x51b1c4,4);
        if(target!=0x222680e0u)return false;
        for(unsigned i=0;i<6;++i){DWORD old=0;
            if(!VirtualProtect(image+pages[i],0x1000,i<3?PAGE_EXECUTE_READ:PAGE_READONLY,&old))return false;
            if(i<3&&!FlushInstructionCache(GetCurrentProcess(),image+pages[i],0x1000))return false;
        }
        return true;
    }
    float distance(const float* p,const float* s,const float* e,bool horizontal) const {
        using Fn=float (__cdecl *)(const float*,const float*,const float*,float*);
        return reinterpret_cast<Fn>(image+(horizontal?0x26c080:0x26be40))(p,s,e,nullptr);
    }
};
struct Precision {
    unsigned prior=0;bool captured=false,ready=false;
    Precision(){
        captured=_controlfp_s(&prior,0,0)==0;
        unsigned current=0;
        ready=captured&&_controlfp_s(&current,_PC_53|_RC_NEAR,_MCW_PC|_MCW_RC)==0&&
            (current&(_MCW_PC|_MCW_RC))==(_PC_53|_RC_NEAR);
    }
    ~Precision(){if(captured){unsigned unused=0;_controlfp_s(&unused,prior,_MCW_PC|_MCW_RC);}}
};
std::uint32_t bits(float f){std::uint32_t u;std::memcpy(&u,&f,4);return u;}
std::uint32_t rng=0x71e66a99u;
float randf(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (static_cast<int>(rng&0xffff)-32768)*.03125f;}
bool self_test(){
    const std::uint32_t words[9]{0x48cc0866u,0x4b5ab3f5u,0x4b6af182u,0x494c0000u,0x4bdab400u,0x4beaf000u,0x43066000u,0xc1a88000u,0x44410600u};
    float v[9],result=0;std::memcpy(v,words,sizeof(v));
    return cone_line_distance(v,v+3,v+6,false,result)&&bits(result)==0x3cbf83d7u;
}
}
int main(int argc,char** argv){
    Precision pc;if(!pc.ready){std::fputs("Cannot establish 53-bit nearest x87 precision\n",stderr);return 2;}
    if(!self_test()){std::fputs("Native distance witness failed\n",stderr);return 1;}
    if(argc==2&&!std::strcmp(argv[1],"--self-test"))return 0;
    if(argc!=2){std::fputs("Usage: sin_server_npc_cone_distance_audit <original server.dll>\n",stderr);return 2;}
    Original original;
    if(!original.open(argv[1])){std::fputs("Original hash, preferred address, mapping or default sqrt pointer failed\n",stderr);return 2;}
    unsigned failures[2]{};
    for(unsigned i=0;i<800000;++i){
        float p[3],s[3],e[3];
        for(unsigned j=0;j<3;++j){
            s[j]=randf();e[j]=randf();p[j]=randf();
            switch(i/100000){
            case 1:s[j]*=32768.f;e[j]=s[j]+randf();p[j]=s[j]+randf();break;
            case 2:s[j]*=.0001f;e[j]=s[j]+randf()*.000001f;p[j]=s[j]+randf()*.0001f;break;
            case 3:s[j]*=32768.f;e[j]=randf();p[j]=randf();break;
            case 4:s[j]*=32768.f;e[j]=randf();p[j]=s[j]+(e[j]-s[j])*.5f+randf()*.0001f;break;
            case 5:s[j]*=32768.f;e[j]=s[j]+randf();p[j]=s[j]+(e[j]-s[j])*.5f+randf()*.0001f;break;
            case 6:s[j]*=32768.f;e[j]=randf();p[j]=s[j]+(e[j]-s[j])*.25f+randf()*.001f;break;
            case 7:s[j]*=1024.f;e[j]=s[j]+randf();p[j]=s[j]+(e[j]-s[j])*.25f+randf()*.001f;break;
            }
        }
        for(unsigned h=0;h<2;++h){
            const float reference=original.distance(p,s,e,h!=0);float actual=0;
            if(!cone_line_distance(p,s,e,h!=0,actual)||bits(actual)!=bits(reference)){
                if(failures[h]<8)std::fprintf(stderr,"case=%u dimension=%u original=%08x native=%08x\n",i,h?2:3,bits(reference),bits(actual));
                ++failures[h];
            }
        }
    }
    std::printf("ORIGINAL CONE DISTANCE: 800000 cases per dimension; 3D mismatches=%u, 2D mismatches=%u\n",failures[0],failures[1]);
    return failures[0]||failures[1]?1:0;
}
