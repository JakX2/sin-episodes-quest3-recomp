// Phase1680: source-bounded original-free preflight at client.dll
// 0x24197F50 (121 bytes, CDECL one pointer arg, RET0).
// The original sets node+0x10=1 before even checking signed count,
// scans its 0x34-byte records, recurses into type10 record+0x20,
// and for other nonzero types with record+0x12 bit0x80 looks up
// record+4 NAME in the NEXT node+0x0C chain using Phase1679
// 0x24197ED0 cdecl name finder. Nonnull result overwrites the
// record+0x28 alias pointer. Follow next nodes ONLY when their
// node+0x10 byte is still zero.
// All records/pointers are separately host-granted and there is no
// untrusted retail pointer access or original code execution.
#pragma once
#include "native_original_postnetwork_record_name_lookup.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent {
namespace original_postnetwork_preflight_alias {
namespace sw=original_postnetwork_type_switch;
namespace names=original_postnetwork_record_name_lookup;
constexpr std::uintptr_t kOriginalMethodVa=0x24197F50u;
constexpr unsigned kOriginalMethodBytes=121u;
constexpr unsigned kOriginalStackArgs=1u;
constexpr unsigned kOriginalCallerCleanup=4u;
constexpr unsigned kOriginalRetPop=0u;
constexpr unsigned kOriginalNodeStatusOffset=0x10u;
constexpr unsigned kOriginalRecordAliasOffset=0x28u;
constexpr unsigned kOriginalRecordNameOffset=0x04u;
constexpr unsigned kOriginalRecordNestedOffset=0x20u;
constexpr unsigned kOriginalRecordFlagsOffset=0x12u;
constexpr unsigned kOriginalFlagLookupMask=0x80u;
constexpr unsigned kOriginalSpecialType=10u;
constexpr unsigned kSafeMaxNodes=32u;
constexpr unsigned kSafeMaxRecords=4096u;
constexpr unsigned kSafeMaxDepth=12u;
static_assert(sizeof(void*)==4u &&
              kOriginalStackArgs*4u==kOriginalCallerCleanup &&
              names::kOriginalLookupVa==0x24197ED0u &&
              sw::kOriginalRecordStride==0x34u &&
              offsetof(sw::Node,next)==0x0Cu,
              "native preflight cdecl and record/node offsets changed");

struct NodeBlock {
    // The original node fields validated in Phase1671 occupy
    // 0..0x0F. The now-observed original visited byte at 0x10
    // is reserved immediately afterward, not in callback storage.
    sw::Node base{};
    std::uint8_t visited=0u;
    std::uint8_t padding[3]{};
};
static_assert(offsetof(NodeBlock,base)==0u &&
              offsetof(NodeBlock,visited)==kOriginalNodeStatusOffset,
              "original preflight node+0x10 status byte is misplaced");

struct NodeGrant {
    NodeBlock* node=nullptr;
    sw::Record* records=nullptr;
    unsigned capacity=0u;
};
struct Context {
    names::Context* finder=nullptr;
    std::array<NodeGrant,kSafeMaxNodes> nodes{};
    unsigned node_count=0u;
    unsigned depth=0u;
    unsigned processed=0u;
    unsigned examined=0u;
    unsigned type0_skips=0u;
    unsigned recursive_type10=0u;
    unsigned alias_queries=0u;
    unsigned alias_writes=0u;
    unsigned alias_misses=0u;
    unsigned already_visited_next_stops=0u;
    unsigned refused_unowned=0u;
    unsigned refused_budget=0u;

    bool authorize(NodeBlock* node,sw::Record* records,
                   unsigned capacity) noexcept {
        if(!node || node_count>=nodes.size() ||
           capacity>kSafeMaxRecords)return false;
        nodes[node_count++]={node,records,capacity};
        return true;
    }
    const NodeGrant* resolve(const NodeBlock* node) const noexcept {
        for(unsigned i=0u;i<node_count;++i)
            if(nodes[i].node==node)return &nodes[i];
        return nullptr;
    }
    NodeBlock* resolve_base(const sw::Node* ptr) const noexcept {
        for(unsigned i=0u;i<node_count;++i)
            if(&nodes[i].node->base==ptr)return nodes[i].node;
        return nullptr;
    }
    NodeBlock* resolve_raw(std::uint32_t word) const noexcept {
        for(unsigned i=0u;i<node_count;++i)
            if(names::rawptr(&nodes[i].node->base)==word)
                return nodes[i].node;
        return nullptr;
    }
};
// The exact original native signature has ONE CDECL stack pointer,
// therefore only test-owned thread-local context supplies grants.
inline thread_local Context* active_context=nullptr;
struct ScopedContext {
    Context* previous=nullptr;
    explicit ScopedContext(Context& ctx) noexcept
        : previous(active_context) {active_context=&ctx;}
    ~ScopedContext() noexcept {active_context=previous;}
    ScopedContext(const ScopedContext&)=delete;
    ScopedContext& operator=(const ScopedContext&)=delete;
};
struct DepthScope {
    Context* ctx;
    explicit DepthScope(Context* x) noexcept : ctx(x) {++ctx->depth;}
    ~DepthScope() noexcept {--ctx->depth;}
};
struct NativePreflight {
    static __declspec(noinline) void __cdecl Run(
            NodeBlock* first) noexcept {
        Context* ctx=active_context;
        if(!ctx)return;
        if(!first || !ctx->finder ||
           ctx->finder!=names::active_context) {
            ++ctx->refused_unowned;
            return;
        }
        if(ctx->depth>=kSafeMaxDepth) {
            ++ctx->refused_budget;
            return;
        }
        DepthScope depth(ctx);
        unsigned local_hops=0u;
        NodeBlock* cursor=first;
        while(cursor) {
            if(++local_hops>kSafeMaxNodes) {
                ++ctx->refused_budget;
                return;
            }
            const auto* grant=ctx->resolve(cursor);
            if(!grant) {
                ++ctx->refused_unowned;
                return;
            }
            // Original marks the current node unconditionally:
            // no entry guard on a previously marked first node.
            cursor->visited=1u;
            ++ctx->processed;
            const auto signed_count=
                static_cast<std::int32_t>(cursor->base.word4);
            if(signed_count>0) {
                if(!grant->records ||
                   cursor->base.word0!=names::rawptr(grant->records) ||
                   static_cast<unsigned>(signed_count)>grant->capacity) {
                    ++ctx->refused_unowned;
                    return;
                }
                for(std::int32_t i=0;i<signed_count;++i) {
                    if(++ctx->examined>kSafeMaxRecords) {
                        ++ctx->refused_budget;
                        return;
                    }
                    sw::Record* rec=grant->records+i;
                    const auto type=rec->read32(0u);
                    if(type==0u) {
                        ++ctx->type0_skips;
                        continue;
                    }
                    if(type==kOriginalSpecialType) {
                        // A type10 ALWAYS takes this recursion path
                        // instead of the alias/flag path.
                        NodeBlock* child=ctx->resolve_raw(
                            rec->read32(kOriginalRecordNestedOffset));
                        if(!child) {
                            ++ctx->refused_unowned;
                            return;
                        }
                        ++ctx->recursive_type10;
                        Run(child);
                        continue;
                    }
                    if((rec->raw[kOriginalRecordFlagsOffset] &
                        kOriginalFlagLookupMask)==0u ||
                       !cursor->base.next)
                        continue;
                    NodeBlock* next=ctx->resolve_base(cursor->base.next);
                    const char* name=ctx->finder->resolve_name(
                        rec->read32(kOriginalRecordNameOffset));
                    if(!next || !name) {
                        ++ctx->refused_unowned;
                        return;
                    }
                    // Original: Find(name, current_node->next).
                    // Synthetic finder grants a single authorized key
                    // at a time; temporarily authorize exactly this
                    // known host-owned record name, then restore.
                    const char* previous=ctx->finder->authorized_key;
                    ctx->finder->authorized_key=name;
                    ++ctx->alias_queries;
                    sw::Record* selected=names::NativeLookup::Find(
                        name,&next->base);
                    ctx->finder->authorized_key=previous;
                    if(selected) {
                        rec->write32(kOriginalRecordAliasOffset,
                                     names::rawptr(selected));
                        ++ctx->alias_writes;
                    } else ++ctx->alias_misses;
                }
            }
            if(!cursor->base.next)break;
            NodeBlock* next=ctx->resolve_base(cursor->base.next);
            if(!next) {
                ++ctx->refused_unowned;
                return;
            }
            // Source checks the NEXT node's existing visited bit;
            // it does not skip an already-visited starting node.
            if(next->visited!=0u) {
                ++ctx->already_visited_next_stops;
                break;
            }
            cursor=next;
        }
    }
};

} // namespace original_postnetwork_preflight_alias
} // namespace sin_independent
