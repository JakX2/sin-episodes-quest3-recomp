#pragma once
// Verified SiN Episodes (2006) class RTTI and method dispatch addresses.
// Original Windows x86 metadata only. NEVER native Android function pointers.
#include <array>
#include <cstdint>
#include <cstddef>
#include <string_view>
namespace sin_compat::rtti {
struct ClientPanel {
    std::string_view original_class;
    std::uint32_t original_global_cell;
    std::uint32_t original_vtable;
    std::uint32_t original_shutdown_method;
    std::uint8_t original_virtual_byte_offset;
};
inline constexpr std::array<ClientPanel,8> client_shutdown_order{{
    {"CDebugOverlayPanel",0x2431DC90u,0x242ABF0Cu,0x241ECA60u,4},
    {"CNetGraphPanelInterface",0x2431DEA4u,0x242ACC34u,0x241EF9F0u,4},
    {"CFPS",0x2431DD00u,0x242AC1F0u,0x241ED1A0u,4},
    {"CMessageChars",0x2431DE3Cu,0x242ACB38u,0x241EEA30u,4},
    {"CLoadingDisc",0x2431DDD0u,0x242AC8B0u,0x241EE480u,4},
    {"CCenterPrint",0x2431DBECu,0x242ABC4Cu,0x241EC510u,0x1c},
    {"CTextMessage",0x2431DF40u,0x242AD484u,0x241F2320u,4},
    {"CStatsPanelInterface",0x2431EEBCu,0x242AF988u,0x24207670u,4},
}};
struct ServerCache {
    std::string_view original_class;
    std::uint32_t original_vtable;
    std::array<std::uint32_t,3> original_virtual_methods;
    std::uint32_t original_record_insert;
    std::size_t auxiliary_allocation_bytes;
};
inline constexpr std::array<ServerCache,2> cache_handlers{{
    {"CSceneCache",0x2242DCA0u,{0x221A7360u,0x221AB0A0u,0x221AD750u},0x221ACFD0u,28},
    {"CModelSoundCache",0x2240ABC8u,{0x221437E0u,0x221453E0u,0x221454B0u},0x22149770u,24},
}};
// Host mock dispatch only. Callers must supply native functions when an
// independently proven SourceVR ABI exists; these x86 VAs are not callable.
template<class Dispatch>
void run_known_client_shutdown_panel_order(Dispatch&& dispatch) {
    for(const auto& panel:client_shutdown_order)dispatch(panel);
}
template<class Dispatch>
void inspect_original_cache_methods(Dispatch&& dispatch) {
    for(const auto& cache:cache_handlers)
        for(std::size_t i=0;i<cache.original_virtual_methods.size();++i)
            dispatch(cache,i,cache.original_virtual_methods[i]);
}
} // namespace sin_compat::rtti
