#include "native_server_recombinant_melee.h"
#include <cmath>
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
bool finite(const float* v) noexcept{return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
int __cdecl execute(void* a,unsigned n,const ServerRecombinantMajorMeleeRequest* q,const ServerRecombinantMeleeBindings* b,ServerRecombinantMeleeResult* o) noexcept{
    if(!o)return 0;*o={};
    if(!a||n<0xc48||rd<unsigned>(a,0)!=recombinant_major_vtable||!q||!std::isfinite(q->reach)||!finite(q->angular)||!finite(q->impulse)||!b||!b->event||!b->event->policy)return 0;
    const auto& e=*b->event;const auto& p=*e.policy;
    if(p.entry_count>sin_independent::kEntityHandleCapacity||(!p.entries&&p.entry_count))return 0;
    void* enemy=nullptr;if(!e.enemy||!e.enemy(e.context,a,&enemy))return 0;o->enemy_live=enemy!=nullptr;
    void* goal=sin_independent::EntityHandleView(p.entries,p.entry_count).Resolve(rd<unsigned>(a,0xc44));o->goal_live=goal!=nullptr;
    if(goal&&enemy){
        if(!b->throw_object||!b->throw_object(b->context,a,goal))return 0;
        o->throw_calls=1;o->hit=goal;o->completed=1;return 1;
    }
    const unsigned hull=rd<unsigned>(a,0x610);auto& tr=o->trace;
    tr.reach=q->reach;tr.damage=q->damage;tr.type=4;tr.scale=1.f;tr.mode=0;
    ++o->hull_calls;if(!b->hull_mins||!b->hull_mins(b->context,hull,tr.mins)||!finite(tr.mins))return 0;
    ++o->hull_calls;if(!b->hull_maxs||!b->hull_maxs(b->context,rd<unsigned>(a,0x610),tr.maxs)||!finite(tr.maxs))return 0;
    unsigned extent=0;++o->hit_calls;
    if(!b->hit_query||!b->hit_query(b->context,a,&tr,&o->hit,&extent))return 0;
    auto sound=[&](const char* s){++o->sound_calls;return p.sound&&p.sound(p.context,a,s,0,0);};
    if(!sound(o->hit?"NPC_Leperkin.AttackHit":"NPC_Leperkin.AttackMiss"))return 0;
    if(!o->hit){o->completed=1;return 1;}
    bool player=false;++o->player_calls;if(!e.is_player||!e.is_player(e.context,o->hit,&player))return 0;
    if(player){
        if(extent<0xbe)return 0;
        if(rd<unsigned char>(o->hit,0xbd)&0x40){o->player_protected=o->completed=1;return 1;}
        if(!b->player_view||!b->player_view(b->context,o->hit,q->angular))return 0;o->view_calls=1;
        if(!b->player_impulse||!b->player_impulse(b->context,o->hit,q->impulse))return 0;o->impulse_calls=o->completed=1;return 1;
    }
    unsigned token=0;++o->effect_queries;
    if(!b->effect_token||!b->effect_token(b->context,o->hit,&token))return 0;
    bool eligible=false;if(!b->effect_eligible||!b->effect_eligible(b->context,token,&eligible))return 0;
    o->effect_eligible=eligible;if(!eligible){o->completed=1;return 1;}
    o->blood_amount=static_cast<float>(q->damage<30?q->damage:30);
    auto blood=[&](const char* name){
        float v[3]{};bool found=false;++o->attachment_calls;
        if(!b->attachment||!b->attachment(b->context,a,name,v,&found))return false;
        if(!found)return true;
        ++o->effect_queries;if(!finite(v)||!b->effect_token(b->context,o->hit,&token)||!b->blood||!b->blood(b->context,v,token,o->blood_amount))return false;
        ++o->blood_calls;return true;
    };
    if((q->side==0||q->side==2)&&!blood("blood_left"))return 0;
    if((q->side==1||q->side==2)&&!blood("blood_right"))return 0;
    o->completed=1;return 1;
}
const ServerRecombinantMeleeApi api{1,sizeof(ServerRecombinantMeleeApi),&execute};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantMeleeApi=_SiN_GetServerRecombinantMeleeApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMeleeApi* __cdecl SiN_GetServerRecombinantMeleeApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantMeleeApi)?&api:nullptr;
}
