// SDK2004 VFileSystem012 original-free MSVC x86 executable test.
#include "client_engine_filesystem_read.h"
#include "client_stats_coordinator.h"
#include <type_traits>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdint>
namespace fs=sin_independent::engine_fs;
namespace co=sin_independent::stats_coordinator;
static void* expectedStatsContext=nullptr;
static_assert(std::is_same<co::AssetReader,decltype(&fs::ReadForStatsCoordinator)>::value,
    "Engine-backed stats reader must match injected coordinator ABI");
static int existsCalls=0,openCalls=0,closeCalls=0,readCalls=0;
static bool missingDefs=false,missingLayout=false,shortRead=false,hugeSize=false;
static bool rejectOpen=false;
static bool failLayoutRead=false, overlongLayoutRead=false, failLayoutOpen=false;
static int layoutReadCalls=0;
static std::string defs,layout;
struct Handle {const std::string* bytes=nullptr;std::size_t cursor=0;};
static Handle handle;
struct BaseFileSystem {
    // SDK IBaseFileSystem has NO instance fields: secondary IAppSystem
    // must start immediately after this base's 32-bit primary vptr.
    virtual int Read(void* output,int size,void* file) {
        ++readCalls;
        auto* h=static_cast<Handle*>(file);
        if(shortRead || (failLayoutRead && h->bytes==&layout))return 0;
        if(h->bytes==&layout) {
            ++layoutReadCalls;
            if(overlongLayoutRead)return size+1;
        }
        const auto amount=std::min<std::size_t>(
            static_cast<std::size_t>(size),h->bytes->size()-h->cursor);
        // Deliberate partial reads exercise our loop.
        const auto take=std::min<std::size_t>(amount,11);
        std::memcpy(output,h->bytes->data()+h->cursor,take);
        h->cursor+=take;
        return static_cast<int>(take);
    }
    virtual int Write(const void*,int,void*){return -1;}
    virtual void* Open(const char* path,const char* mode,const char* pathId) {
        ++openCalls;
        if(!mode||std::strcmp(mode,"rb")||pathId||rejectOpen ||
           (failLayoutOpen && !std::strcmp(path,fs::kLayout)))return nullptr;
        if(!std::strcmp(path,fs::kDefinitions))handle.bytes=&defs;
        else if(!std::strcmp(path,fs::kLayout))handle.bytes=&layout;
        else return nullptr;
        handle.cursor=0;
        return &handle;
    }
    virtual void Close(void* file) {
        ++closeCalls;
        if(file!=&handle)std::abort();
    }
    virtual void Seek(void*,int,int){}
    virtual unsigned Tell(void*){return 0;}
    virtual unsigned Size(void* file) {
        if(file!=&handle)std::abort();
        const auto result=hugeSize?300000u:static_cast<unsigned>(handle.bytes->size());
        std::fprintf(stderr,"fixture Size(handle) value=%u huge=%d\n",result,hugeSize?1:0);
        return result;
    }
    virtual unsigned Size(const char*,const char*) {
        std::fprintf(stderr,"fixture Size(path) unexpectedly called\n");
        return 0;
    }
    virtual void Flush(void*){}
    virtual bool Precache(const char*,const char*){return false;}
    virtual bool FileExists(const char* path,const char* pathID) {
        ++existsCalls;
        if(pathID)std::abort();
        if(!std::strcmp(path,fs::kDefinitions))return !missingDefs;
        if(!std::strcmp(path,fs::kLayout))return !missingLayout;
        std::abort();
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
struct FileSystem:BaseFileSystem,AppSystem {};
static std::string Definitions(int count) {
    std::string result="\"StatDefinitions\" {\n";
    for(int i=0;i<count;++i) {
        result+="\"stat\" { \"name\" \"STAT_"+std::to_string(i);
        result+="\" \"short_text\" \"#s\" \"long_text\" \"#l\"";
        if(i>=1&&i<=8)result+=" \"percentage_of\" \"STAT_0\"";
        result+=" }\n";
    }
    return result+"}\n";
}
static std::string Layout(int count) {
    std::string result;
    for(int i=0;i<156;++i)
        result+="\"STAT_"+std::to_string(i%count)+"\" \"value\"\n";
    for(int i=0;i<66;++i)result+="\"==========================\"\n";
    return result;
}
static int coordinatorCalls=0;
static bool ResourceForCoordinator(void* context,sin_independent::stats_catalog::Catalog& c) noexcept {
    return context==expectedStatsContext && c.size()==66;
}
static bool LaterForCoordinator(void*,const sin_independent::stats_system::Step&) noexcept {
    ++coordinatorCalls;
    return true;
}
static int Fail(int i) {std::fprintf(stderr,"engine-fs-fixture step=%d\n",i);return i;}
int main() {
    FileSystem owner;
    if(reinterpret_cast<std::uintptr_t>(static_cast<AppSystem*>(&owner))-
       reinterpret_cast<std::uintptr_t>(&owner)!=sizeof(void*))return Fail(1);
    if(fs::Presence(nullptr)!=fs::Status::InvalidAbi)return Fail(2);
    fs::Methods methods;
    if(!fs::Resolve(&owner,methods,true)||!methods.exists||!methods.open||
       !methods.close||!methods.size||!methods.read)return Fail(3);
    defs=Definitions(62);layout=Layout(62);
    if(fs::Presence(&owner)!=fs::Status::BothPresent||existsCalls!=2||
       openCalls!=0||readCalls!=0)return Fail(4);
    missingDefs=true;
    if(fs::Presence(&owner)!=fs::Status::MissingDefinitions ||
       fs::ReadAndValidate(&owner)!=fs::Status::MissingDefinitions ||
       openCalls!=0)return Fail(5);
    missingDefs=false;missingLayout=true;
    if(fs::Presence(&owner)!=fs::Status::MissingLayout ||
       fs::ReadAndValidate(&owner)!=fs::Status::MissingLayout)return Fail(6);
    missingLayout=false;
    fs::Counts counts{};
    const auto verified=fs::ReadAndValidate(&owner,&counts);
    if(verified!=fs::Status::ParsedAndVerified||
       counts.definitions!=62||counts.refs!=8||counts.pairs!=156||
       counts.dividers!=66||openCalls!=closeCalls||readCalls<10) {
        std::fprintf(stderr,
            "read=%s counts=%u,%u,%u,%u io=%d/%d/%d exists=%d\n",
            fs::Name(verified),static_cast<unsigned>(counts.definitions),
            static_cast<unsigned>(counts.refs),static_cast<unsigned>(counts.pairs),
            static_cast<unsigned>(counts.dividers),openCalls,closeCalls,
            readCalls,existsCalls);
        return Fail(7);
    }
    const int closed=closeCalls;
    hugeSize=true;
    if(fs::ReadAndValidate(&owner)!=fs::Status::ReadRefused ||
       closeCalls!=closed+1)return Fail(8);
    hugeSize=false;
    shortRead=true;
    if(fs::ReadAndValidate(&owner)!=fs::Status::ReadRefused ||
       closeCalls!=closed+2)return Fail(9);
    shortRead=false;rejectOpen=true;
    if(fs::ReadAndValidate(&owner)!=fs::Status::ReadRefused ||
       closeCalls!=closed+2)return Fail(10);
    rejectOpen=false;
    defs=Definitions(61);layout=Layout(61);
    if(fs::ReadAndValidate(&owner,&counts)!=fs::Status::UnexpectedCounts ||
       counts.definitions!=61 || openCalls==0 || closeCalls!=openCalls-1)return Fail(11);
    defs=Definitions(62);layout=Layout(62);
    if(fs::ReadAndValidate(&owner)!=fs::Status::ParsedAndVerified ||
       closeCalls!=openCalls-1)return Fail(12);
    std::string providedDefinitions="retain-old-defs";
    std::string providedLayout="retain-old-layout";
    std::string error="previous";
    if(!fs::ReadForStatsCoordinator(&owner,providedDefinitions,providedLayout,error) ||
       providedDefinitions!=defs || providedLayout!=layout || !error.empty() ||
       openCalls!=closeCalls+1)return Fail(13);
    const auto previousDefinitions=providedDefinitions;
    const auto previousLayout=providedLayout;
    shortRead=true;
    if(fs::ReadForStatsCoordinator(&owner,providedDefinitions,providedLayout,error) ||
       error!="read-refused" ||
       providedDefinitions!=previousDefinitions ||
       providedLayout!=previousLayout ||
       openCalls!=closeCalls+1)return Fail(14);
    shortRead=false;missingDefs=true;
    if(fs::ReadForStatsCoordinator(&owner,providedDefinitions,providedLayout,error) ||
       error!="missing-definitions" ||
       providedDefinitions!=previousDefinitions ||
       providedLayout!=previousLayout)return Fail(15);
    missingDefs=false;
    co::State state;
    int statsOwner=1,registry=2;
    expectedStatsContext=&statsOwner;
    co::Hooks hooks{};
    hooks.context=&statsOwner;
    hooks.asset_reader_context=&owner;
    hooks.load_catalog_resources=&ResourceForCoordinator;
    hooks.run_unreconstructed_step=&LaterForCoordinator;
    hooks.read_engine_documents=&fs::ReadForStatsCoordinator;
    coordinatorCalls=0;
    auto run=co::Initialize(state,&statsOwner,&registry,hooks);
    if(!run.succeeded() || run.completed!=7 || run.attempted!=7 ||
       state.scripts.definitions().size()!=62 ||
       state.scripts.display().size()!=222 ||
       coordinatorCalls!=4)return Fail(16);
    shortRead=true;coordinatorCalls=0;
    run=co::Initialize(state,&statsOwner,&registry,hooks);
    if(run.outcome!=co::Outcome::AssetLoadRefused ||
       run.completed!=3||run.attempted!=4||coordinatorCalls!=0 ||
       state.scripts.definitions().size()!=62 ||
       state.scripts.display().size()!=222 ||
       state.last_script_error!="read-refused")return Fail(17);
    shortRead=false;coordinatorCalls=0;
    run=co::Initialize(state,&statsOwner,&registry,hooks);
    if(!run.succeeded()||run.completed!=7||coordinatorCalls!=4 ||
       state.scripts.definitions().size()!=62 ||
       !state.last_script_error.empty())return Fail(18);
    // A failure in the SECOND document must close both opened handles and
    // publish neither document, even after the first read succeeded.
    std::string sentinelDefs="previous-definitions",sentinelLayout="previous-layout";
    const int beforeSecond=openCalls;
    const int beforeSecondClose=closeCalls;
    failLayoutRead=true;
    if(fs::ReadPair(&owner,sentinelDefs,sentinelLayout)!=fs::Status::ReadRefused ||
       sentinelDefs!="previous-definitions" || sentinelLayout!="previous-layout" ||
       openCalls!=beforeSecond+2 || closeCalls!=beforeSecondClose+2 ||
       layoutReadCalls==0)return Fail(19);
    failLayoutRead=false;
    const int beforeOpen=openCalls,beforeClose=closeCalls;
    failLayoutOpen=true;
    if(fs::ReadPair(&owner,sentinelDefs,sentinelLayout)!=fs::Status::ReadRefused ||
       sentinelDefs!="previous-definitions" || sentinelLayout!="previous-layout" ||
       openCalls!=beforeOpen+2 || closeCalls!=beforeClose+1)return Fail(20);
    failLayoutOpen=false;
    // An impossible overlong engine Read return must be refused before
    // any buffer publication. Both handles must still be closed.
    const int beforeOverlongClose=closeCalls;
    overlongLayoutRead=true;
    if(fs::ReadPair(&owner,sentinelDefs,sentinelLayout)!=fs::Status::ReadRefused ||
       sentinelDefs!="previous-definitions" || sentinelLayout!="previous-layout" ||
       closeCalls!=beforeOverlongClose+2)return Fail(21);
    overlongLayoutRead=false;
    if(fs::ReadPair(&owner,sentinelDefs,sentinelLayout)!=fs::Status::TextsRead ||
       sentinelDefs!=defs || sentinelLayout!=layout)return Fail(22);
    std::puts("PASS VFileSystem012 MSVC slots, injected stats coordinator, second-file atomicity, refusal and retry");
    return 0;
}
