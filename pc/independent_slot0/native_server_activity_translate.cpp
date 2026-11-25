#include "native_server_activity_translate.h"
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntRegistrationApi* __cdecl
SiN_GetServerGruntRegistrationApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerJetpackActivityApi* __cdecl
SiN_GetServerJetpackActivityApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityChoiceApi* __cdecl
SiN_GetServerActivityChoiceApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
bool npc(void* a,unsigned bytes,int activity,const ServerNpcActivityTranslateBindings& b,int* out) noexcept {
    if(b.actor_class==NpcActivityGrunt){
        const auto* api=SiN_GetServerGruntRegistrationApi(1,sizeof(ServerGruntRegistrationApi));
        return api&&api->translate(a,bytes,activity,b.ids,&b.grunt,out);
    }
    if(b.actor_class==NpcActivityJetpack){
        const auto* api=SiN_GetServerJetpackActivityApi(1,sizeof(ServerJetpackActivityApi));
        ServerJetpackActivityResult r{};
        if(!api||!api->translate(a,bytes,activity,b.ids,&b.jetpack,&r))return false;
        *out=r.activity;return true;
    }
    return b.actor_class==NpcActivityHosted&&b.other&&b.other(b.context,a,activity,out);
}
bool choice(void* a,int activity,const ServerNpcActivityTranslateBindings& b,bool probe,int* out) noexcept {
    const ServerActivityCompositeView* view=nullptr;
    const int status=b.metadata(b.context,a,&view);
    if(status<0||(status>0&&!view))return false;
    if(!status){*out=probe?-1:0;return true;}
    const auto* api=SiN_GetServerActivityChoiceApi(1,sizeof(ServerActivityChoiceApi));
    ServerActivityChoiceResult result{};
    if(!api||!api->choose(b.activities,b.events,view,activity,read<int>(a,0x3ec),true,&b.random,&result))return false;
    *out=result.selected;return true;
}
int __cdecl translate(void* a,unsigned bytes,int request,const ServerNpcActivityTranslateBindings* b,ServerNpcActivityTranslateResult* out) noexcept {
    if(!out)return 0;*out={};out->activity=out->initial_weapon=out->last_npc=out->final_weapon=request;
    if(!a||bytes<0x3f0||!b||!b->weapon||!b->metadata||!b->activities||!b->events)return 0;
    bool initial_required=false,required=false;
    if(!b->weapon(b->context,a,request,&initial_required,&out->initial_weapon))return 0;
    // First weapon result is saved, but iteration begins with original input.
    int current=request;
    for(unsigned i=0;i<5;++i){
        int translated=0;
        if(!npc(a,bytes,current,*b,&translated))return 0;
        if(translated!=current)out->last_npc=translated;
        if(!b->weapon(b->context,a,translated,&required,&out->final_weapon))return 0;
        ++out->iterations;
        if(out->final_weapon==current)break;
        current=out->final_weapon;
    }
    if(current==15){out->activity=15;return 1;}
    auto available=[&](int activity,int& sequence) noexcept {
        ++out->probes;return choice(a,activity,*b,true,&sequence);
    };
    int seq=-1;
    if(!available(out->final_weapon,seq))return 0;
    if(seq!=-1){out->activity=out->final_weapon;return 1;}
    if(required){
        out->required_missing=1;
        if(b->missing_required)b->missing_required(b->context,a,out->final_weapon);
    }
    if(out->last_npc!=out->final_weapon){
        if(!available(out->last_npc,seq))return 0;
        if(seq!=-1){out->activity=out->last_npc;return 1;}
    }
    if(out->initial_weapon!=out->last_npc){
        if(!available(out->initial_weapon,seq))return 0;
        // Original returns original request, NOT initial weapon activity.
        if(seq!=-1){out->activity=request;return 1;}
    }
    if(request!=out->initial_weapon){
        if(!available(request,seq))return 0;
        if(seq!=-1){out->activity=request;return 1;}
    }
    out->activity=request==10?6:request==6?10:request;
    return 1;
}
int __cdecl select(void* a,unsigned bytes,int request,const ServerNpcActivityTranslateBindings* b,ServerNpcActivitySelectionResult* out) noexcept {
    if(!out)return 0;*out={};out->sequence=-1;out->activity=request;
    if(!translate(a,bytes,request,b,&out->translation))return 0;
    out->activity=out->translation.activity;
    if(request==15){
        if(!b->scripted_sequence||!b->scripted_sequence(b->context,a,&out->sequence))return 0;
    }else{
        ++out->selections;
        if(!choice(a,out->activity,*b,false,&out->sequence))return 0;
        if(out->sequence==-1){
            if(b->missing_sequence)b->missing_sequence(b->context,a,out->activity);
            if(out->activity==10){
                out->activity=6;++out->selections;
                if(!choice(a,6,*b,false,&out->sequence))return 0;
            }
        }
    }
    if(out->sequence==-1)out->sequence=0;
    return 1;
}
const ServerNpcActivityTranslateApi api{1,sizeof(ServerNpcActivityTranslateApi),&translate,&select};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcActivityTranslateApi=_SiN_GetServerNpcActivityTranslateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActivityTranslateApi* __cdecl
SiN_GetServerNpcActivityTranslateApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcActivityTranslateApi)?&api:nullptr;
}
