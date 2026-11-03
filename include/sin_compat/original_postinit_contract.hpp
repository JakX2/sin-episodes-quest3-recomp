#pragma once
// SiN Episodes (2006) post-core Init gate order. Numeric/flow model only.
// Original x86 factory/COM objects and SourceVR ARM64 ABI NOT implemented.
#include <array>
#include <cstddef>
#include <string_view>
namespace sin_compat::postinit {
enum class GateKind { VirtualPointerQuery, DirectBooleanHelper, DirectPointerLookup };
struct Gate {
    std::string_view label;  // Exact original name or neutral original VA.
    GateKind kind;
};
inline constexpr std::array<Gate,7> client_required_late{{
    {"MaterialSystemHardwareConfig012",GateKind::VirtualPointerQuery},
    {"0x24164120",GateKind::DirectBooleanHelper},
    {"0x2417F680",GateKind::DirectBooleanHelper},
    {"0x241EDFB0",GateKind::DirectBooleanHelper},
    {"MatSystemSurface003",GateKind::VirtualPointerQuery},
    {"0x24156860",GateKind::DirectBooleanHelper},
    {"0x24182C60",GateKind::DirectBooleanHelper},
}};
inline constexpr std::array<Gate,4> server_required_late{{
    {"sv_cheats",GateKind::DirectPointerLookup},
    {"0x221F5700",GateKind::DirectBooleanHelper},
    {"0x221AD8A0",GateKind::DirectBooleanHelper},
    {"0x22149B60",GateKind::DirectBooleanHelper},
}};
inline constexpr std::array<std::string_view,2> server_event_resources{{
    "resource/gameevents.res", "resource/ModEvents.res",
}};
inline constexpr std::string_view client_nonfatal_factory="VMaterialSystemStub001";
inline constexpr std::string_view server_nonfatal_factory="VDebugOverlay003";
struct GateResult { bool all_passed; std::size_t first_failed_index; };
// Only models the verified ordered *conditions* after the emitter gate.
// Unmodelled side effects and registration calls occur between conditions.
template<std::size_t N, typename Check>
GateResult check_required_late(const std::array<Gate,N>& gates,Check&& check) {
    for(std::size_t i=0;i<N;++i)
        if(!check(gates[i])) return {false,i};
    return {true,N};
}
// Original server x86 treats nullptr from a 0x510-byte allocation as a
// nonfatal branch to its second successful return path. The allocation is
// distinct from the mandatory gates above.
enum class ServerAllocationBranch { Constructed, NullFallback };
inline ServerAllocationBranch server_allocation_branch(bool allocated) noexcept {
    return allocated ? ServerAllocationBranch::Constructed : ServerAllocationBranch::NullFallback;
}
} // namespace sin_compat::postinit
