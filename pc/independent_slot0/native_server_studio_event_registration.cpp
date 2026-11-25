#include "native_server_studio_event_registration.h"
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const unsigned char* p,unsigned off) noexcept {
    T v{};std::memcpy(&v,p+off,sizeof(v));return v;
}
template<class T> void write(unsigned char* p,unsigned off,T value) noexcept {
    std::memcpy(p+off,&value,sizeof(value));
}
bool range(std::int64_t offset,std::uint64_t length,unsigned bytes) noexcept {
    return offset>=0&&static_cast<std::uint64_t>(offset)<=bytes&&length<=bytes-static_cast<std::uint64_t>(offset);
}
bool overlap(std::int64_t a,std::uint64_t an,std::int64_t b,std::uint64_t bn) noexcept {
    return a<b+static_cast<std::int64_t>(bn)&&b<a+static_cast<std::int64_t>(an);
}
int __cdecl apply(void* storage,unsigned bytes,unsigned desc,const ServerStudioEventRegistrationBindings* b,ServerStudioEventRegistrationResult* out) noexcept {
    if(!out)return 0;
    *out={};out->failed_index=-1;
    if(!storage||!range(desc,0x20,bytes))return 0;
    auto* data=static_cast<unsigned char*>(storage);
    auto* descriptor=data+desc;
    const int count=read<int>(descriptor,0x18);
    if(count>65536)return 0;
    if(count<=0){descriptor[0x0d]|=0x20;return 1;}
    const auto first=static_cast<std::int64_t>(desc)+read<std::int32_t>(descriptor,0x1c);
    const auto event_bytes=static_cast<std::uint64_t>(count)*0x50;
    if(!range(first,event_bytes,bytes)||overlap(first,event_bytes,desc,0x20))return 0;
    // All names must be stable terminated strings, disjoint from the
    // mutable descriptor prefix and event records. Fail before mutation
    // for malformed offsets; original assumes validated studio metadata.
    for(int i=0;i<count;++i){
        auto* record=data+first+i*0x50;
        if(!(read<unsigned>(record,8)&0x400))continue;
        out->failed_index=i;
        const auto name_offset=first+i*0x50+read<std::int32_t>(record,0x4c);
        if(!b||!b->lookup||!b->register_new||!b->type_flags||!range(name_offset,1,bytes))return 0;
        const auto* name=data+name_offset;
        const auto* end=static_cast<const unsigned char*>(std::memchr(name,0,bytes-static_cast<unsigned>(name_offset)));
        if(!end)return 0;
        const auto length=static_cast<std::uint64_t>(end-name)+1;
        if(overlap(name_offset,length,first,event_bytes)||overlap(name_offset,length,desc,0x20))return 0;
    }
    descriptor[0x0d]|=0x20;
    for(int i=0;i<count;++i){
        auto* record=data+first+i*0x50;
        out->failed_index=i;++out->visited;
        const auto type=read<unsigned>(record,8);
        if(!(type&0x400))continue;
        ++out->named;
        const auto offset=first+i*0x50+read<std::int32_t>(record,0x4c);
        const char* name=reinterpret_cast<const char*>(data+offset);
        int id=-1;
        if(!b->lookup(b->context,name,&id))return 0;
        if(id==-1){
            if(!b->register_new(b->context,name,&id)||id<0)return 0;
            write(record,4,id);++out->created;
        }else{
            if(id<0)return 0;
            write(record,4,id);
            unsigned flags=0;
            if(!b->type_flags(b->context,id,&flags))return 0;
            write(record,8,type|flags);++out->existing;
        }
    }
    out->failed_index=-1;return 1;
}
const ServerStudioEventRegistrationApi api{1,sizeof(ServerStudioEventRegistrationApi),&apply};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerStudioEventRegistrationApi=_SiN_GetServerStudioEventRegistrationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerStudioEventRegistrationApi* __cdecl
SiN_GetServerStudioEventRegistrationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerStudioEventRegistrationApi)?&api:nullptr;
}
