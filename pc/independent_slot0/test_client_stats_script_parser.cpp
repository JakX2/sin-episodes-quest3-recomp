// Source-free native MSVC x86 stats-script parser regressions. Optional
// 2-path owner mode uses genuine external game scripts and no retail DLL.
#include "client_stats_script_parser.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
namespace s=sin_independent::stats_scripts;
static std::string Read(const char* p){
    std::ifstream f(p,std::ios::binary);std::ostringstream b;b<<f.rdbuf();
    return b.str();
}
static bool ErrorDefinitions(const char* src,std::vector<s::Definition>& d){
    std::string err;auto previous=d.size();
    return !s::ParseDefinitions(src,d,err)&&d.size()==previous;
}
static bool ErrorLayout(const char* src,const std::vector<s::Definition>& d,
                        std::vector<s::DisplayEntry>& l){
    std::string err;auto previous=l.size();
    return !s::ParseDisplayLayout(src,d,l,err)&&l.size()==previous;
}
int main(int argc,char** argv){
    const char* definitions=R"(
"StatDefinitions" {
 "stat" {"name" "STAT_BASE" "short_text" "#base" "long_text" "#long"}
 "stat" {"name" "STAT_CHILD" "short_text" "#child" "long_text" "#long2" "percentage_of" "STAT_BASE"}
}
)";
    const char* layout=R"(
"=========================="
"#tabs" "tab"
"#time" "title"
"STAT_BASE" "time"
"STAT_CHILD" "percent_of"
"#weapon" "dynamic_block"
)";
    std::vector<s::Definition> d;
    std::vector<s::DisplayEntry> l;
    std::string err;
    if(!s::ParseDefinitions(definitions,d,err)||d.size()!=2||
       d[1].percentage_of!="STAT_BASE")return 1;
    if(!s::ParseDisplayLayout(layout,d,l,err)||l.size()!=6||
       !l[0].divider||l[4].format!="percent_of")return 2;
    if(!ErrorDefinitions("\"Other\" {}",d)||
       !ErrorDefinitions("\"StatDefinitions\" { \"stat\" {\"name\" \"X\"}}",d)||
       !ErrorDefinitions("\"StatDefinitions\" { \"stat\" {\"name\" \"X\" \"name\" \"Y\" \"short_text\" \"S\" \"long_text\" \"L\"}}",d)||
       !ErrorDefinitions("\"StatDefinitions\" { \"stat\" {\"name\" \"X\" \"short_text\" \"S\" \"long_text\" \"L\" \"percentage_of\" \"MISSING\"}}",d)||
       !ErrorDefinitions("\"StatDefinitions\" { \"stat\" {\"name\" \"X\" \"short_text\" \"S\" \"long_text\" \"L\"}",d)||
       !ErrorDefinitions("\"StatDefinitions\" { \"stat\" {\"name\" \"X\" \"short_text\" \"S\" \"long_text\" \"L\"} \"stat\" {\"name\" \"X\" \"short_text\" \"S\" \"long_text\" \"L\"}}",d))return 3;
    if(!ErrorLayout("\"STAT_UNDEFINED\" \"value\"",d,l)||
       !ErrorLayout("\"STAT_CHILD\" \"unknown_format\"",d,l)||
       !ErrorLayout("\"STAT_BASE\" \"percent_of\"",d,l)||
       !ErrorLayout("\"STAT_BASE\"\n\"value\"",d,l)||
       !ErrorLayout("\"STAT_BASE\" {",d,l)||
       !ErrorLayout("\"unterminated",d,l))return 4;
    if(s::ParseDefinitions(std::string(262145,'x'),d,err)||
       s::ParseDisplayLayout(std::string(262145,'x'),d,l,err))return 5;
    if(argc==3){
        const auto originalDefs=Read(argv[1]),originalLayout=Read(argv[2]);
        if(!s::ParseDefinitions(originalDefs,d,err)){
            std::fprintf(stderr,"definitions: %s\n",err.c_str());return 6;
        }
        if(!s::ParseDisplayLayout(originalLayout,d,l,err)){
            std::fprintf(stderr,"layout: %s\n",err.c_str());return 7;
        }
        std::size_t refs=0,dividers=0;
        for(const auto& item:d)refs+=!item.percentage_of.empty();
        for(const auto& item:l)dividers+=item.divider;
        if(d.size()!=62||refs!=8||l.size()!=222||dividers!=66)return 8;
        std::printf("OWNER_ORIGINAL_STATS_SCRIPTS_VERIFIED definitions=%zu percentage_refs=%zu display_pairs=%zu dividers=%zu\n",
                    d.size(),refs,l.size()-dividers,dividers);
    } else if(argc!=1)return 9;
    std::puts("PASS stats scripts definitions/layout grammar and negative fixtures");
    return 0;
}
