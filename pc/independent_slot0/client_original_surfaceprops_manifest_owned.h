#pragma once
// Owned original 2006 surface-properties manifest resource model.
//
// The source original client helper 0x241927A0 (full 247-byte SHA256
// 7357bdfb9934f175e0d67f4a81236d81087a07bb6145918083c44fd255ca6905)
// loads scripts/surfaceproperties_manifest.txt through the existing
// KeyValues LoadFromFile helper with pathID="GAME". Each manifest
// "file" entry is passed to 0x24191EC0, which opens that filename
// via VFileSystem012::Open with "rb", pathID=nullptr, and passes
// the raw file text to the original physics surface-props interface
// virtual slot1. That final registration is NOT implemented here.
//
// Source-verified original retail manifest has two entries:
// scripts/surfaceproperties.txt and scripts/surfaceproperties_hl2.txt.
// The original assets have 84+9 top-level surface definitions and
// 62+9 base keys, all 93 unique case-insensitively. These counts are
// *retail shape diagnostics* only, NOT all Source mod manifest support.
//
// This code stores ONLY independently owned name/count metadata and
// never invokes the proprietary Source surface-registration interface.
#include "client_soundscape_manifest_owned.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sin_independent::original_surfaceprops {
namespace kv=sin_independent::soundscape_manifest;
inline constexpr std::string_view kManifestPath=
    "scripts/surfaceproperties_manifest.txt";
inline constexpr std::string_view kManifestRoot=
    "surfaceproperties_manifest";
inline constexpr std::string_view kFirstSource=
    "scripts/surfaceproperties.txt";
inline constexpr std::string_view kSecondSource=
    "scripts/surfaceproperties_hl2.txt";
inline constexpr std::size_t kMaxManifestBytes=65536u;
inline constexpr std::size_t kMaxSingleSourceBytes=131072u;
inline constexpr std::size_t kMaxTotalBytes=262144u;
inline constexpr std::size_t kMaxSurfaceNames=512u;
inline constexpr unsigned kRetailFirstDefinitions=84u;
inline constexpr unsigned kRetailSecondDefinitions=9u;
inline constexpr unsigned kRetailFirstBaseEntries=62u;
inline constexpr unsigned kRetailSecondBaseEntries=9u;
enum class Status {
    RetailShapeReady, ManifestUnavailable, InvalidManifest,
    UnexpectedSourceList, SourceUnavailable, InvalidSource,
    UnexpectedSurfaceCounts, OwnedSizeLimit, AllocationRefused
};
inline const char* Name(Status status) noexcept {
    switch(status){
        case Status::RetailShapeReady:
            return "original-surfaceprops-owned-shape-ready";
        case Status::ManifestUnavailable:
            return "original-surfaceprops-manifest-missing";
        case Status::InvalidManifest:
            return "original-surfaceprops-manifest-invalid";
        case Status::UnexpectedSourceList:
            return "original-surfaceprops-list-unexpected";
        case Status::SourceUnavailable:
            return "original-surfaceprops-file-missing";
        case Status::InvalidSource:
            return "original-surfaceprops-file-invalid";
        case Status::UnexpectedSurfaceCounts:
            return "original-surfaceprops-counts-unexpected";
        case Status::OwnedSizeLimit:
            return "original-surfaceprops-size-limit";
        case Status::AllocationRefused:
            return "original-surfaceprops-allocation-refused";
    }
    return "unknown";
}
struct Source final {
    std::string path;
    unsigned definitions=0;
    unsigned base_keys=0;
};
struct Snapshot final {
    std::string manifest_root;
    std::vector<Source> sources{};
    unsigned definitions=0;
    unsigned base_keys=0;
    unsigned repeated_names=0;
    std::size_t source_bytes=0;
};
struct Report final {
    Status status=Status::ManifestUnavailable;
    unsigned attempted=0;
    unsigned read=0;
    unsigned definitions=0;
    unsigned base_keys=0;
    unsigned repeated_names=0;
    std::size_t source_bytes=0;
    bool verified() const noexcept {
        return status==Status::RetailShapeReady&&attempted==2&&
               read==2&&definitions==93&&base_keys==71&&
               repeated_names==0;
    }
};
namespace detail {
using Kind=kv::detail::Kind;
using Lexer=kv::detail::Lexer;
inline bool ParseSurfaceSource(std::string_view body,
                               Source& source,
                               std::vector<std::string>& allNames,
                               unsigned& duplicateCount) {
    if(body.empty()||body.size()>kMaxSingleSourceBytes)return false;
    Lexer lex(body);
    unsigned found=0;
    unsigned bases=0;
    while(true) {
        auto name=lex.Next();
        if(name.kind==Kind::End)break;
        if(name.kind!=Kind::Word||name.text.empty()||
           lex.Next().kind!=Kind::Open)
            return false;
        if(allNames.size()>=kMaxSurfaceNames)return false;
        for(const auto& prior:allNames)
            if(kv::detail::EqualsAsciiIgnoreCase(prior,name.text))
                ++duplicateCount;
        allNames.push_back(std::move(name.text));
        ++found;
        while(true) {
            const auto field=lex.Next();
            if(field.kind==Kind::Close)break;
            if(field.kind!=Kind::Word||field.text.empty())
                return false;
            const auto value=lex.Next();
            if(value.kind==Kind::Word){
                if(kv::detail::EqualsAsciiIgnoreCase(field.text,"base"))
                    ++bases;
                continue;
            }
            // Full Source KeyValues allows recursive subtrees.
            // This count parser accepts them only when balanced,
            // without inventing their physics semantics.
            if(value.kind==Kind::Open &&
               kv::detail::SkipObject(lex,1))continue;
            return false;
        }
    }
    source.definitions=found;
    source.base_keys=bases;
    return found>0;
}
} // namespace detail
class OwnedResources final {
public:
    const Snapshot& value() const noexcept{return value_;}
    std::uint64_t revision() const noexcept{return revision_;}
    void Clear() noexcept{value_={};++revision_;}
    // Read(string_view relativePath,const char* originalPathId,
    //      std::string& output) -> bool
    // No native engine objects or VFileSystem pointers used here.
    template<class Read>
    Report Load(Read&& read) noexcept {
        Report result{};
        try{
            std::string rawManifest;
            if(!read(kManifestPath,"GAME",rawManifest))
                return result;
            if(rawManifest.empty()||rawManifest.size()>kMaxManifestBytes){
                result.status=Status::OwnedSizeLimit;
                return result;
            }
            kv::Manifest parsed{};
            if(kv::Parse(rawManifest,parsed)!=kv::Status::Ready||
               !kv::detail::EqualsAsciiIgnoreCase(parsed.root,kManifestRoot)){
                result.status=Status::InvalidManifest;
                return result;
            }
            // The SHIPPED 2006 retail manifest (not every mod)
            // must contain these two exact file names in order.
            if(parsed.file_paths.size()!=2u||
               !kv::detail::EqualsAsciiIgnoreCase(
                    parsed.file_paths[0],kFirstSource)||
               !kv::detail::EqualsAsciiIgnoreCase(
                    parsed.file_paths[1],kSecondSource)){
                result.status=Status::UnexpectedSourceList;
                return result;
            }
            Snapshot staged{};
            staged.manifest_root=std::move(parsed.root);
            std::vector<std::string> names;
            names.reserve(96);
            for(const auto& path:parsed.file_paths){
                ++result.attempted;
                std::string body;
                if(!read(path,nullptr,body)){
                    result.status=Status::SourceUnavailable;
                    return result;
                }
                ++result.read;
                if(body.empty()||body.size()>kMaxSingleSourceBytes||
                   body.size()>kMaxTotalBytes-staged.source_bytes){
                    result.status=Status::OwnedSizeLimit;
                    return result;
                }
                staged.source_bytes+=body.size();
                Source one{};
                one.path=path;
                if(!detail::ParseSurfaceSource(
                    body,one,names,staged.repeated_names)){
                    result.status=Status::InvalidSource;
                    return result;
                }
                staged.definitions+=one.definitions;
                staged.base_keys+=one.base_keys;
                staged.sources.push_back(std::move(one));
            }
            result.definitions=staged.definitions;
            result.base_keys=staged.base_keys;
            result.repeated_names=staged.repeated_names;
            result.source_bytes=staged.source_bytes;
            if(staged.sources.size()!=2u||
               staged.sources[0].definitions!=kRetailFirstDefinitions||
               staged.sources[1].definitions!=kRetailSecondDefinitions||
               staged.sources[0].base_keys!=kRetailFirstBaseEntries||
               staged.sources[1].base_keys!=kRetailSecondBaseEntries||
               staged.repeated_names!=0u){
                result.status=Status::UnexpectedSurfaceCounts;
                return result;
            }
            value_=std::move(staged);
            ++revision_;
            result.status=Status::RetailShapeReady;
            return result;
        }catch(...){
            result.status=Status::AllocationRefused;
            return result;
        }
    }
private:
    Snapshot value_{};
    std::uint64_t revision_=0u;
};
} // namespace sin_independent::original_surfaceprops
