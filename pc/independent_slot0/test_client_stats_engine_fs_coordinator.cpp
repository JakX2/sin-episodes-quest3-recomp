// Independent Windows x86 test: stats coordinator + read-only asset source.
#include "client_stats_coordinator.h"
#include <array>
#include <cstdio>
#include <filesystem>
#include <string>
namespace co=sin_independent::stats_coordinator;
namespace ss=sin_independent::stats_system;
struct Fixture {
    co::State* state=nullptr;
    std::string defs,layout;
    std::array<std::uint32_t,4> later{};
    int reads=0,resources=0,called=0;
    bool refuse=false;
};
static Fixture* current=nullptr;
static std::string MakeDefs(int n) {
    std::string s="\"StatDefinitions\" {\n";
    for(int i=0;i<n;++i) {
        s+="\"stat\" { \"name\" \"STAT_"+std::to_string(i);
        s+="\" \"short_text\" \"#s\" \"long_text\" \"#l\"";
        if(i>0&&i<=8)s+=" \"percentage_of\" \"STAT_0\"";
        s+=" }\n";
    }
    return s+"}\n";
}
static std::string MakeLayout(int n) {
    std::string s;
    for(int i=0;i<156;++i)
        s+="\"STAT_"+std::to_string(i%n)+"\" \"value\"\n";
    for(int i=0;i<66;++i)s+="\"==========================\"\n";
    return s;
}
static bool Read(void* ctx,std::string& d,std::string& l,
                 std::string& err) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    ++f.reads;
    if(f.refuse){err="engine-refused";return false;}
    d=f.defs;l=f.layout;err.clear();return true;
}
static bool Resources(void* ctx,sin_independent::stats_catalog::Catalog& c) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    ++f.resources;
    return f.state && f.state->definitions.staticCount()==55 &&
           f.state->definitions.dynamicCount()==11 && c.size()==66;
}
static bool Later(void*,const ss::Step& s) noexcept {
    if(!current || current->called>=4)return false;
    current->later[current->called++]=s.original_callsite;
    return true;
}
static int Fail(int n){std::fprintf(stderr,"engine-stats step=%d\n",n);return n;}
int main(){
    static_assert(sizeof(void*)==4,"MSVC x86 only");
    co::State state;
    Fixture fixture{};
    fixture.state=&state;fixture.defs=MakeDefs(62);fixture.layout=MakeLayout(62);
    current=&fixture;
    int owner=1,registry=2;
    co::Hooks h{};
    h.context=&fixture;h.load_catalog_resources=&Resources;
    h.run_unreconstructed_step=&Later;h.read_engine_documents=&Read;
    auto r=co::Initialize(state,&owner,&registry,h);
    if(!r.succeeded()||r.attempted!=7||r.completed!=7||
       fixture.reads!=1||fixture.resources!=1||fixture.called!=4||
       state.scripts.definitions().size()!=62||state.scripts.display().size()!=222||
       !state.last_script_error.empty())return Fail(1);
    for(int i=0;i<4;++i)
        if(fixture.later[i]!=ss::kInitSteps[i+3].original_callsite)
            return Fail(2);
    fixture.called=0;fixture.refuse=true;
    r=co::Initialize(state,&owner,&registry,h);
    if(r.outcome!=co::Outcome::AssetLoadRefused||r.completed!=3||
       r.attempted!=4||fixture.called!=0||
       state.scripts.definitions().size()!=62||
       state.last_script_error!="engine-refused")return Fail(3);
    fixture.refuse=false;fixture.layout="\"STAT_UNDEFINED\" \"value\"\n";
    r=co::Initialize(state,&owner,&registry,h);
    if(r.outcome!=co::Outcome::AssetLoadRefused||r.completed!=3||
       r.attempted!=4||state.scripts.definitions().size()!=62||
       state.last_script_error.empty())return Fail(4);
    fixture.defs=MakeDefs(61);fixture.layout=MakeLayout(61);fixture.called=0;
    r=co::Initialize(state,&owner,&registry,h);
    if(!r.succeeded()||fixture.called!=4||
       state.scripts.definitions().size()!=61||
       !state.last_script_error.empty())return Fail(5);
    // Ambiguous disk+engine inputs are refused before resetting storage.
    const std::filesystem::path diskRoot="C:\\synthetic";
    h.game_content_root=&diskRoot;
    int oldReads=fixture.reads,oldResources=fixture.resources;
    r=co::Initialize(state,&owner,&registry,h);
    if(r.outcome!=co::Outcome::ConflictingAssetSources||
       r.attempted!=0||r.completed!=0||fixture.reads!=oldReads||
       fixture.resources!=oldResources||
       state.scripts.definitions().size()!=61)return Fail(6);
    h.game_content_root=nullptr;
    fixture.defs=MakeDefs(62);fixture.layout=MakeLayout(62);fixture.called=0;
    r=co::Initialize(state,&owner,&registry,h);
    if(!r.succeeded()||r.completed!=7||fixture.called!=4||
       state.scripts.definitions().size()!=62||state.scripts.display().size()!=222||
       !state.last_script_error.empty())return Fail(7);
    // Asset callback remains opt-in, preserving all earlier test semantics.
    h.read_engine_documents=nullptr;
    r=co::Initialize(state,&owner,&registry,h);
    if(!r.succeeded()||r.completed!=7||
       state.scripts.definitions().size()!=62)return Fail(8);
    std::puts("PASS synthetic engine-backed stats documents, original seven-stage order, atomic refusal and retry");
    return 0;
}
