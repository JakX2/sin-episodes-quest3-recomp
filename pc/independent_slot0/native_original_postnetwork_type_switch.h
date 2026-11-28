// Phase1672: original-free x86 typed PostNetwork per-node dispatch SHELL.
//
// Original source: client.dll 2006 PE32 0x241996F0, EXACT 1036 bytes
// SHA-gated by audit_sin_original_postnetwork_type_dispatch_13case.py.
// Four stack DWORD arguments, __thiscall RET16:
//    (sequence, node, raw 0x34-byte records, signed count).
//
// This reconstructs the original ENTRY/CACHE/SEQUENCE/FLAG FILTERS,
// +0x34 record step, immutable 27->13 type dispatch tables, case0
// no-op, case12/out-of-range diagnostic handoff and original exit.
// The 11 specialized comparison branches, case10's recursive call,
// logger IAT and all nested original routines are NOT implemented:
// host-owned callbacks mark those opaque boundaries precisely.
//
// No original Source engine object/code is read or executed by CI.
#pragma once

#include "native_original_postnetwork_diagnostic_core_finalizer.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sin_independent {
namespace original_postnetwork_type_switch {

constexpr std::uintptr_t kOriginalDispatcherVa=0x241996F0u;
constexpr unsigned kOriginalDispatcherBytes=1036u;
constexpr unsigned kOriginalStackArgs=4u;
constexpr unsigned kOriginalRetPopBytes=16u;
constexpr unsigned kOriginalRecordStride=0x34u;
constexpr unsigned kOriginalTypeCodeCount=27u;
constexpr unsigned kOriginalCaseCount=13u;
constexpr unsigned kOriginalNodeCacheFieldOffset=8u;
constexpr unsigned kOriginalRecordFlagsOffset=0x12u;
constexpr unsigned kOriginalRecordAliasOffset=0x28u;
constexpr unsigned kOriginalRecordSequenceOffset=0x2Cu;
constexpr unsigned kOriginalThisRecordOffset=0x18u;
constexpr unsigned kOriginalThisCacheOffset=0x1Cu;
constexpr unsigned kOriginalThisNodeOffset=0x20u;
constexpr unsigned kOriginalThisOptionOffset=0x15u;
constexpr unsigned kOriginalThisOptionCopyOffset=0x24u;
constexpr unsigned kOriginalThisActiveOffset=0x25u;
constexpr unsigned kSyntheticMaxRecords=4096u;
constexpr std::array<std::uint8_t,kOriginalTypeCodeCount>
    kOriginalTypeToCase{{
        0,1,2,3,4,5,6,7,8,9,10,0,0,11,
        0,0,0,0,0,0,12,0,12,12,12,12,0
    }};
static_assert(kOriginalStackArgs*4u==kOriginalRetPopBytes &&
              kOriginalRecordStride==52u &&
              kOriginalTypeToCase[10]==10u &&
              kOriginalTypeToCase[20]==12u &&
              kOriginalTypeToCase[11]==0u,
              "native dispatcher four-arg RET16 / 27->13 type map drift");

struct alignas(4) Record {
    // All bytes are invented caller-owned diagnostic memory.
    // Avoid inventing an engine class name or full RTTI identity.
    std::array<std::uint8_t,kOriginalRecordStride> raw{};
    std::uint32_t read32(unsigned off) const noexcept {
        std::uint32_t value=0u;
        if(off+4u<=raw.size())
            std::memcpy(&value,raw.data()+off,sizeof(value));
        return value;
    }
    void write32(unsigned off,std::uint32_t value) noexcept {
        if(off+4u<=raw.size())
            std::memcpy(raw.data()+off,&value,sizeof(value));
    }
    std::uint16_t read16(unsigned off) const noexcept {
        std::uint16_t value=0u;
        if(off+2u<=raw.size())
            std::memcpy(&value,raw.data()+off,sizeof(value));
        return value;
    }
    void write16(unsigned off,std::uint16_t value) noexcept {
        if(off+2u<=raw.size())
            std::memcpy(raw.data()+off,&value,sizeof(value));
    }
};
static_assert(sizeof(Record)==kOriginalRecordStride,
              "native 2006 per-node records must have exact 0x34 stride");

struct DispatchOps {
    void* user=nullptr;
    // The original writes *record.aliasPointer+0x2C, if alias is
    // nonnull. A callback resolves ONLY known synthetic owners;
    // never dereference raw untrusted original addresses in CI.
    void (*update_alias_sequence)(void*,Record*,std::uint32_t,
                                  std::uint32_t) noexcept=nullptr;
    // Cases 1..11 are specialized, including case10 recursion.
    // Each remains opaque and is NEVER simulated as generic equality.
    void (*on_opaque_case)(void*,std::uint8_t,std::uint32_t,
                           Record*,std::uint32_t,std::uint32_t,
                           std::int16_t) noexcept=nullptr;
    // Source-authenticated case12 AND type>26 both invoke original
    // diagnostic IAT. The message formatting is not reconstructed.
    void (*on_opaque_warning)(void*,std::uint32_t,Record*) noexcept=nullptr;
};
struct DispatchEvents {
    unsigned missing_inputs=0u;
    unsigned refused_count=0u;
    unsigned refused_indices=0u;
    unsigned refused_alias_update=0u;
    unsigned visited=0u;
    unsigned alias_updates=0u;
    unsigned repeated_sequence_skips=0u;
    unsigned flag_mode_skips=0u;
    unsigned no_op_case0=0u;
    unsigned opaque_case_calls=0u;
    unsigned recursive_case10_not_implemented=0u;
    unsigned opaque_warnings=0u;
    unsigned missing_callbacks=0u;
};

using Node=original_postnetwork_diagnostic_core_finalizer::ListNode;
static_assert(sizeof(void*)==4u &&
              offsetof(Node,unknown8)==kOriginalNodeCacheFieldOffset,
              "native original node pointer initial-cache DWORD is at +8");

struct alignas(4) Fixture {
    // Layout matches ORIGINAL discriminator object from native body,
    // not the rest of its unobserved internals. Host callbacks follow
    // original-source byte +0x25 and are test-only.
    std::uint32_t mode=0u;         // this+0x00
    std::uint32_t left_bias=0u;    // this+0x04
    std::uint32_t right_bias=0u;   // this+0x08
    std::uint32_t left_index=0u;   // this+0x0C
    std::uint32_t right_index=1u;  // this+0x10
    std::uint8_t unknown14=0u;    // this+0x14
    std::uint8_t option=0u;       // this+0x15
    std::uint16_t unknown16=0u;   // this+0x16
    std::uint32_t current_record=0u;  // this+0x18
    std::uint32_t cached_node_word=0u; // this+0x1C
    std::uint32_t current_node=0u;     // this+0x20
    std::uint8_t copied_option=0u;     // this+0x24
    std::uint8_t active=0u;       // this+0x25
    std::uint16_t unknown26=0u;   // this+0x26
    // Phase1674: original case8/9 both check BYTE this+0x2C
    // after the 56-byte comparison, BEFORE optional REP MOVS
    // correction. Earlier fake callbacks occupied this slot;
    // reserve the original owner fields first, then callbacks.
    std::uint32_t original_unknown28=0u; // this+0x28
    std::uint8_t correction_byte=0u;     // this+0x2C
    std::uint8_t original_unknown2D[3]{}; // +0x2D..+0x2F

    DispatchOps ops{};
    DispatchEvents events{};

    // Original code at 0x241996F0 has 4 stack arguments and a
    // native RET 0x10. The full specialized switch bodies are OPAQUE.
    __declspec(noinline) void __thiscall Dispatch(
        std::uint32_t sequence,Node* node,Record* records,
        std::int32_t count) noexcept {
        if(!node) {
            ++events.missing_inputs;
            return; // safer than original, which derefs [arg2+8]
        }
        // Original entry: save second caller arg at this+0x20.
        current_node=static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(node));
        if(cached_node_word==0u)
            cached_node_word=node->unknown8;
        if(count<=0) {
            cached_node_word=0u;
            return;
        }
        if(!records) {
            ++events.missing_inputs;
            cached_node_word=0u;
            return;
        }
        if(count>static_cast<std::int32_t>(kSyntheticMaxRecords)) {
            ++events.refused_count;
            cached_node_word=0u;
            return;
        }
        const unsigned max_index=(kOriginalRecordStride-12u)/4u;
        for(std::int32_t i=0;i<count;++i) {
            Record* const r=records+i; // exact +0x34 byte stride
            current_record=static_cast<std::uint32_t>(
                reinterpret_cast<std::uintptr_t>(r));
            ++events.visited;
            const auto flags=r->read16(kOriginalRecordFlagsOffset);
            const auto alias=r->read32(kOriginalRecordAliasOffset);
            if(alias!=0u) {
                if(!ops.update_alias_sequence) {
                    ++events.refused_alias_update;
                    continue;
                }
                ops.update_alias_sequence(ops.user,r,alias,sequence);
                ++events.alias_updates;
            }
            // Native compares record+0x2C AFTER possibly changing the
            // object pointed to by record+0x28.
            if(r->read32(kOriginalRecordSequenceOffset)==sequence) {
                ++events.repeated_sequence_skips;
                continue;
            }
            const auto type=r->read32(0u);
            // Original type10 bypasses ALL three early flag/mode
            // tests, but its own recursive implementation is opaque.
            if(type!=10u) {
                const bool common=(flags & 0x0200u)!=0u;
                const bool mode1=mode==1u && (flags&0x0100u)!=0u;
                const bool mode2=mode==2u && (flags&0x0100u)==0u;
                if(common || mode1 || mode2) {
                    ++events.flag_mode_skips;
                    continue;
                }
            }
            if(left_index>max_index || right_index>max_index) {
                ++events.refused_indices;
                continue; // unsafe original offsets not dereferenced
            }
            const auto left=r->read32(
                0x08u+left_index*4u)+left_bias;
            const auto right=r->read32(
                0x08u+right_index*4u)+right_bias;
            // Original sets these two bytes BEFORE switch case
            // dispatch, even if case0 ultimately does nothing.
            copied_option=option;
            active=1u;
            const auto signed_flags=static_cast<std::int16_t>(flags);
            const auto dispatch_type=type<=26u?
                kOriginalTypeToCase[type]:12u;
            if(dispatch_type==0u && type<=26u) {
                ++events.no_op_case0;
            } else if(dispatch_type==12u) {
                ++events.opaque_warnings;
                if(ops.on_opaque_warning)
                    ops.on_opaque_warning(ops.user,type,r);
                else ++events.missing_callbacks;
            } else {
                ++events.opaque_case_calls;
                if(dispatch_type==10u)
                    ++events.recursive_case10_not_implemented;
                if(ops.on_opaque_case)
                    ops.on_opaque_case(ops.user,dispatch_type,type,
                                       r,left,right,signed_flags);
                else ++events.missing_callbacks;
            }
        }
        // Original epilogue ALWAYS resets this+0x1C to zero, even
        // after skipping all records and even for count <=0.
        cached_node_word=0u;
    }
};
static_assert(offsetof(Fixture,option)==kOriginalThisOptionOffset &&
              offsetof(Fixture,current_record)==kOriginalThisRecordOffset &&
              offsetof(Fixture,cached_node_word)==kOriginalThisCacheOffset &&
              offsetof(Fixture,current_node)==kOriginalThisNodeOffset &&
              offsetof(Fixture,copied_option)==kOriginalThisOptionCopyOffset &&
              offsetof(Fixture,active)==kOriginalThisActiveOffset &&
              offsetof(Fixture,correction_byte)==0x2Cu,
              "original 2006 PE32 dispatcher entry/dispatch member offsets differ");

} // namespace original_postnetwork_type_switch
} // namespace sin_independent
