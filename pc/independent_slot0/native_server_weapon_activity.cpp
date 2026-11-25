#include "native_server_weapon_activity.h"
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
constexpr ServerWeaponActivityRow assault[]{
    {16,309,true}, {66,398,true}, {1,345,true}, {72,346,true},
    {6,382,true}, {7,383,true}, {195,201,true}, {196,202,true},
    {197,203,true}, {198,204,true}, {199,205,true}, {200,206,true},
    {208,219,true}, {209,220,true}, {210,221,true}, {211,222,true},
    {212,223,true}, {213,224,true}, {214,225,true}, {215,226,true},
    {216,227,true}, {217,228,true}, {218,229,true}, {230,236,true},
    {231,237,true}, {232,238,true}, {233,239,true}, {234,240,true},
    {235,241,true}, {73,354,false}, {74,355,false}, {75,346,false},
    {76,356,false}, {77,358,false}, {78,383,false}, {79,357,false},
    {80,359,false}, {81,387,false}, {82,354,false}, {83,360,false},
    {84,346,false}, {85,356,false}, {86,361,false}, {87,383,false},
    {88,357,false}, {89,362,false}, {90,387,false}, {7,383,true},
    {8,384,true}, {9,385,true}, {10,386,true}, {11,387,true},
    {12,388,true}, {13,389,true}, {123,333,true}, {18,310,true},
    {5,327,false}, {322,323,false}, {67,399,false}, {400,402,true}
};
constexpr ServerWeaponActivityRow chaingun[]{
    {1,1,true}, {16,312,true}, {66,398,true}, {6,382,true},
    {72,349,true}, {195,201,true}, {196,202,true}, {197,203,true},
    {198,204,true}, {199,205,true}, {200,206,true}, {230,248,true},
    {231,249,true}, {232,250,true}, {233,251,true}, {234,252,true},
    {235,253,true}, {73,363,false}, {74,364,false}, {75,364,false},
    {76,356,false}, {77,358,false}, {78,383,false}, {79,357,false},
    {80,359,false}, {81,387,false}, {82,354,false}, {83,360,false},
    {84,346,false}, {85,356,false}, {86,361,false}, {87,383,false},
    {88,357,false}, {89,362,false}, {90,387,false}, {7,390,true},
    {8,384,true}, {9,385,true}, {10,386,true}, {11,391,true},
    {12,388,true}, {13,389,true}, {123,336,true}, {18,313,true},
    {67,399,true}, {400,402,true}
};
constexpr ServerWeaponActivityRow grenade[]{
    {16,316,true}
};
constexpr ServerWeaponActivityRow magnum[]{
    {1,347,true}, {72,348,true}, {16,314,true}, {66,396,true},
    {7,392,true}, {6,394,true}, {382,394,true}, {11,393,true},
    {10,395,true}, {123,337,true}, {195,201,true}, {196,202,true},
    {197,203,true}, {198,204,true}, {199,205,true}, {200,206,true},
    {64,207,true}, {67,397,false}, {18,315,false}, {5,326,false},
    {322,324,false}, {400,401,false}, {230,242,true}, {231,243,true},
    {232,244,true}, {233,245,true}, {234,246,true}, {235,247,true}
};
constexpr ServerWeaponActivityRow melee[]{
    {64,64,true}, {72,72,true}
};
constexpr ServerWeaponActivityRow scatter[]{
    {1,345,true}, {16,312,true}, {66,398,true}, {6,382,true},
    {72,349,true}, {195,201,true}, {196,202,true}, {197,203,true},
    {198,204,true}, {199,205,true}, {200,206,true}, {230,248,true},
    {231,249,true}, {232,250,true}, {233,251,true}, {234,252,true},
    {235,253,true}, {73,363,false}, {74,364,false}, {75,364,false},
    {76,356,false}, {77,358,false}, {78,383,false}, {79,357,false},
    {80,359,false}, {81,387,false}, {82,354,false}, {83,360,false},
    {84,346,false}, {85,356,false}, {86,361,false}, {87,383,false},
    {88,357,false}, {89,362,false}, {90,387,false}, {7,390,true},
    {8,384,true}, {9,385,true}, {10,386,true}, {11,391,true},
    {12,388,true}, {13,389,true}, {123,336,true}, {18,313,true},
    {67,399,true}, {400,402,true}
};
constexpr ServerWeaponActivityRow smg[]{
    {16,309,true}, {66,398,true}, {1,345,true}, {72,346,true},
    {6,382,true}, {7,383,true}, {73,354,false}, {74,355,false},
    {75,346,false}, {76,356,false}, {77,358,false}, {78,383,false},
    {79,357,false}, {80,359,false}, {81,387,false}, {82,354,false},
    {83,360,false}, {84,346,false}, {85,356,false}, {86,361,false},
    {87,383,false}, {88,357,false}, {89,362,false}, {90,387,false},
    {7,383,true}, {8,384,true}, {9,385,true}, {10,386,true},
    {11,387,true}, {12,388,true}, {13,389,true}, {123,333,true},
    {18,310,true}, {5,327,false}, {322,323,false}, {67,399,false},
    {400,402,true}
};
template<unsigned N> void expose(const ServerWeaponActivityRow (&rows)[N],const ServerWeaponActivityRow** out,unsigned* n) noexcept {*out=rows;*n=N;}
int __cdecl table(unsigned kind,const ServerWeaponActivityRow** out,unsigned* count) noexcept {
    if(!out||!count)return 0;*out=nullptr;*count=0;
    switch(kind){
    case WeaponActivityBase:return 1;
    case WeaponActivityAssault:expose(assault,out,count);return 1;
    case WeaponActivityChaingun:expose(chaingun,out,count);return 1;
    case WeaponActivityGrenade:expose(grenade,out,count);return 1;
    case WeaponActivityMagnum:case WeaponActivityNpcPistol:expose(magnum,out,count);return 1;
    case WeaponActivityMelee:expose(melee,out,count);return 1;
    case WeaponActivityScatter:expose(scatter,out,count);return 1;
    case WeaponActivitySmg:expose(smg,out,count);return 1;
    default:return 0;
    }
}
int __cdecl translate(unsigned kind,int input,bool* required,int* out) noexcept {
    if(!out)return 0;*out=input;
    const ServerWeaponActivityRow* rows=nullptr;unsigned count=0;
    if(!table(kind,&rows,&count))return 0;
    for(unsigned i=0;i<count;++i)if(rows[i].input==input){
        *out=rows[i].output;if(required)*required=rows[i].required;break;
    }
    return 1;
}
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
bool valid(const ServerWeaponActivityBindings* b) noexcept {
    return b&&b->count<=sin_independent::kEntityHandleCapacity&&(!b->count||b->entries);
}
void* active(void* a,const ServerWeaponActivityBindings& b) noexcept {
    return sin_independent::EntityHandleView(b.entries,b.count).Resolve(read<std::uint32_t>(a,0x78c));
}
int __cdecl actor_translate(void* a,unsigned bytes,int input,const ServerWeaponActivityBindings* b,bool* required,int* out) noexcept {
    if(!out)return 0;*out=input;
    if(!a||bytes<0x790||!valid(b))return 0;
    void* weapon=active(a,*b);
    if(!weapon){if(required)*required=false;return 1;}
    unsigned kind=0;
    if(!b->classify||!b->classify(b->context,weapon,&kind))return 0;
    return translate(kind,input,required,out);
}
int __cdecl capabilities(void* a,unsigned bytes,const ServerWeaponActivityBindings* b,unsigned* out) noexcept {
    if(!out)return 0;*out=0;
    if(!a||bytes<0x8f0||!valid(b))return 0;
    *out=read<unsigned>(a,0x8ec);
    void* weapon=active(a,*b);
    if(!weapon)return 1;
    unsigned kind=0,flags=0;
    if(!b->classify||!b->classify(b->context,weapon,&kind)||kind>WeaponActivitySmg)return 0;
    if(kind==WeaponActivityBase){
        if(!b->base_capabilities||!b->base_capabilities(b->context,weapon,&flags))return 0;
    }else flags=kind==WeaponActivityMelee?0x8000u:0x2000u;
    *out|=flags;return 1;
}
const ServerWeaponActivityApi api{1,sizeof(ServerWeaponActivityApi),&table,&translate,&actor_translate,&capabilities};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerWeaponActivityApi=_SiN_GetServerWeaponActivityApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponActivityApi* __cdecl
SiN_GetServerWeaponActivityApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerWeaponActivityApi)?&api:nullptr;
}
