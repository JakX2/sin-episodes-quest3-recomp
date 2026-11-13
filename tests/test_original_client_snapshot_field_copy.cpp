#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_snapshot_field_copy.hpp"
#include "sin_compat/original_client_snapshot_record_router.hpp"

namespace native=sin_compat::original_client_snapshot_field_copy;
namespace router=sin_compat::original_client_snapshot_record_router;
using Bytes=std::vector<std::uint8_t>;

static void verify_four_real_native_entrypoints() {
    static_assert(native::kCopyDwordsType5VA==0x24198a10u);
    static_assert(native::kCopyBytesType6VA==0x24198a40u);
    static_assert(native::kCopyDwordsType1VA==0x24198a70u);
    static_assert(native::kCopyCStringType2VA==0x24198b40u);
    static_assert(native::kType5Callsite==0x241999cbu);
    static_assert(native::kType6Callsite==0x24199993u);
    static_assert(native::kType1Callsite==0x2419985bu);
    static_assert(native::kType2Callsite==0x2419988fu);
    static_assert(router::kTypeCases[1]==1u);
    static_assert(router::kTypeCases[2]==2u);
    static_assert(router::kTypeCases[5]==5u);
    static_assert(router::kTypeCases[6]==6u);
}
static void test_two_original_dword_helpers_are_identical() {
    const Bytes source{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    Bytes a(20,0xeeu),b(20,0xeeu);
    const native::CopyContext ctx{1u};
    native::copy_dwords_type1(ctx,0u,source,2u,a,3u,3u);
    native::copy_dwords_type5(ctx,0u,source,2u,b,3u,3u);
    assert(a==b);
    assert((Bytes(a.begin()+3,a.begin()+15)==
            Bytes(source.begin()+2,source.begin()+14)));
    assert(a[2]==0xeeu && a[15]==0xeeu);
    native::copy_dwords_type1(ctx,0u,source,0u,a,0u,0u);
    assert(a==b);
}
static void test_original_two_x86_gate_predicates() {
    const Bytes source{0x11u,0x22u,0x33u,0x44u,0u};
    for (const auto operation:{0u,1u,2u,0xffffffffu}) {
        for (const std::uint8_t flag:{std::uint8_t{0u},std::uint8_t{1u},
                                     std::uint8_t{2u},std::uint8_t{0xffu}}) {
            Bytes a(6,0xccu),b=a,c=a,d=a;
            native::copy_dwords_type1({flag},operation,source,0u,a,0u,1u);
            native::copy_dwords_type5({flag},operation,source,0u,b,0u,1u);
            native::copy_bytes_type6({flag},operation,source,0u,c,0u,3u);
            native::copy_c_string_type2({flag},operation,source,0u,d,0u);
            const bool perform=flag!=0u && operation!=1u;
            assert(native::should_copy({flag},operation)==perform);
            if(!perform) {
                assert((a==Bytes(6,0xccu)) && (b==Bytes(6,0xccu)));
                assert((c==Bytes(6,0xccu)) && (d==Bytes(6,0xccu)));
            } else {
                assert(a[0]==0x11u && a[3]==0x44u && a[4]==0xccu);
                assert(b==a);
                assert(c[0]==0x11u && c[2]==0x33u && c[3]==0xccu);
                assert(d[0]==0x11u && d[4]==0u && d[5]==0xccu);
            }
        }
    }
}
static void test_rep_movsd_then_movsb_tail() {
    const Bytes source{10,11,12,13,14,15,16,17,18};
    for(std::uint32_t count=0u;count<=8u;++count) {
        Bytes destination(10,0xeau);
        native::copy_bytes_type6({1u},0u,source,0u,destination,1u,count);
        for(std::size_t i=0;i<count;++i)
            assert(destination[1u+i]==source[i]);
        assert(destination[0]==0xeau && destination[1u+count]==0xeau);
    }
    // Native REP MOVSD reads each entire source DWORD BEFORE writing it,
    // unlike a single-byte forward loop for overlapping memory.
    Bytes overlaps{0,1,2,3,4,5,6,7,8,9,10,11};
    native::copy_dwords_type1({1u},0u,overlaps,0u,overlaps,1u,2u);
    const Bytes expected{0,0,1,2,3,3,5,6,7,9,10,11};
    assert(overlaps==expected);

    Bytes overlap_tail{0,1,2,3,4,5,6,7,8};
    native::copy_bytes_type6({1u},0u,overlap_tail,0u,overlap_tail,1u,6u);
    const Bytes expected_tail{0,0,1,2,3,3,3,7,8};
    assert(overlap_tail==expected_tail);
}
static void test_c_string_scans_and_copies_terminator() {
    const Bytes source{'A','B','C',0,'x','y',0,'z'};
    Bytes dest(16,0xfbu);
    native::copy_c_string_type2({1u},0u,source,0u,dest,2u);
    assert(dest[1]==0xfbu && dest[2]=='A' && dest[3]=='B');
    assert(dest[4]=='C' && dest[5]==0 && dest[6]==0xfbu);
    native::copy_c_string_type2({1u},0u,source,6u,dest,10u);
    assert(dest[10]==0u && dest[11]==0xfbu);
    native::copy_c_string_type2({1u},0u,source,4u,dest,7u);
    assert(dest[7]=='x' && dest[8]=='y' && dest[9]==0);
}
static void test_guards_are_host_only_and_do_not_mask_native_skip() {
    const Bytes src{1u,2u,3u,4u};
    Bytes dest(4,0);
    bool failed=false;
    try {native::copy_dwords_type1({1u},0u,src,0u,dest,0u,0xffffffffu);}
    catch(const std::overflow_error&){failed=true;}
    assert(failed);
    failed=false;
    try {native::copy_bytes_type6({1u},0u,src,0u,dest,0u,5u);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed);
    failed=false;
    try {native::copy_c_string_type2({1u},0u,src,0u,dest,0u);}
    catch(const std::runtime_error&){failed=true;}
    assert(failed);
    // Both original early returns avoid touching caller memory entirely.
    native::copy_dwords_type5({0u},0u,src,99u,dest,88u,0xffffffffu);
    native::copy_c_string_type2({1u},1u,src,99u,dest,88u);
    assert((dest==Bytes(4,0u)));
}
int main() {
    verify_four_real_native_entrypoints();
    test_two_original_dword_helpers_are_identical();
    test_original_two_x86_gate_predicates();
    test_rep_movsd_then_movsb_tail();
    test_c_string_scans_and_copies_terminator();
    test_guards_are_host_only_and_do_not_mask_native_skip();
}
