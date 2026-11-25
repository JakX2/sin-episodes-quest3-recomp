#include "native_client_overlay_watchers.h"
#include "native_weapon_datatables.h"
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
constexpr unsigned map_offset=0x788,layer_offset=0x7a4,watch_offset=0x7b8,actor_extent=0x7cc;
constexpr unsigned layer_stride=0x20,watch_stride=0x44;
struct Mapping { int flags; unsigned char* row; unsigned char* watcher; };
static_assert(sizeof(Mapping)==12);
template<class T> T read(const void* p,unsigned off=0) noexcept {
    T value{};std::memcpy(&value,static_cast<const unsigned char*>(p)+off,sizeof(value));return value;
}
template<class T> void write(void* p,unsigned off,const T& value) noexcept {
    std::memcpy(static_cast<unsigned char*>(p)+off,&value,sizeof(value));
}
bool valid(const LayerVector& v) noexcept {
    return v.count>=0&&v.capacity>=v.count&&v.capacity<=65536&&(!v.capacity||v.memory);
}
bool args(void* actor,unsigned bytes,int target) noexcept {
    return actor&&bytes>=actor_extent&&target>=0&&target<=overlay_limit;
}
// 240c0780 -> 240c0270: reserve does not change count, even when zeroing
// already-allocated dormant rows. Do not relocate registered live rows:
// the real receive graph reserves the maximum before its first length update.
int __cdecl reserve(void* actor,unsigned bytes,int target,const ClientOverlayWatcherBindings* b) noexcept {
    if(!args(actor,bytes,target))return 0;
    auto v=read<LayerVector>(actor,layer_offset);
    if(!valid(v)||v.count>overlay_limit)return 0;
    if(v.count>=target)return 1;
    if(v.capacity<target){
        if(v.count||v.grow_size==-1||!b||!b->grow_layers)return 0;
        auto* data=b->grow_layers(b->context,v.memory,static_cast<unsigned>(target)*layer_stride);
        if(!data)return 0;
        v.memory=data;v.capacity=target;
    }
    v.elements=v.memory;
    write(actor,layer_offset,v);
    std::memset(v.memory+v.count*layer_stride,0,(target-v.count)*layer_stride);
    return 1;
}
// 240c3500: remove old mappings, mutate layer vector, resize watcher
// vector, append {1,row,watcher}, Setup, GetType, amount, SetAmount.
int __cdecl resize(void* actor,unsigned bytes,int target,const ClientOverlayWatcherBindings* b) noexcept {
    if(!args(actor,bytes,target))return 0;
    auto layers=read<LayerVector>(actor,layer_offset);
    auto watchers=read<LayerVector>(actor,watch_offset);
    auto map=read<LayerVector>(actor,map_offset);
    if(!valid(layers)||!valid(watchers)||!valid(map)||layers.count>overlay_limit||watchers.count!=layers.count)return 0;
    const int old=layers.count;
    if(old==target)return 1;
    if(target>layers.capacity||target>watchers.capacity||!b||!b->setup||!b->type||!b->amount||!b->set_amount||
       (target>old&&!b->construct)||(target<old&&!b->destroy))return 0;
    int removed=0;
    for(int i=0;i<old;++i){
        for(int j=0;j<map.count;++j){
            if(read<Mapping>(map.memory,j*sizeof(Mapping)).row==layers.memory+i*layer_stride){++removed;break;}
        }
    }
    if(map.count-removed+target>map.capacity)return 0;
    // First matching row only, mirroring 240c04e0.
    for(int i=0;i<old;++i){
        for(int j=0;j<map.count;++j){
            if(read<Mapping>(map.memory,j*sizeof(Mapping)).row!=layers.memory+i*layer_stride)continue;
            std::memmove(map.memory+j*sizeof(Mapping),map.memory+(j+1)*sizeof(Mapping),(map.count-j-1)*sizeof(Mapping));
            --map.count;write(actor,map_offset,map);break;
        }
    }
    if(target>old){
        for(int i=old;i<target;++i){
            auto* row=layers.memory+i*layer_stride;
            std::memset(row,0,0x0c);std::memset(row+0x10,0,0x10);
        }
    }
    layers.count=target;layers.elements=layers.memory;write(actor,layer_offset,layers);
    if(target>old){
        watchers.count=target;watchers.elements=watchers.memory;write(actor,watch_offset,watchers);
        for(int i=old;i<target;++i)b->construct(b->context,watchers.memory+i*watch_stride);
    }else{
        for(int i=old;i>target;--i)b->destroy(b->context,watchers.memory+(i-1)*watch_stride);
        watchers.count=target;write(actor,watch_offset,watchers);
    }
    for(int i=0;i<target;++i){
        auto* row=layers.memory+i*layer_stride;
        auto* watcher=watchers.memory+i*watch_stride;
        write(map.memory,map.count*sizeof(Mapping),Mapping{1,row,watcher});
        ++map.count;map.elements=map.memory;write(actor,map_offset,map);
        b->setup(b->context,watcher,row,1);
        const int type=b->type(b->context,watcher);
        const float amount=b->amount(b->context,actor,type);
        b->set_amount(b->context,watcher,amount);
    }
    return 1;
}
const ClientOverlayWatchersApi api{1,sizeof(ClientOverlayWatchersApi),&reserve,&resize};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetClientOverlayWatchersApi=_SiN_GetClientOverlayWatchersApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayWatchersApi* __cdecl
SiN_GetClientOverlayWatchersApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ClientOverlayWatchersApi)?&api:nullptr;
}
