// Synthetic-only fixture for the optional post-sound retail asset probe.
#include "client_stats_live_asset_probe.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <string>
#include <system_error>
namespace fs=std::filesystem;
namespace probe=sin_independent::stats_live_probe;
static int Fail(int step){ std::fprintf(stderr,"stats-live-asset-fixture step=%d\n",step);return step; }
static std::string Definitions(int n) {
    std::string text="\"StatDefinitions\" {\n";
    for(int i=0;i<n;++i) {
        text+="\"stat\" { \"name\" \"STAT_"+std::to_string(i);
        text+="\" \"short_text\" \"#s\" \"long_text\" \"#l\"";
        if(i>=1 && i<=8) text+=" \"percentage_of\" \"STAT_0\"";
        text+=" }\n";
    }
    return text+"}\n";
}
static std::string Display() {
    std::string text;
    for(int i=0;i<156;++i)
        text+="\"STAT_"+std::to_string(i%62)+"\" \"value\"\n";
    for(int i=0;i<66;++i)
        text+="\"==========================\"\n";
    return text;
}
static bool Write(const fs::path& path,const std::string& content) {
    std::ofstream stream(path,std::ios::binary);
    if(!stream)return false;
    stream<<content;
    return bool(stream);
}
int main() {
    probe::Counts counts{1,1,1,1};
    if(probe::Validate(nullptr,&counts)!=probe::Result::MissingAbsoluteRoot ||
       counts.definitions!=0 || counts.display_pairs!=0)return Fail(1);
    const fs::path relative("SE1");
    if(probe::Validate(&relative)!=probe::Result::MissingAbsoluteRoot)return Fail(2);
    const auto root=fs::temp_directory_path()/"sin-stats-live-probe-fixture";
    std::error_code ec;
    fs::remove_all(root,ec);
    fs::create_directories(root/"scripts"/"stats",ec);
    if(ec)return Fail(3);
    const auto folder=root/"scripts"/"stats";
    const auto defs=folder/"stats_definitions.txt";
    const auto layout=folder/"stats_display_layout.txt";
    if(!Write(defs,Definitions(62))||!Write(layout,Display()))return Fail(4);
    if(probe::Validate(&root,&counts)!=probe::Result::Verified ||
       counts.definitions!=62 || counts.percentage_refs!=8 ||
       counts.display_pairs!=156 || counts.dividers!=66)return Fail(5);
    if(!Write(defs,Definitions(61)))return Fail(6);
    if(probe::Validate(&root,&counts)!=probe::Result::UnexpectedRetailCounts ||
       counts.definitions!=61 || counts.dividers!=66)return Fail(7);
    if(!Write(defs,Definitions(62)) ||
       !Write(layout,"\"STAT_MISSING\" \"value\"\n"))return Fail(8);
    if(probe::Validate(&root,&counts)!=probe::Result::ReadOrParseRefused ||
       counts.definitions!=0 || counts.display_pairs!=0)return Fail(9);
    if(!Write(layout,Display()))return Fail(10);
    fs::remove(defs,ec);
    if(probe::Validate(&root)!=probe::Result::ReadOrParseRefused)return Fail(11);
    if(!Write(defs,Definitions(62)))return Fail(12);
    if(probe::Validate(&root)!=probe::Result::Verified)return Fail(13);
    fs::remove_all(root,ec);
    return 0;
}
