#pragma once
// Phase225: independent portable C++17 reconstruction of the THREE
// original 2006 SiN client.dll shared serializer TRACE wrappers.
// Original Q_vsnprintf, tier0 Msg, game callback and engine object layout
// remain external: this is NOT a ready-to-load gameplay client.dll.
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace sin_compat::original_client_shared_trace {
inline constexpr std::uint32_t kDifferenceVA=0x24197910u;
inline constexpr std::uint32_t kStatusVA=0x241979C0u;
inline constexpr std::uint32_t kPreviewVA=0x24197FE0u;
inline constexpr std::uint32_t kOriginalQvsnprintfIAT=0x2427625Cu;
inline constexpr std::uint32_t kOriginalTier0MsgIAT=0x242761ECu;
inline constexpr std::uint32_t kOriginalTypeNamesVA=0x242FDAE0u;
inline constexpr std::uint32_t kOriginalPreviewGlobalVA=0x242FA458u;
inline constexpr std::uint32_t kOriginalStackProbeVA=0x242493E0u;
inline constexpr std::size_t kOriginalFormatBufferBytes=4096u;

inline constexpr std::array<const char*,26> kOriginalTypeNames{{
    "FIELD_VOID","FIELD_FLOAT","FIELD_STRING","FIELD_VECTOR",
    "FIELD_QUATERNION","FIELD_INTEGER","FIELD_BOOLEAN","FIELD_SHORT",
    "FIELD_CHARACTER","FIELD_COLOR32","FIELD_EMBEDDED","FIELD_CUSTOM",
    "FIELD_CLASSPTR","FIELD_EHANDLE","FIELD_EDICT",
    "FIELD_POSITION_VECTOR","FIELD_TIME","FIELD_TICK",
    "FIELD_MODELNAME","FIELD_SOUNDNAME","FIELD_INPUT",
    "FIELD_FUNCTION","FIELD_VMATRIX","FIELD_VMATRIX_WORLDSPACE",
    "FIELD_MATRIX3X4_WORLDSPACE","FIELD_INTERVALFIELD_MODELINDEX"
}};
inline const char* original_type_name(std::uint32_t type) {
    if(type>=kOriginalTypeNames.size())
        throw std::out_of_range("host-only unknown original field type table index");
    return kOriginalTypeNames[type];
}
struct Descriptor {
    std::uint32_t type_00{};
    std::optional<std::string> name_pointer_04; // null original pointer -> "NULL"
    std::uint16_t flags_12{};
};
struct TraceObject {
    // Logical fields correspond ONLY to original offsets and flags; not
    // the original complete class, allocator or real pointer identity.
    std::optional<Descriptor> descriptor_18;
    std::string group_string_1c;
    bool difference_pending_24{};
    bool status_pending_25{};
    bool suppress_difference_if_callback_16{};
    std::uint32_t difference_count_28{};
    bool callback_pointer_30_nonnull{};
    std::string preview_label_38;
};

enum class MessageKind { FirstDifferenceNewline, DifferenceLine, PreviewLine };
struct MessageEvent {
    MessageKind kind{};
    std::string original_format;
    std::uint32_t counter_or_preview_number{};
    std::string group_or_label;
    std::string descriptor_name;
    std::string message;
};
struct StatusEvent {
    std::string original_formatted_message;
    std::string group;
    std::string field_name;
    std::string original_field_type;
    bool descriptor_flag_08{};
    bool descriptor_flag_10{};
    bool operation_not_equal_one{};
    bool operation_equals_two{};
};
struct ITraceHost {
    virtual ~ITraceHost()=default;
    // Caller supplies raw original i386 varargs as 32-bit stack words.
    // The real vstdlib.dll!Q_vsnprintf implementation/ABI and %f/%s
    // pointer lifetimes are NOT reconstructed; the host owns conversion.
    virtual std::string q_vsnprintf(const std::string& format,
                                   const std::vector<std::uint32_t>& words)=0;
    virtual void msg(const MessageEvent& event)=0;
    virtual void status_callback(const StatusEvent& event)=0;
    // Native preview pulls DWORD [DWORD 0x242FA458 + 0x18].
    virtual std::int32_t original_preview_global_number()=0;
};
inline std::string host_limited_formatted_text(
        ITraceHost& host,const std::string& format,
        const std::vector<std::uint32_t>& words) {
    std::string result=host.q_vsnprintf(format,words);
    // Host-only conservative bound for 4096-byte native scratch buffer:
    // actual Q_vsnprintf error/truncation return is NOT emulated.
    if(result.size()>=kOriginalFormatBufferBytes)
        result.resize(kOriginalFormatBufferBytes-1u);
    return result;
}
inline std::string field_name(const TraceObject& state) {
    if(!state.descriptor_18)return "empty";
    return state.descriptor_18->name_pointer_04.value_or("NULL");
}

// Original 166-byte C3-return varargs wrapper 0x24197910.
// Increments DWORD(this+0x28) UNCONDITIONALLY, even when disabled.
// Only if BYTE(this+0x24)!=0, and not (this+0x16 && this+0x30),
// formats through Q_vsnprintf 4096. On first difference prints "\n"
// via tier0 Msg; then Msg("%03i %s::%s - %s"), and clears +0x24.
// Disabled/suppressed branches retain +0x24 for possible future calls.
inline void difference(TraceObject& state,const std::string& format,
        const std::vector<std::uint32_t>& raw_varargs,ITraceHost& host) {
    ++state.difference_count_28; // Native i386 32-bit wrapping ADD.
    if(!state.difference_pending_24)return;
    if(state.suppress_difference_if_callback_16 &&
       state.callback_pointer_30_nonnull)return;
    const auto name=field_name(state);
    const auto message=host_limited_formatted_text(host,format,raw_varargs);
    if(state.difference_count_28==1u)
        host.msg({MessageKind::FirstDifferenceNewline,"\n",0u,{},{},{}});
    host.msg({MessageKind::DifferenceLine,"%03i %s::%s - %s",
              state.difference_count_28,state.group_string_1c,
              name,message});
    state.difference_pending_24=false;
}

// Original 185-byte C3-return wrapper 0x241979C0.
// Enabled only when BYTE(this+0x25)!=0 AND DWORD(this+0x30)!=0.
// Uses descriptor type -> original 26-name FIELD_* table and short
// flags at descriptor+0x12. Callback at native +0x30 receives:
// (this+0x1C, field name, type name, flag8, flag10,
//  operation!=1, operation==2, formatted_message).
// Clears BYTE(this+0x25) AFTER the callback.
inline void status(TraceObject& state,std::uint32_t operation,
        const std::string& format,
        const std::vector<std::uint32_t>& raw_varargs,ITraceHost& host) {
    if(!state.status_pending_25 || !state.callback_pointer_30_nonnull)return;
    const auto name=field_name(state);
    const auto message=host_limited_formatted_text(host,format,raw_varargs);
    if(!state.descriptor_18)
        throw std::runtime_error("host-only original callback descriptor null dereference refusal");
    const auto& descriptor=*state.descriptor_18;
    host.status_callback({
        message,state.group_string_1c,name,
        original_type_name(descriptor.type_00),
        (descriptor.flags_12 & 0x0100u)!=0u,
        (descriptor.flags_12 & 0x0400u)!=0u,
        operation!=1u,operation==2u
    });
    state.status_pending_25=false;
}

// Original 93-byte C3-return wrapper 0x24197FE0, no enable gate.
// Always Q_vsnprintf into a native 4096-byte stack buffer followed by
// Msg("%i %s %s : %s\n", global+0x18, this+0x38,
// descriptor+0x04, formatted_message).
// Original descriptor is DEREFERENCED without a null guard. Host-only
// explicit refusal avoids inventing a name for that invalid case.
inline void preview(const TraceObject& state,const std::string& format,
        const std::vector<std::uint32_t>& raw_varargs,ITraceHost& host) {
    const auto message=host_limited_formatted_text(host,format,raw_varargs);
    if(!state.descriptor_18 || !state.descriptor_18->name_pointer_04)
        throw std::runtime_error("host-only original preview descriptor/name null pointer");
    host.msg({MessageKind::PreviewLine,"%i %s %s : %s\n",
              static_cast<std::uint32_t>(host.original_preview_global_number()),
              state.preview_label_38,*state.descriptor_18->name_pointer_04,message});
}
} // namespace sin_compat::original_client_shared_trace
