#include "native_server_recombinant_melee.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xc70]{},enemy[0xc0]{},goal[4]{};
    sin_independent::EntityHandleEntry entries[2]{};
    bool live=true,hit=true,player=true,eligible=true,left=true,right=true;
    std::string calls;
    template<class T>void put(void* a,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(a)+off,&v,sizeof(v));}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantMeleeApi>(GetProcAddress(server,server_recombinant_melee_export));
    assert(get&&!GetProcAddress(client,server_recombinant_melee_export)&&!get(2,sizeof(ServerRecombinantMeleeApi)));
    const auto* api=get(1,sizeof(ServerRecombinantMeleeApi));assert(api);
    F f;f.put(f.actor,0,recombinant_major_vtable);f.put(f.actor,0x610,4u);f.put(f.actor,0xc44,0xffffffffu);
    f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.goal);f.entries[1].serial=7;
    ServerRecombinantPolicyBindings policy{};policy.context=&f;policy.entries=f.entries;policy.entry_count=2;
    policy.sound=[](void* p,void* a,const char* s,int x,int y) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&!x&&!y&&!std::strcmp(s,f.hit?"NPC_Leperkin.AttackHit":"NPC_Leperkin.AttackMiss"));f.calls+='S';return 1;};
    ServerRecombinantMajorEventBindings event{};event.policy=&policy;event.context=&f;
    event.enemy=[](void* p,void*,void** out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='E';*out=f.live?f.enemy:nullptr;return 1;};
    event.is_player=[](void* p,void* a,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.enemy);f.calls+='P';*out=f.player;return 1;};
    ServerRecombinantMeleeBindings b{&event,&f};
    b.throw_object=[](void* p,void* a,void* g) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&g==f.goal);f.calls+='T';return 1;};
    b.hull_mins=[](void* p,unsigned token,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(token==4);f.calls+='M';v[0]=-1;v[1]=-2;v[2]=0;return 1;};
    b.hull_maxs=[](void* p,unsigned token,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(token==4);f.calls+='X';v[0]=1;v[1]=2;v[2]=4;return 1;};
    b.hit_query=[](void* p,void* a,const ServerRecombinantMeleeTrace* t,void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&t->reach==85&&t->mins[0]==-1&&t->maxs[2]==4&&t->damage==40&&t->type==4&&t->scale==1&&!t->mode);f.calls+='H';*out=f.hit?f.enemy:nullptr;*n=sizeof(f.enemy);return 1;};
    b.player_view=[](void* p,void* a,const float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.enemy&&v[0]==5&&v[1]==1&&v[2]==2);f.calls+='V';return 1;};
    b.player_impulse=[](void* p,void* a,const float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.enemy&&v[0]==200&&!v[1]&&!v[2]);f.calls+='I';return 1;};
    b.effect_token=[](void* p,void* a,unsigned* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.enemy);f.calls+='C';*out=7;return 1;};
    b.effect_eligible=[](void* p,unsigned token,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(token==7);f.calls+='G';*out=f.eligible;return 1;};
    b.attachment=[](void* p,void* a,const char* name,float v[3],bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor);bool left=!std::strcmp(name,"blood_left");assert(left||!std::strcmp(name,"blood_right"));f.calls+=left?'L':'R';*out=left?f.left:f.right;v[0]=left?1:2;return 1;};
    b.blood=[](void* p,const float v[3],unsigned token,float amount) noexcept ->int{assert((v[0]==1||v[0]==2)&&token==7&&amount==30);static_cast<F*>(p)->calls+='B';return 1;};
    ServerRecombinantMajorMeleeRequest q{85,40,{5,1,2},{200,0,0},2};ServerRecombinantMeleeResult o{};
    auto call=[&](){f.calls.clear();return api->execute(f.actor,sizeof(f.actor),&q,&b,&o);};
    f.put(f.actor,0xc44,0x7001u);assert(call()&&o.throw_calls==1&&o.hit==f.goal&&!o.hit_calls&&f.calls=="ET");
    f.live=false;f.hit=false;assert(call()&&o.goal_live&&!o.enemy_live&&!o.hit&&f.calls=="EMXHS");
    f.live=true;f.hit=true;f.put(f.actor,0xc44,0x8001u);assert(call()&&o.view_calls==1&&o.impulse_calls==1&&!o.goal_live&&f.calls=="EMXHSPVI");
    f.enemy[0xbd]=0x40;assert(call()&&o.player_protected&&!o.view_calls&&f.calls=="EMXHSP");f.enemy[0xbd]=0;
    f.player=false;f.eligible=false;assert(call()&&!o.effect_eligible&&!o.blood_calls&&f.calls=="EMXHSPCG");
    f.eligible=true;assert(call()&&o.blood_calls==2&&o.blood_amount==30&&f.calls=="EMXHSPCGLCBRCB");
    f.left=false;assert(call()&&o.attachment_calls==2&&o.blood_calls==1&&f.calls=="EMXHSPCGLRCB");
    q.side=0;assert(call()&&!o.blood_calls&&f.calls=="EMXHSPCGL");
    q.side=1;assert(call()&&o.blood_calls==1&&f.calls=="EMXHSPCGRCB");
    q.side=99;assert(call()&&!o.attachment_calls&&f.calls=="EMXHSPCG");
    FreeLibrary(server);FreeLibrary(client);return 0;
}
