#pragma once
// Independent 32-bit Source 2004 VFileSystem012 read-only probe.
// SDK 2004: IFileSystem : IBaseFileSystem, IAppSystem. Primary slots
// Read=0, Open=2, Close=3, Size(handle)=7, FileExists=10.
// This is an SDK-guided ABI experiment, NOT yet owner-retail verified.
#if !defined(_M_IX86)
#error Windows MSVC x86 only
#endif
#include "client_known_gamesystem_init.h"
#include "client_stats_asset_loader.h"
#include "client_stats_coordinator.h"
#include "client_stats_user_message_hooks.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

namespace sin_independent::engine_fs {
inline constexpr const char* kDefinitions="scripts/stats/stats_definitions.txt";
inline constexpr const char* kLayout="scripts/stats/stats_display_layout.txt";
inline constexpr std::size_t kLimit=256u*1024u;
enum class Status {
    InvalidAbi, MissingDefinitions, MissingLayout, BothPresent,
    ReadRefused, UnexpectedCounts, ParsedAndVerified, TextsRead
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
        case Status::TextsRead:return "texts-read";
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
    // MSVC x86 emits the overloaded Size(const char*,const char*) at
    // slot 6 and Size(FileHandle_t) at slot 7 for this interface layout.
    auto* size=VerifiedMethod(fileSystem,7,refused);
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
                    std::string& output,const char* mode="rb",
                    const char* pathID=nullptr) {
    void* handle=f.open(fileSystem,path,mode,pathID);
    if(!handle)return false;
    CloseGuard guard{fileSystem,handle,f.close};
    const unsigned length=f.size(fileSystem,handle);
    if(length==0 || length>kLimit)return false;
    std::string buffer(length,'\0');
    std::size_t consumed=0;
    while(consumed<length) {
        // Binary resources must return precisely Size(handle) bytes. The
        // original 2006 stats LAYOUT is instead opened with "rt". On a CRT
        // backed filesystem this can translate each CRLF to LF, even when
        // Size(handle) still reports the physical size. Owner-original
        // layout evidence: 8666 on-disk bytes, 262 CRLF line endings.
        const int requested=static_cast<int>(length-consumed);
        const int returned=f.read(fileSystem,&buffer[consumed],requested,handle);
        if(returned<0 || returned>requested)return false;
        if(returned==0) {
            // Do not accept an arbitrary truncated document or nonprogress.
            // The ONLY short-EOF allowance is precisely one removed CR per
            // resulting LF, with no remaining CR octets. This establishes
            // a self-consistent CRLF-to-LF translation; the independent
            // Stats layout parser still verifies all required entries.
            const bool textMode=mode && std::strcmp(mode,"rt")==0;
            if(!textMode || consumed==0)return false;
            std::size_t newlines=0;
            for(std::size_t i=0;i<consumed;++i) {
                if(buffer[i]=='\r')return false;
                if(buffer[i]=='\n')++newlines;
            }
            if(newlines==0 || length-consumed!=newlines)return false;
            buffer.resize(consumed);
            output.swap(buffer);
            return true;
        }
        consumed+=static_cast<std::size_t>(returned);
    }
    output.swap(buffer);
    return true;
}
// This lower-level reader returns BOTH raw documents only after both
// bounded engine reads complete. On refusal, caller strings stay intact.
inline Status ReadPair(void* fileSystem,std::string& definitions,
                       std::string& layout) noexcept {
    const auto presence=Presence(fileSystem);
    if(presence!=Status::BothPresent)return presence;
    Methods methods{};
    if(!Resolve(fileSystem,methods,true))return Status::InvalidAbi;
    try {
        std::string first,second;
        if(!ReadOne(fileSystem,methods,kDefinitions,first) ||
           !ReadOne(fileSystem,methods,kLayout,second))
            return Status::ReadRefused;
        definitions.swap(first);
        layout.swap(second);
        return Status::TextsRead;
    } catch(...) {
        return Status::ReadRefused;
    }
}
inline bool ReadForStatsCoordinator(void* engineFileSystem,
                                    std::string& definitions,
                                    std::string& layout,
                                    std::string& error) noexcept {
    const auto status=ReadPair(engineFileSystem,definitions,layout);
    if(status!=Status::TextsRead) {
        try {error=Name(status);} catch(...) {error.clear();}
        return false;
    }
    error.clear();
    return true;
}
// Original 2006 Stats definitions resource precedes display layout.
// Enforce two precise, read-only game-relative paths and return either
// whole text atomically; this is not original KeyValues object creation.
inline bool ReadForStagedStats(void* engineFileSystem,const char* path,
                               std::string& output,std::string& error) noexcept {
    auto refuse=[&error](const char* reason) noexcept -> bool {
        try {error=reason;} catch(...) {}
        return false;
    };
    if(!path || (std::strcmp(path,kDefinitions)!=0 &&
                 std::strcmp(path,kLayout)!=0))
        return refuse("unrecognized stats resource");
    Methods methods{};
    if(!Resolve(engineFileSystem,methods,true))
        return refuse("invalid-abi");
    try {
        std::string data;
        // SHA-matched 2006 client:
        // KeyValues::LoadFromFile in 0x241FC5A0 opens definitions
        // using "rb" and pathID="GAME", while 0x24204D90 opens
        // layout with "rt" and pathID=nullptr.
        const bool definitions=std::strcmp(path,kDefinitions)==0;
        if(!ReadOne(engineFileSystem,methods,path,data,
                    definitions?"rb":"rt",definitions?"GAME":nullptr))
            return refuse("read-refused");
        output.swap(data);
        error.clear();
        return true;
    } catch(...) {
        return refuse("read-refused");
    }
}
inline Status ReadAndValidate(void* fileSystem,Counts* counts=nullptr) noexcept {
    if(counts)*counts={};
    std::string definitions,layout;
    const auto readStatus=ReadPair(fileSystem,definitions,layout);
    if(readStatus!=Status::TextsRead)return readStatus;
    try {
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
// Next retail milestone after the original 0x53494E6B combined-read probe.
// The original source trace showed TWO different reads during Init:
// definitions "rb"/"GAME", then layout "rt"/nullptr. This adapter calls
// those exact SDK2004 VFileSystem012 methods through ReadForStagedStats,
// binds 62 names into independent 66-slot owners, and parses layout.
// It does NOT invoke original KeyValues, native Stats owner callbacks,
// register HUD methods or return success from original VClient011::Init.
enum class StagedStatus {
    OwnedStagedVerified, InvalidEngineAbi, DefinitionsRefused,
    LayoutRefused, UnexpectedCounts
};
inline const char* StagedName(StagedStatus s) noexcept {
    switch(s) {
        case StagedStatus::OwnedStagedVerified:
            return "owned-staged-verified-not-engine-init";
        case StagedStatus::InvalidEngineAbi:return "invalid-engine-abi";
        case StagedStatus::DefinitionsRefused:return "definitions-stage-refused";
        case StagedStatus::LayoutRefused:return "layout-stage-refused";
        case StagedStatus::UnexpectedCounts:return "unexpected-staged-counts";
    }
    return "unknown";
}
struct StagedCounts {
    std::size_t catalog=0,definitions=0,refs=0,pairs=0,dividers=0;
    std::size_t completed=0,attempted=0;
    std::size_t owned_user_message_hooks=0;
    int hook_ids[3]={-1,-1,-1};
};
// Stage-four UI layout attachment is still a safe no-op, but the last
// THREE original CUserMessages::HookMessage steps now perform genuine
// independently owned metadata registrations, not inert placeholders.
// This is NOT the game's native CUserMessages instance or dispatch.
inline StagedStatus ReadBindAndValidateStaged(
    void* engineFileSystem,StagedCounts* observed=nullptr) noexcept {
    if(observed)*observed={};
    Methods methods{};
    if(!Resolve(engineFileSystem,methods,true))
        return StagedStatus::InvalidEngineAbi;
    try {
        stats_coordinator::State state;
        stats_messages::OwnedRegistry messageHooks;
        stats_coordinator::Hooks hooks{};
        hooks.context=&state;
        hooks.stats_resource_context=engineFileSystem;
        hooks.read_stats_resource=&ReadForStagedStats;
        hooks.run_unreconstructed_step=
            &stats_messages::RegisterKnownStatsStep;
        // Only independent C++ owner objects are passed here.
        const auto result=stats_coordinator::Initialize(
            state,&state,&messageHooks,hooks);
        StagedCounts report{};
        report.completed=result.completed;
        report.attempted=result.attempted;
        report.owned_user_message_hooks=messageHooks.count();
        for(std::size_t i=0;i<3;++i)
            report.hook_ids[i]=messageHooks.at(i);
        if(result.outcome==stats_coordinator::Outcome::ResourceLoaderRefused) {
            if(observed)*observed=report;
            return StagedStatus::DefinitionsRefused;
        }
        if(result.outcome==stats_coordinator::Outcome::AssetLoadRefused) {
            if(observed)*observed=report;
            return StagedStatus::LayoutRefused;
        }
        if(!result.succeeded()) {
            if(observed)*observed=report;
            return StagedStatus::UnexpectedCounts;
        }
        report.catalog=state.catalog.size();
        report.definitions=state.scripts.definitions().size();
        for(const auto& definition:state.scripts.definitions())
            if(!definition.percentage_of.empty())++report.refs;
        for(const auto& display:state.scripts.display()) {
            if(display.divider)++report.dividers;
            else ++report.pairs;
        }
        if(observed)*observed=report;
        if(report.catalog!=66 || state.catalog_text.count()!=62 ||
           !messageHooks.Complete() || report.owned_user_message_hooks!=3 ||
           report.hook_ids[0]!=27 || report.hook_ids[1]!=28 ||
           report.hook_ids[2]!=5 ||
           report.definitions!=62 || report.refs!=8 ||
           report.pairs!=156 || report.dividers!=66 ||
           report.completed!=7 || report.attempted!=7 ||
           !state.catalog.at(0) ||
           !state.catalog_text.at(0) ||
           state.catalog_text.at(51) ||
           state.catalog_text.at(52) ||
           state.catalog_text.at(53) ||
           state.catalog_text.at(54)) {
            return StagedStatus::UnexpectedCounts;
        }
        return StagedStatus::OwnedStagedVerified;
    } catch(...) {
        return StagedStatus::UnexpectedCounts;
    }
}
} // namespace sin_independent::engine_fs
