#include "native_server_activity_dictionary.h"
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEventDictionaryApi* __cdecl
SiN_GetServerEventDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
const ServerEventDictionaryApi* backend() noexcept {
    return SiN_GetServerEventDictionaryApi(1,sizeof(ServerEventDictionaryApi));
}
int __cdecl lookup(const ServerActivityDictionary* d,const char* name,int* id) noexcept {
    const auto* api=backend();if(!api||!d){if(id)*id=-1;return 0;}return api->lookup(&d->names,name,id);
}
int __cdecl shared(ServerActivityDictionary* d,const char* name,int id) noexcept {
    const auto* api=backend();if(!api||!d)return 0;
    // 220e6f60 writes 224e887c before checking duplicate name/ID.
    d->last_shared_request=id;return api->shared(&d->names,name,id,0);
}
int __cdecl private_id(ServerActivityDictionary* d,const char* name,int* id) noexcept {
    const auto* api=backend();if(!api||!d){if(id)*id=-1;return 0;}return api->private_id(&d->names,name,id);
}
int __cdecl clear(ServerActivityDictionary* d) noexcept {
    const auto* api=backend();return api&&d&&api->clear(&d->names);
}
int __cdecl reset_highest(ServerActivityDictionary* d) noexcept {
    const auto* api=backend();return api&&d&&api->reset_highest(&d->names);
}
bool good_name(const char* s) noexcept {
    if(!s||!*s)return false;
    for(unsigned i=0;i<128;++i){const unsigned char c=s[i];if(!c)return true;if(c>=128)return false;}return false;
}
unsigned char fold(unsigned char c) noexcept {return c>='A'&&c<='Z'?static_cast<unsigned char>(c+32):c;}
bool equal(const char* a,const char* b) noexcept {
    for(unsigned i=0;i<128;++i){const auto x=fold(a[i]),y=fold(b[i]);if(x!=y)return false;if(!x)return true;}return false;
}
int __cdecl seed(ServerActivityDictionary* d,const ServerActivitySeed* entries,unsigned count) noexcept {
    if(!d||!entries||count!=921||d->names.count||d->names.capacity<count||
       d->names.capacity>65535||!d->names.entries||d->names.highest<0)return 0;
    // Validate the complete ordered seed before mutation. Seed names must
    // be borrowed independently of the destination, stable until return.
    for(unsigned i=0;i<count;++i){
        if(entries[i].id!=static_cast<int>(i)||!good_name(entries[i].name))return 0;
        for(unsigned j=0;j<i;++j)if(equal(entries[i].name,entries[j].name))return 0;
    }
    for(unsigned i=0;i<count;++i)if(!shared(d,entries[i].name,entries[i].id))return 0;
    return 1;
}
const ServerActivityDictionaryApi api{1,sizeof(ServerActivityDictionaryApi),&lookup,&shared,&private_id,&clear,&reset_highest,&seed};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerActivityDictionaryApi=_SiN_GetServerActivityDictionaryApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityDictionaryApi* __cdecl
SiN_GetServerActivityDictionaryApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerActivityDictionaryApi)?&api:nullptr;
}
