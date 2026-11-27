#include "native_server_recombinant_spit.h"
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorOriginPointApi* __cdecl SiN_GetServerRecombinantMinorOriginPointApi(unsigned,unsigned) noexcept;
#include <cmath>
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool finite(const float* v) noexcept{return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
int __cdecl execute(void* a,unsigned n,float now,const ServerRecombinantSpitBindings* b,ServerRecombinantSpitResult* o) noexcept{
    if(!o)return 0;*o={};
    if(!a||n<0xc50||rd<unsigned>(a,0)!=recombinant_minor_vtable||!b||!b->event||!std::isfinite(now))return 0;
    const auto& e=*b->event;void* target=nullptr;
    auto enemy=[&](){++o->enemy_queries;return e.enemy&&e.enemy(e.context,a,&target);};
    if(!enemy())return 0;if(!target){o->completed=1;return 1;}o->enemy_live=1;
    auto& q=o->request;
    ++o->attachment_calls;
    if(!b->attachment||!b->attachment(b->context,a,"Acid_spit",q.muzzle)||!finite(q.muzzle)||!enemy()||!target)return 0;
    if(rd<unsigned>(a,0xb8)&0x800u){
        ++o->origin_refreshes;if(!b->refresh_origin||!b->refresh_origin(b->context,a))return 0;
    }
    float origin[3]{};std::memcpy(origin,static_cast<const unsigned char*>(a)+0x2bc,12);
    ++o->aim_calls;
    if(!finite(origin)||!b->aim_point||!b->aim_point(b->context,target,origin,1,o->aim)||!finite(o->aim))return 0;
    for(unsigned i=0;i<3;++i)o->direction[i]=static_cast<float>(double(o->aim[i])-q.muzzle[i]);
    if(!std::isfinite(b->gravity)||!std::isfinite(b->damage)||!enemy()||!target||!e.target_point||!e.target_point(e.context,target,o->target)||!finite(o->target))return 0;
    o->target[2]=static_cast<float>(double(o->target[2])-10.);
    o->gravity=b->gravity<1.f?1.f:b->gravity;
    ++o->ballistic_calls;
    if(!b->ballistic||!b->ballistic(b->context,q.muzzle,o->target,o->gravity,1.f,o->ballistic)||!finite(o->ballistic))return 0;
    ++o->normalizations;if(!finite(o->direction)||!b->normalize||!b->normalize(b->context,o->direction)||!finite(o->direction))return 0;
    auto draw=[&](float lo,float hi,float& x){
        ++o->random_calls;return e.random&&e.random(e.context,lo,hi,&x)&&std::isfinite(x)&&x>=lo&&x<=hi;
    };
    for(unsigned i=0;i<3;++i){
        if(!draw(-.05f,i==2?0.f:.05f,o->draws[i]))return 0;
        o->direction[i]=static_cast<float>(double(o->direction[i])+o->draws[i]);
    }
    q.damage=b->damage;q.alternate=b->alternate;
    for(unsigned i=0;i<3;++i)q.velocity[i]=q.alternate?o->ballistic[i]:static_cast<float>(double(o->direction[i])*900.);
    if(!finite(q.velocity))return 0;
    if(!b->projectile||!b->projectile(b->context,a,&q))return 0;o->projectiles=1;
    ServerRecombinantSpitEffect effect{};
    std::memcpy(effect.muzzle,q.muzzle,12);std::memcpy(effect.ballistic,o->ballistic,12);
    effect.first_token=rd<unsigned>(a,0xc48);effect.second_token=rd<unsigned>(a,0xc4c);
    if(!b->effects||!b->effects(b->context,a,&effect))return 0;o->effects=1;
    if(!draw(1.5f,2.f,o->draws[3]))return 0;
    o->deadline=static_cast<float>(double(now)+o->draws[3]);if(!std::isfinite(o->deadline))return 0;
    wr(a,0xc3c,o->deadline);o->deadline_written=1;
    const float prior=rd<float>(a,0xc44);if(!std::isfinite(prior))return 0;
    // Retail compares AH&5, parity jump: >= .5 takes increment,
    // < .5 resets to .5 (not min/clamp of prior+.1).
    o->probability=prior<.5f?.5f:static_cast<float>(double(prior)+.1f);
    if(!std::isfinite(o->probability))return 0;
    wr(a,0xc44,o->probability);o->probability_written=o->completed=1;return 1;
}
const ServerRecombinantSpitApi api{1,sizeof(ServerRecombinantSpitApi),&execute};

struct ClassPointFrame {
    void* actor;
    void* last_enemy;
    const ServerRecombinantSpitClassPointBindings& bindings;
    const ServerRecombinantEventBindings& original;
    ServerRecombinantSpitClassPointResult& result;
};
int __cdecl class_enemy(void* p,void* actor,void** out) noexcept {
    auto& f=*static_cast<ClassPointFrame*>(p);
    if(actor!=f.actor||!f.original.enemy)return 0;
    const int ok=f.original.enemy(f.original.context,actor,out);
    if(ok&&out)f.last_enemy=*out;
    return ok;
}
int __cdecl class_random(void* p,float lo,float hi,float* out) noexcept {
    auto& f=*static_cast<ClassPointFrame*>(p);
    return f.original.random&&f.original.random(f.original.context,lo,hi,out);
}
int __cdecl class_target_point(void* p,void* entity,float value[3]) noexcept {
    auto& f=*static_cast<ClassPointFrame*>(p);
    if(!entity||entity!=f.last_enemy||!value)return 0;
    ++f.result.point_calls;
    unsigned bytes=0;
    if(!f.bindings.extent||!f.bindings.extent(f.bindings.context,entity,&bytes)||bytes<4)return 0;
    const unsigned vt=rd<unsigned>(entity,0);
    if(vt==recombinant_minor_vtable){
        ++f.result.minor_calls;
        const auto* native=SiN_GetServerRecombinantMinorOriginPointApi(
            1,sizeof(ServerRecombinantMinorOriginPointApi));
        return native&&f.bindings.minor_point&&
            native->point(entity,bytes,value,f.bindings.minor_point,&f.result.minor_point);
    }
    ++f.result.delegated_calls;
    return f.original.target_point&&
        f.original.target_point(f.original.context,entity,value);
}
int __cdecl execute_class_point(void* actor,unsigned bytes,float now,
    const ServerRecombinantSpitClassPointBindings* b,
    ServerRecombinantSpitClassPointResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!actor||bytes<0xc50u||rd<unsigned>(actor,0)!=recombinant_minor_vtable||
       !b||!b->spit||!b->spit->event||!b->extent)return 0;
    auto& original=*b->spit->event;
    ClassPointFrame frame{actor,nullptr,*b,original,*out};
    auto event=original;
    event.context=&frame;event.enemy=&class_enemy;
    event.target_point=&class_target_point;event.random=&class_random;
    auto spit=*b->spit;spit.event=&event;
    return execute(actor,bytes,now,&spit,&out->spit);
}
const ServerRecombinantSpitClassPointApi class_point_api{
    1,sizeof(ServerRecombinantSpitClassPointApi),&execute_class_point
};

}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantSpitApi=_SiN_GetServerRecombinantSpitApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantSpitApi* __cdecl SiN_GetServerRecombinantSpitApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantSpitApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantSpitClassPointApi=_SiN_GetServerRecombinantSpitClassPointApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantSpitClassPointApi* __cdecl
SiN_GetServerRecombinantSpitClassPointApi(unsigned version,unsigned size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerRecombinantSpitClassPointApi)?&class_point_api:nullptr;
}
