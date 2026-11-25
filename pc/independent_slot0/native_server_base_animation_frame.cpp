#include "native_server_base_animation_frame.h"
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerAnimationFrameClockApi* __cdecl
SiN_GetServerAnimationFrameClockApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerBaseAnimationCycleApi* __cdecl
SiN_GetServerBaseAnimationCycleApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
struct Call {const ServerBaseAnimationFrameBindings* b;unsigned bytes;float now;const ServerBaseAnimationCycleApi* cycle;};
int __cdecl run(void* actor,unsigned bytes,float now,const ServerBaseAnimationFrameBindings* b) noexcept {
    if(!actor||bytes<0x47c||!b||!b->cycle.duration||!b->cycle.finish_threshold||
       !b->cycle.movement_distance||!b->cycle.invalidate||!b->cycle.cache)return 0;
    unsigned flags=0;std::memcpy(&flags,static_cast<unsigned char*>(actor)+0x390,4);
    if((flags&8)&&!b->width_transition)return 0;
    const auto* clock=SiN_GetServerAnimationFrameClockApi(1,sizeof(ServerAnimationFrameClockApi));
    const auto* cycle=SiN_GetServerBaseAnimationCycleApi(1,sizeof(ServerBaseAnimationCycleApi));
    if(!clock||!cycle)return 0;
    Call call{b,bytes,now,cycle};
    ServerAnimationFrameClockBindings binding{};binding.context=&call;binding.layers.vectors=b->cycle.vectors;
    binding.transition=[](void* p,void* a) noexcept -> int {
        auto& c=*static_cast<Call*>(p);return c.b->width_transition?c.b->width_transition(c.b->context,a):1;};
    binding.duration=[](void* p,void* a,int i,double* out) noexcept -> int {
        auto& c=*static_cast<Call*>(p);return c.b->cycle.duration(c.b->cycle.context,a,i,out);};
    binding.step=[](void* p,void* a,float delta) noexcept -> int {
        auto& c=*static_cast<Call*>(p);return c.cycle->step(a,c.bytes,delta,c.now,&c.b->cycle);};
    return clock->base(actor,bytes,now,&binding);
}
const ServerBaseAnimationFrameApi api{1,sizeof(ServerBaseAnimationFrameApi),&run};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerBaseAnimationFrameApi=_SiN_GetServerBaseAnimationFrameApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerBaseAnimationFrameApi* __cdecl
SiN_GetServerBaseAnimationFrameApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerBaseAnimationFrameApi)?&api:nullptr;
}
