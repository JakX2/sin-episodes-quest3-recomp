// Original-free Win32 MSVC test of original 2006 Stats CUserMessages hooks.
// This fixture NEVER decodes proprietary bitbuf values beyond their
// 16-bit ID headers and never dispatches the original callback addresses.
#include "client_stats_user_message_hooks.h"
#include "client_stats_coordinator.h"
#include "client_stats_original_id_map.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
namespace m=sin_independent::stats_messages;
namespace stats=sin_independent::stats_system;
namespace c=sin_independent::stats_coordinator;
static int Fail(int which) {
    std::fprintf(stderr,"stats-user-message-hook-fixture failure=%d\n",which);
    return which;
}
static std::string OriginalDefs() {
    std::string out="\"StatDefinitions\" {\n";
    for(std::size_t i=0;i<sin_independent::stat_name_ids::kCount;++i) {
        if(i>=51&&i<=54)continue;
        out+="\"stat\" {\"name\" \""+
             std::string(sin_independent::stat_name_ids::NameForId(i))+
             "\" \"short_text\" \"#s\" \"long_text\" \"#l\"";
        if(i>0&&i<=8)out+=" \"percentage_of\" \"STAT_TOTAL_TIME\"";
        out+="}\n";
    }
    return out+"}\n";
}
static std::string OriginalLayout(){
    std::string out;
    for(int i=0;i<156;++i){
        const std::size_t idx=static_cast<std::size_t>(i)%66;
        if(idx>=51&&idx<=54)out+="\"#synthetic\" \"title\"\n";
        else out+="\""+std::string(sin_independent::stat_name_ids::NameForId(idx))+
                  "\" \"value\"\n";
    }
    for(int i=0;i<66;++i)out+="\"==========================\"\n";
    return out;
}
struct SyntheticDocuments {
    std::string definitions,layout;
    int reads=0;
};
static bool Source(void* object,const char* path,std::string& text,
                   std::string& error) noexcept {
    auto* source=static_cast<SyntheticDocuments*>(object);
    if(!source||!path)return false;
    ++source->reads;
    if(std::strcmp(path,"scripts/stats/stats_definitions.txt")==0)
        text=source->definitions;
    else if(std::strcmp(path,"scripts/stats/stats_display_layout.txt")==0)
        text=source->layout;
    else {
        error="unexpected resource";
        return false;
    }
    error.clear();
    return true;
}
int main() {
    if(sin_independent::kUserMessageCount!=39 ||
       sin_independent::FindUserMessage("ResetHUD")!=5 ||
       sin_independent::FindUserMessage("StaticStat")!=27 ||
       sin_independent::FindUserMessage("DynamicStat")!=28 ||
       sin_independent::FindUserMessage("Unknown")!=-1 ||
       sin_independent::kUserMessages[5].bytes!=1 ||
       sin_independent::kUserMessages[27].bytes!=-1 ||
       sin_independent::kUserMessages[28].bytes!=-1)return Fail(1);
    m::OwnedRegistry hooks;
    if(hooks.Complete()||hooks.count()||hooks.at(0)!=-1)return Fail(2);
    for(std::size_t i=0;i<3;++i){
        if(!hooks.RegisterStatsStep(stats::kInitSteps[4+i]) ||
           hooks.at(i)!=m::kOriginalStatsHooks[i].message_id ||
           hooks.count()!=i+1)return Fail(3);
    }
    if(!hooks.Complete() ||
       hooks.RegisterStatsStep(stats::kInitSteps[6]) ||
       hooks.RegisterStatsStep(stats::kInitSteps[3]))return Fail(4);
    hooks.Clear();
    auto wrong=stats::kInitSteps[4];
    wrong.original_callback_va^=1;
    if(hooks.RegisterStatsStep(wrong) || hooks.count()!=0)return Fail(5);
    wrong=stats::kInitSteps[4];
    wrong.argument="DynamicStat";
    if(hooks.RegisterStatsStep(wrong) || hooks.count()!=0)return Fail(6);
    for(std::size_t i=0;i<3;++i)
        if(!hooks.RegisterStatsStep(stats::kInitSteps[i+4]))return Fail(7);
    const std::uint8_t staticMin[3]={0,0,0xAA};
    const std::uint8_t staticMax[4]={54,0,0x11,0x22};
    const std::uint8_t staticBad[2]={55,0};
    const std::uint8_t dynamicMin[3]={55,0,0xAB};
    const std::uint8_t dynamicMax[3]={65,0,0xCD};
    const std::uint8_t dynamicBad[2]={66,0};
    const std::uint8_t dynamicBelow[2]={54,0};
    const std::uint8_t reset[1]={0x10};
    m::Header header{};
    if(m::InspectHeader(hooks,27,staticMin,sizeof(staticMin),&header)!=
           m::HeaderStatus::KnownHeaderOnly ||
       header.stat_id!=0||header.opaque_bytes!=1||
       header.opaque_value[0]!=0xAA)return Fail(8);
    if(m::InspectHeader(hooks,27,staticMax,sizeof(staticMax),&header)!=
           m::HeaderStatus::KnownHeaderOnly ||
       header.stat_id!=54||header.opaque_bytes!=2)return Fail(9);
    if(m::InspectHeader(hooks,27,staticBad,sizeof(staticBad))!=
           m::HeaderStatus::StaticIdRefused)return Fail(10);
    if(m::InspectHeader(hooks,28,dynamicMin,sizeof(dynamicMin),&header)!=
           m::HeaderStatus::KnownHeaderOnly||header.stat_id!=55)return Fail(11);
    if(m::InspectHeader(hooks,28,dynamicMax,sizeof(dynamicMax),&header)!=
           m::HeaderStatus::KnownHeaderOnly||header.stat_id!=65)return Fail(12);
    if(m::InspectHeader(hooks,28,dynamicBad,sizeof(dynamicBad))!=
           m::HeaderStatus::DynamicIdRefused ||
       m::InspectHeader(hooks,28,dynamicBelow,sizeof(dynamicBelow))!=
           m::HeaderStatus::DynamicIdRefused)return Fail(13);
    if(m::InspectHeader(hooks,5,reset,sizeof(reset),&header)!=
           m::HeaderStatus::KnownHeaderOnly||header.stat_id!=-1||
       header.opaque_bytes!=1)return Fail(14);
    if(m::InspectHeader(hooks,5,reset,0)!=
           m::HeaderStatus::ResetLengthRefused ||
       m::InspectHeader(hooks,5,reset,2)!=
           m::HeaderStatus::ResetLengthRefused)return Fail(15);
    if(m::InspectHeader(hooks,27,staticMin,1)!=
           m::HeaderStatus::PayloadTruncated ||
       m::InspectHeader(hooks,27,nullptr,1)!=
           m::HeaderStatus::InvalidMessage ||
       m::InspectHeader(hooks,29,staticMin,3)!=
           m::HeaderStatus::InvalidMessage ||
       m::InspectHeader(hooks,27,staticMin,256u*1024u+1)!=
           m::HeaderStatus::PayloadExceedsLimit)return Fail(16);
    hooks.Clear();
    if(m::InspectHeader(hooks,27,staticMin,3)!=
           m::HeaderStatus::MissingHook)return Fail(17);
    // Full independent *stats catalog* bootstrap with a real (owned)
    // 3-name message registry, replacing three inert callback stand-ins.
    SyntheticDocuments scripts{OriginalDefs(),OriginalLayout()};
    c::State state;
    c::Hooks params{};
    params.context=&state;
    params.stats_resource_context=&scripts;
    params.read_stats_resource=&Source;
    params.run_unreconstructed_step=&m::RegisterKnownStatsStep;
    auto result=c::Initialize(state,&state,&hooks,params);
    if(!result.succeeded() || result.completed!=7||result.attempted!=7||
       !hooks.Complete() || hooks.count()!=3 ||
       state.scripts.definitions().size()!=62 ||
       state.scripts.display().size()!=222 ||
       state.catalog_text.count()!=62 ||
       state.definitions.staticCount()!=55 ||
       state.definitions.dynamicCount()!=11 ||
       scripts.reads!=2)return Fail(18);
    // A repeated Init without explicit HookMessage registry reset is
    // deliberately refused, avoiding silently multiplying handlers.
    const auto second=c::Initialize(state,&state,&hooks,params);
    if(second.succeeded() || second.completed!=4||second.attempted!=5 ||
       hooks.count()!=3)return Fail(19);
    hooks.Clear();
    const auto retry=c::Initialize(state,&state,&hooks,params);
    if(!retry.succeeded()||!hooks.Complete() ||
       state.scripts.definitions().size()!=62)return Fail(20);
    std::puts("PASS client Stats: 39 protocol IDs, three owned user-message hooks, 16-bit headers, 7-stage integration");
    return 0;
}
