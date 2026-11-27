#include "native_server_recombinant_events.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerEventDictionaryApi* __cdecl SiN_GetServerEventDictionaryApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
constexpr ServerRecombinantEventDefinition defs[]{
    {"AE_LEAP_ATTACK",0x2259d470,0x2237e160},
    {"AE_LEAP_TELEGRAPH",0x2259d46c,0x2237e17a},
    {"AE_SPIT",0x2259d468,0x2237e194},
    {"AE_POUNCE_SLASH",0x2259d464,0x2237e1ae},
    {"AE_POUNCE_OFF",0x2259d460,0x2237e1c8}
};
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
int __cdecl definitions(const ServerRecombinantEventDefinition** out,unsigned* n) noexcept{
    if(!out||!n)return 0;*out=defs;*n=5;return 1;
}
int __cdecl valid(const ServerRecombinantEventRegistration* r) noexcept{
    if(!r||!r->ready||r->count!=5||!r->owner||r->generation!=r->owner->generation)return 0;
    const auto* api=SiN_GetServerEventDictionaryApi(1,sizeof(ServerEventDictionaryApi));if(!api)return 0;
    for(unsigned i=0;i<5;++i){
        int id=-1;if(!api->lookup(r->owner,defs[i].name,&id)||id<0||id!=r->ids[i])return 0;
        bool found=false;for(unsigned j=0;j<r->owner->count;++j)if(r->owner->entries[j].id==id){found=r->owner->entries[j].private_event;break;}
        if(!found)return 0;
    }
    return 1;
}
int __cdecl register_minor(ServerEventDictionary* d,ServerRecombinantEventRegistration* r) noexcept{
    if(!r)return 0;*r={};for(auto& id:r->ids)id=-1;
    const auto* api=SiN_GetServerEventDictionaryApi(1,sizeof(ServerEventDictionaryApi));if(!api||!d)return 0;
    r->owner=d;r->generation=d->generation;
    for(unsigned i=0;i<5;++i){if(!api->private_id(d,defs[i].name,&r->ids[i]))return 0;++r->count;}
    r->ready=1;if(!valid(r)){r->ready=0;return 0;}return 1;
}
struct Call{
    void* a;const ServerRecombinantEventBindings& b;ServerRecombinantEventResult& o;
    bool enemy(void*& e){++o.enemy_queries;return b.enemy&&b.enemy(b.context,a,&e);}
    bool finite(const float* v){return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
    bool telegraph(){
        void* e=nullptr;
        if(!enemy(e))return false;if(!e)return true;
        // 2237af80 checks again, then reacquires before virtual target query.
        if(!enemy(e)||!e||!enemy(e)||!e)return false;
        o.target_live=1;bool special=false;
        if(!b.special||!b.special(b.context,e,&special))return false;
        if(special){if(!b.target_point||!b.target_point(b.context,e,o.point))return false;}
        else{if(!b.center||!b.center(b.context,e,o.point))return false;o.point[2]=static_cast<float>(double(o.point[2])+10.);}
        if(!finite(o.point)||!b.player_probe||!b.player_probe(b.context,e))return false;
        ++o.player_probes;std::memcpy(static_cast<unsigned char*>(a)+0xc18,o.point,12);
        wr<unsigned char>(a,0xc40,1);o.cache_written=1;return true;
    }
    bool leap(){
        if(rd<unsigned char>(a,0xc41)){o.attached_blocked=1;return true;}
        void* e=nullptr;if(!enemy(e))return false;o.target_live=e!=nullptr;
        if(e){
            const bool cached=rd<unsigned char>(a,0xc40)!=0;
            if(cached)std::memcpy(o.point,static_cast<unsigned char*>(a)+0xc18,12);
            else if(!b.target_point||!b.target_point(b.context,e,o.point))return false;
            if(!finite(o.point)||!b.launch||!b.launch(b.context,a,0,o.point,0))return false;
            o.launch_calls=1;o.cache_consumed=cached;wr<unsigned char>(a,0xc40,0);
        }else{
            // Original global zero vector 225885b8.
            if(!b.launch||!b.launch(b.context,a,1,o.point,0))return false;o.launch_calls=1;
        }
        const float prior=rd<float>(a,0xc44);if(!std::isfinite(prior))return false;
        o.probability=prior>.5f?.5f:static_cast<float>(double(prior)-.1f);
        wr(a,0xc44,o.probability);return true;
    }
    bool slash(){
        const auto& p=*b.policy;
        if(p.entry_count>sin_independent::kEntityHandleCapacity||(!p.entries&&p.entry_count))return false;
        void* e=sin_independent::EntityHandleView(p.entries,p.entry_count).Resolve(rd<unsigned>(a,0xc30));
        o.target_live=e!=nullptr;if(!e)return true;
        if(!b.pounce_slash||!b.pounce_slash(b.context,a,e))return false;o.slash_calls=1;
        // Original first draw -> y, second -> x; z is zero.
        float first=0,second=0;
        auto draw=[&](float& x){++o.random_calls;return b.random&&b.random(b.context,5.f,10.f,&x)&&std::isfinite(x)&&x>=5.f&&x<=10.f;};
        if(!draw(first)||!draw(second))return false;
        o.angles[0]=second;o.angles[1]=first;o.angles[2]=0;
        if(!b.angular_effect||!b.angular_effect(b.context,e,o.angles))return false;o.angular_calls=1;return true;
    }
};
int __cdecl minor(void* a,unsigned n,const void* event,unsigned bytes,const ServerRecombinantEventBindings* b,ServerRecombinantEventResult* o) noexcept{
    if(!o)return 0;*o={};o->route=-1;
    if(!a||n<0xc48||rd<unsigned>(a,0)!=recombinant_minor_vtable||!event||bytes<4||!b||!b->policy||!valid(b->registration))return 0;
    o->event=rd<int>(event,0);
    for(int i=0;i<5;++i)if(b->registration->ids[i]==o->event){o->route=i;break;}
    Call c{a,*b,*o};o->custom=o->route>=0;
    switch(o->route){
    case 0:o->completed=c.leap();break;
    case 1:o->completed=c.telegraph();break;
    case 2:o->spit_calls=1;o->completed=b->spit&&b->spit(b->context,a);break;
    case 3:o->completed=c.slash();break;
    case 4:o->off_calls=1;o->completed=b->pounce_off&&b->pounce_off(b->context,a);break;
    default:o->delegated=1;o->completed=b->parent&&b->parent(b->context,a,event,bytes);break;
    }return o->completed;
}
const ServerRecombinantEventsApi api{1,sizeof(ServerRecombinantEventsApi),&definitions,&register_minor,&valid,&minor};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantEventsApi=_SiN_GetServerRecombinantEventsApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantEventsApi* __cdecl SiN_GetServerRecombinantEventsApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantEventsApi)?&api:nullptr;
}
