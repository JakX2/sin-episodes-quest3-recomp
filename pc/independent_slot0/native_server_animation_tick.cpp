#include "native_server_animation_tick.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTransitionApi* __cdecl
SiN_GetServerNpcTransitionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActivityFrameApi* __cdecl
SiN_GetServerNpcActivityFrameApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
bool resolve(const ServerNpcAnimationTickBindings& b,std::uint32_t word,unsigned min,const void** p) noexcept {
    *p=nullptr;unsigned extent=0;
    return word&&b.storage(b.context,word,p,&extent)&&*p&&extent>=min;
}
bool current_path(void* actor,const ServerNpcAnimationTickBindings& b,std::uint32_t& component,const void** path,unsigned min) noexcept {
    component=read<std::uint32_t>(actor,0x8f8);
    const void* data=nullptr;
    return resolve(b,component,0x28,&data)&&resolve(b,read<std::uint32_t>(data,0x24),min,path);
}
int __cdecl run(void* actor,unsigned bytes,const ServerNpcRunAnimationInput* in,const ServerNpcAnimationTickBindings* b,ServerNpcAnimationTickResult* out) noexcept {
    if(!out)return 0;*out={};out->movement_activity=-1;
    if(!actor||bytes<0x90c||!in||!b||!b->storage||!std::isfinite(in->now))return 0;
    const auto& t=b->transition.assignment.translation;
    if((t.actor_class!=NpcActivityGrunt&&t.actor_class!=NpcActivityJetpack)||t.activities!=b->frame.activities||
       t.events!=b->frame.frame.dictionary.dictionary)return 0;
    const auto* transition=SiN_GetServerNpcTransitionApi(1,sizeof(ServerNpcTransitionApi));
    const auto* animation=SiN_GetServerNpcActivityFrameApi(1,sizeof(ServerNpcActivityFrameApi));
    if(!transition||!animation)return 0;
    // +108 -> 2204c6d0 -> 22074090 -> 2209d0f0
    std::uint32_t component=0;const void* path=nullptr;
    if(!current_path(actor,*b,component,&path,0x48))return 0;
    out->path_suppressed=read<std::uint32_t>(path,0x44)!=0;
    const int pending=read<int>(actor,0x880);
    if(!out->path_suppressed&&(pending==6||pending==7||pending==10||pending==11)){
        out->movement_checked=1;
        bool promoted=false;
        if(!b->promote_path||!b->promote_path(b->context,actor,component,&promoted))return 0;
        out->promoted=promoted;
        if(!promoted){
            if(!current_path(actor,*b,component,&path,0x28))return 0;
            const int desired=read<std::uint32_t>(path,0)?read<int>(path,0x24):-1;
            out->movement_activity=desired>0?desired:1;
            if(!transition->set_pending(actor,bytes,out->movement_activity,&b->transition,&out->movement))return 0;
            out->movement_requested=1;
        }
    }
    // Eight verified +294 targets are direct 220f44e0.
    auto original=*in;original.clear_npc_flags=false;
    if(!animation->run(actor,bytes,&original,&b->frame,&out->animation))return 0;
    const auto word=read<std::uint32_t>(actor,0x78c);
    if(word!=0xffffffffu){
        void* weapon=nullptr;
        if(!b->weapon)return 0;
        const int live=b->weapon(b->context,word,&weapon);
        if(live<0||(live>0&&!weapon))return 0;
        if(live){
            if(!b->weapon_post||!b->weapon_post(b->context,weapon,actor))return 0;
            out->weapon_posted=1;
        }
    }
    return 1;
}
const ServerNpcAnimationTickApi api{1,sizeof(ServerNpcAnimationTickApi),&run};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcAnimationTickApi=_SiN_GetServerNpcAnimationTickApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcAnimationTickApi* __cdecl
SiN_GetServerNpcAnimationTickApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcAnimationTickApi)?&api:nullptr;
}
