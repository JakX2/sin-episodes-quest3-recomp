#pragma once
// Phase 483: independently owned soundscape definition tree and name graph.
//
// The 2006 SHA-authenticated C_SoundscapeSystem::Init walks manifest
// entries and loads each referenced KeyValues soundscape document.
// Real scripts have top-level soundscape names, nested playlooping,
// playrandom/rndwave and playsoundscape blocks, including repeated keys.
// This parser preserves their structure, without running Source sound,
// IFileSystem, KeyValues or any original DLL code.
#include "client_soundscape_manifest_owned.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sin_independent::soundscape_catalog {
inline constexpr std::size_t kMaxDefinitions=4096u;
inline constexpr std::size_t kMaxNodes=24000u;
inline constexpr unsigned kMaxDepth=20u;
enum class Status {
    Ready, MissingManifestSnapshot, MalformedDefinition,
    EmptyDefinitionFile, DuplicateSoundscapeName,
    TooManyDefinitions, TooManyNodes, NestingTooDeep, AllocationRefused
};
inline const char* StatusName(Status status) noexcept {
    switch(status){
        case Status::Ready:return "independent-soundscape-catalog-ready";
        case Status::MissingManifestSnapshot:return "soundscape-staging-not-ready";
        case Status::MalformedDefinition:return "malformed-soundscape-keyvalues";
        case Status::EmptyDefinitionFile:return "soundscape-document-has-no-definitions";
        case Status::DuplicateSoundscapeName:return "ambiguous-soundscape-name";
        case Status::TooManyDefinitions:return "soundscape-definition-cap";
        case Status::TooManyNodes:return "soundscape-node-cap";
        case Status::NestingTooDeep:return "soundscape-depth-cap";
        case Status::AllocationRefused:return "soundscape-catalog-allocation-refused";
    }
    return "unknown";
}
struct Node final {
    std::string key;
    std::string scalar;
    std::vector<Node> children;
    bool is_block=false;
};
struct Definition final {
    std::string name;
    std::string source;
    std::vector<Node> children;
};
struct Reference final {
    std::string owner;
    std::string target;
    std::string source;
};
struct Counts final {
    std::size_t documents=0;
    std::size_t definitions=0;
    std::size_t property_nodes=0;
    std::size_t looping_blocks=0;
    std::size_t random_blocks=0;
    std::size_t random_wave_blocks=0;
    std::size_t wave_properties=0;
    std::size_t nested_soundscape_blocks=0;
    std::size_t unresolved_references=0;
};
struct CatalogState final {
    std::vector<Definition> definitions;
    std::vector<Reference> references;
    std::vector<Reference> unresolved;
    Counts counts{};
};

namespace detail {
using Token=sin_independent::soundscape_manifest::detail::Token;
using Kind=sin_independent::soundscape_manifest::detail::Kind;
using Lexer=sin_independent::soundscape_manifest::detail::Lexer;
inline bool Eq(std::string_view a,std::string_view b) noexcept {
    return sin_independent::soundscape_manifest::detail::
        EqualsAsciiIgnoreCase(a,b);
}
// Each Source KeyValues block owns a vector of ordered children.
// Repeated child keys are expected (playlooping, wave etc.).
inline Status ParseChildren(Lexer& lexer,std::vector<Node>& children,
                           std::size_t& nodeCount,unsigned depth){
    if(depth>kMaxDepth)return Status::NestingTooDeep;
    while(true){
        auto key=lexer.Next();
        if(key.kind==Kind::Close)return Status::Ready;
        if(key.kind!=Kind::Word||key.text.empty())
            return Status::MalformedDefinition;
        auto payload=lexer.Next();
        Node node{};
        node.key=std::move(key.text);
        if(payload.kind==Kind::Word)
            node.scalar=std::move(payload.text);
        else if(payload.kind==Kind::Open){
            node.is_block=true;
            const Status child=ParseChildren(
                lexer,node.children,nodeCount,depth+1);
            if(child!=Status::Ready)return child;
        }else return Status::MalformedDefinition;
        if(nodeCount==kMaxNodes)return Status::TooManyNodes;
        ++nodeCount;
        children.push_back(std::move(node));
    }
}
inline Status ParseDocument(std::string_view text,std::string_view source,
                            CatalogState& out){
    if(text.size()>soundscape_manifest::kMaxSingleSoundscapeBytes)
        return Status::MalformedDefinition;
    Lexer lexer(text);
    std::size_t before=out.definitions.size();
    while(true){
        auto name=lexer.Next();
        if(name.kind==Kind::End)break;
        if(name.kind!=Kind::Word||name.text.empty()||
           lexer.Next().kind!=Kind::Open)
            return Status::MalformedDefinition;
        if(out.definitions.size()>=kMaxDefinitions)
            return Status::TooManyDefinitions;
        Definition next{};
        next.name=std::move(name.text);
        next.source=source;
        const Status contents=ParseChildren(
            lexer,next.children,out.counts.property_nodes,1u);
        if(contents!=Status::Ready)return contents;
        out.definitions.push_back(std::move(next));
    }
    return out.definitions.size()==before?
        Status::EmptyDefinitionFile:Status::Ready;
}
inline void Visit(const std::vector<Node>& nodes,const Definition& owner,
                  CatalogState& state){
    for(const auto& node:nodes){
        if(Eq(node.key,"wave")&&!node.is_block)
            ++state.counts.wave_properties;
        if(!node.is_block)continue;
        if(Eq(node.key,"playlooping"))
            ++state.counts.looping_blocks;
        else if(Eq(node.key,"playrandom"))
            ++state.counts.random_blocks;
        else if(Eq(node.key,"rndwave"))
            ++state.counts.random_wave_blocks;
        else if(Eq(node.key,"playsoundscape")){
            ++state.counts.nested_soundscape_blocks;
            for(const auto& item:node.children){
                if(Eq(item.key,"name")&&!item.is_block){
                    state.references.push_back({
                        owner.name,item.scalar,owner.source});
                    break;
                }
            }
        }
        Visit(node.children,owner,state);
    }
}
} // namespace detail

class OwnedCatalog final {
public:
    const CatalogState& value() const noexcept{return state_;}
    void Clear() noexcept{state_={};}
    const Definition* Find(std::string_view name) const noexcept {
        for(const auto& item:state_.definitions)
            if(detail::Eq(item.name,name))return &item;
        return nullptr;
    }
    Status Build(const soundscape_manifest::Snapshot& snapshot) noexcept {
        if(snapshot.files.empty() ||
           snapshot.manifest.file_paths.size()!=snapshot.files.size())
            return Status::MissingManifestSnapshot;
        try {
            CatalogState pending{};
            pending.counts.documents=snapshot.files.size();
            for(std::size_t i=0;i<snapshot.files.size();++i){
                const auto& file=snapshot.files[i];
                if(file.path!=snapshot.manifest.file_paths[i])
                    return Status::MissingManifestSnapshot;
                const auto status=detail::ParseDocument(
                    file.text,file.path,pending);
                if(status!=Status::Ready)return status;
            }
            pending.counts.definitions=pending.definitions.size();
            // The original KeyValues may accept name collisions; our
            // owned lookup rejects ambiguously duplicated names.
            for(std::size_t i=0;i<pending.definitions.size();++i){
                for(std::size_t j=0;j<i;++j){
                    if(detail::Eq(pending.definitions[i].name,
                                  pending.definitions[j].name))
                        return Status::DuplicateSoundscapeName;
                }
            }
            for(const auto& def:pending.definitions)
                detail::Visit(def.children,def,pending);
            for(const auto& link:pending.references){
                bool found=false;
                for(const auto& def:pending.definitions){
                    if(detail::Eq(link.target,def.name)){
                        found=true;
                        break;
                    }
                }
                if(!found)pending.unresolved.push_back(link);
            }
            pending.counts.unresolved_references=pending.unresolved.size();
            state_=std::move(pending);
            return Status::Ready;
        }catch(...){return Status::AllocationRefused;}
    }
private:
    CatalogState state_{};
};
} // namespace sin_independent::soundscape_catalog
