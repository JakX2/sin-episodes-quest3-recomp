#include "sin_compat/original_grunt_registry_refresh_phase586.hpp"
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::phase586;
int main(){
    assert(direct_record_stride==212u);
    static_assert(sizeof(verified_shared_activities)/sizeof(verified_shared_activities[0])==8);
    assert(verified_shared_activities[2].value==6);
    assert(std::string(verified_shared_activities[2].literal_name)=="ACT_WALK");
    assert(verified_shared_activities[3].value==10);
    assert(std::string(verified_shared_activities[3].literal_name)=="ACT_RUN");
    assert(verified_shared_activities[4].value==15);
    assert(std::string(verified_shared_activities[4].literal_name)=="ACT_SCRIPT_CUSTOM_MOVE");
    assert(verified_shared_activities[7].value==154);
    assert(std::string(verified_shared_activities[7].literal_name)=="ACT_DO_NOT_DISTURB");
    assert(!IndexWithOriginalDirectClamp(0,0u));
    assert(IndexWithOriginalDirectClamp(-1,3u)==0u);
    assert(IndexWithOriginalDirectClamp(99,3u)==0u);
    assert(IndexWithOriginalDirectClamp(2,3u)==2u);
    assert(DirectRecordByteOffset(2,3u)==424u);
    assert(Revision(15,false,{1,9})==15);
    assert(Revision(15,true,{14,9,22,3})==3);
    assert(Revision(15,true,{14})==15);
    std::vector<std::string> log;
    NameResolvers h;
    h.original_220e6cb0=[&](const std::string& s){log.push_back("primary:"+s);return s=="known"?99:-1;};
    h.original_220e6fe0=[&](const std::string& s){log.push_back("fallback:"+s);return s=="Unknown"?0:42;};
    Entry e{"known",0x02,-1};PrepareRecord(e,h);assert(e.numeric_key==99 && e.flags==0x12);
    assert((log==std::vector<std::string>{"primary:known"}));
    log.clear();Entry other{"Unknown",0,-1};PrepareRecord(other,h);
    assert(other.flags==0x10&&other.numeric_key==0);
    assert((log==std::vector<std::string>{"primary:Unknown","fallback:Unknown"}));
    log.clear();Entry empty{"",0,-9};PrepareRecord(empty,h);assert(empty.flags==0x10&&empty.numeric_key==-9&&log.empty());
    Registry r{{{"known",0,0},{"Unknown",0,0},{"",0,7}},1};
    RefreshAll(nullptr,66,h,[&](std::size_t,Entry&){assert(false);});
    auto post=[&](std::size_t i,Entry& e){log.push_back("post:"+std::to_string(i)+":"+std::to_string(e.numeric_key));};
    RefreshAll(&r,66,h,post);
    assert(r.stamp==66 && r.entries[0].numeric_key==99 && r.entries[1].numeric_key==0 && r.entries[2].numeric_key==7);
    assert((log==std::vector<std::string>{"primary:known","post:0:99","primary:Unknown","fallback:Unknown","post:1:0","post:2:7"}));
    assert((r.entries[0].flags&0x10) && (r.entries[1].flags&0x10) && (r.entries[2].flags&0x10));
}