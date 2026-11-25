#include "native_server_grunt_animation.h"
#include <cmath>
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T> void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool extend(void* actor,double until) noexcept {
    if(until>read<float>(actor,0xc90)){write(actor,0xc90,static_cast<float>(until));return true;}
    return false;
}
int __cdecl handle(void* grunt,unsigned bytes,const ServerAnimationEvent* e,float now,const ServerGruntAnimationBindings* b,ServerGruntAnimationResult* out) noexcept {
    if(!out)return -1;
    *out={};
    if(!grunt||bytes<0xfcc||!e||!b||!std::isfinite(now))return -1;
    const int id=e->event;
    // Fixed event 0x14 precedes both dynamic IDs.
    if(id==0x14){
        out->route=GruntAnimBlocking;if(!b->debug)return -1;
        b->debug(b->context,"BLOCKING!\n");write<std::uint8_t>(grunt,0xc35,1);return 1;
    }
    if(id==b->begin_altfire){out->route=GruntAnimBeginAlt;return 1;}
    if(id==b->altfire){
        out->route=GruntAnimAlt;
        if(!b->weapon||!b->weapon_event)return -1;
        void* weapon=nullptr;
        if(!b->weapon(b->context,grunt,read<std::uint32_t>(grunt,0x78c),&weapon)||!weapon)return -1;
        const double until=static_cast<double>(now)+10.;
        if(!std::isfinite(static_cast<float>(until))||!std::isfinite(read<float>(grunt,0xc90)))return -1;
        const auto squad=read<std::uint32_t>(grunt,0x9c4);
        void** peers=nullptr;unsigned count=0;
        if(squad&&(!b->peers||!b->peers(b->context,grunt,squad,&peers,&count)||count>4096||(count&&!peers)))return -1;
        for(unsigned i=0;i<count;++i)if(!peers[i]||!std::isfinite(read<float>(peers[i],0xc90)))return -1;
        // 2235e90a: weapon +4c0 first, then 22358200 deadline,
        // then raw +c38 decrement. Never updates +fc8 on this branch.
        b->weapon_event(b->context,weapon,0xc1f,grunt);
        extend(grunt,until);
        for(unsigned i=0;i<count;++i)if(extend(peers[i],until))++out->peers_updated;
        write(grunt,0xc38,read<std::uint32_t>(grunt,0xc38)-1u);
        return 1;
    }
    switch(id){
    case 2:{
        out->route=GruntAnimWeaponReset;
        if(!b->weapon||!b->clear_condition)return -1;
        void* weapon=nullptr;
        if(!b->weapon(b->context,grunt,read<std::uint32_t>(grunt,0x78c),&weapon))return -1;
        if(weapon){
            if(!b->weapon_mode||!b->weapon_value||!b->weapon_store)return -1;
            b->weapon_mode(b->context,weapon,9,0);
            const int a=b->weapon_value(b->context,weapon,0x404);
            b->weapon_store(b->context,weapon,0x4ec,a);
            const int c=b->weapon_value(b->context,weapon,0x408);
            b->weapon_store(b->context,weapon,0x4f0,c);
        }
        for(int cond=3;cond<=5;++cond)b->clear_condition(b->context,grunt,cond);
        break;
    }
    case 4:case 8:out->route=GruntAnimMark;break;
    case 10:
        out->route=GruntAnimActivity;
        if(!b->activity||!b->activity_request)return -1;
        {const int a=b->activity(b->context,grunt);b->activity_request(b->context,grunt,a,0,2);}
        break;
    // Dedicated object-interaction bodies, not the shared parent route.
    case 3:case 7:case 9:out->route=GruntAnimUnrecovered;return 0;
    default:
        out->route=GruntAnimParent;if(!b->parent)return -1;
        b->parent(b->context,grunt,e);return 1;
    }
    write(grunt,0xfc8,id);return 1;
}
const ServerGruntAnimationApi api{1,sizeof(ServerGruntAnimationApi),&handle};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerGruntAnimationApi=_SiN_GetServerGruntAnimationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntAnimationApi* __cdecl
SiN_GetServerGruntAnimationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerGruntAnimationApi)?&api:nullptr;
}
