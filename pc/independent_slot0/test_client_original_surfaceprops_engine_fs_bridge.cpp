// MSVC x86 original 2006 surfaceprops VFileSystem012 ABI fixture.
// Pure synthetic paths/SurfaceData definitions, no commercial assets
// or actual game physics registration in public CI.
#include "client_original_surfaceprops_engine_fs_bridge.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
namespace b=sin_independent::surfaceprops_engine_fs;
namespace o=sin_independent::original_surfaceprops;
static int Fail(int n){
    std::fprintf(stderr,"source original surfaceprops engine filesystem=%d\n",n);
    return n;
}
static std::string Document(const char* prefix,int n,int bases){
    std::string output="// 2006 original-free fabricated surface keys\n";
    const bool first=std::strcmp(prefix,"retail")==0;
    for(int i=0;i<n;++i){
        const std::string name=first&&i==0?"default":
            std::string(prefix)+"_"+std::to_string(i);
        output+="\""+name+"\" {\n";
        if((first&&i>=1&&i<=bases)||(!first&&i<bases))
            output+="\"base\" \"default\"\n";
        output+="\"density\" \"1000\"\n\"friction\" \"0.3\"\n}\n";
    }
    return output;
}
struct Handle {
    const std::string* data=nullptr;
    std::size_t pos=0;
};
struct FSBase {
    std::map<std::string,std::string> files{};
    Handle current{};
    int opens=0,closes=0,reads=0,manifest_game=0,scripts_null=0,bad_abi=0;
    bool refuse_read=false,huge_size=false;
    std::string missing{};
    virtual int Read(void* dest,int wanted,void* h) {
        ++reads;
        if(refuse_read)return 0;
        if(h!=&current||!current.data||wanted<0||!dest){
            ++bad_abi;return 0;
        }
        const auto left=current.data->size()-current.pos;
        const auto n=std::min<std::size_t>(
            static_cast<std::size_t>(wanted),std::min<std::size_t>(left,27u));
        if(n)std::memcpy(dest,current.data->data()+current.pos,n);
        current.pos+=n;
        return static_cast<int>(n);
    }
    virtual int Write(const void*,int,void*){++bad_abi;return -1;}
    virtual void* Open(const char* path,const char* mode,const char* id) {
        ++opens;
        if(!path||!mode||std::strcmp(mode,"rb")!=0){
            ++bad_abi;return nullptr;
        }
        if(std::strcmp(path,"scripts/surfaceproperties_manifest.txt")==0){
            if(!id||std::strcmp(id,"GAME")!=0){
                ++bad_abi;return nullptr;
            }
            ++manifest_game;
        }else{
            if(id!=nullptr){
                ++bad_abi;return nullptr;
            }
            ++scripts_null;
        }
        if(missing==path)return nullptr;
        const auto it=files.find(path);
        if(it==files.end())return nullptr;
        current={&it->second,0};
        return &current;
    }
    virtual void Close(void* h) {
        ++closes;
        if(h!=&current||!current.data){++bad_abi;return;}
        current={};
    }
    virtual void Seek(void*,int,int){++bad_abi;}
    virtual unsigned Tell(void*){++bad_abi;return 0;}
    virtual unsigned Size(void* h) {
        if(h!=&current||!current.data){++bad_abi;return 0;}
        return huge_size?300000u:static_cast<unsigned>(current.data->size());
    }
    virtual unsigned Size(const char*,const char*){
        ++bad_abi;return 0;
    }
    virtual void Flush(void*){++bad_abi;}
    virtual bool Precache(const char*,const char*){
        ++bad_abi;return false;
    }
    virtual bool FileExists(const char*,const char*){
        ++bad_abi;return false;
    }
    virtual bool IsFileWritable(const char*,const char*){return false;}
    virtual bool SetFileWritable(const char*,bool,const char*){return false;}
    virtual long GetFileTime(const char*,const char*){return 0;}
};
struct AppSystem {
    virtual bool Connect(void*){return false;}
    virtual void Disconnect(){}
    virtual void* QueryInterface(const char*){return nullptr;}
    virtual int Init(){return 0;}
    virtual void Shutdown(){}
};
struct FileSystem : FSBase,AppSystem {};
int main(){
    static_assert(sizeof(void*)==4,"native Source 2004 x86");
    FileSystem engine{};
    const std::string manifest=
       "\"surfaceproperties_manifest\" { "
       "\"file\" \"scripts/surfaceproperties.txt\" "
       "\"file\" \"scripts/surfaceproperties_hl2.txt\" }";
    engine.files={
        {"scripts/surfaceproperties_manifest.txt",manifest},
        {"scripts/surfaceproperties.txt",Document("retail",84,62)},
        {"scripts/surfaceproperties_hl2.txt",Document("hl2",9,9)}
    };
    b::Bridge bridge{};
    auto report=bridge.ReadOnly(&engine);
    if(report.status!=b::Status::SourceShapeVerified||
       !report.verified() ||report.owned_result.attempted!=2 ||
       report.owned_result.read!=2 ||
       report.owned_result.definitions!=93 ||
       report.owned_result.base_keys!=71 ||
       report.owned_result.repeated_names!=0 ||
       bridge.snapshot().sources.size()!=2 ||
       bridge.snapshot().source_bytes==0 ||
       engine.opens!=3 ||engine.closes!=3||
       engine.manifest_game!=1 ||engine.scripts_null!=2||
       engine.bad_abi!=0 ||engine.reads<4)
        return Fail(1);

    report=bridge.ReadOnly(nullptr);
    if(report.status!=b::Status::InvalidFileSystem ||
       report.verified() ||bridge.snapshot().sources.size()!=2)
        return Fail(2);

    engine.missing="scripts/surfaceproperties_hl2.txt";
    const int closedBefore=engine.closes;
    report=bridge.ReadOnly(&engine);
    if(report.status!=b::Status::ManifestOrSourceRefused ||
       report.verified()||report.owned_result.attempted!=2 ||
       report.owned_result.read!=1 ||
       engine.closes!=closedBefore+2 ||
       bridge.snapshot().sources.size()!=2)
        return Fail(3);
    engine.missing.clear();

    engine.files["scripts/surfaceproperties_manifest.txt"]=
       "\"surfaceproperties_manifest\" { "
       "\"file\" \"scripts/../evil.txt\" }";
    report=bridge.ReadOnly(&engine);
    if(report.status!=b::Status::ManifestOrSourceRefused ||
       report.verified()||report.owned_result.attempted!=0 ||
       bridge.snapshot().sources.size()!=2)
        return Fail(4);
    engine.files["scripts/surfaceproperties_manifest.txt"]=manifest;

    engine.refuse_read=true;
    report=bridge.ReadOnly(&engine);
    if(report.status!=b::Status::ManifestOrSourceRefused ||
       report.verified()||bridge.snapshot().sources.size()!=2)
        return Fail(5);
    engine.refuse_read=false;
    engine.huge_size=true;
    report=bridge.ReadOnly(&engine);
    if(report.status!=b::Status::ManifestOrSourceRefused ||
       report.verified()||bridge.snapshot().sources.size()!=2)
        return Fail(6);
    engine.huge_size=false;

    report=bridge.ReadOnly(&engine);
    if(!report.verified()||engine.bad_abi!=0||
       bridge.snapshot().definitions!=93)return Fail(7);
    bridge.Clear();
    if(!bridge.snapshot().sources.empty())return Fail(8);
    std::puts("PASS original 2006 surfaceprops VFileSystem012 native: manifest rb/GAME, two scripts rb/nullptr, 93 independent defs and 71 base keys, rollback/unsafe/short-read/ABI refusal");
    return 0;
}
