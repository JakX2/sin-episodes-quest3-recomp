#pragma once
// Independent 32-bit Source 2004 VFileSystem012 read-only probe.
// SDK 2004: IFileSystem : IBaseFileSystem, IAppSystem. Primary slots
// Read=0, Open=2, Close=3, Size(handle)=6, FileExists=10.
// This is an SDK-guided ABI experiment, NOT yet owner-retail verified.
#if !defined(_M_IX86)
#error Windows MSVC x86 only
#endif
#include "client_known_gamesystem_init.h"
#include "client_stats_asset_loader.h"
#include <cstddef>
#include <cstdio>
#include <string>
#include <utility>

namespace sin_independent::engine_fs {
inline constexpr const char* kDefinitions="scripts/stats/stats_definitions.txt";
inline constexpr const char* kLayout="scripts/stats/stats_display_layout.txt";
inline constexpr std::size_t kLimit=256u*1024u;
enum class Status {
    InvalidAbi, MissingDefinitions, MissingLayout, BothPresent,
    ReadRefused, UnexpectedCounts, ParsedAndVerified
};
inline const char* Name(Status value) noexcept {
    switch(value) {
        case Status::InvalidAbi:return "invalid-abi";
        case Status::MissingDefinitions:return "missing-definitions";
        case Status::MissingLayout:return "missing-layout";
        case Status::BothPresent:return "both-present";
        case Status::ReadRefused:return "read-refused";
        case Status::UnexpectedCounts:return "unexpected-counts";
        case Status::ParsedAndVerified:return "parsed-and-verified";
    }
    return "unknown";
}
struct Counts {std::size_t definitions=0, refs=0, pairs=0, dividers=0;};
using Exists=bool (__thiscall *)(void*,const char*,const char*);
using Open=void* (__thiscall *)(void*,const char*,const char*,const char*);
using Close=void (__thiscall *)(void*,void*);
using Size=unsigned (__thiscall *)(void*,void*);
using Read=int (__thiscall *)(void*,void*,int,void*);
struct Methods {
    Exists exists=nullptr;
    Open open=nullptr;
    Close close=nullptr;
    Size size=nullptr;
    Read read=nullptr;
};
inline bool Resolve(void* fileSystem,Methods& out,bool deep) noexcept {
    using sin_independent::known_game_system::CallResult;
    using sin_independent::known_game_system::VerifiedMethod;
    out={};
    CallResult refused=CallResult::InvalidObject;
    auto* exists=VerifiedMethod(fileSystem,10,refused);
    if(!exists)return false;
    out.exists=reinterpret_cast<Exists>(exists);
    if(!deep)return true;
    auto* open=VerifiedMethod(fileSystem,2,refused);
    auto* close=VerifiedMethod(fileSystem,3,refused);
    auto* size=VerifiedMethod(fileSystem,6,refused);
    auto* read=VerifiedMethod(fileSystem,0,refused);
    if(!open||!close||!size||!read)return false;
    out.open=reinterpret_cast<Open>(open);
    out.close=reinterpret_cast<Close>(close);
    out.size=reinterpret_cast<Size>(size);
    out.read=reinterpret_cast<Read>(read);
    return true;
}
inline Status Presence(void* fileSystem) noexcept {
    Methods f{};
    if(!Resolve(fileSystem,f,false))return Status::InvalidAbi;
    if(!f.exists(fileSystem,kDefinitions,nullptr))
        return Status::MissingDefinitions;
    if(!f.exists(fileSystem,kLayout,nullptr))
        return Status::MissingLayout;
    return Status::BothPresent;
}
struct CloseGuard {
    void* owner=nullptr;
    void* handle=nullptr;
    Close close=nullptr;
    ~CloseGuard() noexcept {if(owner&&handle&&close)close(owner,handle);}
};
inline bool ReadOne(void* fileSystem,const Methods& f,const char* path,
                    std::string& output) {
    void* handle=f.open(fileSystem,path,"rb",nullptr);
    if(!handle)return false;
    CloseGuard guard{fileSystem,handle,f.close};
    const unsigned length=f.size(fileSystem,handle);
    if(length==0 || length>kLimit)return false;
    std::string buffer(length,'\0');
    std::size_t consumed=0;
    while(consumed<length) {
        // Read exactly remaining bytes, stopping on zero/negative or
        // impossible overlong responses to prevent nonprogress loops.
        const int requested=static_cast<int>(length-consumed);
        const int returned=f.read(fileSystem,&buffer[consumed],requested,handle);
        if(returned<=0 || returned>requested)return false;
        consumed+=static_cast<std::size_t>(returned);
    }
    output.swap(buffer);
    return true;
}
inline Status ReadAndValidate(void* fileSystem,Counts* counts=nullptr) noexcept {
    if(counts)*counts={};
    const auto presence=Presence(fileSystem);
    if(presence!=Status::BothPresent)return presence;
    Methods f{};
    if(!Resolve(fileSystem,f,true))return Status::InvalidAbi;
    try {
        std::string definitions,layout;
        if(!ReadOne(fileSystem,f,kDefinitions,definitions) ||
           !ReadOne(fileSystem,f,kLayout,layout))
            return Status::ReadRefused;
        stats_assets::Documents parsed;
        std::string error;
        if(!parsed.LoadTexts(definitions,layout,error))return Status::ReadRefused;
        Counts n{};
        n.definitions=parsed.definitions().size();
        for(const auto& entry:parsed.definitions())
            if(!entry.percentage_of.empty())++n.refs;
        for(const auto& entry:parsed.display())
            if(entry.divider)++n.dividers;
            else ++n.pairs;
        if(counts)*counts=n;
        if(n.definitions!=62||n.refs!=8||n.pairs!=156||n.dividers!=66)
            return Status::UnexpectedCounts;
        return Status::ParsedAndVerified;
    } catch(...) {
        return Status::ReadRefused;
    }
}
} // namespace sin_independent::engine_fs
