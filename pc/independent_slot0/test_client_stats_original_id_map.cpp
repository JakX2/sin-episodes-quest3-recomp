// Windows x86 synthetic verification of original 2006 66-stat ID table
// and atomic projection into separately owned stats catalog text.
#include "client_stats_original_catalog_binding.h"
#include <array>
#include <cstdio>
#include <string>
#include <vector>
namespace ids=sin_independent::stat_name_ids;
namespace binding=sin_independent::stats_binding;
namespace scripts=sin_independent::stats_scripts;
namespace catalog=sin_independent::stats_catalog;
static int Fail(int stage) {
    std::fprintf(stderr,"original-stats-id-map stage=%d\n",stage);
    return stage;
}
static bool IsUnscripted(std::size_t id) noexcept {
    return id==51||id==52||id==53||id==54;
}
static std::vector<scripts::Definition> ScriptFixture() {
    std::vector<scripts::Definition> entries;
    for(std::size_t id=0;id<ids::kCount;++id) {
        if(IsUnscripted(id))continue;
        scripts::Definition def;
        def.name=std::string(ids::NameForId(id));
        def.short_text="#short_"+std::to_string(id);
        def.long_text="#long_"+std::to_string(id);
        if(id>0&&id<=8)def.percentage_of="STAT_TOTAL_TIME";
        entries.push_back(std::move(def));
    }
    return entries;
}
int main(){
    static_assert(sizeof(void*)==4,"native original record ABI is Win32");
    static_assert(sizeof(catalog::Entry)==12,"original catalog record");
    if(ids::kCount!=66||ids::Lookup({})!=-1||
       ids::Lookup("STAT_NOT_IN_GAME")!=-1 ||
       ids::Lookup("stat_total_time")!=-1 ||
       ids::NameForId(66)!="")return Fail(1);
    for(std::size_t i=0;i<66;++i) {
        if(ids::Lookup(ids::NameForId(i))!=static_cast<int>(i))
            return Fail(2);
        for(std::size_t j=i+1;j<66;++j)
            if(ids::NameForId(i)==ids::NameForId(j))
                return Fail(3);
    }
    // Script row order differs from canonical DLL ID order
    // because STAT_EXPECTED_TIME is an original-but-unlisted entry.
    if(ids::Lookup("STAT_EXPECTED_TIME")!=2 ||
       ids::Lookup("STAT_TOTAL_TIME_IN_COMBAT")!=3 ||
       ids::Lookup("STAT_NUM_FLAILS")!=54 ||
       ids::Lookup("STAT_GAME_PLAY_VAR_AVERAGES")!=65)
        return Fail(4);
    catalog::Catalog owner;
    binding::Projection projection;
    std::string error;
    auto definitions=ScriptFixture();
    if(definitions.size()!=62)return Fail(5);
    if(binding::Bind(owner,projection,definitions,error) ||
       projection.count()!=0 ||
       error!="catalog not initialized to 66 slots")return Fail(6);
    owner.Rebuild();
    if(!binding::Bind(owner,projection,definitions,error) ||
       !error.empty() || projection.count()!=62)return Fail(7);
    for(std::size_t id=0;id<66;++id) {
        const auto* text=projection.at(id);
        const auto* record=owner.at(id);
        if(!record)return Fail(8);
        if(IsUnscripted(id)) {
            if(text||record->identifier!=-1)return Fail(9);
            continue;
        }
        if(!text || text->short_text!="#short_"+std::to_string(id) ||
           text->long_text!="#long_"+std::to_string(id) ||
           text->percentage_id!=(id>=1&&id<=8?0:-1) ||
           record->identifier!=text->percentage_id ||
           record->first || record->second)
            return Fail(10);
    }
    // Mutating or rejecting a later source must preserve last-good text
    // AND all 66 catalog identifiers, without dangling original pointers.
    auto broken=definitions;
    broken[0].name="STAT_NOT_IN_GAME";
    if(binding::Bind(owner,projection,broken,error) ||
       error!="unrecognized stat identifier" ||
       projection.count()!=62 || !projection.at(0) ||
       owner.at(1)->identifier!=0)return Fail(11);
    broken=definitions;
    broken[1].name=broken[0].name;
    if(binding::Bind(owner,projection,broken,error) ||
       error!="duplicate stat identifier" ||
       projection.count()!=62)return Fail(12);
    broken=definitions;
    broken[1].percentage_of="STAT_UNKNOWN";
    if(binding::Bind(owner,projection,broken,error) ||
       error!="invalid percentage target ID" ||
       owner.at(1)->identifier!=0)return Fail(13);
    broken=definitions;
    broken[1].percentage_of="STAT_NUM_FLAILS";
    if(binding::Bind(owner,projection,broken,error) ||
       error!="unbound percentage target" ||
       owner.at(1)->identifier!=0)return Fail(14);
    // Null owner slots are not dereferenced; the previously bound
    // projection remains intact. Rebuild then recovers all 66.
    owner.Rebuild(nullptr,[](void*,std::size_t id) noexcept {
        return id!=7;
    });
    if(binding::Bind(owner,projection,definitions,error) ||
       error!="unallocated catalog record" ||
       projection.count()!=62)return Fail(15);
    owner.Rebuild();
    if(!binding::Bind(owner,projection,definitions,error) ||
       projection.count()!=62 || owner.at(7)->identifier!=0 ||
       !error.empty())return Fail(16);
    // New document containing no script rows is valid and clears
    // all source-text bindings but not the 66 compiled IDs.
    if(!binding::Bind(owner,projection,{},error) ||
       projection.count()!=0 || projection.at(0) ||
       owner.at(7)->identifier!=-1)return Fail(17);
    if(!binding::Bind(owner,projection,definitions,error) ||
       projection.count()!=62 || owner.at(7)->identifier!=0)
        return Fail(18);
    std::puts("PASS original 66 IDs, 62 source bindings, 4 compiled-only IDs, atomic failure and retry");
    return 0;
}
