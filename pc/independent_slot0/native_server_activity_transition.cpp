#include "native_server_activity_transition.h"
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActivityAssignmentApi* __cdecl
SiN_GetServerNpcActivityAssignmentApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActivityTranslateApi* __cdecl
SiN_GetServerNpcActivityTranslateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityMetadataApi* __cdecl
SiN_GetServerActivityMetadataApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T> void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
int __cdecl choose(const ServerTransitionView* v,int from,int goal,int direction,bool forward_only,ServerTransitionChoice* out) noexcept {
    if(!out)return 0;*out={};out->sequence=goal;out->direction=forward_only?1:direction;
    if(!v)return 1;
    if(v->count>65536||v->node_count>65535||(v->count&&!v->sequences))return 0;
    if(from<0||goal<0||static_cast<unsigned>(from)>=v->count||static_cast<unsigned>(goal)>=v->count)return 1;
    const auto& source=v->sequences[from];const auto& target=v->sequences[goal];
    if(!source.entry_node||!target.entry_node){out->direction=1;return 1;}
    const unsigned start=out->direction>0?source.exit_node:source.entry_node;
    const unsigned end=target.entry_node;
    if(start==end){out->direction=1;return 1;}
    unsigned next=end;
    if(!v->composite){
        const auto size=static_cast<std::uint64_t>(v->node_count)*v->node_count;
        if(!start||start>v->node_count||end>v->node_count||!v->matrix||size>v->matrix_bytes)return 0;
        next=v->matrix[(start-1)*v->node_count+(end-1)];
        if(!next)return 1;
        if(next>v->node_count)return 0;
    }
    for(unsigned i=0;i<v->count;++i){
        ++out->scanned;const auto& candidate=v->sequences[i];
        if(candidate.entry_node==start&&candidate.exit_node==next){out->direction=1;out->sequence=static_cast<int>(i);return 1;}
        if(candidate.reversible&&candidate.exit_node==start&&candidate.entry_node==next){
            out->direction=-1;out->sequence=forward_only?-1:static_cast<int>(i);return 1;
        }
    }
    out->missing_edge=1;return 1;
}
bool valid(void* a,unsigned bytes,const ServerNpcTransitionBindings* b) noexcept {
    return a&&bytes>=0x90c&&b&&b->assignment.translation.metadata;
}
int metadata(void* a,const ServerNpcTransitionBindings& b,const ServerActivityCompositeView** out) noexcept {
    *out=nullptr;const auto& t=b.assignment.translation;
    const int status=t.metadata(t.context,a,out);
    if(status<0||(status>0&&!*out))return -1;
    if(!status)*out=nullptr;
    return status;
}
bool select_pending(void* a,unsigned bytes,int activity,const ServerNpcTransitionBindings& b,ServerNpcTransitionResult& out) noexcept {
    const auto* api=SiN_GetServerNpcActivityTranslateApi(1,sizeof(ServerNpcActivityTranslateApi));
    if(!api||!api->select(a,bytes,activity,&b.assignment.translation,&out.pending))return false;
    write(a,0x884,out.pending.sequence);write(a,0x888,out.pending.activity);write(a,0x88c,out.pending.translation.initial_weapon);
    out.reselected=1;return true;
}
int __cdecl set_pending(void* a,unsigned bytes,int activity,const ServerNpcTransitionBindings* b,ServerNpcTransitionResult* out) noexcept {
    if(!out)return 0;*out={};out->transition.sequence=-1;
    if(!valid(a,bytes,b))return 0;
    if(activity==2){out->skipped=1;return 1;}
    const auto* assignment=SiN_GetServerNpcActivityAssignmentApi(1,sizeof(ServerNpcActivityAssignmentApi));
    if(!assignment)return 0;
    if(!activity)return assignment->request(a,bytes,0,&b->assignment,&out->assignment);
    write(a,0x880,activity);
    if(activity==154){out->skipped=1;return 1;}
    const ServerActivityCompositeView* view=nullptr;
    const int available=metadata(a,*b,&view);
    if(available<0)return 0;
    if(!available){out->skipped=1;return 1;}
    return select_pending(a,bytes,activity,*b,*out);
}
int __cdecl update(void* a,unsigned bytes,const ServerNpcTransitionBindings* b,ServerNpcTransitionResult* out) noexcept {
    if(!out)return 0;*out={};out->transition.sequence=-1;
    if(!valid(a,bytes,b))return 0;
    int activity=read<int>(a,0x878),pending=read<int>(a,0x880);
    if(read<int>(a,0x8c)==2||(read<int>(a,0x850)==4&&activity!=2)||pending==154){out->skipped=1;return 1;}
    const ServerActivityCompositeView* view=nullptr;
    const int available=metadata(a,*b,&view);
    if(available<0)return 0;
    if(!available||(activity==pending&&read<int>(a,0x3ec)==read<int>(a,0x884))){out->skipped=1;return 1;}
    if(activity==2){if(!read<std::uint8_t>(a,0x3e0)){out->skipped=1;return 1;}}
    else if(!select_pending(a,bytes,pending,*b,*out))return 0;
    const auto* assignment=SiN_GetServerNpcActivityAssignmentApi(1,sizeof(ServerNpcActivityAssignmentApi));
    if(!assignment||!b->nodes)return 0;
    const ServerTransitionView* nodes=nullptr;
    const int status=b->nodes(b->context,a,&nodes);
    if(status<0||(status>0&&!nodes))return 0;
    if(!status)nodes=nullptr;
    const int from=read<int>(a,0x3ec),goal=read<int>(a,0x884);
    if(!choose(nodes,from,goal,1,true,&out->transition))return 0;
    if(out->transition.missing_edge&&b->missing_edge)b->missing_edge(b->context,a,from,goal);
    if(out->transition.sequence==-1){
        out->fallback_request=1;return assignment->request(a,bytes,read<int>(a,0x880),&b->assignment,&out->assignment);
    }
    if(out->transition.sequence==goal){
        const ServerNpcActivityAssignmentInput in{read<int>(a,0x880),goal,read<int>(a,0x888),read<int>(a,0x88c)};
        return assignment->assign(a,bytes,&in,&b->assignment,&out->assignment);
    }
    // 22049ce0: sequence activity query, then selection only to obtain
    // translated and weapon activities; retain the actual intermediate index.
    int translated=2,weapon=2;
    const int index=out->transition.sequence;
    const ServerActivityCompositeView* current=nullptr;
    const int have=metadata(a,*b,&current);
    if(have<0)return 0;
    if(have&&index>=0&&static_cast<unsigned>(index)<current->count){
        const auto* api=SiN_GetServerActivityMetadataApi(1,sizeof(ServerActivityMetadataApi));
        ServerSequenceChoiceEntry entry{};
        if(!api||!current->descriptors||!api->get(b->assignment.translation.activities,current->descriptors+index,&entry))return 0;
        if(entry.activity!=-1){
            const auto* translator=SiN_GetServerNpcActivityTranslateApi(1,sizeof(ServerNpcActivityTranslateApi));
            ServerNpcActivitySelectionResult mapped{};
            if(!translator||!translator->select(a,bytes,entry.activity,&b->assignment.translation,&mapped))return 0;
            translated=mapped.activity;weapon=mapped.translation.initial_weapon;
        }
    }
    out->intermediate=1;
    const ServerNpcActivityAssignmentInput in{2,index,translated,weapon};
    return assignment->assign(a,bytes,&in,&b->assignment,&out->assignment);
}
const ServerNpcTransitionApi api{1,sizeof(ServerNpcTransitionApi),&choose,&set_pending,&update};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcTransitionApi=_SiN_GetServerNpcTransitionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTransitionApi* __cdecl
SiN_GetServerNpcTransitionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcTransitionApi)?&api:nullptr;
}
