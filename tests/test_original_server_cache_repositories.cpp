#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "sin_compat/original_server_cache_repositories.hpp"
namespace cache_repo = sin_compat::original_server_cache_repositories;
struct FakeHost {
    std::vector<std::string> actions;
    std::vector<std::uint32_t> header;
    std::size_t next = 0;
    std::uint32_t metadata=77;
    std::uintptr_t file=0x42;
    bool check_pass=true;
    void no_filename() { actions.push_back("missing-name"); }
    std::uint32_t compute_metadata(const cache_repo::RepositoryConfig& cfg) {
        actions.push_back("metadata:"+std::string(cfg.filename)); return metadata;
    }
    std::uintptr_t open_file(std::string_view path,std::string_view mode,std::string_view path_id) {
        assert(mode=="rb" && path_id=="MOD");
        actions.push_back("open:"+std::string(path));return file;
    }
    std::uint32_t read_dword(std::uintptr_t handle) {
        assert(handle==file && file);
        actions.push_back("read");
        return next<header.size()?header[next++]:0u;
    }
    void process_record(const cache_repo::RepositoryConfig&,std::uintptr_t handle,std::uint32_t i) {
        assert(handle==file);actions.push_back("record:"+std::to_string(i));
    }
    void reject(const cache_repo::RepositoryConfig&,cache_repo::Rejection reason) {
        actions.push_back("reject:"+std::to_string(static_cast<int>(reason)));
    }
    void close_file(std::uintptr_t handle) {
        assert(handle==file);actions.push_back("close");
    }
    void invalidate_file(std::string_view path,std::string_view id) {
        assert(id=="MOD");actions.push_back("invalidate:"+std::string(path));
    }
    void reset_invalid_cache(const cache_repo::RepositoryConfig&) {actions.push_back("reset-invalid");}
    void clear_repository_data(const cache_repo::RepositoryConfig&) {actions.push_back("clear");}
    bool reload_check(const cache_repo::RepositoryConfig&) {actions.push_back("check");return check_pass;}
    void rebuild_repository(const cache_repo::RepositoryConfig&) {actions.push_back("rebuild");}
    bool has(const std::string& value) const {
        for(const auto& a:actions) if(a==value)return true;
        return false;
    }
    std::size_t count(const std::string& value) const {
        std::size_t n=0;for(const auto& a:actions)if(a==value)++n;return n;
    }
};
int main() {
    assert(cache_repo::kScene.filename=="scene.cache");
    assert(cache_repo::kScene.expected_version==3u);
    assert(cache_repo::kModelSounds.filename=="modelsounds.cache");
    assert(cache_repo::kModelSounds.expected_version==2u);
    assert(cache_repo::kFileSystemSlot==0x225721A4u);
    for(const auto cfg:{cache_repo::kScene,cache_repo::kModelSounds}) {
        cache_repo::State state;FakeHost h;
        h.header={2u,cfg.expected_version,77u,3u};
        assert(cache_repo::initialize(cfg,state,h));
        assert(state.initialized && !state.cache_invalid_or_missing);
        assert(h.count("record:0")==1 && h.count("record:1")==1 && h.count("record:2")==1);
        assert(h.count("read")==4 && h.actions.back()=="close");
        const auto count=h.actions.size();
        assert(cache_repo::initialize(cfg,state,h)); // native no-op fast path
        assert(h.actions.size()==count);
    }
    {
        cache_repo::State state;FakeHost h;h.file=0;
        assert(cache_repo::initialize(cache_repo::kScene,state,h));
        assert(state.initialized && state.cache_invalid_or_missing);
        assert(!h.has("close") && !h.has("invalidate:scene.cache"));
    }
    {
        cache_repo::State state;FakeHost h;
        const cache_repo::RepositoryConfig missing{0,"",1,0,0};
        assert(!cache_repo::initialize(missing,state,h));
        assert(state.initialized && h.has("missing-name"));
        assert(cache_repo::initialize(missing,state,h)); // original stuck flag quirk
        assert(h.count("missing-name")==1);
    }
    for (int error=0;error<3;++error) {
        cache_repo::State state;FakeHost h;
        h.header={2,3,77,0};
        if(error==0)h.header[0]=7;
        if(error==1)h.header[1]=2;
        if(error==2)h.header[2]=99;
        assert(cache_repo::initialize(cache_repo::kScene,state,h));
        assert(h.has("reject:"+std::to_string(error)));
        assert(h.actions[h.actions.size()-2]=="close");
        assert(h.actions.back()=="invalidate:scene.cache");
        assert(state.cache_invalid_or_missing);
        assert(h.count("read")==static_cast<std::size_t>(error+1));
    }
    {
        FakeHost h;cache_repo::State state;h.metadata=0;
        h.header={2,3,0xDEADBEEFu,static_cast<std::uint32_t>(-1)};
        assert(cache_repo::initialize(cache_repo::kScene,state,h));
        assert(!h.has("invalidate:scene.cache") && !h.has("record:0"));
    }
    {
        FakeHost h;cache_repo::State state;h.header={2,3,77,100000};
        bool threw=false;
        try{(void)cache_repo::initialize(cache_repo::kScene,state,h,2);}catch(const std::length_error&){threw=true;}
        assert(threw); // host-only limit, not original branch
    }
    {
        FakeHost h;cache_repo::State state;state.initialized=true;
        state.cache_invalid_or_missing=true;h.file=0;
        assert(cache_repo::reload(cache_repo::kScene,state,h));
        assert(h.actions[0]=="reset-invalid" && h.actions[1]=="clear");
        assert(h.has("open:scene.cache"));
    }
    {
        FakeHost h;cache_repo::State state;state.initialized=true;
        state.reload_gate=true;h.check_pass=false;
        h.header={2,2,77,0};
        assert(cache_repo::reload(cache_repo::kModelSounds,state,h));
        assert(h.actions[0]=="check" && h.actions[1]=="rebuild" && h.actions[2]=="clear");
    }
    {
        FakeHost h;cache_repo::State state;state.initialized=true;
        state.reload_gate=true;h.header={2,2,77,0};
        assert(cache_repo::reload(cache_repo::kModelSounds,state,h));
        assert(h.actions[0]=="check" && h.actions[1]=="clear");
        assert(!h.has("rebuild"));
    }
    {
        FakeHost h;cache_repo::State state;h.header={2,2,77,0};
        assert(cache_repo::reload(cache_repo::kModelSounds,state,h));
        assert(!h.has("check") && !h.has("clear"));
    }
    return 0;
}