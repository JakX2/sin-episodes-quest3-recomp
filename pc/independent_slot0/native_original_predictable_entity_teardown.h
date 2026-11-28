// Phase1667: original-free staged C_BaseEntity predictable cleanup.
//
// Exact original native method 0x240CA9F0, 61 bytes, whole SHA pinned
// in research/tools/audit_sin_original_predictable_entity_teardown.py.
// Its original order is: read entity+0xB4 handle, call registry cleanup,
// call primary cleanup, CLEAR entity+0x2E8 predictable flag, call
// primary vptr +0x44, then pass returned EAX into final helper.
//
// NO real original engine service/registry/entity object or method is
// used in CI. Each opaque original callee is a separately injected
// callback from synthetic host-owned objects and may be absent.
// Missing callback refuses ALL state changes instead of crashing.
#pragma once

#include <cstddef>
#include <cstdint>

namespace sin_independent {
namespace original_predictable_entity_teardown {

constexpr std::uintptr_t kOriginalMethodVa=0x240CA9F0u;
constexpr std::uintptr_t kOriginalHandleMemberOffset=0xB4u;
constexpr std::uintptr_t kOriginalPredictableByteOffset=0x2E8u;
constexpr unsigned kOriginalCallbackVptrOffset=0x44u;
constexpr unsigned kOriginalCallbackVtableSlot=17u;
constexpr unsigned kOriginalNativeStackPopBytes=0u;

struct Ops {
    void* user=nullptr;
    std::uint32_t (*read_handle)(void*,void*) noexcept=nullptr;
    void (*unregister_handle)(void*,void*,std::uint32_t) noexcept=nullptr;
    void (*primary_cleanup)(void*,void*) noexcept=nullptr;
    void (*clear_predictable)(void*,void*) noexcept=nullptr;
    std::uint32_t (*entity_virtual_17)(void*,void*) noexcept=nullptr;
    void (*final_cleanup)(void*,void*,std::uint32_t) noexcept=nullptr;
};
enum class Status {
    MissingCallbacks,
    MissingEntity,
    Complete,
};
struct Result {
    Status status=Status::MissingCallbacks;
    std::uint32_t saved_handle=0u;
    std::uint32_t virtual_return=0u;
    unsigned completed_stages=0u;
    unsigned original_method_invocations=0u;
};
inline Result Run(void* entity,const Ops& ops) noexcept {
    Result out{};
    if(!entity) {
        out.status=Status::MissingEntity;
        return out;
    }
    if(!ops.read_handle || !ops.unregister_handle ||
       !ops.primary_cleanup || !ops.clear_predictable ||
       !ops.entity_virtual_17 || !ops.final_cleanup)
        return out;
    const auto handle=ops.read_handle(ops.user,entity);
    out.saved_handle=handle;
    ops.unregister_handle(ops.user,entity,handle);
    ++out.completed_stages;
    ops.primary_cleanup(ops.user,entity);
    ++out.completed_stages;
    ops.clear_predictable(ops.user,entity);
    ++out.completed_stages;
    // The retail code clears the flag BEFORE virtual dispatch. A
    // virtual callback may observe the entity as nonpredictable.
    const auto token=ops.entity_virtual_17(ops.user,entity);
    out.virtual_return=token;
    ++out.completed_stages;
    ops.final_cleanup(ops.user,entity,token);
    ++out.completed_stages;
    out.status=Status::Complete;
    return out;
}
} // namespace original_predictable_entity_teardown
} // namespace sin_independent
