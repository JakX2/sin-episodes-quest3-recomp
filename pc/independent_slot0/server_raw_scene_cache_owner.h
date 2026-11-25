#pragma once
#include "native_npc_scene_playback.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <utility>

namespace sin_port::map_host {
namespace raw_npc=sin_port::native_weapon_tables;
// Replacement resource owner: serialized host calls, not retail manager layout.
// Reader and loaded DLL API must outlive the owner. Borrowed bytes are valid
// until the matching unlock. The owner must outlive every binding borrower.
class RawSceneCacheOwner {
public:
    struct Reader {
        void* context=nullptr;
        int (*read)(void*,const char*,std::string&,bool&) noexcept=nullptr;
    };
    RawSceneCacheOwner(const raw_npc::SceneRawCacheEnsureApi& api,Reader reader,
                       std::size_t budget=64u*1024u*1024u):api_(api),reader_(reader),budget_(budget){
        auto& h=ensure_;h.context=this;
        h.symbol=[](void* p,const char* key,std::uint16_t* out) noexcept -> int{
            auto& o=self(p);
            try{
                auto it=o.symbols_.find(key);
                if(it!=o.symbols_.end()){*out=it->second;return 1;}
                if(o.symbols_.size()>=65535)return 0;
                const auto id=static_cast<std::uint16_t>(o.symbols_.size());
                o.symbols_.emplace(key,id);*out=id;return 1;
            }catch(...){return 0;}};
        h.find=[](void* p,std::uint16_t id,void** out,unsigned* n) noexcept -> int{
            auto& o=self(p);auto it=o.rows_.find(id);
            *out=it==o.rows_.end()?nullptr:it->second.data();*n=*out?24u:0u;return 1;};
        h.touch=[](void* p,std::uint32_t id) noexcept -> int{return self(p).resources_.count(id)!=0;};
        h.prepare=[](void*,unsigned n) noexcept -> int{return n==0x1390;};
        h.reserve=[](void* p,std::uint32_t* out) noexcept -> int{
            auto& o=self(p);if(o.next_==absent)return 0;*out=o.next_++;return 1;};
        h.create=[](void* p,std::uint32_t id,const char* key,std::uint32_t* out) noexcept -> int{
            auto& o=self(p);Resource data;
            if(!o.read(key,data)||!o.publish(id,std::move(data)))return 0;
            *out=id;return 1;};
        h.discard=[](void* p,std::uint32_t id) noexcept -> int{return self(p).discard(id);};
        h.insert=[](void* p,std::uint16_t id,std::uint32_t handle) noexcept -> int{
            auto& o=self(p);if(!o.resources_.count(handle))return 0;
            try{Row row{};std::memcpy(row.data()+0x14,&handle,4);return o.rows_.emplace(id,row).second;}
            catch(...){return 0;}};
        auto& b=bindings_;b.context=this;
        b.ensure=[](void* p,const char* path) noexcept -> int{return self(p).ensure(path);};
        b.find=[](void* p,const char* path,std::uint32_t* out) noexcept -> int{
            auto& o=self(p);if(o.busy_||!out)return 0;
            char key[256];if(!normalize(path,key))return 0;
            auto row=o.row(key);*out=row?handle(*row):absent;return 1;};
        b.lock=[](void* p,std::uint32_t id,const void** out,unsigned* n) noexcept -> int{
            auto& o=self(p);if(o.busy_||!out||!n)return 0;
            auto it=o.resources_.find(id);if(it==o.resources_.end()||it->second.locked)return 0;
            auto& data=it->second;
            if(!data.resident){*out=nullptr;*n=0;return 1;}
            if(!data.found)return 0;
            data.locked=true;*out=data.bytes.c_str();*n=static_cast<unsigned>(data.bytes.size());return 1;};
        b.reload=[](void* p,const char* path,std::uint32_t id,std::uint32_t* out) noexcept -> int{
            return self(p).reload(path,id,out);};
        b.unlock=[](void* p,std::uint32_t id) noexcept -> int{
            auto& o=self(p);if(o.busy_)return 0;
            auto it=o.resources_.find(id);if(it==o.resources_.end()||!it->second.locked)return 0;
            it->second.locked=false;return 1;};
    }
    RawSceneCacheOwner(const RawSceneCacheOwner&)=delete;
    RawSceneCacheOwner& operator=(const RawSceneCacheOwner&)=delete;
    const raw_npc::NamedSceneCacheBindings& bindings() const noexcept{return bindings_;}
    std::size_t resident_bytes() const noexcept{return bytes_;}
    std::size_t size() const noexcept{return rows_.size();}
    bool evict(const char* path) noexcept{
        char key[256];if(busy_||!normalize(path,key))return false;
        auto* r=row(key);if(!r)return false;
        auto it=resources_.find(handle(*r));if(it==resources_.end()||it->second.locked)return false;
        auto& data=it->second;bytes_-=data.bytes.size();std::string{}.swap(data.bytes);
        data.resident=false;return true;
    }
    bool clear() noexcept{
        if(busy_)return false;
        for(const auto& x:resources_)if(x.second.locked)return false;
        resources_.clear();rows_.clear();symbols_.clear();bytes_=0;
        // Never recycle handles during this owner's lifetime.
        return true;
    }
private:
    static constexpr std::uint32_t absent=0xffffffffu;
    using Row=std::array<unsigned char,24>;
    struct Resource {std::string bytes;bool resident=true,found=false,locked=false;};
    struct Busy {bool& flag;explicit Busy(bool& f):flag(f){flag=true;}~Busy(){flag=false;}};
    static RawSceneCacheOwner& self(void* p){return *static_cast<RawSceneCacheOwner*>(p);}
    static std::uint32_t handle(const Row& row) noexcept{
        std::uint32_t out;std::memcpy(&out,row.data()+0x14,4);return out;}
    // Native ensure normalizes case/slashes too. Refuse truncation at this
    // host boundary rather than aliasing distinct oversized input names.
    static bool normalize(const char* path,char (&key)[256]) noexcept{
        if(!path)return false;
        for(unsigned i=0;i<256;++i){
            char c=path[i];if(!c){key[i]=0;return i!=0;}
            if(c=='/')c='\\';if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');key[i]=c;
        }
        return false;
    }
    Row* row(const char* key) noexcept{
        // Avoid allocations on lookup (the map uses transparent comparison).
        auto s=symbols_.find(key);if(s==symbols_.end())return nullptr;
        auto it=rows_.find(s->second);return it==rows_.end()?nullptr:&it->second;
    }
    bool read(const char* key,Resource& data) noexcept{
        if(!reader_.read||!reader_.read(reader_.context,key,data.bytes,data.found))return false;
        if(!data.found){data.bytes.clear();return true;}
        return data.bytes.size()<=0xfffffffeu&&data.bytes.size()<=budget_-bytes_;
    }
    bool publish(std::uint32_t id,Resource&& data) noexcept{
        const auto n=data.bytes.size();
        try{if(!resources_.emplace(id,std::move(data)).second)return false;}
        catch(...){return false;}
        bytes_+=n;return true;
    }
    bool discard(std::uint32_t id) noexcept{
        auto it=resources_.find(id);if(it==resources_.end()||it->second.locked)return false;
        bytes_-=it->second.bytes.size();resources_.erase(it);return true;
    }
    int ensure(const char* path) noexcept{
        char key[256];
        if(busy_||!normalize(path,key)||api_.version!=raw_npc::version||
           api_.size!=sizeof(api_)||!api_.ensure||!reader_.read)return 0;
        Busy busy(busy_);
        if(api_.ensure(&ensure_,key)!=1)return 0;
        auto* r=row(key);if(!r)return 0;
        const auto id=handle(*r);auto it=resources_.find(id);
        if(it==resources_.end())return 0;
        // Missing files are not negative-cached. A later ensure can retry.
        if(it->second.resident&&!it->second.found){
            if(!discard(id))return 0;
            rows_.erase(symbols_.find(key)->second);
        }
        return 1;
    }
    int reload(const char* path,std::uint32_t old,std::uint32_t* out) noexcept{
        char key[256];if(busy_||!out||!normalize(path,key))return 0;
        auto* r=row(key);if(!r||handle(*r)!=old)return 0;
        auto it=resources_.find(old);
        if(it==resources_.end()||it->second.resident||it->second.locked||next_==absent)return 0;
        Busy busy(busy_);Resource data;
        if(!read(key,data))return 0;
        // A normal source miss is not a transport failure. The named loader
        // treats the absent sentinel as an ordinary missing scene. Retire
        // the old evicted handle, allowing a subsequent ensure to retry.
        if(!data.found){
            if(!discard(old))return 0;
            rows_.erase(symbols_.find(key)->second);*out=absent;return 1;
        }
        const auto id=next_++;
        if(!publish(id,std::move(data)))return 0;
        std::memcpy(r->data()+0x14,&id,4);(void)discard(old);*out=id;return 1;
    }
    const raw_npc::SceneRawCacheEnsureApi& api_;
    Reader reader_;
    std::size_t budget_,bytes_=0;
    std::uint32_t next_=1;
    std::map<std::string,std::uint16_t,std::less<>> symbols_;
    std::map<std::uint16_t,Row> rows_;
    std::map<std::uint32_t,Resource> resources_;
    raw_npc::SceneRawCacheEnsureBindings ensure_{};
    raw_npc::NamedSceneCacheBindings bindings_{};
    bool busy_=false;
};
}
