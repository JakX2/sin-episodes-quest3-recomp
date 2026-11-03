#pragma once
// SiN Episodes 2006 x86 shutdown behavior, host-only C++17 reference.
// Not a SourceVR ABI, real file decoder, original DLL, or playable port.
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>
namespace sin_compat::shutdown {
inline constexpr std::string_view scene_cache_name="scene.cache";
inline constexpr std::string_view model_sounds_cache_name="modelsounds.cache";
inline constexpr std::uint32_t scene_cache_expected_version=3;
inline constexpr std::uint32_t model_sounds_cache_expected_version=2;
inline constexpr std::string_view server_game_cfg_command="exec game.cfg\n";
struct CacheState {
    bool started=false;       // original cache singleton +0x3D
    bool dirty=false;         // original +0x3C; provisional semantic name
    bool check_on_close=false;// original +0x4E; provisional name
};
enum class CacheClosePath {NotStarted,Dirty,ConditionalCheck,SimpleCleanup};
// Branch-only reference. The original branch target helpers and file effects
// have not yet been reconstructed. No disk I/O occurs here.
template<typename DirtyFn,typename CheckFn,typename FallbackFn,typename CleanupFn>
CacheClosePath close_cache(CacheState& state,DirtyFn&& on_dirty,
                          CheckFn&& check,FallbackFn&& on_check_failure,
                          CleanupFn&& finish) {
    if(!state.started)return CacheClosePath::NotStarted;
    state.started=false;
    if(state.dirty) {
        on_dirty();
        finish();
        return CacheClosePath::Dirty;
    }
    if(state.check_on_close) {
        if(!check())on_check_failure();
        finish();
        return CacheClosePath::ConditionalCheck;
    }
    finish();
    return CacheClosePath::SimpleCleanup;
}
struct ClientSystem {
    void* identity=nullptr;
    void (*shutdown_virtual_0x34)(void*)=nullptr;
};
// Original VClient011 vtable slot 1 at 0x240FF3D0 processes systems in
// reverse index order and then writes zero to count at this+0x10.
inline void close_client_systems_reverse(std::vector<ClientSystem>& systems,
                                         std::size_t& active_count) {
    const std::size_t count=active_count<systems.size()?active_count:systems.size();
    for(std::size_t i=count;i>0;--i) {
        const auto& item=systems[i-1];
        if(item.shutdown_virtual_0x34)item.shutdown_virtual_0x34(item.identity);
    }
    active_count=0;
}
// ServerGameDLL003 vtable slot 8 at 0x221E79B0 closes model sounds first,
// then scenes; five virtual+0x28 dispatches; helper reset; optional release
// via virtual+0 with argument 1, and nulls the pointer.
template<typename Models,typename Scenes,typename Unregister,typename Reset,typename Release>
void close_server_level(Models&& models,Scenes&& scenes,Unregister&& unregister_five,
                        Reset&& reset,void*& optional_instance,Release&& release) {
    models();
    scenes();
    for(std::size_t i=0;i<5;++i)unregister_five(i);
    reset();
    if(optional_instance) {
        release(optional_instance,1);
        optional_instance=nullptr;
    }
}
} // namespace sin_compat::shutdown
