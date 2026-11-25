#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "server_raw_scene_cache_owner.h"
#include "server_scene_directory_source.h"
using namespace sin_port::native_weapon_tables;
using sin_port::map_host::RawSceneCacheOwner;
#define CHECK(x) do { if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();} } while(0)
struct Source{
    unsigned reads=0;bool fail=false,missing=false,empty=false;
    static int read(void* p,const char* key,std::string& bytes,bool& found) noexcept{
        auto& s=*static_cast<Source*>(p);++s.reads;
        if(s.fail)return 0;
        found=!s.missing&&!std::strcmp(key,R"(scenes\root.vcd)");
        try{bytes=found&&!s.empty?"actor root":"";return 1;}catch(...){return 0;}
    }
};

static void directory_test(const SceneRawCacheEnsureApi& api){
    namespace fs=std::filesystem;
    using sin_port::map_host::SceneDirectorySource;
    struct Temp {
        fs::path path;
        ~Temp(){if(!path.empty()){std::error_code ec;fs::remove_all(path,ec);}}
    } temp;
    std::error_code ec;auto root=fs::temp_directory_path(ec);CHECK(!ec);
    for(unsigned i=0;i<256;++i){
        auto path=root/("sin-native-raw-cache-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(i));
        ec.clear();if(fs::create_directory(path,ec)){temp.path=path;break;}
        CHECK(!ec||ec==std::errc::file_exists);
    }
    CHECK(!temp.path.empty());CHECK(fs::create_directory(temp.path/"scenes"));
    auto write=[&](const char* name,const char* text){
        std::ofstream out(temp.path/"scenes"/name,std::ios::binary|std::ios::trunc);
        out<<text;out.close();CHECK(out.good());};
    write("Root.VCD","actor root");write("empty.vcd","");
    SceneDirectorySource reader;const auto dir=temp.path.string();CHECK(reader.open(dir.c_str()));
    RawSceneCacheOwner owner(api,{&reader,&SceneDirectorySource::read_raw});
    const auto& b=owner.bindings();auto p=b.context;std::uint32_t id=0,new_id=0;
    CHECK(b.ensure(p,"SCENES/ROOT.VCD")&&b.find(p,"scenes/root.vcd",&id)&&id!=0xffffffffu);
    const void* data=nullptr;unsigned n=0;
    CHECK(b.lock(p,id,&data,&n)&&n==10&&!std::memcmp(data,"actor root",10)&&b.unlock(p,id));
    write("Root.VCD","changed");
    CHECK(b.ensure(p,"scenes/root.vcd")&&b.lock(p,id,&data,&n)&&n==10&&b.unlock(p,id));
    CHECK(owner.evict("scenes/root.vcd")&&b.reload(p,"scenes/root.vcd",id,&new_id));
    CHECK(new_id!=id&&b.lock(p,new_id,&data,&n)&&n==7&&!std::memcmp(data,"changed",7)&&b.unlock(p,new_id));
    CHECK(b.ensure(p,"scenes/empty.vcd")&&b.find(p,"scenes/empty.vcd",&id)&&id!=0xffffffffu);
    CHECK(b.lock(p,id,&data,&n)&&data&&!n&&b.unlock(p,id));
    CHECK(b.ensure(p,"scenes/missing.vcd")&&b.find(p,"scenes/missing.vcd",&id)&&id==0xffffffffu);
    CHECK(!b.ensure(p,"scenes/../root.vcd")&&!b.ensure(p,R"(scenes\\root.vcd)"));
    CHECK(owner.clear());
}

int main(int argc,char** argv){
    CHECK(argc==2);auto dll=LoadLibraryA(argv[1]);CHECK(dll);
    const auto get=reinterpret_cast<GetSceneRawCacheEnsureApi>(GetProcAddress(dll,scene_raw_cache_ensure_export));
    CHECK(get);const auto* api=get(version,sizeof(SceneRawCacheEnsureApi));CHECK(api);
    {
        Source source;RawSceneCacheOwner owner(*api,{&source,&Source::read},16);
        const auto& b=owner.bindings();auto p=b.context;std::uint32_t id=0,other=0;
        CHECK(b.ensure(p,"Scenes/ROOT.vcd")&&owner.size()==1&&source.reads==1&&owner.resident_bytes()==10);
        CHECK(b.ensure(p,R"(scenes\root.vcd)")&&source.reads==1);
        CHECK(b.find(p,"Scenes/Root.VCD",&id)&&id!=0xffffffffu);
        const void* bytes=nullptr;unsigned n=0;
        CHECK(b.lock(p,id,&bytes,&n)&&bytes&&n==10&&!std::memcmp(bytes,"actor root",10));
        CHECK(!owner.evict("scenes/root.vcd")&&!owner.clear()&&!b.lock(p,id,&bytes,&n));
        CHECK(b.unlock(p,id)&&!b.unlock(p,id));
        CHECK(owner.evict("scenes/root.vcd")&&owner.resident_bytes()==0);
        CHECK(b.lock(p,id,&bytes,&n)&&!bytes&&!n);
        source.fail=true;other=99;CHECK(!b.reload(p,"scenes/root.vcd",id,&other)&&other==99);
        source.fail=false;
        CHECK(b.reload(p,"scenes/root.vcd",id,&other)&&other!=id&&owner.resident_bytes()==10);
        CHECK(!b.lock(p,id,&bytes,&n)&&b.lock(p,other,&bytes,&n)&&n==10&&b.unlock(p,other));
        CHECK(owner.clear()&&!owner.size()&&!owner.resident_bytes());
        source.missing=true;CHECK(b.ensure(p,"scenes/root.vcd")&&!owner.size());
        CHECK(b.find(p,"scenes/root.vcd",&id)&&id==0xffffffffu);
        source.missing=false;source.empty=true;
        CHECK(b.ensure(p,"scenes/root.vcd")&&owner.size()==1);
        CHECK(b.find(p,"scenes/root.vcd",&id)&&id!=other&&id!=0xffffffffu);
        CHECK(b.lock(p,id,&bytes,&n)&&bytes&&!n&&b.unlock(p,id));
        CHECK(owner.clear());
        Source too_big;RawSceneCacheOwner bounded(*api,{&too_big,&Source::read},9);
        const auto& x=bounded.bindings();
        CHECK(!x.ensure(x.context,"scenes/root.vcd")&&!bounded.size()&&!bounded.resident_bytes());
    }
    directory_test(*api);
    CHECK(FreeLibrary(dll));std::puts("PASS loaded native raw scene ensure with persistent host owner");
}
