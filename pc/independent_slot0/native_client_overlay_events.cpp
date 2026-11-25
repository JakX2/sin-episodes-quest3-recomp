#include "native_client_overlay_events.h"
#include "native_weapon_datatables.h"
#include <cmath>
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {
    T value{};std::memcpy(&value,static_cast<const unsigned char*>(p)+off,sizeof(value));return value;
}
template<class T> void write(void* p,unsigned off,T value) noexcept {
    std::memcpy(static_cast<unsigned char*>(p)+off,&value,sizeof(value));
}
bool eligible(const ClientOverlayEvent& e) noexcept {
    // 240c2053..2067 and 240c20d5..20e9:
    // new event system bit 0x400 requires client bit 0x10,
    // otherwise the numeric event must be >=5000.
    return (e.type&0x400)?(e.type&0x10)!=0:e.event>=5000;
}
int __cdecl dispatch(void* actor,unsigned bytes,float frame_time,int sequence_count,
    const ClientOverlayEventBindings* b,ClientOverlayEventResult* result) noexcept {
    if(!result)return 0;
    *result={};result->aborted_slot=-1;result->failed_slot=-1;
    if(!actor||bytes<0x7cc||!std::isfinite(frame_time)||sequence_count<0)return 0;
    if(frame_time==0.f)return 1;
    if(!b||!b->sequence||!b->emit)return 0;
    const auto v=read<LayerVector>(actor,0x7a4);
    if(v.count<0||v.count>overlay_limit||v.capacity<v.count||
       (v.count&&!v.memory)||bytes<0x7cc+static_cast<unsigned>(v.count)*4)return 0;
    // Preserve original raw index order, including zero-weight layers.
    for(int i=0;i<v.count;++i){
        const auto* row=v.memory+i*0x20;
        const int sequence=read<int>(row,0);
        ++result->visited;
        if(sequence>=sequence_count)continue;
        if(sequence<0){result->failed_slot=i;return 0;} // original unsafe indexing
        ClientOverlayEventView view{};
        if(!b->sequence(b->context,actor,sequence,&view)||view.count>65536||(view.count&&!view.events)){
            result->failed_slot=i;return 0;
        }
        if(!view.count)continue; // original leaves baseline unchanged
        const float cycle=read<float>(row,0x14);
        float previous=read<float>(actor,0x7cc+i*4);
        if(!std::isfinite(cycle)||!std::isfinite(previous)){result->failed_slot=i;return 0;}
        if(cycle==previous)continue;
        const bool backward=cycle<previous;
        if(backward&&static_cast<double>(previous)-cycle<=.5){
            // 240c2031 -> 240c21a1: exit the entire overlay pass.
            result->aborted_slot=i;return 1;
        }
        for(unsigned j=0;j<view.count;++j)if(!std::isfinite(view.events[j].cycle)){
            result->failed_slot=i;return 0;
        }
        if(backward){
            // Tail events strictly after previous; then restart at -.01.
            for(unsigned j=0;j<view.count;++j){
                const auto& e=view.events[j];
                if(eligible(e)&&e.cycle>previous){b->emit(b->context,actor,i,&e);++result->emitted;}
            }
            previous=-.01f;write(actor,0x7cc+i*4,previous);++result->wrapped;
        }
        // Head/ordinary interval is (previous,current]. Preserve record order.
        for(unsigned j=0;j<view.count;++j){
            const auto& e=view.events[j];
            if(eligible(e)&&e.cycle>previous&&e.cycle<=cycle){b->emit(b->context,actor,i,&e);++result->emitted;}
        }
        write(actor,0x7cc+i*4,cycle);
    }
    return 1;
}
const ClientOverlayEventsApi api{1,sizeof(ClientOverlayEventsApi),&dispatch};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetClientOverlayEventsApi=_SiN_GetClientOverlayEventsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayEventsApi* __cdecl
SiN_GetClientOverlayEventsApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ClientOverlayEventsApi)?&api:nullptr;
}
