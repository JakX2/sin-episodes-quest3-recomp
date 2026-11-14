#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>
#include "sin_compat/original_client_entitylist_handle_dispatch.hpp"

namespace n=sin_compat::original_client_entitylist_handle_dispatch;
using n::Bytes;
struct Entities final:n::IEntityVirtualHost {
    std::vector<std::pair<std::uint32_t,std::uint32_t>> calls;
    std::uint32_t invoke(std::uint32_t pointer,std::uint32_t offset) override {
        assert(pointer!=0u);
        calls.emplace_back(pointer,offset);
        return pointer^offset;
    }
};
static Bytes fixture() {
    // The original interface at parent+0x10014 uses table entries
    // starting at parent+4, each 16 bytes.
    Bytes parent(0x10028u,0u);
    n::write_u32(parent,4u,0x33330000u);
    n::write_u32(parent,8u,1u);  // handle 0x1000
    n::write_u32(parent,20u,0x11110000u);
    n::write_u32(parent,24u,1u); // handle 0x1001
    n::write_u32(parent,36u,0x11110000u);
    n::write_u32(parent,40u,2u); // handle 0x2002, same resolved entity
    n::write_u32(parent,52u,0x22220000u);
    n::write_u32(parent,56u,3u); // handle 0x3003
    return parent;
}
static void verify_native_addresses_and_vtable_slots() {
    static_assert(n::kOriginalVClientEntityList003Vtable==0x24297390u);
    static_assert(n::kOriginalEmbeddedInterfaceOffset==0x10014u);
    static_assert(n::kOriginalSlotMask==0xFFFu);
    static_assert(n::kOriginalSerialShift==12u);
    static_assert(n::kOriginalSlotIndexBias==0x1001u);
    static_assert(n::kOriginalParentEntryOffset==4u);
    static_assert(n::kOriginalSlotStride==16u);
    static_assert(n::kOriginalMaxSlot==0x7FFu);
    static_assert(n::kOriginalHighestOccupiedOffset==0x10024u);
    static_assert(n::kVerifiedOriginalNativeMethods==7u);
    static_assert(n::kVerifiedOriginalNativeBytes==262u);
    static_assert(sizeof(n::kMethods)/sizeof(n::Method)==7u);
    std::uint32_t code_bytes=0u,virtual_count=0u,private_count=0u;
    for(const auto& method:n::kMethods) {
        code_bytes+=method.code_bytes;
        if(method.vtable_slot<0)++private_count;
        else {
            ++virtual_count;
            assert(method.vtable_slot>=1 && method.vtable_slot<=4);
        }
    }
    assert(code_bytes==262u && virtual_count==4u && private_count==3u);
}
static void verify_ehandle_slot_serial_and_null_cases() {
    auto parent=fixture();
    const auto self=std::size_t{n::kOriginalEmbeddedInterfaceOffset};
    assert(n::interface_slot2_resolve_handle(parent,self,0x1000u)==0x33330000u);
    assert(n::interface_slot2_resolve_handle(parent,self,0x1001u)==0x11110000u);
    assert(n::interface_slot2_resolve_handle(parent,self,0x2002u)==0x11110000u);
    assert(n::interface_slot2_resolve_handle(parent,self,0x3003u)==0x22220000u);
    assert(n::interface_slot2_resolve_handle(parent,self,0x2001u)==0u);
    assert(n::interface_slot2_resolve_handle(parent,self,0x1002u)==0u);
    assert(n::interface_slot2_resolve_handle(parent,self,0xFFFFFFFFu)==0u);
    // Unknown entry with an initialized buffer but serial mismatch.
    assert(n::interface_slot2_resolve_handle(parent,self,0x1004u)==0u);
    // Null pointer is accepted if serial matches.
    n::write_u32(parent,4u+4u*16u,4u);
    assert(n::interface_slot2_resolve_handle(parent,self,0x4004u)==0u);
    bool refused=false;
    try {n::interface_slot2_resolve_handle(parent,self,0x1FFFu);}
    catch(const std::out_of_range&) {refused=true;}
    assert(refused);
    refused=false;
    try {n::interface_slot2_resolve_handle(parent,0u,0x1001u);}
    catch(const std::out_of_range&) {refused=true;}
    assert(refused);
}
static void verify_handle_to_virtual_dispatch_offsets() {
    auto parent=fixture();
    Entities host;
    const auto self=std::size_t{n::kOriginalEmbeddedInterfaceOffset};
    assert(n::interface_slot1_handle_dispatch_10(parent,self,0x1001u,host)==
           (0x11110000u^0x10u));
    assert(host.calls.size()==1u);
    assert(host.calls[0]==std::make_pair(0x11110000u,0x10u));
    assert(n::interface_slot4_handle_dispatch_18(parent,self,0x2002u,host)==
           (0x11110000u^0x18u));
    assert(host.calls.size()==2u);
    assert(host.calls[1]==std::make_pair(0x11110000u,0x18u));
    assert(n::interface_slot1_handle_dispatch_10(parent,self,0x2001u,host)==0u);
    assert(n::interface_slot4_handle_dispatch_18(parent,self,0xFFFFFFFFu,host)==0u);
    assert(host.calls.size()==2u);
}
static void verify_index_lookup_is_not_serial_lookup() {
    auto parent=fixture();
    Entities host;
    const auto self=std::size_t{n::kOriginalEmbeddedInterfaceOffset};
    // Native slot3 receives a signed index and directly reads a slot's
    // entity pointer: no serial check, unlike native slot2 EHandle.
    assert(n::interface_slot3_index_dispatch_18(parent,self,1,host)==
           (0x11110000u^0x18u));
    assert(host.calls.back()==std::make_pair(0x11110000u,0x18u));
    assert(n::interface_slot3_index_dispatch_18(parent,self,-1,host)==0u);
    assert(host.calls.size()==1u);
    n::write_u32(parent,20u,0u);
    assert(n::interface_slot3_index_dispatch_18(parent,self,1,host)==0u);
    assert(host.calls.size()==1u);
    bool refused=false;
    try {n::interface_slot3_index_dispatch_18(parent,self,0x7FFFFFFF,host);}
    catch(const std::exception&){refused=true;}
    assert(refused);
}
static void verify_two_parent_index_virtual_helpers() {
    auto parent=fixture();
    Entities host;
    assert(n::parent_index_dispatch_1c(parent,0u,1,host)==
           (0x11110000u^0x1Cu));
    assert(host.calls.back()==std::make_pair(0x11110000u,0x1Cu));
    assert(n::parent_index_dispatch_0c(parent,0u,2,host)==
           (0x11110000u^0x0Cu));
    assert(host.calls.back()==std::make_pair(0x11110000u,0x0Cu));
    assert(n::parent_index_dispatch_1c(parent,0u,-2,host)==0u);
    assert(n::parent_index_dispatch_0c(parent,0u,-1,host)==0u);
    assert(host.calls.size()==2u);
    n::write_u32(parent,20u,0u);
    assert(n::parent_index_dispatch_1c(parent,0u,1,host)==0u);
    bool refused=false;
    try {n::parent_index_dispatch_0c(parent,0u,0x7FFFFFFF,host);}
    catch(const std::exception&){refused=true;}
    assert(refused);
}
static void verify_highest_live_slot_scan_and_empty_case() {
    Bytes parent(0x10028u,0u);
    assert(n::parent_recompute_highest_live_slot(parent,0u)==-1);
    assert(n::load_u32(parent,0x10024u)==0xFFFFFFFFu);
    n::write_u32(parent,4u+16u*1u,0x11110000u);
    assert(n::parent_recompute_highest_live_slot(parent,0u)==1);
    assert(n::load_u32(parent,0x10024u)==1u);
    n::write_u32(parent,4u+16u*0x7FFu,0x77770000u);
    assert(n::parent_recompute_highest_live_slot(parent,0u)==0x7FF);
    assert(n::load_u32(parent,0x10024u)==0x7FFu);
    n::write_u32(parent,4u+16u*0x7FFu,0u);
    assert(n::parent_recompute_highest_live_slot(parent,0u)==1);
    n::write_u32(parent,4u+16u*1u,0u);
    n::write_u32(parent,4u,0xABCDu);
    assert(n::parent_recompute_highest_live_slot(parent,0u)==0);
    n::write_u32(parent,4u,0u);
    assert(n::parent_recompute_highest_live_slot(parent,0u)==-1);
}
static void verify_safety_does_not_fake_original_crashes() {
    auto parent=fixture();
    bool refused=false;
    try {n::parent_recompute_highest_live_slot(parent,parent.size());}
    catch(const std::exception&){refused=true;}
    assert(refused);
    const Bytes too_small(32u,0u);
    Bytes mutable_small=too_small;
    refused=false;
    try {n::parent_recompute_highest_live_slot(mutable_small,0u);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused && mutable_small==too_small);
    refused=false;
    try {n::load_u32(parent,parent.size());}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
    refused=false;
    try {n::offset_by(std::numeric_limits<std::size_t>::max(),1);}
    catch(const std::overflow_error&){refused=true;}
    assert(refused);
}
int main() {
    verify_native_addresses_and_vtable_slots();
    verify_ehandle_slot_serial_and_null_cases();
    verify_handle_to_virtual_dispatch_offsets();
    verify_index_lookup_is_not_serial_lookup();
    verify_two_parent_index_virtual_helpers();
    verify_highest_live_slot_scan_and_empty_case();
    verify_safety_does_not_fake_original_crashes();
}
