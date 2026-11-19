// SDK2004 VFileSystem012 original-free MSVC x86 executable test.
#include "client_engine_filesystem_read.h"
#include "client_stats_coordinator.h"
#include "client_stats_original_id_map.h"
#include <type_traits>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdint>
namespace fs=sin_independent::engine_fs;
namespace co=sin_independent::stats_coordinator;
namespace ids=sin_independent::stat_name_ids;
static_assert(std::is_same<co::StagedResourceReader,
    decltype(&fs::ReadForStagedStats)>::value,"staged engine reader ABI");
static void* expectedStatsContext=nullptr;
static_assert(std::is_same<co::AssetReader,decltype(&fs::ReadForStatsCoordinator)>::value,
    "Engine-backed stats reader must match injected coordinator ABI");
static int existsCalls=0,openCalls=0,closeCalls=0,readCalls=0;
static bool missingDefs=false,missingLayout=false,shortRead=false,hugeSize=false;
static bool rejectOpen=false;
static bool rejectLayoutOpen=false;
static bool stagedParams=false;
static int fixtureAbiViolations=0;
static const char* fixtureAbiReason=nullptr;
static void FixtureAbiViolation(const char* reason) noexcept {
    ++fixtureAbiViolations;
    fixtureAbiReason=reason;
    std::fprintf(stderr,"fixture ABI mismatch: %s\n",reason);
}
static std::string defs,layout;
struct Handle {const std::string* bytes=nullptr;std::size_t cursor=0;};
static Handle handle;
struct BaseFileSystem {
    // SDK IBaseFileSystem has NO instance fields: secondary IAppSystem
    // must start immediately after this base's 32-bit primary vptr.
    virtual int Read(void* output,int size,void* file) {
        ++readCalls;
        if(file!=&handle || !handle.bytes) {
            FixtureAbiViolation("Read invalid handle");
            return 0;
        }
        if(size<0 || (!output && size>0)) {
            FixtureAbiViolation("Read invalid buffer/size");
            return 0;
        }
        auto* h=static_cast<Handle*>(file);
        if(shortRead)return 0;
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
        if(!path || !mode || rejectOpen ||
           (rejectLayoutOpen && !std::strcmp(path,fs::kLayout)))return nullptr;
        if(stagedParams) {
            if(!std::strcmp(path,fs::kDefinitions) &&
               (std::strcmp(mode,"rb") || !pathId ||
                std::strcmp(pathId,"GAME"))) {FixtureAbiViolation("definitions Open mode/pathID");return nullptr;}
            if(!std::strcmp(path,fs::kLayout) &&
               (std::strcmp(mode,"rt") || pathId)) {FixtureAbiViolation("layout Open mode/pathID");return nullptr;}
        } else if(std::strcmp(mode,"rb") || pathId) {FixtureAbiViolation("legacy Open mode/pathID");return nullptr;}
        if(!std::strcmp(path,fs::kDefinitions))handle.bytes=&defs;
        else if(!std::strcmp(path,fs::kLayout))handle.bytes=&layout;
        else return nullptr;
        handle.cursor=0;
        return &handle;
    }
    virtual void Close(void* file) {
        ++closeCalls;
        if(file!=&handle) {FixtureAbiViolation("invalid file handle");return;}
    }
    virtual void Seek(void*,int,int){}
    virtual unsigned Tell(void*){return 0;}
    virtual unsigned Size(void* file) {
        if(file!=&handle || !handle.bytes) {FixtureAbiViolation("invalid Size handle");return 0;}
        const auto result=hugeSize?300000u:static_cast<unsigned>(handle.bytes->size());
        std::fprintf(stderr,"fixture Size(handle) value=%u huge=%d\n",result,hugeSize?1:0);
        return result;
    }
    virtual unsigned Size(const char*,const char*) {
        FixtureAbiViolation("Size(path) overload unexpectedly called");
        return 0;
    }
    virtual void Flush(void*){}
    virtual bool Precache(const char*,const char*){return false;}
    virtual bool FileExists(const char* path,const char* pathID) {
        ++existsCalls;
        if(!path) {FixtureAbiViolation("FileExists null path");return false;}
        if(pathID) {FixtureAbiViolation("FileExists pathID");return false;}
        if(!std::strcmp(path,fs::kDefinitions))return !missingDefs;
        if(!std::strcmp(path,fs::kLayout))return !missingLayout;
        FixtureAbiViolation("FileExists unexpected path");
        return false;
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
static std::string OriginalDefs() {
    std::string result="\"StatDefinitions\" {\n";
    for(std::size_t id=0;id<ids::kCount;++id) {
        if(id>=51&&id<=54)continue;
        result+="\"stat\" { \"name\" \""+std::string(ids::NameForId(id));
        result+="\" \"short_text\" \"#s\" \"long_text\" \"#l\"";
        if(id>0 && id<=8)result+=" \"percentage_of\" \"STAT_TOTAL_TIME\"";
        result+=" }\n";
    }
    return result+"}\n";
}
static std::string OriginalLayout(){
    std::string result;
    for(std::size_t i=0;i<156;++i){
        const auto id=i%ids::kCount;
        result+=(id>=51&&id<=54)?
            "\"#synthetic_title\" \"title\"\n":
            "\""+std::string(ids::NameForId(id))+"\" \"value\"\n";
    }
    for(int i=0;i<66;++i)result+="\"==========================\"\n";
    return result;
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
    // Staged reads intentionally use the original per-resource Open mode/pathID.
    // Switch the fixture contract before exercising that API, not afterward.
    stagedParams=true;
    std::string single="old";
    error="old";
    if(!fs::ReadForStagedStats(&owner,fs::kDefinitions,single,error) ||
       single!=defs || !error.empty())return Fail(19);
    single="retain-me";
    if(fs::ReadForStagedStats(&owner,"scripts/../../unsafe",single,error) ||
       error!="unrecognized stats resource" || single!="retain-me")
        return Fail(20);
    // A failed staged read must not publish partial text, leak its handle,
    // or poison a later successful read. Exercise each original resource mode.
    const int stagedClosedBefore=closeCalls;
    shortRead=true;
    single="preserve-definitions";
    error.clear();
    if(fs::ReadForStagedStats(&owner,fs::kDefinitions,single,error) ||
       single!="preserve-definitions" || error!="read-refused" ||
       closeCalls!=stagedClosedBefore+1)return Fail(25);
    shortRead=false;
    missingLayout=true;
    // Staged loading opens the exact resource directly; no presence probe.
    // An Open refusal must leave the output unchanged.
    rejectOpen=true;
    single="preserve-layout";
    error.clear();
    if(fs::ReadForStagedStats(&owner,fs::kLayout,single,error) ||
       single!="preserve-layout" || error!="read-refused" ||
       closeCalls!=stagedClosedBefore+1)return Fail(26);
    rejectOpen=false;
    missingLayout=false;
    if(!fs::ReadForStagedStats(&owner,fs::kLayout,single,error) ||
       single!=layout || !error.empty() ||
       closeCalls!=stagedClosedBefore+2)return Fail(27);
    defs=OriginalDefs();layout=OriginalLayout();
    stagedParams=true;
    co::State staged;
    hooks.read_engine_documents=nullptr;
    hooks.asset_reader_context=nullptr;
    hooks.load_catalog_resources=nullptr;
    hooks.stats_resource_context=&owner;
    hooks.read_stats_resource=&fs::ReadForStagedStats;
    coordinatorCalls=0;
    run=co::Initialize(staged,&statsOwner,&registry,hooks);
    if(!run.succeeded() || run.completed!=7 || run.attempted!=7 ||
       staged.catalog_text.count()!=62 ||
       staged.catalog.at(1)->identifier!=0 ||
       staged.catalog.at(54)->identifier!=-1 ||
       staged.scripts.definitions().size()!=62 ||
       staged.scripts.display().size()!=222 ||
       coordinatorCalls!=4)return Fail(21);
    shortRead=true;coordinatorCalls=0;
    run=co::Initialize(staged,&statsOwner,&registry,hooks);
    if(run.outcome!=co::Outcome::ResourceLoaderRefused ||
       run.completed!=2 || run.attempted!=3 || coordinatorCalls!=0 ||
       staged.scripts.definitions().size()!=62 ||
       staged.scripts.display().size()!=222 ||
       staged.last_script_error!="read-refused")return Fail(22);
    shortRead=false;coordinatorCalls=0;
    run=co::Initialize(staged,&statsOwner,&registry,hooks);
    if(!run.succeeded() || run.completed!=7 || coordinatorCalls!=4 ||
       staged.catalog_text.count()!=62 ||
       !staged.last_script_error.empty())return Fail(23);
    // Failure in stage four must retain both previously published documents and
    // the bound 62-record catalog; the next attempt must recover completely.
    const std::string retainedDefs=staged.scripts.definitions().empty()?"":defs;
    const std::size_t retainedCount=staged.catalog_text.count();
    const int beforeLayoutRefusal=closeCalls;
    rejectLayoutOpen=true;
    coordinatorCalls=0;
    run=co::Initialize(staged,&statsOwner,&registry,hooks);
    if(run.outcome!=co::Outcome::AssetLoadRefused ||
       run.completed!=3 || run.attempted!=4 || coordinatorCalls!=0 ||
       staged.catalog_text.count()!=retainedCount ||
       staged.scripts.definitions().size()!=62 ||
       staged.scripts.display().size()!=222 ||
       defs!=retainedDefs || closeCalls!=beforeLayoutRefusal+1)
        return Fail(28);
    rejectLayoutOpen=false;
    coordinatorCalls=0;
    run=co::Initialize(staged,&statsOwner,&registry,hooks);
    if(!run.succeeded() || run.completed!=7 || run.attempted!=7 ||
       coordinatorCalls!=4 || staged.catalog_text.count()!=62 ||
       staged.scripts.definitions().size()!=62 ||
       staged.scripts.display().size()!=222 || !staged.last_script_error.empty())
        return Fail(29);
    if(fixtureAbiViolations) {
        std::fprintf(stderr,"fixture ABI violations=%d last=%s\n",fixtureAbiViolations,
                     fixtureAbiReason?fixtureAbiReason:"unknown");
        return Fail(24);
    }
    std::puts("PASS VFileSystem012 MSVC slots, injected stats coordinator, partial reads, rollback and retry");
    return 0;
}
