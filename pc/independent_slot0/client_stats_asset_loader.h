#pragma once
// Independently owned reader for the original 2006 SiN stats files.
// Standard C++17 host IO, NOT the proprietary IFileSystem/KeyValues ABI.
#include "client_stats_script_parser.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
namespace sin_independent::stats_assets {
class Documents final {
public:
    Documents()=default;
    Documents(const Documents&)=delete;
    Documents& operator=(const Documents&)=delete;
    Documents(Documents&&)=delete;
    Documents& operator=(Documents&&)=delete;
    bool LoadTexts(std::string_view definitions,std::string_view layout,
                   std::string& error){
        std::vector<stats_scripts::Definition> d;
        std::vector<stats_scripts::DisplayEntry> l;
        if(!stats_scripts::ParseDefinitions(definitions,d,error))return false;
        if(!stats_scripts::ParseDisplayLayout(layout,d,l,error))return false;
        definitions_.swap(d);layout_.swap(l);
        error.clear();return true;
    }
    bool LoadContentRoot(const std::filesystem::path& se1,
                         std::string& error){
        const auto folder=se1/"scripts"/"stats";
        std::string defs,layout;
        if(!ReadBounded(folder/"stats_definitions.txt",defs,error))return false;
        if(!ReadBounded(folder/"stats_display_layout.txt",layout,error))return false;
        return LoadTexts(defs,layout,error);
    }
    // Both parsed vectors publish together after all third/fourth-stage
    // validations. std::vector's default allocator makes swap noexcept.
    void Swap(Documents& other) noexcept {
        definitions_.swap(other.definitions_);
        layout_.swap(other.layout_);
    }
    const std::vector<stats_scripts::Definition>& definitions() const noexcept {
        return definitions_;
    }
    const std::vector<stats_scripts::DisplayEntry>& display() const noexcept {
        return layout_;
    }
    const stats_scripts::Definition* Find(std::string_view name) const noexcept {
        return stats_scripts::FindDefinition(definitions_,name);
    }
private:
    static bool ReadBounded(const std::filesystem::path& path,
                            std::string& result,std::string& error){
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        if(!file){error="cannot open "+path.generic_string();return false;}
        const auto size=file.tellg();
        if(size<0||size>static_cast<std::streamoff>(256*1024)){
            error="file exceeds 256 KiB input limit";return false;
        }
        file.seekg(0);
        std::string temp(static_cast<std::size_t>(size),'\0');
        if(size>0&&!file.read(&temp[0],static_cast<std::streamsize>(size))){
            error="short read of "+path.generic_string();return false;
        }
        result.swap(temp);return true;
    }
    std::vector<stats_scripts::Definition> definitions_;
    std::vector<stats_scripts::DisplayEntry> layout_;
};
} // namespace sin_independent::stats_assets
