#pragma once
#include "native_server_vfe_cache.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
inline void check_server_vfe_cache(const ServerVfeCacheApi& api){
    struct Fixture {
        alignas(4) unsigned char source[128]{},copies[2][128]{};
        unsigned reads{},closes{},allocs{},frees{};bool missing{},bad{};
        char path[160]{};
    } f;
    std::memcpy(f.source,"EFV",4);const int length=128;std::memcpy(f.source+0x48,&length,4);
    ServerVfeCacheEntry entries[2]{};ServerVfeCache cache{entries,0,2};
    ServerVfeCacheBindings h{};h.context=&f;
    h.read=[](void* p,const char* path,const unsigned char** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.reads;std::strcpy(f.path,path);
        if(f.missing)return 0;
        *out=f.source;*n=sizeof(f.source);return 1;
    };
    h.close=[](void* p,const unsigned char* source) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(source==f.source);++f.closes;return 1;
    };
    h.allocate=[](void* p,unsigned n,void** out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);if(n!=128||f.allocs>=2)return 0;*out=f.copies[f.allocs++];return 1;
    };
    h.release=[](void* p,void* q) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(q==f.copies[0]||q==f.copies[1]);++f.frees;return 1;
    };
    ServerExpressionBank a{},b{};
    assert(api.load(&cache,&h,"phonemes",&a)==1&&a.data==f.copies[0]&&a.size==128);
    assert(!std::strcmp(f.path,"expressions/phonemes.vfe")&&f.reads==1&&f.closes==1&&f.allocs==1&&cache.count==1);
    a.data[0x70]=42;
    assert(api.load(&cache,&h,"PHONEMES",&b)==1&&a.data==b.data&&b.data[0x70]==42&&f.reads==1);
    f.missing=true;
    assert(api.load(&cache,&h,"random",&b)==0&&!b.data&&!b.size&&cache.count==1);
    assert(api.load(&cache,&h,"random",&b)==0&&f.reads==3&&f.closes==1);
    f.missing=false;
    assert(api.load(&cache,&h,"random",&b)==1&&b.data==f.copies[1]&&cache.count==2);
    assert(api.load(&cache,&h,"other",&a)==-1&&a.data==f.copies[0]&&cache.count==2&&f.closes==3);
    assert(api.clear(&cache,&h)&&!cache.count&&f.frees==2);
    f.source[0]=0;assert(api.load(&cache,&h,"broken",&a)==-1&&!cache.count&&f.closes==4&&f.allocs==2);
}
}
