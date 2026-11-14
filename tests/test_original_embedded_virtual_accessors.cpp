#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_embedded_virtual_accessors.hpp"

namespace n=sin_compat::original_embedded_virtual_accessors;
using n::Bytes;

static void original_vtable_entries_and_abi() {
    static_assert(n::kSelectedOriginalMethodCount==12u);
    static_assert(n::kOriginalReviewedX86Bytes==112u);
    static_assert(n::kServerHltvVtable==0x22453DA0u);
    static_assert(n::kClientEntityListVtable==0x24297390u);
    static_assert(n::kServerHltvEmbeddedOffset==8u);
    static_assert(n::kClientEntityListEmbeddedOffset==0x10014u);
    std::size_t server=0,client=0,total_bytes=0;
    for(const auto& m:n::kMethods){
        if(m.server){++server;assert(m.native_va>=0x22000000u);}
        else{++client;assert(m.native_va>=0x24000000u);}
        total_bytes+=m.native_code_bytes;
        assert(m.return_pop_bytes==0u || m.return_pop_bytes==4u);
    }
    assert(server==7u && client==5u && total_bytes==112u);
}

static void server_hltv_read_status_and_x87_float() {
    Bytes blob(56u,0u);
    const std::size_t self=8u; // synthetic embedded interface start
    n::write_u32(blob,self+4u,0u);
    assert(n::server_get_04(blob,self)==0u);
    assert(!n::server_slot1_nonzero_04(blob,self));
    n::write_u32(blob,self+4u,0xFFFFFFFFu);
    assert(n::server_get_04(blob,self)==0xFFFFFFFFu);
    assert(n::server_slot1_nonzero_04(blob,self));
    n::write_u32(blob,self+12u,0xDEADBEEFu);
    n::write_u32(blob,self+16u,123u);
    assert(n::server_slot4_get_0c(blob,self)==0xDEADBEEFu);
    assert(n::server_slot5_get_10(blob,self)==123u);
    n::write_u32(blob,self+8u,0x3FA00000u); // IEEE754 1.25
    assert(n::server_slot7_f32_08(blob,self)==1.25f);
    n::write_u32(blob,self+8u,0x7FC00000u); // quiet NaN bits
    const float nan=n::server_slot7_f32_08(blob,self);
    assert(nan!=nan);
}

static void server_hltv_copy_three_dwords_in_native_forward_order() {
    Bytes source(64u,0u);
    const std::size_t self=4u;
    n::write_u32(source,self+0x14u,0xA1u);
    n::write_u32(source,self+0x18u,0xB2u);
    n::write_u32(source,self+0x1Cu,0xC3u);
    Bytes dest(16u,0u);
    n::server_slot6_forward_three_dwords(source,self,dest,0u);
    assert(n::load_u32(dest,0u)==0xA1u);
    assert(n::load_u32(dest,4u)==0xB2u);
    assert(n::load_u32(dest,8u)==0xC3u);
    // Same byte vector as original source and destination; subsequent
    // MOV reads the overwritten source DWORD after forward overlap.
    n::server_slot6_forward_three_dwords(source,self,source,self+0x18u);
    assert(n::load_u32(source,self+0x14u)==0xA1u);
    assert(n::load_u32(source,self+0x18u)==0xA1u);
    assert(n::load_u32(source,self+0x1Cu)==0xA1u);
    assert(n::load_u32(source,self+0x20u)==0xA1u);
    bool refused=false;
    try {n::server_slot6_forward_three_dwords(source,self,dest,8u);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
}

static void server_native_global_pointer_rebases_not_dereferences() {
    assert(n::kServerGlobalAddressVA==0x2251B054u);
    assert(n::server_slot8_rebased_global_token()==0x2251B054u);
    assert(n::server_slot8_rebased_global_token(0x23000000u)==0x2351B054u);
    bool refused=false;
    try {n::server_slot8_rebased_global_token(0xFFFFFFFFu);}
    catch(const std::overflow_error&){refused=true;}
    assert(refused);
}

static void client_entitylist_stride8_indexed_values() {
    Bytes bytes(96u,0u);
    const std::size_t self=8u;
    n::write_u32(bytes,self+0x14u,0xAABBCCDDu);
    n::write_u32(bytes,self+0x18u,0xBAD00BADu); // skip next +4 slot
    n::write_u32(bytes,self+0x1Cu,0x1234ABCDu);
    n::write_u32(bytes,self+0x24u,0x99887766u);
    assert(n::client_slot0_indexed_dword(bytes,self,0u)==0xAABBCCDDu);
    assert(n::client_slot0_indexed_dword(bytes,self,1u)==0x1234ABCDu);
    assert(n::client_slot0_indexed_dword(bytes,self,2u)==0x99887766u);
    bool refused=false;
    try {n::client_slot0_indexed_dword(bytes,self,0xFFFFFFFFu);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
    refused=false;
    try {n::client_slot0_indexed_dword(bytes,std::numeric_limits<std::size_t>::max(),1u);}
    catch(const std::overflow_error&){refused=true;}
    assert(refused);
}

static void client_low_byte_selector_addition_and_setter() {
    Bytes bytes(40u,0u);
    const std::size_t self=4u;
    n::write_u32(bytes,self+4u,0xFFFFFFFEu);
    n::write_u32(bytes,self+12u,6u);
    n::write_u32(bytes,self+16u,0x12345678u);
    assert(n::client_slot5_base_adjusted(bytes,self,0u)==0xFFFFFFFEu);
    assert(n::client_slot5_base_adjusted(bytes,self,2u)==0xFFFFFFFEu);
    assert(n::client_slot5_base_adjusted(bytes,self,1u)==4u);
    // x86 compares the low BYTE of the 32-bit input, not whole DWORD.
    assert(n::client_slot5_base_adjusted(bytes,self,0x00000101u)==4u);
    assert(n::client_slot5_base_adjusted(bytes,self,0x00000100u)==0xFFFFFFFEu);
    assert(n::client_slot6_get_10(bytes,self)==0x12345678u);
    assert(n::client_slot7_set_08(bytes,self,0xF00DCAFEu)==0xF00DCAFEu);
    assert(n::client_slot8_get_08(bytes,self)==0xF00DCAFEu);
    bool refused=false;
    try {n::client_slot7_set_08(bytes,bytes.size(),10u);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
}

int main(){
    original_vtable_entries_and_abi();
    server_hltv_read_status_and_x87_float();
    server_hltv_copy_three_dwords_in_native_forward_order();
    server_native_global_pointer_rebases_not_dereferences();
    client_entitylist_stride8_indexed_values();
    client_low_byte_selector_addition_and_setter();
}
