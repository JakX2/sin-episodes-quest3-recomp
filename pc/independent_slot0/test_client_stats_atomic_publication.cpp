// Original-free native MSVC Win32 regression: pending Stats catalog/text/docs
// must not change the last published trio on stage-three/four refusal.
#include "client_stats_coordinator.h"
#include "client_stats_original_id_map.h"
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

namespace co=sin_independent::stats_coordinator;
namespace ids=sin_independent::stat_name_ids;
namespace steps=sin_independent::stats_system;
namespace catalog=sin_independent::stats_catalog;
namespace binding=sin_independent::stats_binding;
namespace assets=sin_independent::stats_assets;

static_assert(sizeof(void*)==4,"native PE32 is required");
static_assert(noexcept(std::declval<catalog::Catalog&>().Swap(
    std::declval<catalog::Catalog&>())),"catalog publication must be noexcept");
static_assert(noexcept(std::declval<binding::Projection&>().Swap(
    std::declval<binding::Projection&>())),"text publication must be noexcept");
static_assert(noexcept(std::declval<assets::Documents&>().Swap(
    std::declval<assets::Documents&>())),"scripts publication must be noexcept");

struct Fixture {
    co::State state;
    std::string sourceDefinitions,sourceLayout;
    int owner=42,registry=84;
    int definitionReads=0,layoutReads=0,laterCalls=0;
    bool refuseDefinition=false,refuseLayout=false;
    bool rejectFirstLayoutHook=false;
    bool sawReplacedText=false;
    int deniedSlot=-1;
};
static Fixture* current=nullptr;

static std::string Definitions(int omit=-1) {
    std::string s="\"StatDefinitions\" {\n";
    for(std::size_t id=0;id<ids::kCount;++id) {
        if(id>=51&&id<=54)continue;
        if(id==static_cast<std::size_t>(omit))continue;
        s+="\"stat\" { \"name\" \""+std::string(ids::NameForId(id))+
           "\" \"short_text\" \"#s_"+std::to_string(id)+
           "\" \"long_text\" \"#l_"+std::to_string(id)+"\"";
        if(id>=1&&id<=8)s+=" \"percentage_of\" \"STAT_TOTAL_TIME\"";
        s+=" }\n";
    }
    return s+"}\n";
}
static std::string Layout(int pairCount=156) {
    std::string s;
    for(int i=0;i<pairCount;++i) {
        const std::size_t id=static_cast<std::size_t>(i)%ids::kCount;
        if(id>=51&&id<=54)s+="\"#synthetic_title\" \"title\"\n";
        else s+="\""+std::string(ids::NameForId(id))+"\" \"value\"\n";
    }
    for(int i=0;i<66;++i)s+="\"==========================\"\n";
    return s;
}
static bool Allow(void* context,std::size_t id) noexcept {
    return static_cast<int>(id)!=static_cast<Fixture*>(context)->deniedSlot;
}
static bool Read(void* context,const char* path,std::string& output,
                 std::string& error) noexcept {
    auto& f=*static_cast<Fixture*>(context);
    if(std::strcmp(path,"scripts/stats/stats_definitions.txt")==0){
        ++f.definitionReads;
        if(f.refuseDefinition){error="definition-refused";return false;}
        output=f.sourceDefinitions;error.clear();return true;
    }
    if(std::strcmp(path,"scripts/stats/stats_display_layout.txt")==0){
        ++f.layoutReads;
        if(f.refuseLayout){error="layout-refused";return false;}
        output=f.sourceLayout;error.clear();return true;
    }
    error="unexpected-path";return false;
}
static bool Later(void* obj,const steps::Step& step) noexcept {
    auto& f=*current;
    const bool registration=step.kind==steps::StepKind::RegisterCallback;
    if(obj!=(registration?static_cast<void*>(&f.registry):
                           static_cast<void*>(&f.owner)))return false;
    ++f.laterCalls;
    if(step.kind==steps::StepKind::LoadDisplayLayout){
        const auto* published=f.state.catalog_text.at(1);
        f.sawReplacedText=published &&
            published->short_text=="#s_NEW_1" &&
            f.state.catalog.at(1)->identifier==0 &&
            f.state.scripts.definitions().size()==62 &&
            f.state.scripts.display().size()==222;
        if(f.rejectFirstLayoutHook)return false;
    }
    return true;
}
static int Fail(int step) {
    std::fprintf(stderr,"stats-three-owner-atomicity stage=%d\n",step);
    return step;
}
struct Snapshot {
    const catalog::Entry* record1=nullptr;
    const binding::Text* text1=nullptr;
    const sin_independent::stats_scripts::Definition* definition0=nullptr;
    const sin_independent::stats_scripts::DisplayEntry* display0=nullptr;
    std::size_t names=0,layouts=0,bindings=0;
    int index=-1;
    std::string value;
};
static Snapshot Capture(const co::State& state) {
    Snapshot result{};
    result.record1=state.catalog.at(1);
    result.text1=state.catalog_text.at(1);
    result.definition0=state.scripts.definitions().data();
    result.display0=state.scripts.display().data();
    result.names=state.scripts.definitions().size();
    result.layouts=state.scripts.display().size();
    result.bindings=state.catalog_text.count();
    result.index=result.record1?result.record1->identifier:-1;
    result.value=result.text1?result.text1->short_text:"";
    return result;
}
static bool Same(const co::State& state,const Snapshot& before) {
    const auto after=Capture(state);
    return after.record1==before.record1 &&
           after.text1==before.text1 &&
           after.definition0==before.definition0 &&
           after.display0==before.display0 &&
           after.names==before.names &&
           after.layouts==before.layouts &&
           after.bindings==before.bindings &&
           after.index==before.index &&
           after.value==before.value;
}
int main() {
    Fixture f;
    current=&f;
    f.sourceDefinitions=Definitions();
    f.sourceLayout=Layout();
    co::Hooks h{};
    h.context=&f;
    h.stats_resource_context=&f;
    h.read_stats_resource=&Read;
    h.run_unreconstructed_step=&Later;
    h.allow_catalog=&Allow;
    auto run=co::Initialize(f.state,&f.owner,&f.registry,h);
    if(!run.succeeded()||run.completed!=7||run.attempted!=7||
       f.state.catalog_text.count()!=62||f.state.scripts.display().size()!=222||
       f.state.catalog.at(1)->identifier!=0)return Fail(1);
    const auto baseline=Capture(f.state);
    if(baseline.value!="#s_1")return Fail(2);

    f.refuseDefinition=true;
    f.laterCalls=0;
    run=co::Initialize(f.state,&f.owner,&f.registry,h);
    if(run.outcome!=co::Outcome::ResourceLoaderRefused||
       run.completed!=2||run.attempted!=3||f.laterCalls!=0||
       !Same(f.state,baseline))return Fail(3);
    f.refuseDefinition=false;

    f.refuseLayout=true;
    f.laterCalls=0;
    run=co::Initialize(f.state,&f.owner,&f.registry,h);
    if(run.outcome!=co::Outcome::AssetLoadRefused||
       run.completed!=3||run.attempted!=4||f.laterCalls!=0||
       !Same(f.state,baseline))return Fail(4);
    f.refuseLayout=false;

    f.sourceLayout="\"STAT_UNDEFINED\" \"value\"\n";
    run=co::Initialize(f.state,&f.owner,&f.registry,h);
    if(run.outcome!=co::Outcome::AssetLoadRefused||
       run.completed!=3||run.attempted!=4||!Same(f.state,baseline))
        return Fail(5);
    f.sourceLayout=Layout(155);
    run=co::Initialize(f.state,&f.owner,&f.registry,h);
    if(run.outcome!=co::Outcome::AssetLoadRefused||
       run.completed!=3||run.attempted!=4||
       f.state.last_script_error!="unexpected original display counts"||
       !Same(f.state,baseline))return Fail(6);
    f.sourceLayout=Layout();

    f.sourceDefinitions=Definitions(2);
    run=co::Initialize(f.state,&f.owner,&f.registry,h);
    if(run.outcome!=co::Outcome::ResourceLoaderRefused||
       run.completed!=2||run.attempted!=3||
       f.state.last_script_error!="unexpected original definitions count"||
       !Same(f.state,baseline))return Fail(7);
    f.sourceDefinitions=Definitions();
    f.deniedSlot=7;
    run=co::Initialize(f.state,&f.owner,&f.registry,h);
    if(run.outcome!=co::Outcome::ResourceLoaderRefused||
       run.completed!=2||run.attempted!=3||
       f.state.last_script_error!="unallocated catalog record"||
       !Same(f.state,baseline))return Fail(8);
    f.deniedSlot=-1;

    // Both files valid, but the original layout callback refuses.
    // The callback must see the new ownership snapshot while executing;
    // after its refusal the published resource trio returns to baseline.
    f.sourceDefinitions=Definitions();
    auto pos=f.sourceDefinitions.find("#s_1");
    if(pos==std::string::npos)return Fail(9);
    f.sourceDefinitions.replace(pos,4,"#s_NEW_1");
    f.rejectFirstLayoutHook=true;
    f.sawReplacedText=false;
    f.laterCalls=0;
    run=co::Initialize(f.state,&f.owner,&f.registry,h);
    if(run.outcome!=co::Outcome::LaterStepRefused||
       run.completed!=3||run.attempted!=4||f.laterCalls!=1||
       !f.sawReplacedText||!Same(f.state,baseline))return Fail(10);

    f.rejectFirstLayoutHook=false;
    f.laterCalls=0;
    run=co::Initialize(f.state,&f.owner,&f.registry,h);
    if(!run.succeeded()||run.completed!=7||run.attempted!=7||
       f.laterCalls!=4 || Same(f.state,baseline) ||
       f.state.catalog_text.at(1)->short_text!="#s_NEW_1" ||
       f.state.catalog.at(1)->identifier!=0||
       f.state.scripts.definitions().size()!=62||
       f.state.scripts.display().size()!=222||
       !f.state.last_script_error.empty())return Fail(11);
    std::puts("PASS Stats catalog, projection and documents transact together across stage-three/four failures");
    return 0;
}
