// Phase 486: real compiled Win32 VFileSystem012 fake-engine acceptance
// of the EXACT target original 7/301/242/3 source inventory counts.
// No retail executable, original scripts or game engine in this test.
#include "client_soundscape_retail_gate.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace r=sin_independent::soundscape_retail_gate;
namespace fs=sin_independent::soundscape_engine_fs;
struct Handle final {
    const std::string* data=nullptr;
    std::size_t cursor=0;
};
struct IFileSystemFixture {
    std::map<std::string,std::string> files{};
    Handle file{};
    int open_count=0,close_count=0,read_count=0,abi_failures=0;
    std::string missing{};
    virtual int Read(void* output,int size,void* handle) {
        ++read_count;
        if(handle!=&file||!file.data||size<0){
            ++abi_failures;return 0;
        }
        const auto remaining=file.data->size()-file.cursor;
        const auto count=std::min<std::size_t>(
            static_cast<std::size_t>(size),
            std::min<std::size_t>(remaining,17));
        if(count)std::memcpy(output,file.data->data()+file.cursor,count);
        file.cursor+=count;
        return static_cast<int>(count);
    }
    virtual int Write(const void*,int,void*){++abi_failures;return 0;}
    virtual void* Open(const char* path,const char* mode,const char* id) {
        ++open_count;
        if(!path||!mode||!id||std::strcmp(mode,"rb")!=0 ||
           std::strcmp(id,"GAME")!=0){++abi_failures;return nullptr;}
        if(path==missing)return nullptr;
        const auto it=files.find(path);
        if(it==files.end())return nullptr;
        file={&it->second,0};return &file;
    }
    virtual void Close(void* handle){
        ++close_count;
        if(handle!=&file||!file.data){++abi_failures;return;}
        file={};
    }
    virtual void Seek(void*,int,int){++abi_failures;}
    virtual unsigned Tell(void*){++abi_failures;return 0;}
    virtual unsigned Size(void* handle){
        if(handle!=&file||!file.data){++abi_failures;return 0;}
        return static_cast<unsigned>(file.data->size());
    }
    virtual unsigned Size(const char*,const char*){++abi_failures;return 0;}
    virtual void Flush(void*){++abi_failures;}
    virtual bool Precache(const char*,const char*){++abi_failures;return false;}
    virtual bool FileExists(const char*,const char*){++abi_failures;return false;}
    virtual bool IsFileWritable(const char*,const char*){return false;}
    virtual bool SetFileWritable(const char*,bool,const char*){return false;}
    virtual long GetFileTime(const char*,const char*){return 0;}
};
struct AppSystemFixture {
    virtual bool Connect(void*){return false;}
    virtual void Disconnect(){}
    virtual void* QueryInterface(const char*){return nullptr;}
    virtual int Init(){return 0;}
    virtual void Shutdown(){}
};
struct FileSystem final: IFileSystemFixture,AppSystemFixture {};
static int Fail(int n){
    std::fprintf(stderr,"owner retail soundscape gate synthetic fixture failed %d\n",n);
    return n;
}
int main(){
    static_assert(sizeof(void*)==4,"Win32 Source 2004 ABI");
    FileSystem engine{};
    const char* paths[]{
        "scripts/soundscapes.txt",
        "scripts/soundscapes_intro.txt",
        "scripts/soundscapes_docks.txt",
        "scripts/soundscapes_druglab.txt",
        "scripts/soundscapes_pit.txt",
        "scripts/soundscapes_highrise.txt",
        "scripts/soundscapes_finale.txt"
    };
    std::string manifest="soundscaples_manifest { ";
    for(const char* path:paths){
        manifest+="\"file\" \""+std::string(path)+"\" ";
        engine.files[path]="";
    }
    manifest+="}";
    engine.files["scripts/soundscapes_manifest.txt"]=manifest;
    unsigned next=0;
    for(const char* path:paths){
        std::string& body=engine.files[path];
        for(unsigned j=0;j<43;++j){
            const unsigned id=next++;
            body+="\"S"+std::to_string(id)+"\" { ";
            if(id==0){
                for(int i=0;i<239;++i)
                    body+="\"playsoundscape\" { \"name\" \"S1\" } ";
                for(int i=0;i<3;++i)
                    body+="\"playsoundscape\" { \"name\" \"absent_"+std::to_string(i)+"\" } ";
            } else {
                body+="\"playlooping\" { \"wave\" \"synthetic.wav\" } ";
            }
            body+="}\n";
        }
    }
    if(next!=301)return Fail(1);
    auto r1=r::Verify(&engine);
    if(!r1.verified() || r1.status!=r::Status::CatalogVerified ||
       r1.bridge_result!=fs::Status::OwnedCatalogReady ||
       r1.counts.files!=7 || r1.counts.definitions!=301 ||
       r1.counts.nested_references!=242 ||
       r1.counts.unresolved_references!=3 ||
       engine.open_count!=8 || engine.close_count!=8 ||
       engine.abi_failures!=0)return Fail(2);

    // A changed yet well-formed catalog must produce a distinct
    // count-mismatch result, never false-positive retail proof.
    const std::string prior=engine.files[paths[1]];
    engine.files[paths[1]]="\"single\" { \"playlooping\" { \"wave\" \"x.wav\" } }";
    auto less=r::Verify(&engine);
    if(less.status!=r::Status::OriginalCountsMismatch ||
       less.counts.definitions!=259 || less.verified() ||
       engine.abi_failures!=0)return Fail(3);
    engine.files[paths[1]]=prior;

    // Owner missing original resource: refuse without deref, and all
    // handles opened so far must still have been closed.
    engine.missing=paths[6];
    const auto beforeOpen=engine.open_count;
    const auto beforeClose=engine.close_count;
    auto noFile=r::Verify(&engine);
    if(noFile.status!=r::Status::FilesystemOrParseRefused ||
       noFile.bridge_result!=fs::Status::ManifestOrDependencyRefused ||
       noFile.verified() ||
       engine.open_count-beforeOpen!=8 ||
       engine.close_count-beforeClose!=7)return Fail(4);
    engine.missing.clear();

    auto noEngine=r::Verify(nullptr);
    if(noEngine.status!=r::Status::FilesystemOrParseRefused ||
       noEngine.bridge_result!=fs::Status::InvalidEngineAbi ||
       noEngine.counts.files!=0)return Fail(5);

    if(!r::ValidateExpectedRetail({7,301,242,3},"soundscaples_manifest") ||
       r::ValidateExpectedRetail({7,301,242,2},"soundscaples_manifest") ||
       r::ValidateExpectedRetail({7,300,242,3},"soundscaples_manifest") ||
       r::ValidateExpectedRetail({7,301,242,3},"soundscapes_manifest"))
        return Fail(6);

    // A fully restored synthetic fixture can retry deterministically.
    auto again=r::Verify(&engine);
    if(!again.verified()||engine.abi_failures!=0)return Fail(7);
    std::puts("PASS synthetic native Source2004 VFileSystem012 -> 7 files, 301 defs, 242 refs, 3 unresolved; fail-closed count/path checks and 8 handled file closures");
    return 0;
}
