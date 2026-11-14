#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_snapshot_vector_copy.hpp"

namespace native=sin_compat::original_client_snapshot_vector_copy;
using Bytes=native::Bytes;

static Bytes sequential(std::size_t n) {
    Bytes b;
    for(std::size_t i=0;i<n;++i)
        b.push_back(static_cast<std::uint8_t>(i&0xffu));
    return b;
}
static void test_original_addresses_and_record_strides() {
    static_assert(native::kType3CopyVA==0x24198DA0u);
    static_assert(native::kType4CopyVA==0x24198DE0u);
    static_assert(native::kType3RouterCall==0x241998C3u);
    static_assert(native::kType4RouterCall==0x241998FDu);
    static_assert(native::kType3RecordBytes==12u);
    static_assert(native::kType4RecordBytes==16u);
}
static void test_stride12_and_stride16_both_move_whole_records() {
    const Bytes source=sequential(64u);
    Bytes first(64u,0xeeu),second(64u,0xeeu);
    native::copy_type3({1u},0u,source,3u,first,4u,2u);
    native::copy_type4({1u},0u,source,3u,second,4u,2u);
    for(std::size_t i=0;i<24u;++i)
        assert(first[4u+i]==source[3u+i]);
    for(std::size_t i=0;i<32u;++i)
        assert(second[4u+i]==source[3u+i]);
    assert(first[3]==0xeeu && first[28]==0xeeu);
    assert(second[3]==0xeeu && second[36]==0xeeu);
    // Count zero must neither read nor write valid empty spans.
    const Bytes empty;
    Bytes dest;
    native::copy_type3({1u},0u,empty,0u,dest,0u,0u);
    native::copy_type4({1u},0u,empty,0u,dest,0u,0u);
    assert(dest.empty());
}
static void test_original_early_skip_preserves_invalid_pointer_shortcuts() {
    const Bytes empty;
    Bytes dest{17u,18u,19u,20u};
    const auto original=dest;
    native::copy_type3({0u},0u,empty,999u,dest,999u,0xffffffffu);
    native::copy_type4({0u},2u,empty,999u,dest,999u,0xffffffffu);
    native::copy_type3({1u},1u,empty,999u,dest,999u,0xffffffffu);
    native::copy_type4({1u},1u,empty,999u,dest,999u,0xffffffffu);
    assert(dest==original);
    // Operation status 2 is NOT an original skip condition.
    const Bytes src{1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,11u,12u,
                    13u,14u,15u,16u};
    native::copy_type3({1u},2u,src,0u,dest,0u,1u);
}
static void test_x86_forward_dword_overlap_not_memmove() {
    Bytes type3=sequential(18u);
    native::copy_type3({1u},0u,type3,0u,type3,1u,1u);
    assert((type3==Bytes{0,0,1,2,3,3,5,6,7,7,9,10,11,13,14,15,16,17}));
    Bytes type4=sequential(20u);
    native::copy_type4({1u},0u,type4,0u,type4,1u,1u);
    assert((type4==Bytes{0,0,1,2,3,3,5,6,7,7,9,10,11,11,13,14,15,17,18,19}));
}
static void test_host_only_unsigned_x86_product_refusal() {
    const Bytes empty;
    Bytes dest;
    bool refused=false;
    try {native::copy_type3({1u},0u,empty,0u,dest,0u,
                            std::numeric_limits<std::uint32_t>::max()/12u+1u);}
    catch(const std::overflow_error&) {refused=true;}
    assert(refused);
    refused=false;
    try {native::copy_type4({1u},0u,empty,0u,dest,0u,
                            std::numeric_limits<std::uint32_t>::max()/16u+1u);}
    catch(const std::overflow_error&) {refused=true;}
    assert(refused);
    // Native skip conditions must precede this host-only overflow refusal.
    native::copy_type3({1u},1u,empty,999u,dest,999u,0xffffffffu);
    native::copy_type4({0u},0u,empty,999u,dest,999u,0xffffffffu);
    assert(dest.empty());
    refused=false;
    try {native::copy_type3({1u},0u,Bytes{1u},0u,dest,0u,1u);}
    catch(const std::out_of_range&) {refused=true;}
    assert(refused);
}
int main() {
    test_original_addresses_and_record_strides();
    test_stride12_and_stride16_both_move_whole_records();
    test_original_early_skip_preserves_invalid_pointer_shortcuts();
    test_x86_forward_dword_overlap_not_memmove();
    test_host_only_unsigned_x86_product_refusal();
}
