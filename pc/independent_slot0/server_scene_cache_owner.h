#pragma once
#include "native_jessica_script.h"
#include "scene_cache_codec.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace sin_port::map_host {
namespace cache_npc=sin_port::native_weapon_tables;
// Replacement host owner, not a claim of binary-compatible CUtlRBTree
// storage. Native index/metadata policies are executed through exports.
// All calls are serialized; external callbacks and APIs outlive this owner.
class SceneCacheOwner {
public:
    using Services=cache_npc::SceneResponseServicesBindings;
    struct Leaves {
        Services external{};
        int (__cdecl *filename)(void*,const char*,std::uint32_t*) noexcept=nullptr;
        // Consume vector storage on success, retain on failure.
        int (__cdecl *release_vector)(void*,std::uint32_t) noexcept=nullptr;
        std::uint32_t metadata_vtable=0;
        unsigned stamp_mode=1;
    };
    struct CodecLeaves {
        void* context=nullptr;
        int (__cdecl *sound_index)(void*,const char*,std::uint32_t*) noexcept=nullptr;
        int (__cdecl *sound_name)(void*,std::uint32_t,const char**) noexcept=nullptr;
        int (__cdecl *grow)(void*,void* vector,unsigned count) noexcept=nullptr;
        int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept=nullptr;
    };
    SceneCacheOwner(const cache_npc::SceneMetadataIndexApi& api,const Leaves& leaves):
        api_(api),leaves_(leaves){
        put(cache_.data(),0x48,leaves.stamp_mode);
        auto& i=index_;i.context=this;i.metadata_vtable=leaves.metadata_vtable;
        i.filename=[](void* p,const char* path,std::uint32_t* out) noexcept -> int{
            auto& o=self(p);return o.leaves_.filename(o.leaves_.external.context,path,out);};
        i.find=[](void* p,std::uint32_t id,std::uint16_t* out) noexcept -> int{
            auto& o=self(p);const auto it=o.lookup_.find(id);*out=it==o.lookup_.end()?0xffff:it->second;return 1;};
        i.row=[](void* p,std::uint16_t id,void** out,unsigned* n) noexcept -> int{
            auto& o=self(p);if(id>=o.rows_.size())return 0;*out=o.rows_[id].bytes.data();*n=16;return 1;};
        i.allocate=[](void*,unsigned n,void** out,unsigned* bytes) noexcept -> int{
            if(n!=0x1c)return 0;auto* data=new(std::nothrow) unsigned char[n];
            if(!data)return 0;*out=data;*bytes=n;return 1;};
        i.release=[](void*,void* data) noexcept -> int{delete[] static_cast<unsigned char*>(data);return 1;};
        i.append=[](void* p,void* data,std::uint32_t* out) noexcept -> int{
            auto& o=self(p);if(o.metadata_.size()>=65535)return 0;
            try{o.metadata_.emplace_back();}catch(...){return 0;}
            o.metadata_.back().reset(static_cast<unsigned char*>(data));*out=static_cast<std::uint32_t>(o.metadata_.size()-1);return 1;};
        i.undo_append=[](void* p,std::uint32_t slot,void* data) noexcept -> int{
            auto& o=self(p);if(o.metadata_.empty()||slot!=o.metadata_.size()-1||o.metadata_.back().get()!=data)return 0;
            (void)o.metadata_.back().release();o.metadata_.pop_back();return 1;};
        i.insert=[](void* p,const void* payload,unsigned n,std::uint16_t* out) noexcept -> int{
            auto& o=self(p);if(n!=16||!o.pending_||o.rows_.size()>=65535)return 0;
            const auto key=word(payload,0),slot=word(payload,12);
            if(o.lookup_.count(key)||slot>=o.metadata_.size())return 0;
            try{
                Row row;row.path=o.pending_;std::memcpy(row.bytes.data(),payload,16);
                const auto id=static_cast<std::uint16_t>(o.rows_.size());o.rows_.push_back(std::move(row));
                try{if(!o.lookup_.emplace(key,id).second){o.rows_.pop_back();return 0;}}
                catch(...){o.rows_.pop_back();return 0;}
                *out=id;return 1;
            }catch(...){return 0;}};
        auto& b=bindings_;b.context=this;b.cache=cache_.data();b.cache_bytes=static_cast<unsigned>(cache_.size());
        b.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{
            auto& o=self(p);return o.ready()&&o.leaves_.external.storage(o.leaves_.external.context,w,out,n);};
        b.model_name=[](void* p,void* npc,const char** out) noexcept -> int{
            auto& o=self(p);return o.ready()&&o.leaves_.external.model_name(o.leaves_.external.context,npc,out);};
        b.expand=[](void* p,const char* model,const char* name,char* out,unsigned n) noexcept -> int{
            auto& o=self(p);return o.ready()&&o.leaves_.external.expand(o.leaves_.external.context,model,name,out,n);};
        b.row=[](void* p,const char* name,void** out,unsigned* n) noexcept -> int{return self(p).ensure(name,out,n);};
        b.stamp=[](void* p,const char* name,unsigned mode,std::uint32_t* out) noexcept -> int{
            auto& o=self(p);return o.ready()&&o.leaves_.external.stamp(o.leaves_.external.context,name,mode,out);};
        b.metadata=[](void* p,std::uint32_t slot,void** out,unsigned* n) noexcept -> int{
            auto& o=self(p);if(!o.ready()||slot>=o.metadata_.size()||!o.metadata_[slot])return 0;
            *out=o.metadata_[slot].get();*n=0x1c;return 1;};
        b.refresh=[](void* p,void* meta,const char* path) noexcept -> int{
            auto& o=self(p);if(!o.ready()||o.refreshing_)return 0;
            o.refreshing_=true;const int status=o.leaves_.external.refresh(o.leaves_.external.context,meta,path);o.refreshing_=false;return status;};
    }
    SceneCacheOwner(const SceneCacheOwner&)=delete;
    SceneCacheOwner& operator=(const SceneCacheOwner&)=delete;
    ~SceneCacheOwner(){(void)clear();}
    bool ready()const noexcept{
        const auto& h=leaves_.external;
        return !closing_&&api_.version==cache_npc::version&&api_.size==sizeof(api_)&&api_.ensure&&leaves_.metadata_vtable&&
            leaves_.filename&&leaves_.release_vector&&h.storage&&h.model_name&&h.expand&&h.stamp&&h.refresh;
    }
    const Services& bindings()const noexcept{return bindings_;}
    std::size_t size()const noexcept{return rows_.size();}
    bool dirty()const noexcept{return cache_[0x3c]!=0;}
    // Presence is a filename-handle lookup only. Do not allocate a
    // metadata row or refresh it as a side effect of checking presence.
    bool contains(const char* name,unsigned* present) noexcept{
        if(!ready()||pending_||refreshing_||!name||!present)return false;
        std::uint32_t key=0;if(!leaves_.filename(leaves_.external.context,name,&key))return false;
        *present=lookup_.find(key)!=lookup_.end();return true;
    }
    bool invalidate(const char* name) noexcept{
        if(!ready()||pending_||refreshing_||!name)return false;
        std::uint32_t key=0;if(!leaves_.filename(leaves_.external.context,name,&key))return false;
        const auto it=lookup_.find(key);if(it==lookup_.end())return false;
        put(rows_[it->second].bytes.data(),8,0xfffffffeu);return true;
    }
    // Import only into an empty owner, before NPCs borrow metadata.
    // Unknown sound names are skipped like original 221ab0a0. The first
    // subsequent admission resolves current stamps against imported stamps.
    // On failure clear the partial cache; failed cleanup freezes access.
    bool import_repository(const scene_cache::Repository& in,const CodecLeaves& codec) noexcept{
        if(!ready()||pending_||refreshing_||!rows_.empty()||!metadata_.empty()||
           !codec.sound_index||!codec.grow||!codec.storage||in.records.size()>scene_cache::max_records)return false;
        const auto fail=[&](){(void)clear();return false;};
        try{
            for(const auto& item:in.records){
                if(!scene_cache::detail::valid_name(item.path)||item.sounds.size()>65535)return fail();
                std::vector<std::uint32_t> ids;ids.reserve(item.sounds.size());
                for(const auto& name:item.sounds){
                    std::uint32_t id=0xffffffffu;
                    if(!scene_cache::detail::valid_name(name)||!codec.sound_index(codec.context,name.c_str(),&id))return fail();
                    if(id!=0xffffffffu)ids.push_back(id);
                }
                void* row=nullptr;unsigned n=0;const auto count=rows_.size();
                if(!ensure(item.path.c_str(),&row,&n)||n!=16||rows_.size()!=count+1)return fail(); // duplicate filename handle
                const auto slot=word(row,12);
                if(slot>=metadata_.size()||!metadata_[slot])return fail();
                auto* meta=metadata_[slot].get();put(meta,4,item.duration_bits);
                if(!ids.empty()){
                    if(!codec.grow(codec.context,meta+8,static_cast<unsigned>(ids.size()))||
                       word(meta,0x14)||word(meta,0xc)<ids.size())return fail();
                    const auto w=word(meta,8);void* dest=nullptr;unsigned extent=0;
                    if(!w||!codec.storage(codec.context,w,&dest,&extent)||!dest||extent<ids.size()*4)return fail();
                    std::memcpy(dest,ids.data(),ids.size()*4);put(meta,0x14,static_cast<std::uint32_t>(ids.size()));put(meta,0x18,w);
                }
                put(row,4,item.saved_stamp);put(row,8,0xfffffffeu);
            }
            return true;
        }catch(...){return fail();}
    }
    // Snapshot names instead of runtime indices; caller encodes/writes
    // atomically. Preserve existing output on failure.
    bool snapshot(std::uint32_t checksum,const CodecLeaves& codec,scene_cache::Repository& out) const noexcept{
        if(!ready()||pending_||refreshing_||!codec.sound_name||!codec.storage)return false;
        try{
            scene_cache::Repository tmp;tmp.checksum=checksum;tmp.records.reserve(rows_.size());
            for(const auto& row:rows_){
                const auto slot=word(row.bytes.data(),12);
                if(slot>=metadata_.size()||!metadata_[slot])return false;
                const auto* meta=metadata_[slot].get();scene_cache::Record item;
                item.path=row.path;item.saved_stamp=word(row.bytes.data(),4);item.duration_bits=word(meta,4);
                const auto count=word(meta,0x14),capacity=word(meta,0xc);
                if(count>65535||count>capacity)return false;
                if(count){
                    void* data=nullptr;unsigned extent=0;
                    if(!codec.storage(codec.context,word(meta,8),&data,&extent)||!data||extent<count*4)return false;
                    item.sounds.reserve(count);
                    for(unsigned i=0;i<count;++i){
                        const char* name=nullptr;
                        if(!codec.sound_name(codec.context,word(data,i*4),&name)||!name)return false;
                        std::string copied(name);if(!scene_cache::detail::valid_name(copied))return false;
                        item.sounds.push_back(std::move(copied));
                    }
                }
                tmp.records.push_back(std::move(item));
            }
            out=std::move(tmp);return true;
        }catch(...){return false;}
    }
    // Borrowed row/metadata pointers become invalid on successful clear.
    // A failed vector release freezes all access; caller must retry clear
    // before destroying the owner or external allocator.
    bool clear() noexcept{
        if(pending_||refreshing_)return false;
        closing_=true;
        for(auto& m:metadata_)if(m){
            const auto w=word(m.get(),8);
            if(w&&(!leaves_.release_vector||!leaves_.release_vector(leaves_.external.context,w)))return false;
            m.reset();
        }
        rows_.clear();lookup_.clear();metadata_.clear();cache_.fill(0);put(cache_.data(),0x48,leaves_.stamp_mode);closing_=false;return true;
    }
private:
    struct Row {alignas(4) std::array<unsigned char,16> bytes{};std::string path;};
    static SceneCacheOwner& self(void* p){return *static_cast<SceneCacheOwner*>(p);}
    static std::uint32_t word(const void* p,unsigned off) noexcept{std::uint32_t x;std::memcpy(&x,static_cast<const unsigned char*>(p)+off,4);return x;}
    static void put(void* p,unsigned off,std::uint32_t x) noexcept{std::memcpy(static_cast<unsigned char*>(p)+off,&x,4);}
    int ensure(const char* path,void** out,unsigned* bytes) noexcept{
        if(!ready()||pending_||refreshing_||!path)return 0;
        pending_=path;const int status=api_.ensure(&index_,path,out,bytes);pending_=nullptr;return status;
    }
    const cache_npc::SceneMetadataIndexApi& api_;
    Leaves leaves_;
    alignas(4) std::array<unsigned char,0x50> cache_{};
    std::deque<Row> rows_;
    std::map<std::uint32_t,std::uint16_t> lookup_;
    std::vector<std::unique_ptr<unsigned char[]>> metadata_;
    cache_npc::SceneMetadataIndexBindings index_{};
    Services bindings_{};
    const char* pending_=nullptr;
    bool closing_=false,refreshing_=false;
};
}
