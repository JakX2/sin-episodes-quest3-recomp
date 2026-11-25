#include "native_server_activity_metadata.h"
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityDictionaryApi* __cdecl
SiN_GetServerActivityDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEventDictionaryApi* __cdecl
SiN_GetServerEventDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerLazySequenceChoiceApi* __cdecl
SiN_GetServerLazySequenceChoiceApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const unsigned char* p,unsigned off) noexcept {T v{};std::memcpy(&v,p+off,sizeof(v));return v;}
template<class T> void write(unsigned char* p,unsigned off,T v) noexcept {std::memcpy(p+off,&v,sizeof(v));}
bool descriptor_ok(const ServerActivityDescriptor* d) noexcept {
    return d&&d->storage&&d->offset<=d->bytes&&d->bytes-d->offset>=0x20;
}
bool resolve_name(const ServerActivityDescriptor& d,const char*& name) noexcept {
    auto* data=static_cast<unsigned char*>(d.storage);
    const auto at=static_cast<std::int64_t>(d.offset)+read<std::int32_t>(data+d.offset,8);
    if(at<0||static_cast<std::uint64_t>(at)>=d.bytes)return false;
    const auto* str=data+at;
    const auto* end=static_cast<const unsigned char*>(std::memchr(str,0,d.bytes-static_cast<unsigned>(at)));
    if(!end||end-str>=128)return false;
    if(at<static_cast<std::int64_t>(d.offset)+0x20&&at+(end-str)+1>d.offset)return false;
    for(auto p=str;p<end;++p)if(*p>=128)return false;
    name=reinterpret_cast<const char*>(str);return true;
}
bool register_impl(ServerActivityDictionary* dict,const ServerActivityDescriptor* d,ServerSequenceChoiceEntry* out,bool force) noexcept {
    if(!out||!dict||!descriptor_ok(d))return false;
    auto* p=static_cast<unsigned char*>(d->storage)+d->offset;
    if(force||!(p[0x0d]&0x10)){
        const char* name=nullptr;
        if(!resolve_name(*d,name))return false;
        p[0x0d]|=0x10;
        if(*name){
            const auto* api=SiN_GetServerActivityDictionaryApi(1,sizeof(ServerActivityDictionaryApi));
            int id=-1;
            if(!api||!api->lookup(dict,name,&id))return false;
            if(id==-1&&!api->private_id(dict,name,&id))return false;
            write(p,0x10,id);
        }
    }
    *out={read<int>(p,0x10),read<int>(p,0x14)};return true;
}
int __cdecl register_activity(ServerActivityDictionary* dict,const ServerActivityDescriptor* d,ServerSequenceChoiceEntry* out) noexcept {
    return register_impl(dict,d,out,true);
}
int __cdecl get(ServerActivityDictionary* dict,const ServerActivityDescriptor* d,ServerSequenceChoiceEntry* out) noexcept {
    return register_impl(dict,d,out,false);
}
int __cdecl refresh(ServerActivityDictionary* activities,ServerEventDictionary* events,const ServerActivityMetadataView* v,ServerActivityMetadataResult* out) noexcept {
    if(!out)return 0;*out={};out->failed_index=-1;
    if(!activities||!events||&activities->names==events||!v||!v->generation||v->count>65536||(v->count&&!v->descriptors))return 0;
    const auto generation=activities->names.generation;
    if(*v->generation==generation)return 1;
    const auto* dictionary=SiN_GetServerEventDictionaryApi(1,sizeof(ServerEventDictionaryApi));
    if(!dictionary)return 0;
    // Reject invalid activity-name offsets before the first mutation.
    for(unsigned i=0;i<v->count;++i){
        const auto& d=v->descriptors[i];const char* name=nullptr;
        if(!descriptor_ok(&d)||!resolve_name(d,name)){out->failed_index=static_cast<int>(i);return 0;}
    }
    for(unsigned i=0;i<v->count;++i){
        const auto& d=v->descriptors[i];auto* p=static_cast<unsigned char*>(d.storage)+d.offset;
        out->failed_index=static_cast<int>(i);
        const char* name=nullptr;
        if(!resolve_name(d,name))return 0;
        const bool named=*name!=0;
        ServerSequenceChoiceEntry entry{};
        if(!register_activity(activities,&d,&entry))return 0;
        if(named)++out->activity_writes;
        ServerStudioEventRegistrationResult result{};
        if(!dictionary->apply(events,d.storage,d.bytes,d.offset,&result))return 0;
        ++out->visited;out->events_existing+=result.existing;out->events_created+=result.created;
    }
    *v->generation=generation;out->refreshed=1;out->failed_index=-1;return 1;
}
bool group_valid(const ServerActivityCompositeView* v) noexcept {
    if(!v||!v->groups||!v->group_count||v->group_count>4096||v->count>65536||(v->count&&!v->descriptors))return false;
    for(unsigned i=0;i<v->group_count;++i){
        const auto& g=v->groups[i];
        if(!g.word||(i==0?g.parent!=-1:(g.parent<0||static_cast<unsigned>(g.parent)>=i)))return false;
        for(unsigned j=0;j<i;++j)if(g.word==v->groups[j].word)return false;
    }
    return true;
}
int __cdecl generation(const ServerActivityCompositeView* v,std::uint32_t* out) noexcept {
    if(!out||!group_valid(v))return 0;
    auto value=*v->groups[0].word;
    for(unsigned i=1;i<v->group_count;++i){
        if(v->groups[i].parent!=0)continue;
        const auto candidate=*v->groups[i].word;
        // signed original JL; use sign-bit bias to avoid implementation-defined cast
        if((candidate^0x80000000u)<(value^0x80000000u))value=candidate;
    }
    *out=value;return 1;
}
struct Scan {
    ServerActivityDictionary* activities;
    const ServerActivityCompositeView* view;
    ServerActivityChoiceResult* result;
};
int __cdecl scan_entry(void* context,unsigned i,ServerSequenceChoiceEntry* entry) noexcept {
    auto& s=*static_cast<Scan*>(context);
    const auto& d=s.view->descriptors[i];
    s.result->failed_index=static_cast<int>(i);
    if(!descriptor_ok(&d))return 0;
    const auto* p=static_cast<const unsigned char*>(d.storage)+d.offset;
    const bool lazy=!(p[0xd]&0x10);
    if(!get(s.activities,&d,entry))return 0;
    ++s.result->scanned;if(lazy)++s.result->lazy_registrations;
    return 1;
}
int __cdecl choose_activity(ServerActivityDictionary* a,ServerEventDictionary* e,const ServerActivityCompositeView* v,
    int activity,int current,bool looping,const ServerSequenceChoiceBindings* b,ServerActivityChoiceResult* out) noexcept {
    if(!out)return 0;*out={};out->selected=-1;out->failed_index=-1;out->refresh.failed_index=-1;
    if(!v){out->selected=0;return 1;}
    if(!a||!e||&a->names==e)return 0;
    std::uint32_t gen=0;
    if(!generation(v,&gen))return 0;
    ServerActivityMetadataView single{v->descriptors,v->count,&gen};
    if(!refresh(a,e,&single,&out->refresh)){out->failed_index=out->refresh.failed_index;return 0;}
    if(out->refresh.refreshed)
        for(unsigned i=0;i<v->group_count;++i)*v->groups[i].word=gen;
    const auto* choice=SiN_GetServerLazySequenceChoiceApi(1,sizeof(ServerLazySequenceChoiceApi));
    if(!choice)return 0;
    Scan context{a,v,out};
    ServerLazySequenceChoiceView entries{&context,v->count,&scan_entry};
    if(!choice->choose(&entries,activity,current,looping,b,&out->selected))return 0;
    out->failed_index=-1;return 1;
}
const ServerActivityChoiceApi choice_api{1,sizeof(ServerActivityChoiceApi),&generation,&choose_activity};
const ServerActivityMetadataApi api{1,sizeof(ServerActivityMetadataApi),&register_activity,&get,&refresh};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerActivityMetadataApi=_SiN_GetServerActivityMetadataApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityMetadataApi* __cdecl
SiN_GetServerActivityMetadataApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerActivityMetadataApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerActivityChoiceApi=_SiN_GetServerActivityChoiceApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityChoiceApi* __cdecl
SiN_GetServerActivityChoiceApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerActivityChoiceApi)?&choice_api:nullptr;
}
