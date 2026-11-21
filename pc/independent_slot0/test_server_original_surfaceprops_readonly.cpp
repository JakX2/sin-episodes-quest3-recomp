// Phase 521 original-free native MSVC x86 fake-server Physics Factory +
// VFileSystem012 bridge. No original assets, physics mutators or game boot.
#include "server_original_surfaceprops_readonly.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
namespace s=sin_independent::server_original_surfaceprops_readonly;
namespace f=sin_independent::server_original_physics_factory;
static int Fail(int n){std::fprintf(stderr,"phase521 fake server filesystem test: %d\n",n);return n;}
static std::string Document(const char* prefix,int count,int bases){
    std::string txt="// fake original-free 2006 property definitions\n";
    const bool first=std::strcmp(prefix,"retail")==0;
    for(int i=0;i<count;++i){
        const std::string name=first&&i==0?"default":
            std::string(prefix)+"_"+std::to_string(i);
        txt+="\""+name+"\" {\n";
        if((first&&i>=1&&i<=bases)||(!first&&i<bases))
            txt+="\"base\" \"default\"\n";
        txt+="\"density\" \"1000\"\n}\n";
    }
    return txt;
}
struct FileHandle {const std::string* value=nullptr;std::size_t offset=0;};
struct Filesystem {
    std::map<std::string,std::string> values{};
    FileHandle handle{};
    int open=0,close=0,read=0,manifest=0,sources=0,bad=0;
    bool refuse=false;
    virtual int Read(void* dest,int amount,void* token){
        ++read;
        if(refuse||token!=&handle||!handle.value||!dest||amount<0) {
            ++bad;return 0;
        }
        const auto left=handle.value->size()-handle.offset;
        const auto step=std::min<std::size_t>(static_cast<std::size_t>(amount),std::min<std::size_t>(left,27u));
        if(step)std::memcpy(dest,handle.value->data()+handle.offset,step);
        handle.offset+=step;
        return static_cast<int>(step);
    }
    virtual int Write(const void*,int,void*){++bad;return -1;}
    virtual void* Open(const char* path,const char* mode,const char* group){
        ++open;
        if(!path||!mode||std::strcmp(mode,"rb")!=0){++bad;return nullptr;}
        if(std::strcmp(path,"scripts/surfaceproperties_manifest.txt")==0){
            if(!group||std::strcmp(group,"GAME")!=0){++bad;return nullptr;}
            ++manifest;
        } else {
            if(group!=nullptr){++bad;return nullptr;}
            ++sources;
        }
        auto it=values.find(path);
        if(it==values.end())return nullptr;
        handle={&it->second,0};return &handle;
    }
    virtual void Close(void* h){
        ++close;
        if(h!=&handle||!handle.value){++bad;return;}
        handle={};
    }
    virtual void Seek(void*,int,int){++bad;}
    virtual unsigned Tell(void*){++bad;return 0;}
    // Real VFileSystem012 slot 6 is the PATH overload; slot 7 is
    // Size(handle). This exact overload order is original-owner tested
    // and crucial to the MSVC Win32 fake-engine vtable fixture.
    virtual unsigned Size(const char*,const char*){++bad;return 0;}
    virtual unsigned Size(void* h){
        if(h!=&handle||!handle.value){++bad;return 0;}
        return static_cast<unsigned>(handle.value->size());
    }
    virtual void Flush(void*){++bad;}
    virtual bool Precache(const char*,const char*){++bad;return false;}
    virtual bool FileExists(const char*,const char*){++bad;return false;}
};
static int requests=0;
static int unavailable=-1;
static bool correct_order=true;
static void* __cdecl Factory(const char* name,int* code){
    if(code!=nullptr||requests>=3||
       std::strcmp(name,f::kNames[requests])!=0)correct_order=false;
    const int which=requests++;
    if(which==unavailable)return nullptr;
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(which+1));
}
static void Reset(){requests=0;unavailable=-1;correct_order=true;}
int main(){
    static_assert(sizeof(void*)==4,"original x86 server ABI");
    Filesystem io{};
    const std::string manifest=
        "\"surfaceproperties_manifest\" { "
        "\"file\" \"scripts/surfaceproperties.txt\" "
        "\"file\" \"scripts/surfaceproperties_hl2.txt\" }";
    io.values={
        {"scripts/surfaceproperties_manifest.txt",manifest},
        {"scripts/surfaceproperties.txt",Document("retail",84,62)},
        {"scripts/surfaceproperties_hl2.txt",Document("hl2",9,9)}
    };
    const auto result=s::Inspect(&Factory,&io);
    if(!result.verified()||result.outcome!=s::Outcome::Verified||
       !correct_order||requests!=3||io.open!=3||io.close!=3||
       io.manifest!=1||io.sources!=2||io.bad!=0||io.read<4)
        return Fail(1);
    Reset();unavailable=1;
    const int opens=io.open;
    const auto missing=s::Inspect(&Factory,&io);
    if(missing.outcome!=s::Outcome::NoPhysics ||
       requests!=2||!correct_order||io.open!=opens)return Fail(2);
    Reset();
    const auto nofs=s::Inspect(&Factory,nullptr);
    if(nofs.outcome!=s::Outcome::NoFileSystem||
       requests!=3||!correct_order)return Fail(3);
    io.values["scripts/surfaceproperties_manifest.txt"]=
        "\"surfaceproperties_manifest\" { \"file\" \"../evil.txt\" }";
    Reset();
    const auto malformed=s::Inspect(&Factory,&io);
    if(malformed.outcome!=s::Outcome::InvalidSources||
       malformed.verified()||io.bad!=0)return Fail(4);
    io.values["scripts/surfaceproperties_manifest.txt"]=manifest;
    Reset();
    const auto recovered=s::Inspect(&Factory,&io);
    if(!recovered.verified()||!correct_order)return Fail(5);
    std::puts("PASS native fake original server: 3/3 physics factory, rb/GAME and rb/nullptr 2:2:93:71:0, missing physics, invalid ABI, refused manifest, recovery");
    return 0;
}
