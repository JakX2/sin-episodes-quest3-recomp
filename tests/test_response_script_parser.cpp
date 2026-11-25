#include "../pc/independent_slot0/response_script_parser.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
using namespace sin_port::response_script;
static bool read_file(void* p,const std::string& n,std::string& out){
    if(n.find("..")!=std::string::npos||n.empty()||n[0]=='/'||n.find('\\')!=std::string::npos)return false;
    std::ifstream f(*static_cast<std::string*>(p)+"/"+n,std::ios::binary);
    if(!f)return false;
    out.assign(std::istreambuf_iterator<char>(f),{});return true;
}
int main(int argc,char** argv){
    Parser p;
    p.include=[](void*,const std::string& n,std::string& out){
        if(n!="talker/fixture.txt")return false;
        out=R"(response nested
{
 sequential
 scene "scenes/test.vcd" delay 3
 speak fallback weight 2
}
rule example
{
 criteria idle
 response nested
 respeakdelay 3
 matchonce
}
)";return true;};
    assert(p.parse("root",R"(enumeration "state"
{
 idle 1
 idle 2
}
criterion idle npcstate "[state::idle]" required
response empty
#include "talker/fixture.txt"
rule discarded
{
 criteria unknown
 response empty
}
)"));
    assert(p.db.enums.size()==1&&p.db.criteria.size()==2&&p.db.groups.size()==2&&p.db.rules.size()==1);
    assert(p.db.files.size()==2&&p.db.warnings.size()==2);
    assert(p.db.groups[0].entries.empty()&&p.db.groups[1].sequential&&p.db.groups[1].entries.size()==2);
    assert(p.db.groups[1].params.delay==3&&p.db.groups[1].entries[1].weight==2);
    assert(p.db.rules[0].once&&p.db.rules[0].criteria.size()==2);
    assert(p.db.criteria[1].key=="respeakdelay"&&p.db.criteria[1].expression=="3"&&!p.db.criteria[1].required);
    bool yes=false;
    assert(rr::match(p.db.criteria[0].matcher.data(),rr::matcher_bytes,"[state::idle]",{&p.db,enum_lookup},yes)&&yes);
    assert(!p.parse("root","response bad {\n scene x\n")&&p.db.groups.empty());
    assert(!p.parse("root","#include missing\n"));
    if(argc==2){
        std::string root=argv[1],data;
        p.include=read_file;p.context=&root;
        if(!read_file(&root,"talker/response_rules.txt",data)||!p.parse("talker/response_rules.txt",data)){
            std::cerr<<"retail parse failed: "<<p.error<<"\n";return 1;
        }
        unsigned entries=0;
        for(const auto& g:p.db.groups)entries+=unsigned(g.entries.size());
        std::cout<<"files="<<p.db.files.size()<<" enums="<<p.db.enums.size()<<" criteria="<<p.db.criteria.size()<<" groups="<<p.db.groups.size()<<" rules="<<p.db.rules.size()<<" entries="<<entries<<" warnings="<<p.db.warnings.size()<<"\n";
        for(const auto& w:p.db.warnings)std::cout<<w<<"\n";
        assert(p.db.files.size()==6&&p.db.enums.size()==7&&p.db.criteria.size()==137&&p.db.groups.size()==61&&p.db.rules.size()==63&&entries==134);
    }
}
