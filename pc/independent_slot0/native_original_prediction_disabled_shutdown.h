// Phase1666: entirely original-free diagnostic model of the 2006 PE32
// CPrediction disabled-cl_predict fallback at original 0x24195720.
//
// The 118-byte native original function is WHOLE-SHA authenticated in
// audit_sin_original_prediction_disabled_fallback.py. Its exact source
// behavior is a BACKWARD client-entity-list traversal, separating the
// native IsPredictable flag into two distinct cleanup branches:
//   predictable -> original direct call 0x240CA9F0;
//   client-created -> embedded entity+8 vptr slot1 at byte offset +4.
// It logs "Shutdown %i predictable entities and %i client-created
// entities\n" only when either count is nonzero.
//
// No retail entity, original DLL, real engine callback, or commercial
// machine-code body is present here. Every pointer is host-owned.
// Public CI can inject an invented entity list and callback semantics.
#pragma once

#include <cstddef>
#include <cstdint>

namespace sin_independent {
namespace original_prediction_disabled_shutdown {

constexpr unsigned kOriginalClientListCountVptrOffset=4u;
constexpr unsigned kOriginalClientListGetVptrOffset=0u;
constexpr unsigned kOriginalPredictableFieldOffset=0x2E8u;
constexpr unsigned kOriginalClientCreatedEmbeddedVptrOffset=8u;
constexpr unsigned kOriginalClientCreatedShutdownSlot=1u;
constexpr unsigned kOriginalClientCreatedShutdownVptrOffset=4u;
constexpr unsigned kSyntheticMaxSafeEntities=4096u;
constexpr std::uintptr_t kOriginalFallbackVa=0x24195720u;
constexpr std::uintptr_t kOriginalPredictableShutdownVa=0x240CA9F0u;

struct Ops {
    void* user=nullptr;
    std::int32_t (*count)(void*) noexcept=nullptr;
    void* (*get)(void*,std::int32_t) noexcept=nullptr;
    bool (*predictable)(void*,void*) noexcept=nullptr;
    void (*shutdown_predictable)(void*,void*) noexcept=nullptr;
    void (*shutdown_client_created)(void*,void*) noexcept=nullptr;
    void (*print_summary)(void*,unsigned,unsigned) noexcept=nullptr;
};
enum class Status {
    MissingBackend,
    UnsafeCount,
    Complete,
};
struct Result {
    Status status=Status::MissingBackend;
    unsigned entity_gets=0u;
    unsigned null_entities=0u;
    unsigned predictable_shutdowns=0u;
    unsigned client_created_shutdowns=0u;
    bool printed_summary=false;
    unsigned original_method_invocations=0u;
};

inline Result Run(const Ops& ops) noexcept {
    Result out{};
    if(!ops.count || !ops.get || !ops.predictable ||
       !ops.shutdown_predictable || !ops.shutdown_client_created)
        return out;
    const auto count=ops.count(ops.user);
    // Original code takes Count()-1 and tests signed <0; a
    // negative count therefore does no work. The native model
    // refuses a negative or oversized fake result explicitly.
    if(count<0 || count>static_cast<std::int32_t>(
       kSyntheticMaxSafeEntities)) {
        out.status=Status::UnsafeCount;
        return out;
    }
    out.status=Status::Complete;
    for(std::int32_t i=count;i>0;) {
        --i;
        void* const entity=ops.get(ops.user,i);
        ++out.entity_gets;
        if(!entity) {
            ++out.null_entities;
            continue;
        }
        if(ops.predictable(ops.user,entity)) {
            ops.shutdown_predictable(ops.user,entity);
            ++out.predictable_shutdowns;
        } else {
            ops.shutdown_client_created(ops.user,entity);
            ++out.client_created_shutdowns;
        }
    }
    if(out.predictable_shutdowns!=0u ||
       out.client_created_shutdowns!=0u) {
        if(ops.print_summary) {
            // original C varargs PUSH client_created count, PUSH
            // predictable count, PUSH format; visible format order
            // is predictable count FIRST, client-created SECOND.
            ops.print_summary(ops.user,out.predictable_shutdowns,
                              out.client_created_shutdowns);
            out.printed_summary=true;
        }
    }
    return out;
}
} // namespace original_prediction_disabled_shutdown
} // namespace sin_independent
