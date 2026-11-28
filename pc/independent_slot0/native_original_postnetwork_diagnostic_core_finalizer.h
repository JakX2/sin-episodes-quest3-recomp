// Phase1671: original-free source-bounded diagnostic core + list final.
//
// Original 2006 PE32 0x24199100, exactly 139 native bytes:
// __thiscall three stack DWORDs RET12. Clear this+0x34, remember
// first pointer at this+0x38. Gate on nonempty first string,
// nonnegative ConVar parent value equal to arg2, and original
// 0x1000 global debug flag / optional label. Invoke ONLY an
// opaque 0x24197ED0 stand-in and store its return at this+0x34.
//
// Original 2006 PE32 0x24199B50, exactly 65 native bytes:
// __thiscall two stack DWORDs RET8. Process supplied FIRST node
// unconditionally and follow each node+0x0C linked pointer.
// Pass original sequence number, node, node+0 and node+4 WORDs
// to unknown 0x241996F0, as an explicit opaque callback.
//
// No original game code, binaries, or real Source objects here.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent {
namespace original_postnetwork_diagnostic_core_finalizer {

constexpr std::uintptr_t kCoreVa=0x24199100u;
constexpr std::uintptr_t kFinalizerVa=0x24199B50u;
constexpr std::uintptr_t kOpaqueFormatterVa=0x24197ED0u;
constexpr std::uintptr_t kOpaquePerNodeVa=0x241996F0u;
constexpr unsigned kCoreNativeArgs=3u;
constexpr unsigned kCoreNativeRetPop=12u;
constexpr unsigned kFinalizerNativeArgs=2u;
constexpr unsigned kFinalizerNativeRetPop=8u;
constexpr unsigned kCoreOutputOffset=0x34u;
constexpr unsigned kCoreInputOffset=0x38u;
constexpr unsigned kNodeNextOffset=0x0Cu;
constexpr unsigned kDebugMask=0x1000u;
constexpr unsigned kSafeMaxNodes=4096u;
static_assert(kCoreNativeArgs*4u==kCoreNativeRetPop &&
              kFinalizerNativeArgs*4u==kFinalizerNativeRetPop,
              "native three/two DWORD __thiscall RET12/RET8 contract drift");

struct DiagnosticGateOps {
    void* user=nullptr;
    std::int32_t expected_index=-1;
    std::uint32_t debug_flags=0u;
    const char* configured_label=nullptr;
    const char* flagged_label=nullptr;
    // The full original 0x24197ED0 body is opaque. This callback
    // models only the observed input label/context and returned word.
    std::uint32_t (*format)(void*,const char*,void*) noexcept=nullptr;
};
struct DiagnosticGateEvents {
    unsigned formatted=0u;
    unsigned safe_refusals=0u;
};
inline DiagnosticGateEvents core_events{};

struct alignas(4) DiagnosticCoreFixture {
    std::array<std::uint8_t,kCoreOutputOffset> opaque{};
    std::uint32_t format_result=0u;
    std::uint32_t input_pointer=0u;
    DiagnosticGateOps ops{};

    std::uintptr_t format_result_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&format_result);
    }
    std::uintptr_t input_pointer_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&input_pointer);
    }
    void set_ops(DiagnosticGateOps cb) noexcept {ops=cb;}

    // Nonvirtual MSVC __thiscall ECX=this, three stack DWORDs,
    // exact native RET 0x0C. The original arg1 is a pointer to a
    // zero-terminated byte string. All public tests supply owned
    // valid pointers, never arbitrary commercial game addresses.
    __declspec(noinline) void __thiscall BuildDiagnostic(
            const char* first_input,std::uint32_t index,
            void* context) noexcept {
        format_result=0u;
        input_pointer=static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(first_input));
        if(!first_input || !first_input[0])return;
        if(ops.expected_index<0 ||
           index!=static_cast<std::uint32_t>(ops.expected_index))
            return;
        const bool special=(ops.debug_flags & kDebugMask)!=0u;
        const char* chosen=special?
            ops.flagged_label:ops.configured_label;
        // The original non-special branch skips when configured
        // text+0x20 is NULL or points to an empty string. The
        // special branch always uses its original literal.
        if(!chosen || (!special && chosen[0]==0u))
            return;
        if(!ops.format) {
            ++core_events.safe_refusals;
            return; // safe-only, original native would deref
        }
        ++core_events.formatted;
        format_result=ops.format(ops.user,chosen,context);
    }
};
static_assert(offsetof(DiagnosticCoreFixture,format_result)==
              kCoreOutputOffset &&
              offsetof(DiagnosticCoreFixture,input_pointer)==
              kCoreInputOffset,
              "original this+0x34/+0x38 diagnostic word offsets changed");

// Original nodes have words at offsets0/4 and a linked pointer+0xC.
// The meaning of word0/word4 and the per-node 0x241996F0 helper
// are deliberately opaque. The pointer size is exactly four bytes.
struct ListNode {
    std::uint32_t word0=0u;
    std::uint32_t word4=0u;
    std::uint32_t unknown8=0u;
    ListNode* next=nullptr;
};
static_assert(sizeof(void*)==4u &&
              offsetof(ListNode,next)==kNodeNextOffset,
              "original 2006 PE32 list-node +0xC link layout changed");

struct FinalizerOps {
    void* user=nullptr;
    void (*apply_entry)(void*,std::uint32_t,ListNode*,
                        std::uint32_t,std::uint32_t) noexcept=nullptr;
};
struct FinalizerEvents {
    unsigned missing_backend_refusals=0u;
    unsigned null_head_refusals=0u;
    unsigned oversized_chain_refusals=0u;
    unsigned processed_entries=0u;
};
inline FinalizerEvents final_events{};

struct ListFinalizerFixture {
    FinalizerOps ops{};
    void set_ops(FinalizerOps cb) noexcept {ops=cb;}

    // Original calls helper on FIRST node before reading next.
    // Original does not validate null head or detect cycles;
    // synthetic model refuses those hazards.
    __declspec(noinline) void __thiscall Finalize(
            std::uint32_t sequence,ListNode* first) noexcept {
        if(!ops.apply_entry) {
            ++final_events.missing_backend_refusals;
            return;
        }
        if(!first) {
            ++final_events.null_head_refusals;
            return;
        }
        ListNode* current=first;
        unsigned processed=0u;
        while(current && processed<kSafeMaxNodes) {
            ListNode* const next=current->next;
            ops.apply_entry(ops.user,sequence,current,
                            current->word0,current->word4);
            ++processed;
            ++final_events.processed_entries;
            current=next;
        }
        if(current)
            ++final_events.oversized_chain_refusals;
    }
};
} // namespace original_postnetwork_diagnostic_core_finalizer
} // namespace sin_independent
