// Phase1673: original 2006 PE32 56-byte PostNetwork REPZ CMPSB gate.
//
// Retail original method at 0x24197A90 has one ECX=this pointer,
// THREE stack DWORD arguments (byteCount, leftBytes, rightBytes),
// and native __thiscall RET 0x0C. Called directly by typed
// dispatcher cases 8 and 9 at two exact source callsites.
//
// 1. If dispatcher byte this+0x14 ==0 -> AL0.
// 2. If current record at this+0x18 has byte+0x13 bit0x04
//    -> AL1 without reading buffers.
// 3. Else native REPZ CMPSB(byteCount, leftBytes, rightBytes)
//    -> AL1 exactly on byte-for-byte equality, AL0 otherwise.
//    If byteCount=0, native ZF is 1 after XOR and returns AL1.
//
// This SAFE original-free subclass reuses exactly the Phase1672
// dispatcher 'this' layout without inserting a virtual table.
// A second trusted owner pointer verifies current_record before
// any synthetic dereference; no original game memory is read.
// Original case8/9 remaining diagnostic/copy helpers are OPAQUE.
#pragma once
#include "native_original_postnetwork_type_switch.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sin_independent {
namespace original_postnetwork_case8_case9_bytecmp {

constexpr std::uintptr_t kOriginalMethodVa=0x24197A90u;
constexpr unsigned kOriginalNativeMethodBytes=56u;
constexpr unsigned kNativeArgWords=3u;
constexpr unsigned kNativeRetPopBytes=12u;
constexpr unsigned kSourceEnabledByteOffset=0x14u;
constexpr unsigned kSourceRecordPointerOffset=0x18u;
constexpr unsigned kSourceRecordForceTrueByteOffset=0x13u;
constexpr unsigned kSourceRecordForceTrueMask=0x04u;
constexpr unsigned kSourceCase8Multiplier=1u;
constexpr unsigned kSourceCase9Multiplier=4u;
constexpr unsigned kSafeCompareMaxBytes=4096u;
static_assert(kNativeArgWords*4u==kNativeRetPopBytes &&
              offsetof(original_postnetwork_type_switch::Fixture,
                       unknown14)==kSourceEnabledByteOffset &&
              offsetof(original_postnetwork_type_switch::Fixture,
                       current_record)==kSourceRecordPointerOffset,
              "native three-arg RET12 and original THIS layout drift");

struct CompareEvents {
    unsigned returned_disabled_zero=0u;
    unsigned forced_true_from_record_flag=0u;
    unsigned compared=0u;
    unsigned equal=0u;
    unsigned unequal=0u;
    unsigned refused_invalid_record=0u;
    unsigned refused_unsafe_count_or_buffers=0u;
};

struct CorrectionEvents {
    unsigned skipped_correction_disabled=0u;
    unsigned skipped_comparison_equal=0u;
    unsigned rejected_unowned_or_oversized_copy=0u;
    unsigned correction_copy_paths=0u;
    unsigned corrected_bytes=0u;
};

struct ComparatorFixture :
    original_postnetwork_type_switch::Fixture {
    // All following fields live BEYOND the previously source-pinned
    // this+0x25 byte. No original dispatcher layout is changed.
    original_postnetwork_type_switch::Record* safe_current_record=nullptr;
    CompareEvents compare_events{};
    // Phase1674: these invented buffer grants are test-only and
    // live AFTER every observed original this+0x2C field. Native
    // game address integers are NEVER dereferenced arbitrarily.
    std::uint8_t* owned_left=nullptr;
    const std::uint8_t* owned_right=nullptr;
    std::size_t owned_capacity=0u;
    CorrectionEvents correction_events{};

    void authorize_copy_buffers(std::uint8_t* left,
                                const std::uint8_t* right,
                                std::size_t capacity) noexcept {
        owned_left=left;
        owned_right=right;
        owned_capacity=capacity;
    }

    void authorize_current_record(
        original_postnetwork_type_switch::Record* record) noexcept {
        safe_current_record=record;
    }

    // Real compiler-generated MSVC x86 nonvirtual thiscall, RET12.
    // Unknown original source and pointer lifetime are substituted
    // only by host-authorized invented buffers.
    __declspec(noinline) std::uint32_t __thiscall CompareBytes(
        std::uint32_t length,
        const std::uint8_t* lhs,
        const std::uint8_t* rhs) noexcept {
        if(unknown14==0u) {
            ++compare_events.returned_disabled_zero;
            return 0u;
        }
        // Original unconditionally dereferences this+0x18 if
        // enabled; original-free diagnostic rejects unowned pointer.
        if(!safe_current_record ||
           current_record!=static_cast<std::uint32_t>(
                 reinterpret_cast<std::uintptr_t>(
                     safe_current_record))) {
            ++compare_events.refused_invalid_record;
            return 0u;
        }
        if((safe_current_record->raw[
              kSourceRecordForceTrueByteOffset] &
              kSourceRecordForceTrueMask)!=0u) {
            ++compare_events.forced_true_from_record_flag;
            return 1u;
        }
        // A zero length native REPZ CMPSB does not dereference
        // either pointer and returns ZF=1. Let nulls through for
        // length0, but not for positive lengths.
        if(length>kSafeCompareMaxBytes ||
           (length!=0u && (!lhs || !rhs))) {
            ++compare_events.refused_unsafe_count_or_buffers;
            return 0u;
        }
        ++compare_events.compared;
        const bool same=length==0u ||
            std::memcmp(lhs,rhs,static_cast<std::size_t>(length))==0;
        if(same)++compare_events.equal;
        else ++compare_events.unequal;
        return same?1u:0u;
    }

    // Extracted source-faithful case8/9 INLINE correction block,
    // NOT a separately existing retail native method. Original
    // 0x24199933 / 0x24199A33 first check BYTE this+0x2C and
    // compare EAX!=1, then copy RIGHT EBX -> LEFT EDI via
    // REP MOVSD floor(count/4), REP MOVSB count%4.
    //
    // This safe facade owns no original engine buffers and refuses
    // non-authorized pointers or counts. For authorized DISJOINT
    // host-owned spans, memcpy reproduces identical final bytes
    // to the native forward REP MOVSD/REP MOVSB instruction pair.
    bool MaybeApplyCase8Case9Correction(
            std::uint32_t count,std::uint32_t compare_result,
            std::uint8_t* destination_left,
            const std::uint8_t* source_right) noexcept {
        if(correction_byte==0u) {
            ++correction_events.skipped_correction_disabled;
            return false;
        }
        if(compare_result==1u) {
            ++correction_events.skipped_comparison_equal;
            return false;
        }
        if(!owned_left || !owned_right ||
           destination_left!=owned_left ||
           source_right!=owned_right ||
           count>kSafeCompareMaxBytes ||
           count>owned_capacity ||
           owned_left==owned_right) {
            ++correction_events.rejected_unowned_or_oversized_copy;
            return false;
        }
        if(count>0u)
            std::memcpy(destination_left,source_right,
                        static_cast<std::size_t>(count));
        ++correction_events.correction_copy_paths;
        correction_events.corrected_bytes+=count;
        return true;
    }
};
static_assert(sizeof(ComparatorFixture)>=
              sizeof(original_postnetwork_type_switch::Fixture),
              "original-free derived comparator cannot change base ABI");

} // namespace original_postnetwork_case8_case9_bytecmp
} // namespace sin_independent
