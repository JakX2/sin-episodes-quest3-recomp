#include "sin_compat/original_rtti_handler_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::rtti;
int main() {
    static_assert(client_shutdown_order.size()==8);
    static_assert(cache_handlers.size()==2);
    std::vector<std::string> names;
    run_known_client_shutdown_panel_order([&](const ClientPanel& item) {
        names.emplace_back(item.original_class);
        assert(item.original_global_cell>=0x24000000u);
        assert(item.original_shutdown_method>=0x24000000u);
        assert(item.original_virtual_byte_offset%4==0);
    });
    assert((names==std::vector<std::string>{
        "CDebugOverlayPanel","CNetGraphPanelInterface","CFPS",
        "CMessageChars","CLoadingDisc","CCenterPrint",
        "CTextMessage","CStatsPanelInterface"}));
    assert(client_shutdown_order[5].original_virtual_byte_offset==0x1c);
    assert(client_shutdown_order[5].original_shutdown_method==0x241EC510u);
    for(std::size_t i=0;i<client_shutdown_order.size();++i)
        if(i!=5)assert(client_shutdown_order[i].original_virtual_byte_offset==4);
    std::size_t count=0;
    inspect_original_cache_methods([&](const ServerCache& cache,std::size_t slot,
                                        std::uint32_t original) {
        assert(slot<3 && original==cache.original_virtual_methods[slot]);
        if(cache.original_class=="CSceneCache") {
            assert(cache.original_record_insert==0x221ACFD0u);
            assert(cache.auxiliary_allocation_bytes==28);
        } else {
            assert(cache.original_class=="CModelSoundCache");
            assert(cache.original_record_insert==0x22149770u);
            assert(cache.auxiliary_allocation_bytes==24);
        }
        ++count;
    });
    assert(count==6);
}
