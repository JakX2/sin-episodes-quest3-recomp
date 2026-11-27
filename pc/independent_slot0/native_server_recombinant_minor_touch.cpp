#include "native_server_recombinant_minor_touch.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorApi* __cdecl SiN_GetServerRecombinantMinorApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
struct Call{
    void* a;unsigned n;void* other;unsigned other_n;float now;
    const ServerRecombinantMinorTouchBindings& b;
    ServerRecombinantMinorTouchResult& o;
    const ServerRecombinantMinorBindings& m;
    const ServerRecombinantPolicyBindings& p;
    bool cond(int id,bool value){
        const auto* ca=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult r{};
        ++o.condition_writes;return ca&&ca->store(a,n,id,NpcConditionCurrent,value,&p.conditions,&r);
    }
    bool schedule_now(){
        if(!m.schedule_touch_think||!m.schedule_touch_think(m.context,a,now,0))return false;
        ++o.think_scheduled;return true;
    }
    bool clear_touch(){
        wr(a,0x7c,0u);o.touch_cleared=1;
        if(!b.install_think||!b.install_think(b.context,a,recombinant_default_think_marker,0,0))return false;
        o.think_installed=1;return true;
    }
    bool detach(){
        const auto* api=SiN_GetServerRecombinantMinorApi(1,sizeof(ServerRecombinantMinorApi));
        if(!api||!api->detach(a,n,now,&m,&o.detach))return false;
        o.detached=1;return true;
    }
    // Return 1 allowed, 0 ordinary reject, -1 invalid owner storage.
    int solid_after_cooldown(){
        const float deadline=rd<float>(a,0xc38);
        if(!std::isfinite(deadline))return -1;
        if(now<deadline){o.cooldown_blocked=1;return 0;}
        if(!other||other_n<0x121)return -1;
        if(!rd<unsigned>(other,0x11c)||(rd<unsigned char>(other,0x120)&0x2cu)){
            o.solid_blocked=1;return 0;
        }
        return 1;
    }
    bool target_contact(){
        // 2237af10: exact schedule82(false) early-out.
        bool same=false;++o.schedule_tests;
        if(!p.schedule_matches||!p.schedule_matches(p.context,a,82,false,&same))return false;
        if(same)return cond(68,false);
        if(!cond(68,true))return false;
        const float zero[]{0,0,0};
        if(!m.set_velocity||!m.set_velocity(m.context,a,zero))return false;
        o.velocity_set=1;
        if(!b.set_movement||!b.set_movement(b.context,a,0,0))return false;
        o.movement_set=1;return schedule_now();
    }
    bool contact(){
        if(!other||other_n<0x21c)return false;
        wr<unsigned char>(a,0xc41,0);o.attached_cleared=1;
        // 22256b20 checks the other entity's object flags before invoking
        // a local-player method; no object implementation is copied.
        if(const unsigned token=rd<unsigned>(other,0x218)){
            unsigned flags=0;++o.object_checks;
            if(!m.object_flags||!m.object_flags(m.context,token,&flags))return false;
            if(flags&4u){
                if(!b.release_player_object||!b.release_player_object(b.context,other))return false;
                o.object_released=1;
            }
        }
        if(rd<unsigned>(other,0x94)&&rd<unsigned>(other,0x9c)){
            if(!b.entity_handle||!b.entity_handle(b.context,other,&o.target_handle))return false;
            wr(a,0xc30,o.target_handle);o.target_written=1;
            if(!target_contact())return false;
            return clear_touch();
        }
        o.grounded=(rd<unsigned char>(a,0xbc)&1u)!=0;
        if(o.grounded)return clear_touch();
        const int gate=solid_after_cooldown();
        if(gate<0)return false;
        if(!gate)return true;
        return detach()&&clear_touch();
    }
    bool detached_contact(){
        o.grounded=(rd<unsigned char>(a,0xbc)&1u)!=0;
        wr<unsigned char>(a,0xc41,0);o.attached_cleared=1;
        if(o.grounded)return clear_touch()&&schedule_now();
        const int gate=solid_after_cooldown();
        return gate>=0&&(!gate||detach());
    }
};
int __cdecl touch(void* a,unsigned n,void* other,unsigned other_n,float now,const ServerRecombinantMinorTouchBindings* b,ServerRecombinantMinorTouchResult* o) noexcept {
    if(!o)return 0;*o={};o->target_handle=0xffffffffu;
    if(!a||n<0xc48||rd<unsigned>(a,0)!=recombinant_minor_vtable||!std::isfinite(now)||
       !b||!b->minor||!b->minor->damage||!b->minor->damage->policy)return 0;
    o->marker=o->final_marker=rd<unsigned>(a,0x7c);
    if(o->marker!=recombinant_minor_contact_marker&&o->marker!=recombinant_minor_detached_contact_marker)return 0;
    Call c{a,n,other,other_n,now,*b,*o,*b->minor,*b->minor->damage->policy};
    o->completed=o->marker==recombinant_minor_contact_marker?c.contact():c.detached_contact();
    o->final_marker=rd<unsigned>(a,0x7c);return o->completed;
}
const ServerRecombinantMinorTouchApi api{1,sizeof(ServerRecombinantMinorTouchApi),&touch};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantMinorTouchApi=_SiN_GetServerRecombinantMinorTouchApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorTouchApi* __cdecl SiN_GetServerRecombinantMinorTouchApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantMinorTouchApi)?&api:nullptr;
}
