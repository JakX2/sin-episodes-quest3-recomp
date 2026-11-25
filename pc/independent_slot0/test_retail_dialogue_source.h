#pragma once
#include <string>
#include <fstream>
namespace sin_port::test_support {
 // Owner-only filesystem source, never copied into the repository. The
 // native create API copies each returned span before the next include.
struct RetailDialogueSource {
    std::string directory,buffer;
    unsigned reads=0;
    bool read(const char* name,std::string& out){
        if(!name||!*name)return false;
        std::string relative(name);
        if(relative[0]=='/'||relative.find("..")!=std::string::npos||
           relative.find('\\')!=std::string::npos||relative.find(':')!=std::string::npos)return false;
        std::ifstream file(directory+"/"+relative,std::ios::binary|std::ios::ate);
        if(!file)return false;
        const auto length=file.tellg();
        if(length<=0||length>4*1024*1024)return false;
        out.resize(static_cast<std::size_t>(length));
        file.seekg(0);
        if(!file.read(out.data(),static_cast<std::streamsize>(out.size())))return false;
        ++reads;return true;
    }
    static int __cdecl include(void* p,const char* name,const char** out,unsigned* bytes) noexcept{
        if(!p||!out||!bytes)return 0;
        try{
            auto& self=*static_cast<RetailDialogueSource*>(p);
            if(!self.read(name,self.buffer))return 0;
            *out=self.buffer.data();*bytes=static_cast<unsigned>(self.buffer.size());return 1;
        }catch(...){return 0;}
    }
};
}
