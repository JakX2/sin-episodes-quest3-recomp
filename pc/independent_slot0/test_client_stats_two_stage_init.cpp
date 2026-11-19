// Synthetic Win32 x86 original-order C_StatsSystem resource-stage fixture.
// Source-original (SHA-matched) third stage: definitions/key names/66 catalog.
// Fourth stage: independent display layout. No retail KeyValues ABI used.
#include "client_stats_coordinator.h"
#include "client_stats_original_id_map.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
namespace co=sin_independent::stats_coordinator;
namespace ids=sin_independent::stat_name_ids;
namespace st=sin_independent::stats_system;
struct FakeFileSystem {
    std::string definitions,layout;
    int defsReads=0,layoutReads=0;
    bool refuseDefs=false,refuseLayout=false;
};
struct FakeGameSystem {
    co::State state;
    int laterCalls=0;
    int owner=42, registry=84;
    bool wrongOrder=false;
};
static FakeGameSystem* current=nullptr;
static std::string Definitions(int omit=-1){
    std::string s="\"StatDefinitions\" {\n";
    for(std::size_t id=0;id<ids::kCount;++id){
        if((id>=51&&id<=54)||static_cast<int>(id)==omit)continue; // four compiled-only IDs
        const std::string name(ids::NameForId(id));
        s+="\"stat\" { \"name\" \""+name+"\" \"short_text\" \"#s_"+
            std::to_string(id)+"\" \"long_text\" \"#l_"+std::to_string(id)+"\"";
        if(id>=1&&id<=8)s+=" \"percentage_of\" \"STAT_TOTAL_TIME\"";
        s+=" }\n";
    }
    return s+"}\n";
}
static std::string Layout(int count=156){
    std::string s;
    for(int i=0;i<count;++i)
        s+="\"STAT_"+std::to_string(i)+"\" \"value\"\n";
    // The fixture uses arbitrary non-STAT titles, so avoid out-of-range
    // display names while still exercising parsed source references.
    s.clear();
    for(int i=0;i<count;++i){
        const auto id=static_cast<std::size_t>(i%66);
        if(id>=51&&id<=54)s+="\"#fixture_title\" \"title\"\n";
        else s+="\""+std::string(ids::NameForId(id))+"\" \"value\"\n";
    }
    for(int i=0;i<66;++i)s+="\"==========================\"\n";
    return s;
}
static bool Read(void* owner,const char* resource,
                 std::string& output,std::string& error) noexcept {
    auto& fs=*static_cast<FakeFileSystem*>(owner);
    if(std::strcmp(resource,"scripts/stats/stats_definitions.txt")==0){
        ++fs.defsReads;
        if(fs.refuseDefs){error="definitions-refused";return false;}
        output=fs.definitions;error.clear();return true;
    }
    if(std::strcmp(resource,"scripts/stats/stats_display_layout.txt")==0){
        ++fs.layoutReads;
        if(fs.refuseLayout){error="layout-refused";return false;}
        output=fs.layout;error.clear();return true;
    }
    error="unexpected-resource";return false;
}
static bool Later(void* context,const st::Step& step) noexcept {
    if(!current||current->laterCalls>=4)return false;
    const bool registryCall=step.kind==st::StepKind::RegisterCallback;
    if(context!=(registryCall?static_cast<void*>(&current->registry):
                                  static_cast<void*>(&current->owner)))
        current->wrongOrder=true;
    if(current->state.catalog_text.count()!=62 ||
       current->state.scripts.definitions().size()!=62 ||
       current->state.scripts.display().size()!=222)
        current->wrongOrder=true;
    if(step.original_callsite!=st::kInitSteps[current->laterCalls+3].original_callsite)
        current->wrongOrder=true;
    ++current->laterCalls;
    return !current->wrongOrder;
}
static int Fail(int n){std::fprintf(stderr,"staged-stats step=%d\n",n);return n;}
int main(){
    static_assert(sizeof(void*)==4,"verify MSVC x86");
    FakeGameSystem game;
    FakeFileSystem fs;
    current=&game;
    fs.definitions=Definitions();fs.layout=Layout();
    co::Hooks hooks{};
    hooks.context=&game;
    hooks.stats_resource_context=&fs;
    hooks.read_stats_resource=&Read;
    hooks.run_unreconstructed_step=&Later;
    auto run=co::Initialize(game.state,&game.owner,&game.registry,hooks);
    if(!run.succeeded() || run.attempted!=7||run.completed!=7 ||
       fs.defsReads!=1||fs.layoutReads!=1||game.laterCalls!=4||
       game.wrongOrder||game.state.catalog_text.count()!=62 ||
       game.state.scripts.definitions().size()!=62 ||
       game.state.scripts.display().size()!=222||
       game.state.catalog.at(1)->identifier!=0||
       game.state.catalog.at(54)->identifier!=-1 ||
       !game.state.last_script_error.empty())return Fail(1);
    game.laterCalls=0;
    fs.refuseDefs=true;
    run=co::Initialize(game.state,&game.owner,&game.registry,hooks);
    if(run.outcome!=co::Outcome::ResourceLoaderRefused ||
       run.attempted!=3||run.completed!=2||game.laterCalls!=0 ||
       fs.layoutReads!=1||game.state.scripts.definitions().size()!=62||
       game.state.last_script_error!="definitions-refused")return Fail(2);
    fs.refuseDefs=false;fs.refuseLayout=true;
    run=co::Initialize(game.state,&game.owner,&game.registry,hooks);
    if(run.outcome!=co::Outcome::AssetLoadRefused ||
       run.attempted!=4||run.completed!=3||game.laterCalls!=0 ||
       game.state.scripts.display().size()!=222 ||
       game.state.last_script_error!="layout-refused")return Fail(3);
    fs.refuseLayout=false;fs.layout="\"STAT_NOT_REGISTERED\" \"value\"\n";
    run=co::Initialize(game.state,&game.owner,&game.registry,hooks);
    if(run.outcome!=co::Outcome::AssetLoadRefused ||
       run.attempted!=4||run.completed!=3||game.laterCalls!=0 ||
       game.state.scripts.display().size()!=222)return Fail(4);
    fs.layout=Layout();fs.definitions="\"StatDefinitions\" { \"stat\" { \"name\" \"STAT_MISSING\" \"short_text\" \"#s\" \"long_text\" \"#l\" } }";
    run=co::Initialize(game.state,&game.owner,&game.registry,hooks);
    if(run.outcome!=co::Outcome::ResourceLoaderRefused ||
       run.attempted!=3||run.completed!=2||game.laterCalls!=0)return Fail(5);
    fs.definitions=Definitions();
    // A parsed, internally consistent but incomplete defs list cannot
    // silently replace the owner's 62 original compiled bindings.
    fs.definitions=Definitions(2);
    run=co::Initialize(game.state,&game.owner,&game.registry,hooks);
    if(run.outcome!=co::Outcome::ResourceLoaderRefused ||
       run.attempted!=3||run.completed!=2||
       game.state.last_script_error!="unexpected original definitions count" ||
       game.state.scripts.definitions().size()!=62 ||
       game.laterCalls!=0)return Fail(10);
    fs.definitions=Definitions();
    // 155 syntactically valid display pairs must not publish over the
    // last good full 156 + 66 layout document.
    fs.layout=Layout(155);
    run=co::Initialize(game.state,&game.owner,&game.registry,hooks);
    if(run.outcome!=co::Outcome::AssetLoadRefused ||
       run.attempted!=4||run.completed!=3 ||
       game.state.last_script_error!="unexpected original display counts" ||
       game.state.scripts.display().size()!=222 ||
       game.laterCalls!=0)return Fail(11);
    fs.layout=Layout();
    game.laterCalls=0;
    run=co::Initialize(game.state,&game.owner,&game.registry,hooks);
    if(!run.succeeded()||run.completed!=7||run.attempted!=7||
       game.laterCalls!=4 || game.wrongOrder ||
       game.state.catalog_text.count()!=62 ||
       !game.state.last_script_error.empty())return Fail(6);
    // Wrong context or mutually exclusive loaders must refuse before
    // changing either existing catalog or parsed document snapshot.
    const int previousDefs=fs.defsReads,previousLayout=fs.layoutReads;
    auto bad=hooks;
    bad.stats_resource_context=nullptr;
    run=co::Initialize(game.state,&game.owner,&game.registry,bad);
    if(run.outcome!=co::Outcome::MissingAssetReaderContext ||
       run.attempted!=0 || fs.defsReads!=previousDefs)return Fail(7);
    bad=hooks;
    const std::filesystem::path root="C:\\synthetic";
    bad.game_content_root=&root;
    run=co::Initialize(game.state,&game.owner,&game.registry,bad);
    if(run.outcome!=co::Outcome::ConflictingAssetSources ||
       run.attempted!=0 || fs.layoutReads!=previousLayout)return Fail(8);
    bad=hooks;
    bad.load_catalog_resources=[](void*,sin_independent::stats_catalog::Catalog&) noexcept{return true;};
    run=co::Initialize(game.state,&game.owner,&game.registry,bad);
    if(run.outcome!=co::Outcome::ConflictingResourceLoaders ||
       run.attempted!=0||fs.defsReads!=previousDefs ||
       game.state.catalog_text.count()!=62)return Fail(9);
    std::puts("PASS original two-stage definitions/catalog then layout, failure boundary and retry");
    return 0;
}
