#include "native_server_npc_run_animation.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerComposedAnimationFrameApi* __cdecl
SiN_GetServerComposedAnimationFrameApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerAnimationEventsApi* __cdecl
SiN_GetServerAnimationEventsApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerSequenceChoiceApi* __cdecl
SiN_GetServerSequenceChoiceApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerSequenceResetApi* __cdecl
SiN_GetServerSequenceResetApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
int run_impl(void* actor,unsigned bytes,const ServerNpcRunAnimationInput* in,const ServerNpcRunAnimationBindings* b,ServerNpcRunAnimationResult* out,
    const ServerNpcNativeSequenceBindings* native,ServerSequenceResetResult* reset_result) noexcept {
    if(!out)return 0;
    *out={};out->selected=-1;out->events.failed_layer=-2;
    if(!actor||bytes<0x8fc||!in||!b||!b->model_available||!std::isfinite(in->now)||
       (in->clear_npc_flags&&bytes<0xe0c))return 0;
    const int available=b->model_available(b->context,actor);
    if(available<0)return 0;
    if(!available)return 1;
    out->model_available=1;
    if(in->debug_enabled&&!b->debug_update)return 0;
    const auto* advance=SiN_GetServerComposedAnimationFrameApi(1,sizeof(ServerComposedAnimationFrameApi));
    const auto* events=SiN_GetServerAnimationEventsApi(1,sizeof(ServerAnimationEventsApi));
    if(!advance||!events||!advance->run(actor,bytes,in->now,in->clear_npc_flags?1u:0u,&b->advance))return 0;
    out->advanced=1;
    if(in->debug_enabled)b->debug_update(b->context,actor,read<std::uint32_t>(actor,0x8f8));
    const int state=read<int>(actor,0x850);
    if(state!=4&&state!=7&&read<int>(actor,0x878)==1){
        unsigned finished=0;
        if(b->finished){if(!b->finished(b->context,actor,&finished))return 0;}
        else finished=read<std::uint8_t>(actor,0x3e0)&&read<int>(actor,0x3ec)==read<int>(actor,0x884);
        if(finished){
            out->selection_attempted=1;
            const bool looping=read<std::uint8_t>(actor,0x3e1)!=0;
            const int activity=read<int>(actor,0x87c),current=read<int>(actor,0x3ec);
            if(native){
                const auto* chooser=SiN_GetServerSequenceChoiceApi(1,sizeof(ServerSequenceChoiceApi));
                const ServerSequenceChoiceEntry* entries=nullptr;unsigned count=0;
                if(!chooser||!native->choices)return 0;
                const int available=native->choices(native->context,actor,&entries,&count);
                if(available<0||(available>0&&!entries))return 0;
                if(!available){entries=nullptr;count=0;}
                if(!chooser->choose(entries,count,activity,current,looping,&native->choice,&out->selected))return 0;
            }else if(!b->select_sequence||!b->select_sequence(b->context,actor,activity,current,looping,&out->selected))return 0;
            if(out->selected!=-1){
                if(native){
                    const auto* setter=SiN_GetServerSequenceResetApi(1,sizeof(ServerSequenceResetApi));
                    if(!setter||!setter->set(actor,bytes,out->selected,&native->reset,reset_result))return 0;
                }else if(!b->set_sequence||!b->set_sequence(b->context,actor,out->selected))return 0;
                out->sequence_set=1;
            }
        }
    }
    if(!events->dispatch(actor,bytes,actor,in->now,&b->events,&out->events))return 0;
    out->events_dispatched=1;
    return 1;
}
int __cdecl run(void* a,unsigned bytes,const ServerNpcRunAnimationInput* in,const ServerNpcRunAnimationBindings* b,ServerNpcRunAnimationResult* out) noexcept {
    return run_impl(a,bytes,in,b,out,nullptr,nullptr);
}
int __cdecl run_native(void* a,unsigned bytes,const ServerNpcRunAnimationInput* in,const ServerNpcNativeSequenceBindings* b,ServerNpcNativeSequenceFrameResult* out) noexcept {
    if(!out)return 0;
    *out={};
    return run_impl(a,bytes,in,b?&b->frame:nullptr,&out->frame,b,&out->reset);
}
const ServerNpcNativeSequenceFrameApi native_api{1,sizeof(ServerNpcNativeSequenceFrameApi),&run_native};
const ServerNpcRunAnimationApi api{1,sizeof(ServerNpcRunAnimationApi),&run};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcRunAnimationApi=_SiN_GetServerNpcRunAnimationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcRunAnimationApi* __cdecl
SiN_GetServerNpcRunAnimationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcRunAnimationApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeSequenceFrameApi=_SiN_GetServerNpcNativeSequenceFrameApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeSequenceFrameApi* __cdecl
SiN_GetServerNpcNativeSequenceFrameApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcNativeSequenceFrameApi)?&native_api:nullptr;
}
