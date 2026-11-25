#include "native_server_npc_inherited_schedule.h"
#include "native_server_weapon_script_view.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSubclassSchedulerApi* __cdecl SiN_GetServerNpcSubclassSchedulerApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcInheritedScheduleApi* __cdecl SiN_GetServerNpcInheritedScheduleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponScriptViewApi* __cdecl SiN_GetServerWeaponScriptViewApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
struct Call{
    void* a;unsigned n;float now;
    const ServerNpcClassScheduleTranslationBindings& b;
    ServerNpcClassScheduleTranslationResult& o;
    const ServerNpcConditionApi* cond;
    bool ok=true;
    const ServerNpcInheritedScheduleBindings* inherited=nullptr;
    ServerNpcClassScheduleServicesResult* services=nullptr;
    bool has(int id) noexcept{
        ServerNpcConditionResult r{};++o.condition_tests;
        if(!ok||!cond->test(a,n,id,NpcConditionCurrent,&b.conditions,&r)){ok=false;return false;}return r.value!=0;
    }
    void* entity(unsigned off) noexcept{return sin_independent::EntityHandleView(b.entities.entries,b.entities.count).Resolve(rd<std::uint32_t>(a,off));}
    bool slot(int id) noexcept{bool yes=false;if(!ok||!b.squad_slot||!b.squad_slot(b.context,a,id,&yes))ok=false;return yes;}
    bool gate(unsigned v,int id) noexcept{bool yes=false;if(!ok||!b.actor_gate||!b.actor_gate(b.context,a,v,id,&yes))ok=false;return yes;}
    int random(int lo,int hi) noexcept{int v=0;++o.random_calls;if(!ok||!b.random||!b.random(b.context,lo,hi,&v)||v<lo||v>hi)ok=false;return v;}
    bool rules(int id) noexcept{bool yes=false;if(!b.rules_int||!b.rules_int(b.context,id,&yes))ok=false;return yes;}
    bool rules_f(float v) noexcept{bool yes=false;if(!b.rules_float||!b.rules_float(b.context,v,&yes))ok=false;return yes;}
    bool parent(int id,int& out) noexcept{
        ++o.parent_calls;if(!ok)return false;
        if(inherited&&services){
            const auto* api=SiN_GetServerNpcInheritedScheduleApi(1,sizeof(ServerNpcInheritedScheduleApi));
            ++services->inherited_calls;
            if(!api||!api->translate(a,n,id,inherited,&services->inherited))return false;
            out=services->inherited.output;return true;
        }
        return b.parent&&b.parent(b.context,a,id,&out);
    }
    bool burst(void* w,int& value) noexcept {
        if(inherited&&services){
            const auto* api=SiN_GetServerNpcInheritedScheduleApi(1,sizeof(ServerNpcInheritedScheduleApi));
            ++services->burst_calls;return api&&api->burst(w,inherited,&value);
        }
        return b.burst&&b.burst(b.context,w,&value);
    }
    bool interval(void* w,double& value) noexcept {
        if(inherited&&services){
            const auto* api=SiN_GetServerWeaponScriptViewApi(1,sizeof(ServerWeaponScriptViewApi));
            const auto& reg=inherited->entities;unsigned kind=0;++services->interval_calls;
            return api&&reg.classify&&reg.classify(reg.context,w,&kind)&&api->interval(kind,&value);
        }
        return b.interval&&b.interval(b.context,w,&value);
    }
    bool distance(void* e,bool center,double& d) noexcept{return b.distance&&b.distance(b.context,a,e,center,&d)&&std::isfinite(d);}
    bool translate(int id,int& out) noexcept{
        if(++o.steps>8||!ok)return false;
        if(o.subclass==NpcSubclassElite){
            switch(id){case 0x19:case 0x29:case 0x62:out=0x76;break;case 0x27:out=0x77;break;case 0x30:case 0x54:case 0x63:out=0x31;break;default:out=id;}
            return true;
        }
        if(o.subclass==NpcSubclassJetpack){
            if(id==0x17&&rd<std::uint8_t>(a,0x1084)){out=0x76;return true;}
            switch(id){case 0xb:case 0x29:out=0x7a;return true;case 0x21:case 0x58:out=2;return true;case 0x25:case 0x70:out=0x7b;return true;case 0x30:out=0x31;return true;}
        }
        ++o.custom_calls;
        switch(id){
        case 0x12:out=0x52;return true;
        case 0x1a:out=0x5e;return true;
        case 0x19:
            if(rd<std::uint32_t>(a,0x9c4)){
                if(rules(3)&&has(0x15)&&slot(12)){
                    if(!b.cue||!b.cue(b.context,a,0x7e8,0,2))return false;++o.cue_calls;out=0x61;return ok;
                }
            }else if(random(0,1)&&has(0x15)){out=0x60;return ok;}
            out=0x5d;return ok;
        case 0x62:return translate(has(0x14)&&gate(0x79c,0)?0x29:0x4f,out);
        case 0x24:{
            if(has(0xa))return translate(0x19,out);
            void* e=entity(0x8a0);if(e&&(!b.enemy_notice||!b.enemy_notice(b.context,a,e)))return false;
            return translate(0x64,out);
        }
        case 0x57:{
            void* e=entity(0x8a0);bool npc=false;unsigned caps=0;
            if(e&&(!b.enemy_capability||!b.enemy_capability(b.context,e,&npc,&caps)))return false;
            if((npc&&!(caps&0x2000))||rd<std::uint32_t>(a,0x9d0))return translate(0x21,out);
            out=0x57;return true;
        }
        case 0x21:
            wr<std::uint8_t>(a,0xc64,1);
            out=gate(0x790,1)&&slot(9)?0x6a:0x58;return ok;
        case 0x30:
            if(gate(0x78c,1)&&slot(12)&&random(0,100)<20){out=0x63;return ok;}
            out=rules_f(20.f)?0x54:0x31;return ok;
        case 0x29:{
            if(has(4)||has(3))return translate(0x30,out);
            if(gate(0x790,1)&&slot(9)){out=0x6a;return ok;}
            if(!ok)return false;
            void* w=nullptr;
            if(!rd<std::uint8_t>(a,0xc66)||has(0x11)||!(w=entity(0x78c)))wr<std::uint8_t>(a,0xc64,1);
            else if(void* e=entity(0x8a0)){
                double d=0;if(!distance(e,true,d))return false;
                if(d>256.0){
                    wr<std::uint8_t>(a,0xc64,0);bool yes=false;
                    if(!b.crouch_probe||!b.crouch_probe(b.context,a,e,w,&yes))return false;
                    if(!yes)wr<std::uint8_t>(a,0xc64,1);
                }
            }
            out=0x5b;return ok;
        }
        case 0x2a:{
            void* w=entity(0x78c);unsigned caps=0;
            if(w&&(!b.weapon_capability||!b.weapon_capability(b.context,w,&caps)))return false;
            out=caps&0x4000?0x2a:0x5c;return true;
        }
        case 0x50:{
            void* e=entity(0x8a0);bool player=false;
            if(e&&(!b.is_player||!b.is_player(b.context,e,&player)))return false;
            if(player&&rd<std::uint8_t>(a,0xc67)){
                double d=0;if(!distance(e,false,d))return false;
                if(d>=256.0){wr<std::uint8_t>(a,0xc67,0);out=0x56;return true;}
            }
            out=0x50;return true;
        }
        case 0x4f:out=entity(0x8a0)?0x51:0x4f;return true;
        case 0x64:{
            void* e=entity(0x8a0);unsigned bytes=0;
            if(e&&(!b.extent||!b.extent(b.context,e,&bytes)||bytes<0x90))return false;
            out=e&&rd<int>(e,0x8c)==0?0x67:0x64;return true;
        }
        case 0x6b:{
            wr<std::uint8_t>(a,0xca0,1);
            if(void* w=entity(0x78c)){
                int burst=0;double interval=0;
                if(!this->burst(w,burst))return false;wr(a,0xc98,burst);
                if(!this->interval(w,interval)||!std::isfinite(interval))return false;
                const float f=static_cast<float>(interval),deadline=static_cast<float>(now+interval-.1);
                if(!std::isfinite(f)||!std::isfinite(deadline))return false;wr(a,0xc9c,f);
                if(rd<float>(a,0x60c)!=deadline){
                    if(rd<std::uint8_t>(a,0x1f0))wr<std::uint8_t>(a,0x200,1);
                    else if(const auto token=rd<std::uint32_t>(a,0x16c)){if(!b.dirty||!b.dirty(b.context,a,token))return false;++o.dirty_calls;}
                    wr(a,0x60c,deadline);
                }
            }
            if(rd<int>(a,0x878)==0xc4)wr(a,0x878,0);
            if(!b.pending||!b.pending(b.context,a,0xc4))return false;++o.pending_calls;wr(a,0x99c,now);
            return parent(id,out);
        }
        default:return parent(id,out);
        }
    }
};
int translate_impl(void* a,unsigned n,std::uint32_t v,int id,float now,const ServerNpcClassScheduleTranslationBindings* b,ServerNpcClassScheduleTranslationResult* out,const ServerNpcInheritedScheduleBindings* inherited=nullptr,ServerNpcClassScheduleServicesResult* services=nullptr) noexcept{
    if(!out)return 0;*out={};out->input=id;out->output=-1;
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    if(!a||!b||!cls||!cond||!cls->classify(v,&out->subclass)||n<(out->subclass==NpcSubclassJetpack?0x1085u:0xcacu)||!std::isfinite(now))return 0;
    if(b->entities.count>sin_independent::kEntityHandleCapacity||(b->entities.count&&!b->entities.entries))return 0;
    Call q{a,n,now,*b,*out,cond,true,inherited,services};return q.translate(id,out->output);
}
int __cdecl translate(void* a,unsigned n,std::uint32_t v,int id,float now,const ServerNpcClassScheduleTranslationBindings* b,ServerNpcClassScheduleTranslationResult* out) noexcept{
    return translate_impl(a,n,v,id,now,b,out);
}
int __cdecl translate_services(void* a,unsigned n,std::uint32_t v,int id,float now,const ServerNpcClassScheduleTranslationBindings* b,const ServerNpcInheritedScheduleBindings* inherited,ServerNpcClassScheduleServicesResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!b||!inherited)return 0;
    auto bound=*inherited;bound.entities=b->entities;
    return translate_impl(a,n,v,id,now,b,&out->translation,&bound,out);
}
const ServerNpcClassScheduleServicesApi services_api{1,sizeof(ServerNpcClassScheduleServicesApi),&translate_services};
const ServerNpcClassScheduleTranslationApi api{1,sizeof(ServerNpcClassScheduleTranslationApi),&translate};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcClassScheduleTranslationApi=_SiN_GetServerNpcClassScheduleTranslationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcClassScheduleTranslationApi* __cdecl SiN_GetServerNpcClassScheduleTranslationApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcClassScheduleTranslationApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcClassScheduleServicesApi=_SiN_GetServerNpcClassScheduleServicesApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcClassScheduleServicesApi* __cdecl SiN_GetServerNpcClassScheduleServicesApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcClassScheduleServicesApi)?&services_api:nullptr;
}
