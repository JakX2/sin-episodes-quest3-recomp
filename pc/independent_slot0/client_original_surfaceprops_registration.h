#pragma once
// Optional ORIGINAL 2006 surfaceprops ParseSurfaceData experiment.
// Source evidence: client 0x24191EC0 (136 bytes, SHA256
// f5e1bbb19b7d8305690aa65992653d244ee0e8aaaf105e057fb760a889c218b5).
// Native method at VPhysicsSurfaceProps001 vtable byte offset +4 (slot 1),
// thiscall (this, filename, NUL-terminated raw file text), return IGNORED by
// the original. All file handles close BEFORE any parse call.
// Not enabled by normal VClient011::Init, which remains fail-closed.
// Unlike previous read-only diagnostics, this opt-in invokes an ENGINE
// MUTATOR on a disposable game copy. No full game startup is claimed.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Original SiN Episodes physics surfaceprops ABI requires MSVC Win32 x86
#endif
#include "client_engine_filesystem_read.h"
#include "client_known_gamesystem_init.h"
#include "client_original_surfaceprops_manifest_owned.h"
#include <array>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>

namespace sin_independent::original_surfaceprops_registration {
namespace owned=sin_independent::original_surfaceprops;
enum class Outcome {
    InvalidInterface, InvalidFileSystem, ResourceShapeRefused,
    RawSourceRefused, ParseMethodRefused, TwoCallsReturned
};
inline const char* Name(Outcome x) noexcept {
    switch(x) {
        case Outcome::InvalidInterface: return "original-surfaceprops-register-no-interface";
        case Outcome::InvalidFileSystem: return "original-surfaceprops-register-invalid-filesystem";
        case Outcome::ResourceShapeRefused: return "original-surfaceprops-register-resources-refused";
        case Outcome::RawSourceRefused: return "original-surfaceprops-register-raw-shape-refused";
        case Outcome::ParseMethodRefused: return "original-surfaceprops-register-method-refused";
        case Outcome::TwoCallsReturned: return "original-surfaceprops-register-two-calls-returned";
    }
    return "original-surfaceprops-register-unknown";
}
struct Report final {
    Outcome outcome=Outcome::InvalidInterface;
    unsigned source_reads=0;
    unsigned calls_returned=0;
    int first_return=0;
    int second_return=0;
    bool completed_calls() const noexcept {
        return outcome==Outcome::TwoCallsReturned&&
               source_reads==2u&&calls_returned==2u;
    }
};
inline constexpr std::size_t kRetailSourceSizes[2]={23485u,2812u};
using ParseSurfaceData=int (__thiscall *)(void*,const char*,const char*);

// This is deliberately NOT called by the normal production client.
// A successful result means two engine METHOD INVOCATIONS RETURNED;
// it does not establish what the engine registered internally.
inline Report Attempt(void* filesystem,void* surfaceProps) noexcept {
    Report result{};
    if(!surfaceProps)return result;
    using sin_independent::known_game_system::CallResult;
    CallResult refused=CallResult::InvalidObject;
    void* const rawMethod=sin_independent::known_game_system::VerifiedMethod(
        surfaceProps,1,refused);
    if(!rawMethod){
        result.outcome=Outcome::ParseMethodRefused;
        return result;
    }
    engine_fs::Methods methods{};
    if(!engine_fs::Resolve(filesystem,methods,true)){
        result.outcome=Outcome::InvalidFileSystem;
        return result;
    }
    // Read all three original resources with their original search-path
    // choices, without forwarding original engine allocation ownership.
    // The verifier demands the exact 2-source retail ordering and 93/71
    // metadata, before either native ParseSurfaceData call can execute.
    std::array<std::string,2> bodies{};
    owned::OwnedResources verified{};
    const auto read=[&](std::string_view path,const char* pathID,
                        std::string& output)->bool {
        const std::string canonical(path);
        if(!engine_fs::ReadOne(filesystem,methods,canonical.c_str(),output,
                               "rb",pathID))return false;
        if(path==owned::kFirstSource)bodies[0]=output;
        else if(path==owned::kSecondSource)bodies[1]=output;
        return true;
    };
    const auto resources=verified.Load(read);
    if(!resources.verified()) {
        result.outcome=Outcome::ResourceShapeRefused;
        return result;
    }
    result.source_reads=resources.read;
    for(std::size_t i=0;i<2;++i) {
        if(bodies[i].size()!=kRetailSourceSizes[i] ||
           bodies[i].find('\0')!=std::string::npos) {
            result.outcome=Outcome::RawSourceRefused;
            return result;
        }
    }
    // Engine_fs::ReadOne closes all file handles before returning.
    // Both buffers remain independently owned and NUL-terminated here.
    auto parse=reinterpret_cast<ParseSurfaceData>(rawMethod);
    result.first_return=parse(surfaceProps,owned::kFirstSource.data(),
                              bodies[0].c_str());
    ++result.calls_returned;
    result.second_return=parse(surfaceProps,owned::kSecondSource.data(),
                               bodies[1].c_str());
    ++result.calls_returned;
    result.outcome=Outcome::TwoCallsReturned;
    return result;
}
} // namespace sin_independent::original_surfaceprops_registration
