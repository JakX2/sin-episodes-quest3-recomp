// Phase 484: actual Win32 MSVC VFileSystem012 slot ABI through a
// completely synthetic, independently owned filesystem object.
// Never loads a retail game DLL or supplies an untrusted engine pointer.
#include "client_soundscape_engine_fs_bridge.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <utility>

namespace b=sin_independent::soundscape_engine_fs;
namespace es=sin_independent::engine_fs;
static int Fail(int n){
    std::fprintf(stderr,"soundscape filesystem bridge fixture failed=%d\n",n);
    return n;
}
struct Handle {
    const std::string* source=nullptr;
    std::size_t at=0;
};
struct SyntheticFileSystemBase {
    std::map<std::string,std::string> resources;
    Handle current{};
    int open_count=0,read_count=0,close_count=0,abi_errors=0;
    bool short_read=false,huge_size=false;
    std::string missing_path{};
    virtual int Read(void* target,int requested,void* raw) {
        ++read_count;
        if(raw!=&current||!current.source||requested<0||
           (!target&&requested>0)){
            ++abi_errors;return 0;
        }
        if(short_read)return 0;
        const std::size_t remain=current.source->size()-current.at;
        const std::size_t count=std::min<std::size_t>(
            static_cast<std::size_t>(requested),std::min<std::size_t>(remain,9u));
        if(count>0)std::memcpy(
            target,current.source->data()+current.at,count);
        current.at+=count;
        return static_cast<int>(count);
    }
    virtual int Write(const void*,int,void*){++abi_errors;return -1;}
    virtual void* Open(const char* file,const char* mode,const char* pathId) {
        ++open_count;
        if(!file||!mode||std::strcmp(mode,"rb")!=0 ||
           !pathId||std::strcmp(pathId,"GAME")!=0){
            ++abi_errors;return nullptr;
        }
        if(missing_path==file)return nullptr;
        const auto found=resources.find(file);
        if(found==resources.end())return nullptr;
        current={&found->second,0};
        return &current;
    }
    virtual void Close(void* raw) {
        ++close_count;
        if(raw!=&current||!current.source){++abi_errors;return;}
        current={};
    }
    virtual void Seek(void*,int,int){++abi_errors;}
    virtual unsigned Tell(void*){++abi_errors;return 0;}
    virtual unsigned Size(void* raw) {
        if(raw!=&current||!current.source){++abi_errors;return 0;}
        return huge_size?300000u:static_cast<unsigned>(current.source->size());
    }
    virtual unsigned Size(const char*,const char*) {
        ++abi_errors;return 0;
    }
    virtual void Flush(void*){++abi_errors;}
    virtual bool Precache(const char*,const char*) {
        ++abi_errors;return false;
    }
    virtual bool FileExists(const char*,const char*) {
        // KeyValues::LoadFromFile opens directly; this probe never
        // requires preflight FileExists.
        ++abi_errors;return false;
    }
    virtual bool IsFileWritable(const char*,const char*){return false;}
    virtual bool SetFileWritable(const char*,bool,const char*){return false;}
    virtual long GetFileTime(const char*,const char*){return 0;}
};
struct SyntheticAppSystem {
    virtual bool Connect(void*){return false;}
    virtual void Disconnect(){}
    virtual void* QueryInterface(const char*){return nullptr;}
    virtual int Init(){return 0;}
    virtual void Shutdown(){}
};
struct FileSystem: SyntheticFileSystemBase,SyntheticAppSystem {};
int main(){
    static_assert(sizeof(void*)==4,"Source 2004 VFileSystem012 Win32 x86 only");
    FileSystem originalFree{};
    const std::string manifest=
        "soundscaples_manifest { "
        "\"file\" \"scripts/soundscapes.txt\" "
        "\"file\" \"scripts/soundscapes_intro.txt\" }";
    const std::string first=
        "\"A\" { \"playlooping\" { \"wave\" \"a.wav\" } "
        "\"playsoundscape\" { \"name\" \"B\" } }";
    const std::string second=
        "\"B\" { \"playrandom\" { \"rndwave\" { \"wave\" \"b.wav\" } } }";
    originalFree.resources={
        {"scripts/soundscapes_manifest.txt",manifest},
        {"scripts/soundscapes.txt",first},
        {"scripts/soundscapes_intro.txt",second}
    };

    b::Bridge bridge;
    if(bridge.Load(&originalFree)!=b::Status::OwnedCatalogReady ||
       bridge.scripts().files.size()!=2 ||
       bridge.catalog().counts.definitions!=2 ||
       bridge.catalog().counts.nested_soundscape_blocks!=1 ||
       bridge.catalog().counts.unresolved_references!=0 ||
       originalFree.open_count!=3||originalFree.close_count!=3||
       originalFree.abi_errors!=0 || originalFree.read_count<=3)
        return Fail(1);

    // Null ABI pointer is refused before a virtual call and never
    // resets a previously completed owned catalog.
    if(bridge.Load(nullptr)!=b::Status::InvalidEngineAbi ||
       bridge.catalog().counts.definitions!=2)return Fail(2);

    originalFree.missing_path="scripts/soundscapes_intro.txt";
    const int closeBefore=originalFree.close_count;
    if(bridge.Load(&originalFree)!=b::Status::ManifestOrDependencyRefused ||
       originalFree.close_count!=closeBefore+2 ||
       bridge.catalog().counts.definitions!=2 ||
       bridge.scripts().files.size()!=2)return Fail(3);
    originalFree.missing_path.clear();

    // Manifest-level byte stream corruption must not publish a
    // synthetic partial state.
    originalFree.resources["scripts/soundscapes_manifest.txt"]=
        "soundscaples_manifest { \"file\" \"../escape.txt\" }";
    if(bridge.Load(&originalFree)!=b::Status::ManifestOrDependencyRefused ||
       bridge.catalog().counts.definitions!=2)return Fail(4);
    originalFree.resources["scripts/soundscapes_manifest.txt"]=manifest;

    // Resource opened and read successfully but contains broken KV.
    originalFree.resources["scripts/soundscapes_intro.txt"]=
        "\"B\" { \"playrandom\" {";
    if(bridge.Load(&originalFree)!=b::Status::CatalogParseRefused ||
       bridge.catalog().counts.definitions!=2)return Fail(5);
    originalFree.resources["scripts/soundscapes_intro.txt"]=second;

    // The lower-level engine ABI fixture reads in bounded chunks.
    // A non-progress read is refused and the acquired handle closed.
    originalFree.short_read=true;
    if(bridge.Load(&originalFree)!=b::Status::ManifestReadRefused ||
       bridge.catalog().counts.definitions!=2)return Fail(6);
    originalFree.short_read=false;

    // A malicious length above existing lower-level 256KiB reader
    // cap also refuses before reading a partial payload.
    originalFree.huge_size=true;
    if(bridge.Load(&originalFree)!=b::Status::ManifestReadRefused ||
       bridge.catalog().counts.definitions!=2)return Fail(7);
    originalFree.huge_size=false;

    if(bridge.Load(&originalFree)!=b::Status::OwnedCatalogReady ||
       bridge.catalog().counts.definitions!=2 ||
       bridge.scripts().manifest.root!="soundscaples_manifest" ||
       originalFree.abi_errors!=0)return Fail(8);

    bridge.Clear();
    if(!bridge.scripts().files.empty() ||
       !bridge.catalog().definitions.empty())return Fail(9);
    std::puts("PASS Win32 synthetic VFileSystem012 rb/GAME soundscape resources: exact slots, chunk reads, 2-file graph and transactional rollback");
    return 0;
}
