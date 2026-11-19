// SDK2004 VFileSystem012 original-free MSVC x86 executable test.
#include "client_engine_filesystem_read.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdint>
namespace fs=sin_independent::engine_fs;
static int existsCalls=0,openCalls=0,closeCalls=0,readCalls=0;
static bool missingDefs=false,missingLayout=false,shortRead=false,hugeSize=false;
static bool rejectOpen=false;
static std::string defs,layout;
struct Handle {const std::string* bytes=nullptr;std::size_t cursor=0;};
static Handle handle;
struct BaseFileSystem {
    // SDK IBaseFileSystem has NO instance fields: secondary IAppSystem
    // must start immediately after this base's 32-bit primary vptr.
    virtual int Read(void* output,int size,void* file) {
        ++readCalls;
        auto* h=static_cast<Handle*>(file);
        if(shortRead)return 0;
        const auto amount=std::min<std::size_t>(
            static_cast<std::size_t>(size),h->bytes->size()-h->cursor);
        // Deliberate partial reads exercise our loop.
        const auto take=std::min<std::size_t>(amount,11);
        std::memcpy(output,h->bytes->data()+h->cursor,take);
        h->cursor+=take;
        return static_cast<int>(take);
    }
    virtual int Write(const void*,int,void*){return -1;}
    virtual void* Open(const char* path,const char* mode,const char* pathId) {
        ++openCalls;
        if(!mode||std::strcmp(mode,"rb")||pathId||rejectOpen)return nullptr;
        if(!std::strcmp(path,fs::kDefinitions))handle.bytes=&defs;
        else if(!std::strcmp(path,fs::kLayout))handle.bytes=&layout;
        else return nullptr;
        handle.cursor=0;
        return &handle;
    }
    virtual void Close(void* file) {
        ++closeCalls;
        if(file!=&handle)std::abort();
    }
    virtual void Seek(void*,int,int){}
    virtual unsigned Tell(void*){return 0;}
    virtual unsigned Size(void* file) {
        if(file!=&handle)std::abort();
        return hugeSize?300000u:static_cast<unsigned>(handle.bytes->size());
    }
    virtual unsigned Size(const char*,const char*){return 0;}
    virtual void Flush(void*){}
    virtual bool Precache(const char*,const char*){return false;}
    virtual bool FileExists(const char* path,const char* pathID) {
        ++existsCalls;
        if(pathID)std::abort();
        if(!std::strcmp(path,fs::kDefinitions))return !missingDefs;
        if(!std::strcmp(path,fs::kLayout))return !missingLayout;
        std::abort();
    }
    virtual bool IsFileWritable(const char*,const char*){return false;}
    virtual bool SetFileWritable(const char*,bool,const char*){return false;}
    virtual long GetFileTime(const char*,const char*){return 0;}
};
struct AppSystem {
    virtual bool Connect(void*){return false;}
    virtual void Disconnect(){}
    virtual void* QueryInterface(const char*){return nullptr;}
    virtual int Init(){return 0;}
    virtual void Shutdown(){}
};
struct FileSystem:BaseFileSystem,AppSystem {};
static std::string Definitions(int count) {
    std::string result="\"StatDefinitions\" {\n";
    for(int i=0;i<count;++i) {
        result+="\"stat\" { \"name\" \"STAT_"+std::to_string(i);
        result+="\" \"short_text\" \"#s\" \"long_text\" \"#l\"";
        if(i>=1&&i<=8)result+=" \"percentage_of\" \"STAT_0\"";
        result+=" }\n";
    }
    return result+"}\n";
}
static std::string Layout(int count) {
    std::string result;
    for(int i=0;i<156;++i)
        result+="\"STAT_"+std::to_string(i%count)+"\" \"value\"\n";
    for(int i=0;i<66;++i)result+="\"==========================\"\n";
    return result;
}
static int Fail(int i) {std::fprintf(stderr,"engine-fs-fixture step=%d\n",i);return i;}
int main() {
    FileSystem owner;
    if(reinterpret_cast<std::uintptr_t>(static_cast<AppSystem*>(&owner))-
       reinterpret_cast<std::uintptr_t>(&owner)!=sizeof(void*))return Fail(1);
    if(fs::Presence(nullptr)!=fs::Status::InvalidAbi)return Fail(2);
    fs::Methods methods;
    if(!fs::Resolve(&owner,methods,true)||!methods.exists||!methods.open||
       !methods.close||!methods.size||!methods.read)return Fail(3);
    defs=Definitions(62);layout=Layout(62);
    if(fs::Presence(&owner)!=fs::Status::BothPresent||existsCalls!=2||
       openCalls!=0||readCalls!=0)return Fail(4);
    missingDefs=true;
    if(fs::Presence(&owner)!=fs::Status::MissingDefinitions ||
       fs::ReadAndValidate(&owner)!=fs::Status::MissingDefinitions ||
       openCalls!=0)return Fail(5);
    missingDefs=false;missingLayout=true;
    if(fs::Presence(&owner)!=fs::Status::MissingLayout ||
       fs::ReadAndValidate(&owner)!=fs::Status::MissingLayout)return Fail(6);
    missingLayout=false;
    fs::Counts counts{};
    const auto verified=fs::ReadAndValidate(&owner,&counts);
    if(verified!=fs::Status::ParsedAndVerified||
       counts.definitions!=62||counts.refs!=8||counts.pairs!=156||
       counts.dividers!=66||openCalls!=closeCalls||readCalls<10) {
        std::fprintf(stderr,
            "read=%s counts=%u,%u,%u,%u io=%d/%d/%d exists=%d\n",
            fs::Name(verified),static_cast<unsigned>(counts.definitions),
            static_cast<unsigned>(counts.refs),static_cast<unsigned>(counts.pairs),
            static_cast<unsigned>(counts.dividers),openCalls,closeCalls,
            readCalls,existsCalls);
        return Fail(7);
    }
    const int closed=closeCalls;
    hugeSize=true;
    if(fs::ReadAndValidate(&owner)!=fs::Status::ReadRefused ||
       closeCalls!=closed+1)return Fail(8);
    hugeSize=false;
    shortRead=true;
    if(fs::ReadAndValidate(&owner)!=fs::Status::ReadRefused ||
       closeCalls!=closed+2)return Fail(9);
    shortRead=false;rejectOpen=true;
    if(fs::ReadAndValidate(&owner)!=fs::Status::ReadRefused ||
       closeCalls!=closed+2)return Fail(10);
    rejectOpen=false;
    defs=Definitions(61);layout=Layout(61);
    if(fs::ReadAndValidate(&owner,&counts)!=fs::Status::UnexpectedCounts ||
       counts.definitions!=61 || openCalls==0 || closeCalls!=openCalls-1)return Fail(11);
    defs=Definitions(62);layout=Layout(62);
    if(fs::ReadAndValidate(&owner)!=fs::Status::ParsedAndVerified ||
       closeCalls!=openCalls-1)return Fail(12);
    std::puts("PASS SDK2004 primary FileSystem slots, partial reads, close-on-refusal, owner counts");
    return 0;
}
