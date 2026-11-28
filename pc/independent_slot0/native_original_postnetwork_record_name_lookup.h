// Phase1679: reconstruct original 121B CDECL record-name selection
// method 0x24197ED0, the Phase1671 diagnostic core's former
// "opaque formatter" 0x24197ED0. It is actually a record FINDER.
//
// Source: two stack DWORD args (search name, list node), plain RET.
// Read node+0x00 record array and signed count at +0x04, walk
// records in 0x34 strides; skip type0, recurse into type10 +0x20
// BEFORE testing the parent's own name at record+0x04, and follow
// node+0x0C when local records were not found.
// Returns found record pointer or null. Original comparator at
// 0x2425F81B is injected as an opaque host-owned callback.
// All pointers are checked against explicit synthetic grants.
// This NEVER dereferences original game memory or executes retail code.
#pragma once
#include "native_original_postnetwork_cdecl_msg_formatter.h"
#include "native_original_postnetwork_diagnostic_core_finalizer.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent {
namespace original_postnetwork_record_name_lookup {
namespace sw=original_postnetwork_type_switch;
namespace fx=original_postnetwork_cdecl_msg_formatter;
namespace diag=original_postnetwork_diagnostic_core_finalizer;

constexpr std::uintptr_t kOriginalLookupVa=0x24197ED0u;
constexpr unsigned kOriginalLookupBytes=121u;
constexpr std::uintptr_t kOriginalNameCompareVa=0x2425F81Bu;
constexpr unsigned kOriginalArguments=2u;
constexpr unsigned kOriginalCallerCleanup=8u;
constexpr unsigned kOriginalRetPopBytes=0u;
constexpr unsigned kOriginalNodeArrayOffset=0u;
constexpr unsigned kOriginalNodeCountOffset=4u;
constexpr unsigned kOriginalNodeNextOffset=0x0Cu;
constexpr unsigned kOriginalRecordNameOffset=4u;
constexpr unsigned kOriginalRecordNestedOffset=0x20u;
constexpr unsigned kOriginalType10=10u;
constexpr unsigned kSafeMaxNodes=32u;
constexpr unsigned kSafeMaxNames=128u;
constexpr unsigned kSafeMaxDepth=12u;
constexpr unsigned kSafeMaxVisitedRecords=4096u;
static_assert(sizeof(void*)==4u &&
              kOriginalArguments*4u==kOriginalCallerCleanup &&
              sizeof(sw::Record)==0x34u &&
              offsetof(sw::Record,raw)==0u &&
              offsetof(sw::Node,word0)==kOriginalNodeArrayOffset &&
              offsetof(sw::Node,word4)==kOriginalNodeCountOffset &&
              offsetof(sw::Node,next)==kOriginalNodeNextOffset &&
              diag::kOpaqueFormatterVa==kOriginalLookupVa,
              "native original 121-byte cdecl lookup or node ABI drift");

inline std::uint32_t rawptr(const void* ptr) noexcept {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(ptr));
}
struct NodeGrant {
    sw::Node* node=nullptr;
    sw::Record* records=nullptr;
    unsigned capacity=0u;
};
struct NameGrant {
    const char* name=nullptr;
};
struct Context {
    const char* authorized_key=nullptr;
    void* comparator_user=nullptr;
    // Return zero for equality, nonzero for inequality; original
    // comparator's implementation and engine locale remain opaque.
    int (*compare)(void*,const char*,const char*) noexcept=nullptr;
    std::array<NodeGrant,kSafeMaxNodes> nodes{};
    std::array<NameGrant,kSafeMaxNames> names{};
    unsigned node_count=0u;
    unsigned name_count=0u;
    unsigned depth=0u;
    unsigned examined=0u;
    unsigned compared=0u;
    unsigned nested_calls=0u;
    unsigned next_hops=0u;
    unsigned skipped_type0=0u;
    unsigned refused_unowned=0u;
    unsigned refused_budget=0u;
    unsigned missing_backend=0u;

    bool authorize_node(sw::Node* n,sw::Record* data,
                        unsigned capacity) noexcept {
        if(!n || node_count>=nodes.size() || capacity>4096u)return false;
        nodes[node_count++]={n,data,capacity};
        return true;
    }
    bool authorize_name(const char* name) noexcept {
        if(!name || name_count>=names.size())return false;
        names[name_count++]={name};
        return true;
    }
    const NodeGrant* resolve_node(const sw::Node* node) const noexcept {
        for(unsigned i=0u;i<node_count;++i)
            if(nodes[i].node==node)return &nodes[i];
        return nullptr;
    }
    sw::Node* resolve_raw_node(std::uint32_t word) const noexcept {
        for(unsigned i=0u;i<node_count;++i)
            if(rawptr(nodes[i].node)==word)return nodes[i].node;
        return nullptr;
    }
    const char* resolve_name(std::uint32_t word) const noexcept {
        for(unsigned i=0u;i<name_count;++i)
            if(rawptr(names[i].name)==word)return names[i].name;
        return nullptr;
    }
};
// This is explicitly an isolated single-test-session synthetic
// context. The REAL original function has no injected context
// argument, hence the genuine static CDECL uses two stack DWORDs.
inline thread_local Context* active_context=nullptr;
struct ScopedContext {
    Context* prior=nullptr;
    explicit ScopedContext(Context& ctx) noexcept : prior(active_context) {
        active_context=&ctx;
    }
    ~ScopedContext() noexcept {active_context=prior;}
    ScopedContext(const ScopedContext&)=delete;
    ScopedContext& operator=(const ScopedContext&)=delete;
};
struct DepthScope {
    Context* owner=nullptr;
    explicit DepthScope(Context* x) noexcept : owner(x) {++owner->depth;}
    ~DepthScope() noexcept {--owner->depth;}
    DepthScope(const DepthScope&)=delete;
    DepthScope& operator=(const DepthScope&)=delete;
};
struct NativeLookup {
    static __declspec(noinline) sw::Record* __cdecl Find(
            const char* key,sw::Node* first) noexcept {
        auto* ctx=active_context;
        if(!ctx)return nullptr;
        if(!first || !key || key!=ctx->authorized_key) {
            ++ctx->refused_unowned;
            return nullptr;
        }
        if(!ctx->compare) {
            ++ctx->missing_backend;
            return nullptr;
        }
        if(ctx->depth>=kSafeMaxDepth) {
            ++ctx->refused_budget;
            return nullptr;
        }
        DepthScope depth(ctx);
        unsigned local_hops=0u;
        for(sw::Node* cursor=first;cursor;cursor=cursor->next) {
            if(++local_hops>kSafeMaxNodes) {
                ++ctx->refused_budget;
                return nullptr; // synthetic-only cycle refusal
            }
            const auto* grant=ctx->resolve_node(cursor);
            if(!grant) {
                ++ctx->refused_unowned;
                return nullptr;
            }
            // Source count is SIGNED at node+4, and nonpositive
            // counts follow next without reading the data pointer.
            const auto count=static_cast<std::int32_t>(cursor->word4);
            if(count>0) {
                if(!grant->records ||
                   cursor->word0!=rawptr(grant->records) ||
                   static_cast<unsigned>(count)>grant->capacity) {
                    ++ctx->refused_unowned;
                    return nullptr;
                }
                for(std::int32_t i=0;i<count;++i) {
                    if(++ctx->examined>kSafeMaxVisitedRecords) {
                        ++ctx->refused_budget;
                        return nullptr;
                    }
                    sw::Record* rec=grant->records+i;
                    const auto type=rec->read32(0u);
                    if(type==0u) {
                        ++ctx->skipped_type0;
                        continue;
                    }
                    // Original descends only for native type10 and
                    // still tests the parent name after child miss.
                    if(type==kOriginalType10) {
                        const auto child_word=rec->read32(
                            kOriginalRecordNestedOffset);
                        sw::Node* child=ctx->resolve_raw_node(child_word);
                        if(!child) {
                            ++ctx->refused_unowned;
                            return nullptr;
                        }
                        ++ctx->nested_calls;
                        if(auto* found=Find(key,child))return found;
                    }
                    const char* name=ctx->resolve_name(
                        rec->read32(kOriginalRecordNameOffset));
                    if(!name) {
                        ++ctx->refused_unowned;
                        return nullptr;
                    }
                    ++ctx->compared;
                    if(ctx->compare(ctx->comparator_user,name,key)==0)
                        return rec;
                }
            }
            if(cursor->next)++ctx->next_hops;
        }
        return nullptr;
    }
};

// Synthetic native same-owner bridge for the earlier original
// Phase1671 BuildDiagnostic RET12 whose output +0x34 is consumed
// by Phase1672 dispatcher and Phase1678 printf. Only source-pinned
// selection gating is modeled; original engine ConVar/globals remain
// injected host-owned values. Real source has this+0x38 = first arg.
struct IntegratedFixture : fx::Fixture {
    std::int32_t expected_index=0;
    std::uint32_t debug_flags=0u;
    const char* configured_key=nullptr;
    const char* flagged_key=nullptr;
    unsigned selection_calls=0u;
    unsigned lookup_calls=0u;
    unsigned gate_skips=0u;

    __declspec(noinline) void __thiscall SelectDiagnostic(
            const char* first_input,std::uint32_t index,
            sw::Node* records_root) noexcept {
        ++selection_calls;
        diagnostic_record=0u;
        diagnostic_label_word=rawptr(first_input);
        if(!first_input || !first_input[0] || expected_index<0 ||
           index!=static_cast<std::uint32_t>(expected_index)) {
            ++gate_skips;
            return;
        }
        const bool flagged=(debug_flags & diag::kDebugMask)!=0u;
        const char* selected=flagged?flagged_key:configured_key;
        if(!selected || (!flagged && selected[0]=='\0')) {
            ++gate_skips;
            return;
        }
        ++lookup_calls;
        diagnostic_record=rawptr(NativeLookup::Find(selected,records_root));
    }
};
static_assert(offsetof(sw::Fixture,current_record)==0x18u &&
              offsetof(sw::Fixture,diagnostic_record)==0x34u &&
              offsetof(sw::Fixture,diagnostic_label_word)==0x38u,
              "original same-owner record lookup state drift");

} // namespace original_postnetwork_record_name_lookup
} // namespace sin_independent
