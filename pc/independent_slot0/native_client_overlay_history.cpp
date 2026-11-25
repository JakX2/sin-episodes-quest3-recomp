#include "native_client_overlay_history.h"
#include "client_overlay_cycle_math.h"
#include "native_weapon_datatables.h"
#include <cmath>
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {
    T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;
}
template<class T> void write(void* p,unsigned off,T v) noexcept {
    std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));
}
// Original 240c0eb0, time is rounded to float after subtracting delay.
int __cdecl select(std::uint32_t head,int type,float time,float delay,const ClientOverlayHistoryBindings* b,ClientOverlayHistorySelection* out) noexcept {
    if(!out)return 0;
    *out={};
    if(!std::isfinite(time)||!std::isfinite(delay)||!b||!b->resolve)return 0;
    const float target=static_cast<float>(static_cast<double>(time)-delay);
    if(!std::isfinite(target))return 0;
    ClientOverlayHistorySelection s{};
    unsigned steps=0;
    for(auto word=head;word;){
        if(++steps>4096)return 0;
        auto* node=b->resolve(b->context,word);
        if(!node||!std::isfinite(node->time)||node==s.newer)return 0;
        s.older=node;
        if(node->time==0.f)break;
        if(target>=node->time){
            if(!s.newer)s.newer=node;
            else {
                const double gap=static_cast<double>(s.newer->time)-node->time;
                if(gap<=0.0001f)s.older=s.newer;
                else {
                    const double f=(static_cast<double>(target)-node->time)/gap;
                    s.fraction=static_cast<float>(f>2.?2.:f);
                    if(!(type&0x10)&&node->next){
                        auto* p=b->resolve(b->context,node->next);
                        if(!p||p==node||p==s.newer||!std::isfinite(p->time))return 0;
                        s.earlier=p;
                        s.hermite=static_cast<double>(node->time)-p->time>0.0001f;
                    }
                }
            }
            *out=s;return 1;
        }
        s.newer=node;
        word=node->next;
    }
    // If the target precedes all usable samples, use the last valid newer
    // node for both endpoints. A lone zero-time sentinel is also returned.
    if(s.newer)s.older=s.newer;
    else s.newer=s.older;
    *out=s;return 1;
}
// Original 2408afb0: at exactly half a cycle take the wrap branch too.
int __cdecl loop_lerp(float f,float from,float to,float* out) noexcept {
    return out&&overlay_cycle_math::linear(f,from,to,*out);
}

bool sample_ok(const ClientOverlayHistoryNode* p) noexcept {
    return !p||(std::isfinite(p->sample.previous_cycle)&&std::isfinite(p->sample.cycle)&&std::isfinite(p->sample.weight));
}
// Original 240c19a0. Preflight every watcher before mutation. The host
// promises its metadata/virtual services cannot fail or move other rows.
int __cdecl correct(void* actor,unsigned bytes,float time,const ClientOverlayHistoryBindings* b,unsigned* changed) noexcept {
    if(!changed)return 0;
    *changed=0;
    if(!actor||bytes<0x7cc||!std::isfinite(time)||!b||!b->resolve||!b->looping||!b->interpolate)return 0;
    auto v=read<LayerVector>(actor,0x7b8);
    if(v.count<0||v.count>overlay_limit||v.capacity<v.count||(v.count&&!v.memory)||bytes<0x7cc+static_cast<unsigned>(v.count)*4)return 0;
    struct Work {ClientOverlayHistorySelection s{};float fraction{},linear{},looped{};};
    Work work[overlay_limit]{};
    for(int i=0;i<v.count;++i){
        auto* w=v.memory+i*0x44;
        auto& x=work[i];
        if(!select(read<std::uint32_t>(w,0x30),read<int>(w,0x2c),time,read<float>(w,0x3c),b,&x.s))return 0;
        auto& s=x.s;
        if(!s.newer||!s.older||s.newer->sample.sequence==s.older->sample.sequence)continue;
        if(!sample_ok(s.newer)||!sample_ok(s.older)||!sample_ok(s.earlier))return 0;
        if(s.earlier){
            const double dt=static_cast<double>(s.newer->time)-s.older->time;
            if(std::fabs(dt)>0.001f)x.fraction=static_cast<float>((static_cast<double>(s.earlier->time)-s.older->time)/dt);
            const auto& n=s.newer->sample;
            x.linear=static_cast<float>(static_cast<double>(n.previous_cycle)+(static_cast<double>(n.cycle)-n.previous_cycle)*x.fraction);
            if(!std::isfinite(x.linear)||!loop_lerp(x.fraction,n.previous_cycle,n.cycle,&x.looped))return 0;
        }
    }
    for(int i=0;i<v.count;++i){
        auto* w=v.memory+i*0x44;
        auto& x=work[i];auto& s=x.s;
        if(!s.newer||!s.older||s.newer->sample.sequence==s.older->sample.sequence)continue;
        const auto n=s.newer->sample;
        s.older->sample.sequence=n.sequence;
        s.older->sample.cycle=n.previous_cycle;
        s.older->sample.weight=n.weight;
        if(s.earlier){
            s.earlier->sample.sequence=n.sequence;
            s.earlier->sample.cycle=b->looping(b->context,actor,n.sequence)?x.looped:x.linear;
            s.earlier->sample.weight=n.weight;
        }
        write<std::uint8_t>(w,0x34,b->looping(b->context,actor,n.sequence)?1:0);
        b->interpolate(b->context,w,time);
        write<float>(actor,0x7cc+i*4,static_cast<float>(static_cast<double>(n.previous_cycle)-0.01));
        ++*changed;
    }
    return 1;
}
const ClientOverlayHistoryApi api{1,sizeof(ClientOverlayHistoryApi),&select,&loop_lerp,&correct};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetClientOverlayHistoryApi=_SiN_GetClientOverlayHistoryApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayHistoryApi* __cdecl
SiN_GetClientOverlayHistoryApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ClientOverlayHistoryApi)?&api:nullptr;
}
