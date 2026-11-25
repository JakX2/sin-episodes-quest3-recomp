#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <utility>

namespace sin_port::map_host {
// Owner-only, case-insensitive scene directory snapshot. Not the
// original engine's multi-search-path filesystem.
struct SceneDirectorySource {
    std::map<std::string,std::filesystem::path> paths;
    std::map<std::string,std::uint32_t> handles;
    static bool normalize(const char* name,std::string& out){
        if(!name||!*name)return false;
        std::string key(name);
        if(key.size()>511)return false;
        for(char& c:key){if(c=='\\')c='/';else if(c>='A'&&c<='Z')c=static_cast<char>(c+32);}
        if(key.compare(0,7,"scenes/")||key.size()<12||key.compare(key.size()-4,4,".vcd")||key.find(':')!=std::string::npos)return false;
        for(std::size_t begin=0;begin<key.size();){
            auto end=key.find('/',begin);if(end==std::string::npos)end=key.size();
            if(end==begin||key.compare(begin,end-begin,".")==0||key.compare(begin,end-begin,"..")==0)return false;
            begin=end+1;
        }
        out=std::move(key);return true;
    }
    bool open(const char* directory){
        namespace fs=std::filesystem;
        if(!directory||!*directory||!paths.empty())return false;
        std::error_code ec;fs::path root(directory);
        if(!fs::is_directory(root,ec)||ec)return false;
        if(fs::is_directory(root/"scenes",ec)&&!ec)root/="scenes";
        ec.clear();std::map<std::string,fs::path> tmp;
        fs::recursive_directory_iterator it(root,ec),end;
        if(ec)return false;
        for(;it!=end;it.increment(ec)){
            if(ec)return false;
            auto status=it->symlink_status(ec);if(ec)return false;
            if(!fs::is_regular_file(status))continue;
            auto name="scenes/"+it->path().lexically_relative(root).generic_string();
            std::string key;if(!normalize(name.c_str(),key))continue;
            if(!tmp.emplace(key,it->path()).second)return false;
        }
        if(ec||tmp.empty()||tmp.size()>=65535)return false;
        paths=std::move(tmp);return true;
    }
    bool handle(const char* name,std::uint32_t* out){
        std::string key;if(!out||!normalize(name,key))return false;
        auto it=handles.find(key);
        if(it==handles.end()){
            if(handles.size()>=65535)return false;
            it=handles.emplace(key,static_cast<std::uint32_t>(handles.size()+1)).first;
        }
        *out=it->second;return true;
    }
    bool read(const char* name,std::string& data,bool& found)const{return read_impl(name,data,found,false);}
    // Raw playback distinguishes a zero-length file from a missing path.
    static int read_raw(void* p,const char* name,std::string& data,bool& found) noexcept{
        if(!p)return 0;
        try{return static_cast<SceneDirectorySource*>(p)->read_impl(name,data,found,true);}
        catch(...){return 0;}
    }
private:
    bool read_impl(const char* name,std::string& data,bool& found,bool empty_ok)const{
        std::string key;if(!normalize(name,key))return false;
        auto it=paths.find(key);found=it!=paths.end();
        if(!found){data.clear();return true;}
        // Raw playback may reload a formerly indexed file after eviction.
        // A vanished file is a normal miss; permission and I/O errors are
        // not. Do not change the metadata reader's existing semantics.
        if(empty_ok){
            std::error_code ec;auto status=std::filesystem::symlink_status(it->second,ec);
            if(ec==std::errc::no_such_file_or_directory||(!ec&&status.type()==std::filesystem::file_type::not_found)){
                found=false;data.clear();return true;
            }
            if(ec||!std::filesystem::is_regular_file(status))return false;
        }
        std::ifstream stream(it->second,std::ios::binary|std::ios::ate);auto bytes=stream.tellg();
        if(!stream||bytes<0||(!empty_ok&&bytes==0)||bytes>1024*1024)return false;
        std::string tmp(static_cast<std::size_t>(bytes),'\0');stream.seekg(0);
        if(!tmp.empty()&&!stream.read(tmp.data(),static_cast<std::streamsize>(tmp.size())))return false;
        data=std::move(tmp);return true;
    }
};
}
