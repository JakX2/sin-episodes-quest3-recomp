// Phase 482 native MSVC x86 smoke and malformed-file matrix.
// Pure synthetic soundscape text. Original retail files are private.
#include "client_soundscape_manifest_owned.h"
#include <array>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace m=sin_independent::soundscape_manifest;
static int Fail(int id){
    std::fprintf(stderr,"soundscape owned manifest test failed %d\n",id);
    return id;
}
static const std::string sample=R"KV(
soundscaples_manifest
{
    "file"  "scripts/soundscapes.txt"
    // A comment and a repeated "file" key are valid KeyValues.
    "file"  "scripts/soundscapes_intro.txt"
    "FILE"  "scripts/soundscapes_docks.txt"
    "foo" "bar"
    "ignored"
    {
        "subkey" "not a file"
    }
    "file" "scripts/soundscapes_pit.txt"
}
)KV";
struct Provider final {
    std::vector<std::pair<std::string,std::string>> texts{};
    std::string missing{};
    std::string oversized{};
    unsigned reads=0;
    bool Read(std::string_view path,std::string& result){
        ++reads;
        if(path==missing)return false;
        if(path==oversized){
            result.assign(m::kMaxSingleSoundscapeBytes+1,'x');
            return true;
        }
        for(const auto& item:texts)
            if(item.first==path){
                result=item.second;
                return true;
            }
        return false;
    }
};
int main(){
    m::Manifest first{};
    if(m::Parse(sample,first)!=m::Status::Ready ||
       first.root!=m::kObservedRetailRoot ||
       first.unknown_keys!=2 || first.file_paths.size()!=4 ||
       first.file_paths[0]!="scripts/soundscapes.txt" ||
       first.file_paths[1]!="scripts/soundscapes_intro.txt" ||
       first.file_paths[2]!="scripts/soundscapes_docks.txt" ||
       first.file_paths[3]!="scripts/soundscapes_pit.txt")
        return Fail(1);

    Provider provider{};
    provider.texts={
        {"scripts/soundscapes.txt","base"},
        {"scripts/soundscapes_intro.txt","intro"},
        {"scripts/soundscapes_docks.txt","docks"},
        {"scripts/soundscapes_pit.txt","pit"}
    };
    auto get=[&](std::string_view path,std::string& body){
        return provider.Read(path,body);
    };
    m::OwnedSnapshot loaded;
    if(loaded.Load(sample,get)!=m::Status::Ready ||
       loaded.value().files.size()!=4 ||
       loaded.value().total_bytes!=17 ||
       loaded.value().manifest.unknown_keys!=2 ||
       !loaded.Find("scripts/soundscapes_intro.txt") ||
       *loaded.Find("scripts/soundscapes_intro.txt")!="intro" ||
       loaded.Find("scripts/other.txt"))
        return Fail(2);
    provider.missing="scripts/soundscapes_docks.txt";
    if(loaded.Load(sample,get)!=m::Status::MissingFile ||
       loaded.value().files.size()!=4 ||
       loaded.value().total_bytes!=17 ||
       *loaded.Find("scripts/soundscapes_docks.txt")!="docks")
        return Fail(3);
    provider.missing.clear();
    provider.oversized="scripts/soundscapes_intro.txt";
    if(loaded.Load(sample,get)!=m::Status::FileTooLarge ||
       loaded.value().files.size()!=4 ||
       loaded.value().total_bytes!=17)
        return Fail(4);
    provider.oversized.clear();

    m::Manifest unchanged=first;
    const auto refuse=[&](std::string_view text,m::Status expected){
        return m::Parse(text,unchanged)==expected &&
               unchanged.file_paths==first.file_paths &&
               unchanged.root==first.root;
    };
    if(!refuse(R"(root { "file" "../escape.txt" })",
               m::Status::UnsafeFilePath) ||
       !refuse(R"(root { "file" "/scripts/soundscapes.txt" })",
               m::Status::UnsafeFilePath) ||
       !refuse(R"(root { "file" "scripts/../../escape.txt" })",
               m::Status::UnsafeFilePath) ||
       !refuse(R"(root { "file" "scripts\\soundscapes.txt" })",
               m::Status::UnsafeFilePath) ||
       !refuse(R"(root { "file" "scripts//soundscapes.txt" })",
               m::Status::UnsafeFilePath) ||
       !refuse(R"(root { "file" "scripts/foo.cfg" })",
               m::Status::UnsafeFilePath))
        return Fail(5);
    if(!refuse(R"(root { "file" { "bad" "nest" } })",
               m::Status::Malformed) ||
       !refuse(R"(root { "file" "scripts/soundscapes.txt")",
               m::Status::Malformed) ||
       !refuse(R"(root { "file" "scripts/soundscapes.txt" } extra)",
               m::Status::Malformed) ||
       !refuse(R"(root { "file" "scripts/soundscapes.txt" } })",
               m::Status::Malformed) ||
       !refuse(R"(root { "file" "scripts/soundscapes.txt
               })",m::Status::Malformed) ||
       !refuse(R"(root { "ignored" "value" })",
               m::Status::NoFileEntries) ||
       !refuse(R"(root { "ignored" { "nested" { "broken" "text" } })",
               m::Status::Malformed))
        return Fail(6);

    const std::string longManifest(m::kMaxManifestBytes+1,'a');
    if(!refuse(longManifest,m::Status::ManifestTooLarge))
        return Fail(7);
    std::string bigList="root { ";
    for(std::size_t i=0;i<m::kMaxFileEntries+1;++i)
        bigList+="\"file\" \"scripts/soundscapes.txt\" ";
    bigList+=" }";
    if(!refuse(bigList,m::Status::TooManyEntries))return Fail(8);

    // The original game may load the same file more than once.
    // Do NOT silently deduplicate repeated KeyValues file entries.
    const std::string duplicated=
        "soundscaples_manifest { \"file\" \"scripts/soundscapes.txt\" "
        "\"file\" \"scripts/soundscapes.txt\" }";
    m::Manifest repeat{};
    if(m::Parse(duplicated,repeat)!=m::Status::Ready ||
       repeat.file_paths.size()!=2)return Fail(9);
    if(loaded.Load(duplicated,get)!=m::Status::Ready ||
       loaded.value().files.size()!=2 ||
       loaded.value().total_bytes!=8)return Fail(10);

    // BOM and CRLF are tolerated without normalizing the observed root.
    const std::string bom="\xef\xbb\xbf"
        "soundscaples_manifest\r\n{\r\n"
        "\"file\" \"scripts/soundscapes.txt\"\r\n}\r\n";
    if(loaded.Load(bom,get)!=m::Status::Ready ||
       loaded.value().manifest.root!=m::kObservedRetailRoot ||
       loaded.value().files.size()!=1)return Fail(11);

    // Our snapshot has a bound of 4 MiB, independent of Source's
    // permissive KeyValues dynamic growth behavior.
    const auto bigBody=std::string(m::kMaxSingleSoundscapeBytes,'a');
    auto bigReader=[&](std::string_view,std::string& out){
        out=bigBody;return true;
    };
    std::string nine="root { ";
    for(int i=0;i<9;++i)
        nine+="\"file\" \"scripts/soundscapes.txt\" ";
    nine+="}";
    if(loaded.Load(nine,bigReader)!=m::Status::TotalSizeExceeded ||
       loaded.value().files.size()!=1 ||
       loaded.value().total_bytes!=4)return Fail(12);

    loaded.Clear();
    if(loaded.value().files.size()!=0 ||
       loaded.value().total_bytes!=0 || !loaded.value().manifest.root.empty())
        return Fail(13);
    std::puts("PASS original-root soundscape manifest: bounded KV lexer, ordered repeated file keys, unknown entry skip, transactional 4MiB asset staging and path refusals");
    return 0;
}
