#include "native_server_npc_behavior_selection.h"
#include <cstring>
#include <limits>
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
int __cdecl select(void* actor,unsigned bytes,const ServerNpcBehaviorBindings* b,ServerNpcBehaviorResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!actor||bytes<0xc2c||!b)return 0;
    out->prior=read<std::uint32_t>(actor,0xc18);
    const int count=read<int>(actor,0xc28);
    if(count<0||count>4096)return 0;
    for(int i=0;i<count;++i){
        const void* rows=nullptr;unsigned extent=0;const auto word=read<std::uint32_t>(actor,0xc1c);
        if(!word||!b->storage||!b->storage(b->context,word,&rows,&extent)||!rows||extent<4u*(i+1))return 0;
        const auto component=read<std::uint32_t>(rows,4u*i);
        bool eligible=false,accepted=false;++out->visited;
        if(!component||!b->eligible||!b->eligible(b->context,component,&eligible))return 0;
        if(!eligible)continue;
        ++out->eligible;
        // Original reloads vector after eligible callback.
        if(!b->storage(b->context,read<std::uint32_t>(actor,0xc1c),&rows,&extent)||!rows||extent<4u*(i+1))return 0;
        const auto candidate=read<std::uint32_t>(rows,4u*i);
        if(!candidate||!b->accept||!b->accept(b->context,actor,candidate,&accepted))return 0;
        if(accepted){
            ++out->accepted;
            if(!b->storage(b->context,read<std::uint32_t>(actor,0xc1c),&rows,&extent)||!rows||extent<4u*(i+1))return 0;
            out->selected=read<std::uint32_t>(rows,4u*i);if(!out->selected)return 0;break;
        }
        // Stable vector count required during this bounded pass.
        if(read<int>(actor,0xc28)!=count)return 0;
    }
    const auto prior=read<std::uint32_t>(actor,0xc18);
    write(actor,0xc18,out->selected);
    if(prior==out->selected)return 1;
    out->changed=1;
    if(out->selected){
        if(!b->enter||!b->activate||!b->enter(b->context,out->selected)||!b->activate(b->context,out->selected))return 0;
    }
    if(prior&&(!b->leave||!b->reset||!b->leave(b->context,prior)||!b->reset(b->context,actor)))return 0;
    return b->changed&&b->changed(b->context,actor,prior,out->selected);
}
bool schedule_map(void* a,int input,const ServerNpcConditionMapBindings& b,int& global,bool reverse=false) noexcept {
    global=input;
    if(reverse?input<npc_global_condition_base:input>=npc_global_condition_base)return true;
    const void* desc=nullptr;unsigned n=0;
    if(!b.descriptor||!b.descriptor(b.context,a,&desc,&n)||!desc||n<0x18)return false;
    global=-1;if(input==-1)return true;
    const void* row=static_cast<const unsigned char*>(desc)+4;
    std::uint32_t seen[256]{};
    for(unsigned i=0;i<256;++i){
        const int first=read<int>(row,4),last=read<int>(row,8);
        const int start=read<int>(row,0),end=read<int>(row,12);
        if(first!=9999&&(reverse?(input>=start&&input<=end):(input>=first&&input<=last))){
            const auto v=reverse?static_cast<std::int64_t>(first)-start+input:static_cast<std::int64_t>(start)-first+input;
            if(v<std::numeric_limits<int>::min()||v>std::numeric_limits<int>::max())return false;
            global=static_cast<int>(v);return true;
        }
        const auto next=read<std::uint32_t>(row,0x10);
        if(!next)return true;
        for(unsigned j=0;j<i;++j)if(seen[j]==next)return false;
        seen[i]=next;
        if(!b.storage||!b.storage(b.context,next,&row,&n)||!row||n<0x14)return false;
    }
    return false;
}
int __cdecl schedule_is(void* actor,unsigned bytes,int id,bool ideal,const ServerNpcScheduleBindings* b,unsigned* out) noexcept {
    if(!out)return 0;*out=0;
    if(!actor||bytes<0x7a0||!b)return 0;
    const auto token=read<std::uint32_t>(actor,0x798);
    if(!token){*out=id==0||id==npc_global_condition_base;return 1;}
    int global=-1;if(!schedule_map(actor,id,b->conditions,global))return 0;
    if(ideal){*out=read<int>(actor,0x79c)==global;return 1;}
    const void* s=nullptr;unsigned n=0;
    if(!b->storage||!b->storage(b->context,token,&s,&n)||!s||n<4)return 0;
    *out=read<int>(s,0)==global;return 1;
}
int __cdecl to_global(void* a,int id,const ServerNpcConditionMapBindings* b,int* out) noexcept {
    return a&&b&&out&&schedule_map(a,id,*b,*out);
}
int __cdecl to_local(void* a,int id,const ServerNpcConditionMapBindings* b,int* out) noexcept {
    return a&&b&&out&&schedule_map(a,id,*b,*out,true);
}
const ServerNpcScheduleNamespaceApi namespace_api{1,sizeof(ServerNpcScheduleNamespaceApi),&to_global,&to_local};
const ServerNpcBehaviorSelectionApi api{1,sizeof(ServerNpcBehaviorSelectionApi),&select,&schedule_is};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcBehaviorSelectionApi=_SiN_GetServerNpcBehaviorSelectionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcBehaviorSelectionApi* __cdecl
SiN_GetServerNpcBehaviorSelectionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcBehaviorSelectionApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcScheduleNamespaceApi=_SiN_GetServerNpcScheduleNamespaceApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleNamespaceApi* __cdecl
SiN_GetServerNpcScheduleNamespaceApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcScheduleNamespaceApi)?&namespace_api:nullptr;
}
