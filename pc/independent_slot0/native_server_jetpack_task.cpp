#include "native_server_jetpack_task.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl
SiN_GetServerNpcTaskLifecycleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntTaskDispatchApi* __cdecl
SiN_GetServerGruntTaskDispatchApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
int __cdecl route(int id,bool running,unsigned* out) noexcept{
    if(!out)return 0;*out=JetpackTaskBase;
    if(running){if(id==67)*out=JetpackTaskNative;return 1;}
    switch(id){
    case 56:case 60:case 159:case 160:case 163:case 164:case 165:*out=JetpackTaskNative;break;
    case 25:case 166:*out=JetpackTaskHosted;break;
    case 9:case 10:case 40:case 43:*out=JetpackTaskAttachment;break;
    }
    return 1;
}
int __cdecl execute(void* a,unsigned bytes,const ServerNpcScheduleTask* t,bool running,float now,const ServerJetpackTaskBindings* b,ServerJetpackTaskResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!a||bytes<0x1028||!t||!b||!std::isfinite(now)||b->grunt.entities.count>sin_independent::kEntityHandleCapacity||
       (b->grunt.entities.count&&!b->grunt.entities.entries))return 0;
    route(t->id,running,&out->route);
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));if(!life)return 0;
    auto complete=[&](){return life->complete(a,bytes,now,false,&b->grunt.lifecycle,&out->lifecycle)!=0;};
    const auto registry=sin_independent::EntityHandleView(b->grunt.entities.entries,b->grunt.entities.count);
    if(out->route==JetpackTaskBase){
        // 2238b3c0 ordinary and 56/60 attachment branches return false;
        // own wrapper intercepts 56/60 first. No attachment deref here.
        const auto* grunt=SiN_GetServerGruntTaskDispatchApi(1,sizeof(ServerGruntTaskDispatchApi));
        out->delegated=1;return grunt&&grunt->execute(a,bytes,t,running,now,&b->grunt,&out->grunt);
    }
    if(out->route==JetpackTaskHosted)return b->special&&b->special(b->context,a,t);
    if(out->route==JetpackTaskAttachment){
        void* attachment=registry.Resolve(read<std::uint32_t>(a,0x1024));
        out->attachment_live=attachment!=nullptr;
        return attachment&&b->attachment_task&&b->attachment_task(b->context,a,attachment,t);
    }
    if(running){
        return b->grunt.turn_update&&b->grunt.turn_update(b->grunt.context,a)&&complete();
    }
    switch(t->id){
    case 56:case 60:{
        void* e=registry.Resolve(read<std::uint32_t>(a,0x8a0));out->enemy_live=e!=nullptr;
        if(e){
            void* attachment=registry.Resolve(read<std::uint32_t>(a,0x1024));out->attachment_live=attachment!=nullptr;
            if(!attachment||!b->face_enemy||!b->face_enemy(b->context,a,attachment,e))return 0;
        }
        return complete();
    }
    case 159:case 160:{
        const int mode=t->id==159?0:2;out->mode_changed=read<int>(a,0xfec)!=mode;write(a,0xfec,mode);
        return complete();
    }
    case 163:return 1;
    case 164:{
        // 220325c0(.05,0): f32(.05f * 10), round toward +infinity,
        // multiply integer ticks by original double .1, then add global now.
        const float deadline=static_cast<float>(static_cast<double>(now)+.1);
        if(!std::isfinite(deadline))return 0;
        write(a,0x9dc,deadline);out->timer_written=1;return 1;
    }
    case 165:
        write(a,0x8a8,0xffffffffu);out->target_cleared=1;
        if(!b->inspect_clear||!b->inspect_clear(b->context,a,5.f))return 0;
        std::memcpy(static_cast<unsigned char*>(a)+0x100c,b->default_inspect,12);return complete();
    default:return 0;
    }
}
const ServerJetpackTaskApi api{1,sizeof(ServerJetpackTaskApi),&route,&execute};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerJetpackTaskApi=_SiN_GetServerJetpackTaskApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerJetpackTaskApi* __cdecl
SiN_GetServerJetpackTaskApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerJetpackTaskApi)?&api:nullptr;
}
