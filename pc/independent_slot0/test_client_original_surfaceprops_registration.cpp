// Original-free MSVC x86 engine surface-data registration ABI fixture.
// The ONLY engine mutation in public CI is against this invented object.
#include "client_original_surfaceprops_registration.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
namespace reg=sin_independent::original_surfaceprops_registration;
namespace own=sin_independent::original_surfaceprops;
static int Fail(int n) {
    std::fprintf(stderr,"surfaceprops registration fixture failure %d\n",n);
    return n;
}
static std::string Document(const char* prefix,int count,int bases,
                            std::size_t expectedSize) {
    std::string body="// invented physics material surface definitions\n";
    for(int i=0;i<count;++i) {
        const bool primary=std::strcmp(prefix,"fixture")==0;
        const std::string name=primary&&i==0?"default":
            std::string(prefix)+"_"+std::to_string(i);
        body+="\""+name+"\" {\n";
        if((primary&&i>0&&i<=bases)||(!primary&&i<bases))
            body+="\"base\" \"default\"\n";
        body+="\"density\" \"1000\"\n\"friction\" \"0.3\"\n}\n";
    }
    if(body.size()<expectedSize)
        body.append(expectedSize-body.size(),' ');
    return body;
}
struct Handle {
    const std::string* data=nullptr;
    std::size_t position=0;
};
struct FilesBase {
    std::map<std::string,std::string> files{};
    Handle current{};
    std::string missing{};
    int opens=0,closes=0,reads=0,abiErrors=0;
    bool shortRead=false;
    virtual int Read(void* dst,int length,void* handle) {
        ++reads;
        if(shortRead)return 0;
        if(handle!=&current||!current.data||!dst||length<0) {
            ++abiErrors;return 0;
        }
        const std::size_t remaining=current.data->size()-current.position;
        const auto count=std::min<std::size_t>(
            static_cast<std::size_t>(length),
            std::min<std::size_t>(remaining,127u));
        if(count)std::memcpy(dst,current.data->data()+current.position,count);
        current.position+=count;
        return static_cast<int>(count);
    }
    virtual int Write(const void*,int,void*){++abiErrors;return -1;}
    virtual void* Open(const char* path,const char* mode,const char* id){
        ++opens;
        if(!path||!mode||std::strcmp(mode,"rb")!=0) {
            ++abiErrors;return nullptr;
        }
        if(std::strcmp(path,"scripts/surfaceproperties_manifest.txt")==0){
            if(!id||std::strcmp(id,"GAME")!=0){
                ++abiErrors;return nullptr;
            }
        } else if(id){
            ++abiErrors;return nullptr;
        }
        if(missing==path)return nullptr;
        const auto it=files.find(path);
        if(it==files.end())return nullptr;
        current={&it->second,0};
        return &current;
    }
    virtual void Close(void* handle){
        ++closes;
        if(handle!=&current||!current.data){++abiErrors;return;}
        current={};
    }
    virtual void Seek(void*,int,int){++abiErrors;}
    virtual unsigned Tell(void*){++abiErrors;return 0;}
    virtual unsigned Size(void* handle){
        if(handle!=&current||!current.data){++abiErrors;return 0;}
        return static_cast<unsigned>(current.data->size());
    }
    virtual unsigned Size(const char*,const char*){++abiErrors;return 0;}
    virtual void Flush(void*){++abiErrors;}
    virtual bool Precache(const char*,const char*){++abiErrors;return false;}
    virtual bool FileExists(const char*,const char*){++abiErrors;return false;}
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
struct Files : FilesBase,AppSystem {};
struct SurfaceProps {
    Files* fs=nullptr;
    std::vector<std::string> filenames{};
    std::vector<std::size_t> bytes{};
    int callsBeforeClose=0;
    virtual void Reserved0(){}
    virtual int ParseSurfaceData(const char* filename,const char* raw){
        if(!fs||fs->current.data||fs->closes!=3)++callsBeforeClose;
        filenames.emplace_back(filename?filename:"");
        bytes.emplace_back(raw?std::strlen(raw):0u);
        return static_cast<int>(filenames.size()*11u);
    }
};
int main(){
    static_assert(sizeof(void*)==4,"native Win32 x86 fixture");
    Files engine{};
    const std::string manifest=
        "\"surfaceproperties_manifest\" { "
        "\"file\" \"scripts/surfaceproperties.txt\" "
        "\"file\" \"scripts/surfaceproperties_hl2.txt\" }";
    engine.files={
        {"scripts/surfaceproperties_manifest.txt",manifest},
        {"scripts/surfaceproperties.txt",Document("fixture",84,62,23485u)},
        {"scripts/surfaceproperties_hl2.txt",Document("hl2",9,9,2812u)}
    };
    if(engine.files[std::string(own::kFirstSource)].size()!=23485u ||
       engine.files[std::string(own::kSecondSource)].size()!=2812u)
        return Fail(1);
    SurfaceProps props{};
    props.fs=&engine;
    auto report=reg::Attempt(&engine,&props);
    if(!report.completed_calls()||report.first_return!=11||
       report.second_return!=22||props.filenames.size()!=2||
       props.filenames[0]!=own::kFirstSource||
       props.filenames[1]!=own::kSecondSource||
       props.bytes[0]!=23485u||props.bytes[1]!=2812u||
       props.callsBeforeClose!=0||engine.opens!=3||
       engine.closes!=3||engine.abiErrors!=0)return Fail(2);

    const auto baseline=props.filenames.size();
    report=reg::Attempt(&engine,nullptr);
    if(report.outcome!=reg::Outcome::InvalidInterface||
       props.filenames.size()!=baseline)return Fail(3);

    report=reg::Attempt(nullptr,&props);
    if(report.outcome!=reg::Outcome::InvalidFileSystem||
       props.filenames.size()!=baseline)return Fail(4);

    engine.missing=std::string(own::kSecondSource);
    report=reg::Attempt(&engine,&props);
    if(report.outcome!=reg::Outcome::ResourceShapeRefused||
       props.filenames.size()!=baseline||engine.current.data)
        return Fail(5);
    engine.missing.clear();

    engine.files[std::string(own::kFirstSource)].pop_back();
    report=reg::Attempt(&engine,&props);
    if(report.outcome!=reg::Outcome::RawSourceRefused||
       props.filenames.size()!=baseline||engine.current.data)
        return Fail(6);
    engine.files[std::string(own::kFirstSource)].push_back(' ');

    engine.files[std::string(own::kFirstSource)].back()='\0';
    report=reg::Attempt(&engine,&props);
    if(report.completed_calls()||props.filenames.size()!=baseline)
        return Fail(7);
    engine.files[std::string(own::kFirstSource)].back()=' ';

    engine.shortRead=true;
    report=reg::Attempt(&engine,&props);
    if(report.outcome!=reg::Outcome::ResourceShapeRefused||
       props.filenames.size()!=baseline)return Fail(8);
    engine.shortRead=false;

    engine.files["scripts/surfaceproperties_manifest.txt"]=
        "\"surfaceproperties_manifest\" { "
        "\"file\" \"scripts/../wrong.txt\" }";
    report=reg::Attempt(&engine,&props);
    if(report.outcome!=reg::Outcome::ResourceShapeRefused||
       props.filenames.size()!=baseline)return Fail(9);

    if(engine.abiErrors!=0)return Fail(10);
    std::puts("PASS original 2006 surfaceprops: complete pre-read and close, native x86 virtual slot1 calls, ordered filenames, bounded resource refusals; no retail engine invoked in CI");
    return 0;
}
