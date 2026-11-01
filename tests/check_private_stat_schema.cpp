// Optional source-data comparison executable, NOT run automatically in CI.
// The owner supplies original stats_definitions.txt; no content redistributed.
#include "sin_compat/stat_schema.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
using namespace sin_compat;
int main(int argc,char**argv) {
    if(argc!=2) {
        std::cerr<<"usage: sin_stat_schema_private_audit /private/stats_definitions.txt\n";
        return 2;
    }
    std::ifstream f(argv[1]);
    if(!f)return 3;
    const std::string content((std::istreambuf_iterator<char>(f)),{});
    StatSchema s;
    std::string error;
    if(!s.load(content,&error)){std::cerr<<error<<'\n';return 1;}
    if(s.definitions().size()!=62){std::cerr<<"unexpected stat count\n";return 1;}
    std::size_t linked=0;
    for(const auto& item:s.definitions())if(!item.percentage_of.empty())++linked;
    if(linked!=8){std::cerr<<"unexpected percentage reference count\n";return 1;}
    for(const auto& name:{"STAT_TOTAL_TIME_IN_COMBAT","STAT_CHALLENGE_RATING",
                          "STAT_INSTANT_CHALLENGE_RATING","STAT_GAME_PLAY_VAR_AVERAGES"}) {
        if(!s.find(name)){std::cerr<<"missing known stat: "<<name<<'\n';return 1;}
    }
    const auto* ratio=s.find("STAT_TOTAL_TIME_IN_COMBAT");
    if(!ratio || ratio->percentage_of!="STAT_TOTAL_TIME")return 1;
    const auto example=s.percentage("STAT_TOTAL_TIME_IN_COMBAT",
                   {{"STAT_TOTAL_TIME_IN_COMBAT",600},{"STAT_TOTAL_TIME",1200}});
    if(!example || example.percent!=50.0)return 1;
    std::cout<<"62 definitions, 8 valid percentage_of relationships; private stat schema verified\n";
}
