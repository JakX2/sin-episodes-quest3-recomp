// Native C++17 disk loader fixture; no original commercial scripts in CI.
#include "client_stats_asset_loader.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
namespace fs=std::filesystem;
namespace a=sin_independent::stats_assets;
static const char* kDefs=R"text("StatDefinitions" {"stat" {"name" "STAT_HP" "short_text" "#hp" "long_text" "#health"}})text";
static const char* kLayout="\"==========================\"\n\"STAT_HP\" \"value\"\n";
static bool Put(const fs::path& file,const std::string& data){
    std::ofstream f(file,std::ios::binary);f<<data;return bool(f);
}
int main(int argc,char** argv){
    a::Documents docs;
    std::string err;
    if(!docs.LoadTexts(kDefs,kLayout,err)||docs.definitions().size()!=1||
       docs.display().size()!=2||!docs.Find("STAT_HP"))return 1;
    if(docs.LoadTexts(kDefs,"\"STAT_MISSING\" \"value\"",err)||
       docs.definitions().size()!=1||docs.display().size()!=2)return 2;
    if(docs.LoadTexts("INVALID",kLayout,err)||docs.Find("STAT_HP")==nullptr)return 3;
    const fs::path root=fs::temp_directory_path()/"sin-stats-asset-fixture-test";
    std::error_code ec;
    fs::remove_all(root,ec);
    fs::create_directories(root/"scripts"/"stats",ec);
    if(ec)return 4;
    const auto folder=root/"scripts"/"stats";
    if(!Put(folder/"stats_definitions.txt",kDefs)||
       !Put(folder/"stats_display_layout.txt",kLayout))return 5;
    if(!docs.LoadContentRoot(root,err)||docs.definitions().size()!=1||
       docs.display().size()!=2)return 6;
    if(!Put(folder/"stats_display_layout.txt","\"STAT_MISSING\" \"value\""))return 7;
    if(docs.LoadContentRoot(root,err)||!docs.Find("STAT_HP")||
       docs.display().size()!=2)return 8;
    if(!Put(folder/"stats_display_layout.txt",kLayout)||
       !docs.LoadContentRoot(root,err))return 9;
    if(!Put(folder/"stats_definitions.txt",std::string(262145,'a')))return 10;
    if(docs.LoadContentRoot(root,err)||docs.definitions().size()!=1||
       docs.display().size()!=2)return 11;
    fs::remove_all(root,ec);
    if(argc==2){
        if(!docs.LoadContentRoot(argv[1],err)){
            std::fprintf(stderr,"Owner scripts: %s\n",err.c_str());return 12;
        }
        std::size_t refs=0,div=0;
        for(const auto& x:docs.definitions())refs+=!x.percentage_of.empty();
        for(const auto& x:docs.display())div+=x.divider;
        if(docs.definitions().size()!=62||refs!=8||docs.display().size()!=222||div!=66)return 13;
        std::printf("OWNER_ORIGINAL_STATS_ASSETS_LOADED definitions=62 refs=8 pairs=156 dividers=66\n");
    }else if(argc!=1)return 14;
    std::puts("PASS native owned stats file reader atomic reload and malformed/missing file refusal");
    return 0;
}
