// Phase1683: original-free native Win32 x86 reconstruction of the
// authentic 110B CPrediction reinit helper at client 0x24195C20.
// The Phase1682 slot6 enable transition calls this after detecting
// old disabled -> new enabled, before reset fields.
//
// Source: global registry+0x10014 vcall +0x18 gets MAX index
// (inclusive), original per-index 0x24105B50 lookup, original
// IsPredictable 0x240C5500, if entity NONNULL and predicate FALSE
// call opaque 0x240CC2D0 with "ReinitPredictables". Then a
// separate global entity-list Count vcall+4 is printed through
// tier0!Msg("Reinitialized %i predictable entities\n").
// This printed count is NOT the number of opaque reinit calls.
// No original entity/engine pointers are touched. Host-only bounded
// callbacks stand in for all still-unimplemented engine routines.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1683 native prediction reinit fixture requires MSVC x86
#endif
namespace sin_independent {
namespace original_prediction_reinit_predictables {
constexpr std::uintptr_t kOriginalMethodVa=0x24195C20u;
constexpr unsigned kOriginalNativeBytes=110u;
constexpr unsigned kOriginalStackArgs=0u;
constexpr unsigned kOriginalRetPop=0u;
constexpr std::uintptr_t kOriginalGlobalRegistryVa=0x242FA738u;
constexpr std::uintptr_t kOriginalGlobalEntityListVa=0x242F7EC8u;
constexpr unsigned kOriginalRegistrySubobjectOffset=0x10014u;
constexpr unsigned kOriginalRegistryMaxVcallOffset=0x18u;
constexpr unsigned kOriginalCountVcallOffset=4u;
constexpr std::uintptr_t kOriginalIndexLookupVa=0x24105B50u;
constexpr std::uintptr_t kOriginalIsPredictableVa=0x240C5500u;
constexpr std::uintptr_t kOriginalOpaqueReinitVa=0x240CC2D0u;
constexpr std::uintptr_t kOriginalMsgIat=0x242761ECu;
constexpr unsigned kMaxSafeIndexedEntities=4096u;
constexpr char kOriginalReason[]="ReinitPredictables";
constexpr char kOriginalMsgLiteral[]=
    "Reinitialized %i predictable entities\n";

struct Ops {
    void* user=nullptr;
    // The global registry's original +0x10014 virtual +0x18
    // returns the LAST VALID INDEX, not a count.
    std::int32_t (*max_index_inclusive)(void*) noexcept=nullptr;
    // Original 0x24105B50 index resolver (may return nullptr).
    void* (*entity_by_index)(void*,std::int32_t) noexcept=nullptr;
    std::uint8_t (*is_predictable)(void*,void*) noexcept=nullptr;
    // Original 0x240CC2D0 is NOT implemented; only its call
    // position, original label and eligible pointer are modeled.
    void (*reinitialize_opaque)(void*,void*,const char*) noexcept=nullptr;
    // Original separate client entity-list Count (vtable+0x04).
    std::int32_t (*total_count_for_log)(void*) noexcept=nullptr;
    void (*on_msg)(void*,const char*,std::int32_t) noexcept=nullptr;
};
struct Events {
    unsigned called=0u;
    unsigned safe_refusals=0u;
    unsigned empty_max_index=0u;
    unsigned oversized_max_index_refusals=0u;
    unsigned index_lookup_calls=0u;
    unsigned null_entity_skips=0u;
    unsigned predicate_calls=0u;
    unsigned already_predictable_skips=0u;
    unsigned reinitialize_calls=0u;
    unsigned log_count_calls=0u;
    unsigned emitted_messages=0u;
};
// Genuine compiler-generated nonvirtual __thiscall, ECX=this,
// NO stack arguments and plain RET0. All state host-only beyond
// source-observable fields: the original helper accesses globals.
struct Fixture {
    Ops ops{};
    Events events{};

    __declspec(noinline) void __thiscall Reinitialize() noexcept {
        ++events.called;
        if(!ops.max_index_inclusive || !ops.entity_by_index ||
           !ops.is_predictable || !ops.reinitialize_opaque ||
           !ops.total_count_for_log || !ops.on_msg) {
            ++events.safe_refusals;
            return; // host-only; original dereferences engine globals
        }
        const auto maximum=ops.max_index_inclusive(ops.user);
        if(maximum>=static_cast<std::int32_t>(kMaxSafeIndexedEntities)) {
            ++events.oversized_max_index_refusals;
            return; // bound; the original has no such limit
        }
        if(maximum<0) {
            ++events.empty_max_index;
        } else {
            // Critical: the source comparison is i <= maximum,
            // not i < maximum; index 0 always visited when max==0.
            for(std::int32_t i=0;i<=maximum;++i) {
                ++events.index_lookup_calls;
                void* entity=ops.entity_by_index(ops.user,i);
                if(!entity) {
                    ++events.null_entity_skips;
                    continue;
                }
                ++events.predicate_calls;
                if(ops.is_predictable(ops.user,entity)!=0u) {
                    ++events.already_predictable_skips;
                    continue;
                }
                ++events.reinitialize_calls;
                ops.reinitialize_opaque(
                    ops.user,entity,kOriginalReason);
            }
        }
        // Original prints a DIFFERENT global Count value, not
        // the number of reinitialize calls or (maximum+1).
        ++events.log_count_calls;
        const auto total=ops.total_count_for_log(ops.user);
        ++events.emitted_messages;
        ops.on_msg(ops.user,kOriginalMsgLiteral,total);
    }
};
static_assert(kOriginalStackArgs==0u && kOriginalRetPop==0u &&
              sizeof(void*)==4u,
              "native reinitialization original RET0 ABI drift");

} // namespace original_prediction_reinit_predictables
} // namespace sin_independent
