#include "native_server_scripted_animation_events.h"
extern "C" int __cdecl SiN_HandleScriptedNpcAnimationEvent(void*,unsigned,int,const char*,unsigned,
    const sin_port::native_map::ScriptedNpcAnimationEventBindings*,sin_port::native_map::ScriptedNpcAnimationEventResult*) noexcept;
#include <cmath>
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
struct ScriptedEventRoute {
    unsigned receiver_bytes;
    const native_map::ScriptedNpcAnimationEventBindings* bindings;
    ServerScriptedAnimationDispatchResult* result;
};
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T> void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool valid(const ServerAnimationEventSequence& s) noexcept {
    return s.count<=65536&&(!s.count||s.events);
}
int __cdecl next(const ServerAnimationEventSequence* s,float start,float end,unsigned index,float now,ServerAnimationEvent* out) noexcept {
    if(!s||!out||!valid(*s)||!std::isfinite(start)||!std::isfinite(end)||!std::isfinite(now))return -1;
    for(unsigned i=index;i<s->count;++i){
        const auto& e=s->events[i];
        if(!std::isfinite(e.cycle))return -1;
        // New event type excludes client bit. Legacy IDs >=5000 excluded.
        if((e.type&0x400)?(e.type&0x10)!=0:e.event>=5000)continue;
        const bool ordinary=e.cycle>=start&&e.cycle<end;
        const bool wrap=(s->flags&1)&&end<start&&(e.cycle>=start||e.cycle<end);
        if(ordinary||wrap){
            *out={e.event,e.options,e.cycle,now,e.type,nullptr};
            return static_cast<int>(i+1);
        }
    }
    return 0;
}
bool rate(const ServerAnimationEventBindings& b,void* a,int sequence,float playback,float& effective) noexcept {
    double r=0;
    if(!std::isfinite(playback)||!b.cycle_rate(b.context,a,sequence,&r)||!std::isfinite(r))return false;
    effective=static_cast<float>(r*playback);return std::isfinite(effective);
}
double interval(void* actor,float now) noexcept {
    const double current=read<float>(actor,0x2c),previous=read<float>(actor,0x28);
    const double dt=current<now?now-current:current-previous;
    return dt>.2f?.2f:(dt<0.?0.:dt);
}
bool emit_interval(void* actor,void* receiver,int layer,const ServerAnimationEventSequence& s,float start,float end,float cycle,float effective,float now,const ServerAnimationEventBindings& b,ServerAnimationEventResult& result,const ScriptedEventRoute* script=nullptr) noexcept {
    unsigned index=0;
    for(;;){
        ServerAnimationEvent event{};
        const int next_index=next(&s,start,end,index,now,&event);
        if(next_index<0)return false;
        if(!next_index)return true;
        index=static_cast<unsigned>(next_index);
        event.source=actor;
        if(effective>0.f){
            const float adjusted=event.cycle>cycle?static_cast<float>(static_cast<double>(event.cycle)-1.):event.cycle;
            event.time=static_cast<float>(static_cast<double>(read<float>(actor,0x2c))+interval(actor,now)+(static_cast<double>(adjusted)-cycle)/effective);
            if(!std::isfinite(event.time))return false;
        }
        if(script&&event.event>=1001&&event.event<=1003){
            auto& out=*script->result;
            if(!SiN_HandleScriptedNpcAnimationEvent(receiver,script->receiver_bytes,event.event,event.options,64,script->bindings,&out.last)||
               !out.last.handled)return false;
            ++out.script_events;
        }else{
            b.emit(b.context,receiver,layer,&event);
            if(script)++script->result->fallback_events;
        }
        ++result.emitted;
    }
}
bool dispatch_base_impl(void* actor,void* receiver,float now,const ServerAnimationEventBindings* b,ServerAnimationEventResult* out,const ScriptedEventRoute* script=nullptr) noexcept {
    // 220eefe0: base metadata uses the source actor, not the receiver.
    out->failed_layer=-1;
    const float playback=read<float>(actor,0x3c0);
    if(!std::isfinite(playback))return 0;
    if(playback!=0.f){
        const int sequence=read<int>(actor,0x3ec);
        ServerAnimationEventSequence s{};
        const int status=b->sequence(b->context,actor,sequence,&s);
        if(status<0||(status>0&&!valid(s)))return 0;
        if(status>0&&s.count){
            const float cycle=read<float>(actor,0x3e8),start=read<float>(actor,0x398);
            const float end=(!read<std::uint8_t>(actor,0x3e1)&&read<std::uint8_t>(actor,0x3e0))?1.01f:cycle;
            float effective=0;
            if(!std::isfinite(cycle)||!std::isfinite(start)||!rate(*b,actor,sequence,playback,effective))return 0;
            write(actor,0x398,end);
            if(!emit_interval(actor,receiver,-1,s,start,end,cycle,effective,now,*b,*out,script))return 0;
        }
    }
    out->failed_layer=-2;return true;
}
int __cdecl dispatch_base(void* actor,unsigned bytes,void* receiver,float now,const ServerAnimationEventBindings* b,ServerAnimationEventResult* out) noexcept {
    if(!out)return 0;*out={};out->failed_layer=-2;
    if(!actor||bytes<0x3f0||!receiver||!b||!b->sequence||!b->cycle_rate||!b->emit||!std::isfinite(now)||
       !std::isfinite(read<float>(actor,0x28))||!std::isfinite(read<float>(actor,0x2c)))return 0;
    return dispatch_base_impl(actor,receiver,now,b,out);
}
int dispatch_impl(void* actor,unsigned bytes,void* receiver,float now,const ServerAnimationEventBindings* b,ServerAnimationEventResult* out,
    const ScriptedEventRoute* script=nullptr) noexcept {
    if(!out)return 0;
    *out={};out->failed_layer=-2;
    if(!actor||bytes<0x4a8||!receiver||!b||!b->sequence||!b->cycle_rate||!b->storage||!b->emit||!std::isfinite(now)||
       !std::isfinite(read<float>(actor,0x28))||!std::isfinite(read<float>(actor,0x2c)))return 0;
    const int count=read<int>(actor,0x4a0),capacity=read<int>(actor,0x498);
    if(count<0||count>4096||capacity<count)return 0;
    void* data=nullptr;unsigned extent=0;
    if(count&&(!b->storage(b->context,read<std::uint32_t>(actor,0x494),&data,&extent)||!data||extent<static_cast<unsigned>(count)*0x40))return 0;
    if(!dispatch_base_impl(actor,receiver,now,b,out,script))return 0;
    // 220f33c0: metadata and rate use receiver, frame clock/source use actor.
    for(int i=0;i<count;++i){
        auto* row=static_cast<unsigned char*>(data)+i*0x40;
        if(!(read<unsigned>(row,0)&1))continue;
        out->failed_layer=i;++out->layers;
        const int sequence=read<int>(row,8);
        ServerAnimationEventSequence s{};
        const int status=b->sequence(b->context,receiver,sequence,&s);
        if(status<0||(status>0&&!valid(s)))return 0;
        if(!status||!s.count)continue;
        const float cycle=read<float>(row,0xc),start=read<float>(row,0x38);
        float effective=0,end=cycle;
        if(!std::isfinite(cycle)||!std::isfinite(start)||!std::isfinite(s.finish_window)||!rate(*b,receiver,sequence,read<float>(row,0x14),effective))return 0;
        if(!read<std::uint8_t>(row,5)&&(1.-static_cast<double>(effective)*s.finish_window<=cycle||cycle<0.f)){
            write<std::uint8_t>(row,4,1);end=1.f;
        }
        write(row,0x38,end);
        if(!emit_interval(actor,receiver,i,s,start,end,cycle,effective,now,*b,*out,script))return 0;
    }
    out->failed_layer=-2;return 1;
}
int __cdecl dispatch(void* actor,unsigned bytes,void* receiver,float now,const ServerAnimationEventBindings* b,ServerAnimationEventResult* out) noexcept {
    return dispatch_impl(actor,bytes,receiver,now,b,out);
}
int dispatch_scripted(void* actor,unsigned bytes,void* receiver,unsigned receiver_bytes,float now,
    const ServerScriptedAnimationDispatchBindings* b,ServerScriptedAnimationDispatchResult* out) noexcept {
    if(!out)return 0;*out={};out->animation.failed_layer=-2;
    if(!b||!b->animation||!b->script||!b->script->interrupt||!b->script->interrupt->control||receiver_bytes<0x974)return 0;
    const auto& control=*b->script->interrupt->control;
    unsigned actual=0;
    if(!receiver||!control.extent||!control.extent(control.context,receiver,&actual)||actual<receiver_bytes)return 0;
    const ScriptedEventRoute script{receiver_bytes,b->script,out};
    out->completed=dispatch_impl(actor,bytes,receiver,now,b->animation,&out->animation,&script);
    return out->completed;
}
const ServerBaseAnimationEventsApi base_api{1,sizeof(ServerBaseAnimationEventsApi),&dispatch_base};
const ServerAnimationEventsApi api{1,sizeof(ServerAnimationEventsApi),&next,&dispatch};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerAnimationEventsApi=_SiN_GetServerAnimationEventsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerAnimationEventsApi* __cdecl
SiN_GetServerAnimationEventsApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerAnimationEventsApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerBaseAnimationEventsApi=_SiN_GetServerBaseAnimationEventsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerBaseAnimationEventsApi* __cdecl
SiN_GetServerBaseAnimationEventsApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerBaseAnimationEventsApi)?&base_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_DispatchScriptedNpcAnimationEvents=_SiN_DispatchScriptedNpcAnimationEvents")
extern "C" int __cdecl SiN_DispatchScriptedNpcAnimationEvents(void* a,unsigned n,void* receiver,unsigned receiver_bytes,float now,
    const sin_port::native_weapon_tables::ServerScriptedAnimationDispatchBindings* b,
    sin_port::native_weapon_tables::ServerScriptedAnimationDispatchResult* out) noexcept {
    return sin_port::native_weapon_tables::dispatch_scripted(a,n,receiver,receiver_bytes,now,b,out);
}
