#include "native_server_npc_schedule_resolution.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleNamespaceApi* __cdecl
SiN_GetServerNpcScheduleNamespaceApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl
SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcStateApi* __cdecl
SiN_GetServerNpcStateReconsiderApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntScheduleSelectorApi* __cdecl
SiN_GetServerGruntScheduleSelectorApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleStateApi* __cdecl
SiN_GetServerNpcScheduleStateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerJetpackSelectorApi* __cdecl
SiN_GetServerJetpackScheduleSelectorApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntNativeSlotSelectorApi* __cdecl SiN_GetServerGruntNativeSlotSelectorApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool lookup(void* a,int id,const ServerNpcScheduleResolutionBindings& b,ServerNpcScheduleResolutionResult& out) noexcept {
    const auto& sb=b.grunt.schedule;const auto component=read<std::uint32_t>(a,0xc18);
    if(component){
        if(!b.component_schedule||!b.component_schedule(b.context,component,id,&out.token))return false;
        if(out.token){out.component_hit=1;return true;}
    }
    const void* desc=nullptr;unsigned n=0;const auto* ns=SiN_GetServerNpcScheduleNamespaceApi(1,sizeof(ServerNpcScheduleNamespaceApi));
    if(!ns||!sb.conditions.descriptor||!sb.conditions.descriptor(sb.conditions.context,a,&desc,&n)||!desc||n<8)return false;
    if(read<int>(desc,4)==-1)out.global=1;
    else if(!ns->to_global(a,id,&sb.conditions,&out.global))return false;
    std::uint32_t token=b.head;
    // Retail linked-list traversal. Bound corrupted lists, never dereference PE32 words.
    for(unsigned i=0;token&&i<4096;++i){
        const void* row=nullptr;
        if(!sb.storage||!sb.storage(sb.context,token,&row,&n)||!row||n<0x34)return false;
        ++out.visited;
        if(read<int>(row,0)==out.global){out.token=token;return true;}
        const auto next=read<std::uint32_t>(row,0x30);if(next==token)return false;token=next;
    }
    return !token;
}
bool resolve(void* a,int id,const ServerNpcScheduleResolutionBindings& b,ServerNpcScheduleResolutionResult& out) noexcept {
    out.requested=id;
    if(!b.translate||!b.translate(b.context,a,id,&out.translated)||!lookup(a,out.translated,b,out))return false;
    if(!out.token){out.fallback=1;return lookup(a,1,b,out);}
    return true;
}
int __cdecl find(void* a,unsigned bytes,int id,const ServerNpcScheduleResolutionBindings* b,ServerNpcScheduleResolutionResult* out) noexcept {
    if(!out)return 0;*out={};out->requested=out->translated=out->global=out->ideal=-1;
    return a&&bytes>=0xc34&&b&&resolve(a,id,*b,*out);
}
int select_impl(void* a,unsigned bytes,float now,bool failure,const ServerNpcScheduleResolutionBindings* b,ServerNpcScheduleResolutionResult* out,const ServerJetpackSelectorBindings* jet=nullptr,ServerJetpackSelectorResult* jr=nullptr,const ServerNpcGruntSquadSlotsBindings* slots=nullptr,ServerGruntNativeSlotSelectorResult* sr=nullptr) noexcept {
    if(!out)return 0;*out={};out->requested=out->translated=out->global=out->ideal=-1;
    if(!a||bytes<0xdd8||!b||!std::isfinite(now))return 0;
    const auto* ns=SiN_GetServerNpcScheduleNamespaceApi(1,sizeof(ServerNpcScheduleNamespaceApi));
    const auto* ca=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    const auto* state=SiN_GetServerNpcStateReconsiderApi(1,sizeof(ServerNpcStateApi));
    const auto* sa=SiN_GetServerNpcScheduleStateApi(1,sizeof(ServerNpcScheduleStateApi));
    if(!ns||!ca||!state||!sa)return 0;
    const auto& sb=b->grunt.schedule;write<std::uint8_t>(a,0xc30,0);
    int id=0;
    if(failure){
        ServerNpcScheduleStateResult current{};
        if(!sa->inspect(a,bytes,&sb,&current))return 0;
        int local=0;
        if(current.schedule&&!ns->to_local(a,current.global,&sb.conditions,&local))return 0;
        write<std::uint8_t>(a,0xc30,1);
        if(!b->fail_select||!b->fail_select(b->context,a,local,current.has_task?current.task.id:0,read<int>(a,0x7b0),&id))return 0;
        out->failed=1;
    }else{
        ServerNpcConditionResult cr{};
        if(!ca->test(a,bytes,0x3f,NpcConditionCurrent,&sb.conditions,&cr))return 0;
        if(cr.value){id=0x47;out->forced=1;}
        else{
            int current=read<int>(a,0x850);
            if(current==3){
                const auto& e=b->state.entities;
                if(e.count>sin_independent::kEntityHandleCapacity||(e.count&&!e.entries))return 0;
                if(!sin_independent::EntityHandleView(e.entries,e.count).Resolve(read<std::uint32_t>(a,0x8a0))){
                    if(!state->apply(a,bytes,2,now,&b->state,&out->state))return 0;current=2;
                }
            }
            if(current==4||current==7||(b->jetpack&&!jet)){
                if(!b->special_select||!b->special_select(b->context,a,current,&id))return 0;
            }else if(jet){
                const auto* api=SiN_GetServerJetpackScheduleSelectorApi(1,sizeof(ServerJetpackSelectorApi));
                if(!api||!jr||!api->select(a,bytes,now,jet,jr))return 0;
                id=jr->schedule;
            }else if(slots){
                const auto* native=SiN_GetServerGruntNativeSlotSelectorApi(1,sizeof(ServerGruntNativeSlotSelectorApi));
                auto bound=b->grunt;bound.entities=b->state.entities;
                if(!native||!sr||!native->select(a,bytes,now,&bound,slots,sr))return 0;
                out->grunt=sr->selection;id=out->grunt.schedule;
            }else{
                const auto* grunt=SiN_GetServerGruntScheduleSelectorApi(1,sizeof(ServerGruntScheduleSelectorApi));
                if(!grunt||!grunt->select(a,bytes,&b->grunt,&out->grunt))return 0;id=out->grunt.schedule;
            }
            if(!ns->to_global(a,id,&sb.conditions,&out->ideal))return 0;
            write(a,0x79c,out->ideal);
        }
    }
    if(!resolve(a,id,*b,*out))return 0;
    const auto component=read<std::uint32_t>(a,0xc18);
    if(!read<std::uint8_t>(a,0xc30)&&component){
        if(!b->deactivate||!b->deactivate(b->context,a,component))return 0;
        out->component_deactivated=1;
    }
    return 1;
}
int __cdecl select(void* a,unsigned n,float now,bool failure,const ServerNpcScheduleResolutionBindings* b,ServerNpcScheduleResolutionResult* out) noexcept {
    return select_impl(a,n,now,failure,b,out);
}
int __cdecl select_jet(void* a,unsigned n,float now,bool failure,const ServerNpcScheduleResolutionBindings* b,const ServerJetpackSelectorBindings* jet,ServerNpcScheduleResolutionResult* out,ServerJetpackSelectorResult* jr) noexcept {
    if(!jr)return 0;*jr={};jr->schedule=-1;
    if(!a||n<0x1028||!b||!b->jetpack||!jet)return 0;
    auto bound=*jet;bound.grunt=b->grunt;bound.grunt.entities=b->state.entities;
    return select_impl(a,n,now,failure,b,out,&bound,jr);
}
int __cdecl select_slots(void* a,unsigned n,float now,bool failure,const ServerNpcScheduleResolutionBindings* b,const ServerNpcGruntSquadSlotsBindings* slots,ServerNpcScheduleResolutionResult* out,ServerGruntNativeSlotSelectorResult* sr) noexcept{
    if(!slots||!sr)return 0;*sr={};sr->slots.index=sr->slots.selected=-1;
    return select_impl(a,n,now,failure,b,out,nullptr,nullptr,slots,sr);
}
const ServerNpcNativeSlotResolutionApi slot_api{1,sizeof(ServerNpcNativeSlotResolutionApi),&select_slots};
const ServerJetpackScheduleResolutionApi jet_api{1,sizeof(ServerJetpackScheduleResolutionApi),&select_jet};
const ServerNpcScheduleResolutionApi api{1,sizeof(ServerNpcScheduleResolutionApi),&find,&select};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcScheduleResolutionApi=_SiN_GetServerNpcScheduleResolutionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleResolutionApi* __cdecl
SiN_GetServerNpcScheduleResolutionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcScheduleResolutionApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerJetpackScheduleResolutionApi=_SiN_GetServerJetpackScheduleResolutionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerJetpackScheduleResolutionApi* __cdecl
SiN_GetServerJetpackScheduleResolutionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerJetpackScheduleResolutionApi)?&jet_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeSlotResolutionApi=_SiN_GetServerNpcNativeSlotResolutionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeSlotResolutionApi* __cdecl SiN_GetServerNpcNativeSlotResolutionApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeSlotResolutionApi)?&slot_api:nullptr;
}
