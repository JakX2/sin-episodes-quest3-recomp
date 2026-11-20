// Portable independent surfaceproperties manifest + two retail source
// shape fixtures. Original 2006 content and engine resources NOT used.
#include "client_original_surfaceprops_manifest_owned.h"
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
namespace s=sin_independent::original_surfaceprops;
static int Fail(int n){
    std::fprintf(stderr,"original surfaceproperties owner fixture=%d\n",n);
    return n;
}
static std::string MakeSource(const char* prefix,int count,int bases,
                              bool duplicateFirst=false) {
    std::string out="// independent synthetic physics surface test\n";
    for(int i=0;i<count;++i) {
        const std::string name=duplicateFirst&&i==0?"default":
            (i==0&&std::string(prefix)=="retail"?"default":
             std::string(prefix)+"_"+std::to_string(i));
        out+="\""+name+"\" {\n";
        if(i<(std::string(prefix)=="retail"?bases+1:bases))
            if(!(std::string(prefix)=="retail"&&i==0))
                out+="\"base\" \"default\"\n";
        out+="\"density\" \"2000\"\n\"friction\" \"0.5\"\n}\n";
    }
    return out;
}
static const std::string kManifest=
    "\"surfaceproperties_manifest\" { "
    "\"file\" \"scripts/surfaceproperties.txt\" "
    "\"file\" \"scripts/surfaceproperties_hl2.txt\" }";
struct Fixture {
    std::map<std::string,std::string> data;
    int manifest_game=0;
    int script_null=0;
    int abi_errors=0;
    bool throw_script=false;
    bool Read(std::string_view path,const char* scope,
              std::string& out) {
        const bool manifest=path==s::kManifestPath;
        if(manifest) {
            ++manifest_game;
            if(!scope||std::string_view(scope)!="GAME"){
                ++abi_errors;return false;
            }
        }else{
            ++script_null;
            if(scope){++abi_errors;return false;}
            if(throw_script)throw std::runtime_error("injected backend fault");
        }
        auto it=data.find(std::string(path));
        if(it==data.end())return false;
        out=it->second;
        return true;
    }
};
int main(){
    s::OwnedResources owned{};
    Fixture f{};
    f.data={
        {std::string(s::kManifestPath),kManifest},
        {std::string(s::kFirstSource),MakeSource("retail",84,62)},
        {std::string(s::kSecondSource),MakeSource("hl2",9,9)}
    };
    const auto reader=[&](std::string_view path,const char* scope,
                          std::string& output) {
        return f.Read(path,scope,output);
    };
    auto r=owned.Load(reader);
    if(r.status!=s::Status::RetailShapeReady ||!r.verified() ||
       r.attempted!=2 ||r.read!=2 ||
       r.definitions!=93 ||r.base_keys!=71 ||
       r.repeated_names!=0 ||owned.revision()!=1 ||
       owned.value().manifest_root!="surfaceproperties_manifest"||
       owned.value().sources.size()!=2 ||
       owned.value().sources[0].path!=s::kFirstSource ||
       owned.value().sources[1].path!=s::kSecondSource ||
       owned.value().sources[0].definitions!=84 ||
       owned.value().sources[1].definitions!=9 ||
       f.manifest_game!=1 ||f.script_null!=2 ||f.abi_errors!=0)
        return Fail(1);
    const auto expectedRevision=owned.revision();

    f.data.erase(std::string(s::kManifestPath));
    r=owned.Load(reader);
    if(r.status!=s::Status::ManifestUnavailable||
       owned.revision()!=expectedRevision||
       owned.value().sources.size()!=2)return Fail(2);
    f.data[std::string(s::kManifestPath)]=kManifest;

    f.data.erase(std::string(s::kSecondSource));
    r=owned.Load(reader);
    if(r.status!=s::Status::SourceUnavailable||
       r.attempted!=2||r.read!=1||
       owned.revision()!=expectedRevision)return Fail(3);
    f.data[std::string(s::kSecondSource)]=MakeSource("hl2",9,9);

    f.data[std::string(s::kManifestPath)]=
       "\"surfaceproperties_manifest\" { \"file\" \"scripts/../other.txt\" }";
    r=owned.Load(reader);
    if(r.status!=s::Status::InvalidManifest ||
       owned.revision()!=expectedRevision)return Fail(4);
    f.data[std::string(s::kManifestPath)]=
        "\"surfaceproperties_manifest\" { "
        "\"file\" \"scripts/surfaceproperties.txt\" "
        "\"file\" \"scripts/surfaceproperties.txt\" }";
    r=owned.Load(reader);
    if(r.status!=s::Status::UnexpectedSourceList ||
       owned.revision()!=expectedRevision)return Fail(5);
    f.data[std::string(s::kManifestPath)]=kManifest;

    f.data[std::string(s::kSecondSource)]="\"broken\" { \"base\" ";
    r=owned.Load(reader);
    if(r.status!=s::Status::InvalidSource ||
       owned.revision()!=expectedRevision)return Fail(6);
    f.data[std::string(s::kSecondSource)]=MakeSource("hl2",9,9,true);
    r=owned.Load(reader);
    if(r.status!=s::Status::UnexpectedSurfaceCounts ||
       r.repeated_names!=1 ||owned.revision()!=expectedRevision)
        return Fail(7);
    f.data[std::string(s::kSecondSource)]=MakeSource("hl2",8,8);
    r=owned.Load(reader);
    if(r.status!=s::Status::UnexpectedSurfaceCounts ||
       r.definitions!=92 ||r.base_keys!=70 ||
       owned.revision()!=expectedRevision)return Fail(8);
    f.data[std::string(s::kSecondSource)]=MakeSource("hl2",9,9);

    f.data[std::string(s::kSecondSource)]=std::string(
        s::kMaxSingleSourceBytes+1u,'Z');
    r=owned.Load(reader);
    if(r.status!=s::Status::OwnedSizeLimit ||
       owned.revision()!=expectedRevision)return Fail(9);
    f.data[std::string(s::kSecondSource)]=MakeSource("hl2",9,9);
    f.throw_script=true;
    r=owned.Load(reader);
    if(r.status!=s::Status::AllocationRefused ||
       owned.revision()!=expectedRevision)return Fail(10);
    f.throw_script=false;

    r=owned.Load(reader);
    if(!r.verified()||owned.revision()!=expectedRevision+1)return Fail(11);
    owned.Clear();
    if(owned.revision()!=expectedRevision+2||
       !owned.value().sources.empty())return Fail(12);
    std::puts("PASS original 2006 physics surfaceproperties manifest: exact rb/GAME root, 2 rb/null files, synthetic 84+9 definitions and 62+9 base keys, duplicate/malformed/path/size/throw rollback");
    return 0;
}
