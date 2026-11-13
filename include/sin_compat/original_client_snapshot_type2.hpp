#pragma once
// Phase215: clean-room C++17 reference for three original SiN 2006 client.dll
// prediction snapshot type-2 string specialist routines. Original x86 source
// classes and engine ownership remain unknown. No original game code is here.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "sin_compat/original_client_snapshot_field_copy.hpp"

namespace sin_compat::original_client_snapshot_type2 {

inline constexpr std::uint32_t kNativeCompareVA = 0x24198B80u;
inline constexpr std::uint32_t kNativeDiagnosticsVA = 0x24197D00u;
inline constexpr std::uint32_t kNativePreviewVA = 0x241984A0u;
inline constexpr std::uint32_t kCompareCaller = 0x24199883u;
inline constexpr std::uint32_t kDiagnosticsCaller = 0x24199899u;
inline constexpr std::uint32_t kPreviewCaller = 0x241998A3u;

// Independent known FIELD SLICES of the original x86 object. These do not
// establish a full layout, and the +0x18 and +0x34 DWORDs are opaque tokens.
struct OriginalFields {
    std::uint8_t byte_14{}; // Enables equality check and diagnostic events.
    std::uint8_t descriptor_byte_13{}; // Bit 0x04 forces comparison status 1.
    std::uint8_t byte_2c{}; // Phase213 copy-helper gate.
    std::uint32_t dword_18{};
    std::uint32_t dword_34{};
};
using Bytes = std::vector<std::uint8_t>;

inline std::string safe_c_string(const Bytes& bytes, std::size_t offset) {
    // Unlike original unbounded x86 pointer reads, this portable host model
    // requires an in-bounds terminated string.
    if (offset >= bytes.size())
        throw std::out_of_range("host-only original type2 string start bounds");
    auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(offset);
    auto end = std::find(begin, bytes.end(), std::uint8_t{0});
    if (end == bytes.end())
        throw std::runtime_error("host-only original type2 missing NUL");
    return std::string(begin, end);
}

// Original 83-byte x86 method 0x24198B80, native ret 8:
// 0 if byte_14 is zero; 1 if descriptor bit 0x04 set;
// otherwise 1 exactly when the two NUL-terminated byte strings match.
// This method does NOT compare strings case-insensitively.
inline std::uint32_t compare_status(const OriginalFields& fields,
                                    const Bytes& first, std::size_t first_off,
                                    const Bytes& second, std::size_t second_off) {
    if (fields.byte_14 == 0u) return 0u;
    if ((fields.descriptor_byte_13 & 0x04u) != 0u) return 1u;
    return safe_c_string(first, first_off) == safe_c_string(second, second_off)
        ? 1u : 0u;
}

enum class TraceKind { Difference, Status, Preview };
struct TraceEvent {
    TraceKind kind{};
    std::uint32_t operation{};
    std::string net;
    std::string predicted;
};

struct ITraceSink {
    virtual ~ITraceSink() = default;
    virtual void on_event(const TraceEvent& event) = 0;
};

// Original 65-byte x86 method 0x24197D00, native ret 12.
// Format strings in retail DLL: "string differs (net %s pred %s)\\n"
// and "string (%s)\\n". Actual called formatter ownership is opaque;
// preserve branch and call order using a host callback, not engine printf.
inline void diagnostics(const OriginalFields& fields, std::uint32_t operation,
                        const Bytes& predicted, std::size_t predicted_off,
                        const Bytes& net, std::size_t net_off,
                        ITraceSink& sink) {
    if (fields.byte_14 == 0u) return;
    if (operation == 0u) {
        sink.on_event({TraceKind::Difference, operation,
                       safe_c_string(net, net_off),
                       safe_c_string(predicted, predicted_off)});
    }
    sink.on_event({TraceKind::Status, operation, "",
                   safe_c_string(predicted, predicted_off)});
}

// Original 30-byte method 0x241984A0, native ret 12.
// Original-only vtable/state gate: DWORD at this+0x34 == this+0x18.
// Exact original format string "string (%s)".
inline void preview(const OriginalFields& fields,
                    const Bytes& predicted, std::size_t predicted_off,
                    ITraceSink& sink) {
    if (fields.dword_34 != fields.dword_18) return;
    sink.on_event({TraceKind::Preview, 0u, "",
                   safe_c_string(predicted, predicted_off)});
}

// Models the exact direct CALL ordering of the Phase212 type-2 case:
// compare -> Phase213 string copy -> diagnostics -> preview.
// The larger record serializer, ownership, indirect callbacks, and side
// effects are NOT reconstructed. Logging deliberately observes destination
// bytes AFTER the copy call, matching this direct CALL order.
inline std::uint32_t run_direct_type2_calls(
        const OriginalFields& fields, const Bytes& net, std::size_t net_off,
        Bytes& predicted, std::size_t predicted_off, ITraceSink& sink) {
    const auto operation = compare_status(
        fields, predicted, predicted_off, net, net_off);
    original_client_snapshot_field_copy::copy_c_string_type2(
        {fields.byte_2c}, operation, net, net_off, predicted, predicted_off);
    diagnostics(fields, operation, predicted, predicted_off, net, net_off, sink);
    preview(fields, predicted, predicted_off, sink);
    return operation;
}

} // namespace sin_compat::original_client_snapshot_type2
