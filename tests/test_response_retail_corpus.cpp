#include "../pc/independent_slot0/response_script_registry.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
using namespace sin_port::response_script;
static bool load(void* p,const std::string& n,std::string& out){
    if(n.empty()||n.find("..")!=std::string::npos||n[0]=='/'||n.find('\\')!=std::string::npos)return false;
    std::ifstream f(*static_cast<std::string*>(p)+"/"+n,std::ios::binary);
    if(!f)return false;
    out.assign(std::istreambuf_iterator<char>(f),{});return true;
}
struct Audit {
    Registry r;
    std::vector<std::pair<std::string,std::string>> input;
    unsigned rankings=0,choices=0;
    static Audit& self(void* p){return *static_cast<Audit*>(p);}
    rr::Scoring scoring(){
        rr::Scoring s{};s.context=this;
        s.criterion=[](void* p,unsigned i,rr::Bytes* out) noexcept{return self(p).r.row(0x60,rr::criterion_stride,rr::criterion_bytes,i,*out);};
        s.rule=[](void* p,unsigned i,rr::Bytes* out) noexcept{return self(p).r.row(0xb8,rr::rule_stride,rr::rule_bytes,i,*out);};
        s.storage=[](void* p,std::uint32_t w,rr::Bytes* out) noexcept{return self(p).r.storage(w,*out);};
        s.string=[](void* p,std::uint32_t w,const char** out) noexcept{return self(p).r.string(w,*out);};
        s.lookup=[](void* p,const char* key,const char** out,float* weight) noexcept{
            *out="";*weight=rr::ascii_equal(key,"concept")?5.f:1.f;
            for(const auto& kv:self(p).input)if(rr::ascii_equal(kv.first.c_str(),key)){*out=kv.second.c_str();break;}
            return true;};
        s.draw=[](void*,int lo,int,int* out) noexcept{*out=lo;return true;};
        s.enumeration={&r,Registry::enum_lookup};return s;
    }
    int best(const char* expected){
        auto s=scoring();int ties[limit]{},pick=-1;
        assert(rr::best_rule(s,unsigned(r.rule_names.size()),ties,limit,pick));++rankings;
        if(expected){if(pick<0||r.rule_names[unsigned(pick)]!=expected){std::cerr<<"expected "<<expected<<" got "<<(pick<0?"<none>":r.rule_names[unsigned(pick)])<<"\n";std::abort();}}
        else assert(pick==-1);
        return pick;
    }
    int choose(int rule,bool admit_scene=true){
        rr::Bytes rb{},ids{},g{},entries{};
        assert(rule>=0&&r.row(0xb8,rr::rule_stride,rr::rule_bytes,unsigned(rule),rb));
        assert(rb.get<int>(0x20)==1&&r.storage(rb.get<unsigned>(0x14),ids));
        assert(r.row(8,rr::group_stride,rr::group_bytes,ids.get<unsigned>(0),g));
        if(g.get<int>(0xc))assert(r.storage(g.get<unsigned>(0),entries));
        rr::GroupChoice h{};h.context=&admit_scene;
        h.filter=[](void* p,unsigned type,std::uint32_t,unsigned* yes) noexcept{*yes=type!=3||*static_cast<bool*>(p);return true;};
        h.random=[](void*,float lo,float,float* out) noexcept{*out=lo;return true;};
        int rejected[limit*2]{},selected=-2;
        assert(rr::choose_group(const_cast<unsigned char*>(g.p),g.n,const_cast<unsigned char*>(entries.p),entries.n,h,rejected,limit*2,selected));
        ++choices;return selected;
    }
};
int main(int argc,char** argv){
    if(argc!=2){std::cerr<<"usage: test_response_retail_corpus <private scripts directory>\n";return 2;}
    std::string root=argv[1],data;Parser p;p.include=load;p.context=&root;
    assert(load(&root,"talker/response_rules.txt",data)&&p.parse("talker/response_rules.txt",data));
    assert(p.db.files.size()==6&&p.db.enums.size()==7&&p.db.criteria.size()==137&&p.db.groups.size()==61&&p.db.rules.size()==63&&p.db.warnings.empty());
    Audit a;assert(a.r.build(p.db));
    unsigned total=0,empty=0;
    // Verify every group/entry and every rule reference survives materialization.
    for(unsigned i=0;i<p.db.groups.size();++i){
        rr::Bytes g{},rows{};const auto& group=p.db.groups[i];
        assert(a.r.row(8,rr::group_stride,rr::group_bytes,i,g)&&g.get<unsigned>(0xc)==group.entries.size());
        if(group.entries.empty()){++empty;continue;}
        assert(a.r.storage(g.get<unsigned>(0),rows));
        for(unsigned j=0;j<group.entries.size();++j){
            const auto off=j*rr::response_row_bytes;const char* v=nullptr;
            assert(rows.get<unsigned>(off)==group.entries[j].type&&a.r.string(rows.get<unsigned>(off+4),v)&&v==group.entries[j].value);
            assert(rows.get<float>(off+8)==group.entries[j].weight);++total;
        }
    }
    assert(total==134);
    for(unsigned i=0;i<p.db.rules.size();++i){
        rr::Bytes r{},ids{};const auto& rule=p.db.rules[i];
        assert(a.r.row(0xb8,rr::rule_stride,rr::rule_bytes,i,r)&&r.get<unsigned>(0xc)==rule.criteria.size()&&r.get<unsigned>(0x20)==rule.groups.size());
        if(!rule.criteria.empty()){assert(a.r.storage(r.get<unsigned>(0),ids));for(unsigned j=0;j<rule.criteria.size();++j)assert(ids.get<unsigned>(j*4)==rule.criteria[j]);}
        if(!rule.groups.empty()){assert(a.r.storage(r.get<unsigned>(0x14),ids));for(unsigned j=0;j<rule.groups.size();++j)assert(ids.get<unsigned>(j*4)==rule.groups[j]);}
    }
    const char* regions[]={"head","chest","crotch","knee","hand"};
    const char* region_names[]={"Head","Chest","Crotch","Knee","Hand"};
    const char* concepts[]={"TLK_INTERROGATE_COCKY","TLK_INTERROGATE_NERVOUS","TLK_INTERROGATE_TALKING"};
    const char* moods[]={"Cocky","Nervous","Talking"};
    for(unsigned m=0;m<3;++m)for(unsigned r=0;r<5;++r){
        a.input={{"classname","npc_count"},{"PlayerLookingAtEnt","npc_count"},{"PlayerLookingAtPart",regions[r]},{"concept",concepts[m]}};
        const std::string name=std::string("CountPlayerInterrogate")+moods[m]+region_names[r];
        const int winner=a.best(name.c_str());
        for(int expected:{0,1,0,0})assert(a.choose(winner)==expected);
        assert(a.choose(winner,false)==-1);
        a.input[1].second="npc_jessica";a.best(nullptr);
    }
    a.input={{"classname","npc_jessica"},{"concept","TLK_DANGER"}};
    int winner=a.best("JessicaDanger");assert(a.choose(winner)==1&&a.choose(winner)==0&&a.choose(winner)==1);
    a.input[1].second="TLK_SHOT";winner=a.best("JessicaShot");assert(a.choose(winner)==10&&a.choose(winner,false)==-1);
    a.input[1].second="TLK_PLDEAD";winner=a.best("JessicaPlayerDead");assert(a.choose(winner)>=0);
    rr::Bytes rule{};assert(a.r.row(0xb8,rr::rule_stride,rr::rule_bytes,unsigned(winner),rule)&&rule.p[0x28]);
    const_cast<unsigned char*>(rule.p)[0x29]=0;a.best(nullptr);
    a.input={{"classname","npc_jessica"},{"concept","TLK_USE"},{"map","se1_highrise01"}};
    for(unsigned i=1;i<=3;++i){a.input[2].second.back()=char('0'+i);std::string name="JessicaUseDialogHighrise0";name+=char('0'+i);assert(a.choose(a.best(name.c_str()))>=0);}
    a.input[2].second="other";a.best(nullptr);
    a.input={{"classname","npc_jessica"},{"concept","TLK_PLLOOKING"},{"PlayerLookingAtEnt","npc_jessica"},{"PlayerLookingAtPart","chest"},{"PlayerLookingAtPartForSeconds","3.0"}};
    a.best(nullptr);a.input.back().second="3.01";assert(a.choose(a.best("JessicaPlayerLookingAtChest"))>=0);
    a.input[3].second="butt";assert(a.choose(a.best("JessicaPlayerLookingAtButt"))>=0);
    a.input={{"classname","npc_jessica"},{"concept","TLK_PLUSING"},{"PlayerUsingEnt","prop_vehicle_sin"},{"PlayerUsingPart","GearShift"}};
    assert(a.choose(a.best("JessicaPlayerUsingGearShift"))==-1);
    std::cout<<"retail corpus: "<<total<<" entries, "<<empty<<" empty groups, "<<a.rankings<<" ranking checks, "<<a.choices<<" group selections passed\n";
}
