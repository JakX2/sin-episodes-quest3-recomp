#pragma once
// SiN Episodes 2006 original PE32 factory-request core Init contract.
// This is host-only reference logic, NOT an Android SourceVR ABI adapter.
#include <array>
#include <cstddef>
#include <string_view>
namespace sin_compat::init {
enum class FactoryArgument { First, Third };
struct Request { std::string_view name; FactoryArgument source; };
inline constexpr std::array<Request,21> client_core_requests{{
    {"VEngineClient012",FactoryArgument::First},
    {"VEngineModel012",FactoryArgument::First},
    {"VEngineEffects001",FactoryArgument::First},
    {"VEngineCvar002",FactoryArgument::First},
    {"EngineTraceClient003",FactoryArgument::First},
    {"VEngineRenderView011",FactoryArgument::First},
    {"VDebugOverlay003",FactoryArgument::First},
    {"VMaterialSystem076",FactoryArgument::First},
    {"VEngineCache001",FactoryArgument::First},
    {"VModelInfoClient003",FactoryArgument::First},
    {"VEngineVGui001",FactoryArgument::First},
    {"VEngineClientStringTable001",FactoryArgument::First},
    {"SpatialPartition001",FactoryArgument::First},
    {"VEngineShadowMgr002",FactoryArgument::First},
    {"StaticPropMgrClient004",FactoryArgument::First},
    {"IEngineSoundClient003",FactoryArgument::First},
    {"VFileSystem012",FactoryArgument::First},
    {"VEngineRandom001",FactoryArgument::First},
    {"VENGINE_GAMEUIFUNCS_VERSION004",FactoryArgument::First},
    {"GAMEEVENTSMANAGER002",FactoryArgument::First},
    {"VSoundEmitter001",FactoryArgument::First},
}};

inline constexpr std::array<Request,16> server_core_requests{{
    {"VEngineServer021",FactoryArgument::First},
    {"VoiceServer001",FactoryArgument::First},
    {"VEngineCvar002",FactoryArgument::First},
    {"VEngineServerStringTable001",FactoryArgument::First},
    {"StaticPropMgrServer002",FactoryArgument::First},
    {"VEngineRandom001",FactoryArgument::First},
    {"IEngineSoundServer003",FactoryArgument::First},
    {"SpatialPartition001",FactoryArgument::First},
    {"VModelInfoServer001",FactoryArgument::First},
    {"EngineTraceServer003",FactoryArgument::First},
    {"VFileSystem012",FactoryArgument::Third},
    {"GAMEEVENTSMANAGER002",FactoryArgument::First},
    {"VEngineCache001",FactoryArgument::First},
    {"VSoundEmitter001",FactoryArgument::First},
    {"ServerUploadGameStats001",FactoryArgument::First},
    {"ISERVERPLUGINHELPERS001",FactoryArgument::First},
}};

struct ResolveResult { bool resolved; std::size_t first_failed_index; };
// The original x86 code stores each returned pointer immediately, including
// a nullptr on failure; previously stored pointers remain in place.
// No caller-side rollback or version-name substitution is performed.
template<std::size_t N,typename First,typename Third>
ResolveResult resolve_required_core(const std::array<Request,N>& requests,
    First&& first,Third&& third,std::array<void*,N>& stored) {
    for(std::size_t i=0;i<N;++i) {
        const auto& item=requests[i];
        void* pointer=item.source==FactoryArgument::First
            ? first(item.name) : third(item.name);
        stored[i]=pointer;
        if(!pointer)return {false,i};
    }
    return {true,N};
}
// The next original gate calls the sound-emitter interface's virtual +0;
// success here permits the rest of Init, not a complete engine startup.
inline bool may_continue_after_emitter_gate(ResolveResult core,
                                            bool emitter_virtual_zero_return) noexcept {
    return core.resolved && emitter_virtual_zero_return;
}
} // namespace sin_compat::init
