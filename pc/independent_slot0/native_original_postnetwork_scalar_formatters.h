// Phase1675: original-free SOURCE-BOUNDED PostNetwork typed diagnostics.
//
// Original two 2006 PE32 methods:
// 0x24198360, 30B native, __thiscall four-DWORD RET16:
//    if (this+0x34 == this+0x18) call opaque 0x24197FE0(
//      this,"binary (%i bytes)",second_arg_original_DWORD).
// 0x24198380, 33B native, same four-DWORD RET16:
//    if (this+0x34 == this+0x18) call opaque 0x24197FE0(
//      this,"short (%i)",signextend_int16(*second_arg_pointer)).
//
// These are only conditional DIAGNOSTIC FORMATTER GATES, not byte
// comparison, retail logging, or gameplay. Downstream 0x24197FE0
// is an injected invented callback. All owner memory is synthetic.
// The original dereferences unknown arg2 pointers for SHORT; this
// safe fixture requires explicit trusted short-pointer authority.
#pragma once
#include "native_original_postnetwork_type_switch.h"
#include <cstddef>
#include <cstdint>

namespace sin_independent {
namespace original_postnetwork_scalar_formatters {

constexpr std::uintptr_t kOriginalBinaryVa=0x24198360u;
constexpr std::uintptr_t kOriginalShortVa=0x24198380u;
constexpr std::uintptr_t kOriginalOpaqueDownstreamVa=0x24197FE0u;
constexpr unsigned kOriginalBinaryBytes=30u;
constexpr unsigned kOriginalShortBytes=33u;
constexpr unsigned kOriginalArgumentDwords=4u;
constexpr unsigned kOriginalRetPopBytes=16u;
constexpr unsigned kSourceCurrentRecordOffset=0x18u;
constexpr unsigned kSourceSelectedRecordOffset=0x34u;
constexpr char kOriginalBinaryFormat[]="binary (%i bytes)";
constexpr char kOriginalShortFormat[]="short (%i)";
static_assert(kOriginalArgumentDwords*4u==kOriginalRetPopBytes &&
              offsetof(original_postnetwork_type_switch::Fixture,
                       current_record)==kSourceCurrentRecordOffset &&
              offsetof(original_postnetwork_type_switch::Fixture,
                       diagnostic_record)==kSourceSelectedRecordOffset,
              "native original formatter four-arg RET16 and member offsets drift");

enum class FormatKind : unsigned {
    Binary,
    Short,
};
struct FormatOps {
    void* user=nullptr;
    void (*on_format)(void*,void*,FormatKind,
                      const char*,std::int32_t) noexcept=nullptr;
};
struct FormatEvents {
    unsigned candidate_binary_calls=0u;
    unsigned candidate_short_calls=0u;
    unsigned record_mismatch_skips=0u;
    unsigned missing_callback_refusals=0u;
    unsigned unowned_short_refusals=0u;
    unsigned accepted_binary_calls=0u;
    unsigned accepted_short_calls=0u;
};
struct Fixture : original_postnetwork_type_switch::Fixture {
    // All host callbacks lie BEYOND the source-attested this+0x34.
    FormatOps format_ops{};
    const std::int16_t* owned_short_value=nullptr;
    FormatEvents format_events{};

    void authorize_short(const std::int16_t* input) noexcept {
        owned_short_value=input;
    }

    // Original code accepts four DWORDs. Only arg2 affects the
    // observed native method's formatter input. The others have
    // source-authenticated stack footprints, not invented effects.
    __declspec(noinline) void __thiscall FormatBinaryIfSelected(
            std::uint32_t arg1,std::uint32_t binary_length,
            std::uint32_t arg3,std::uint32_t arg4) noexcept {
        (void)arg1;(void)arg3;(void)arg4;
        ++format_events.candidate_binary_calls;
        if(diagnostic_record!=current_record) {
            ++format_events.record_mismatch_skips;
            return;
        }
        if(!format_ops.on_format) {
            ++format_events.missing_callback_refusals;
            return;
        }
        ++format_events.accepted_binary_calls;
        format_ops.on_format(format_ops.user,this,FormatKind::Binary,
                             kOriginalBinaryFormat,
                             static_cast<std::int32_t>(binary_length));
    }

    __declspec(noinline) void __thiscall FormatShortIfSelected(
            std::uint32_t arg1,std::uint32_t short_ptr,
            std::uint32_t arg3,std::uint32_t arg4) noexcept {
        (void)arg1;(void)arg3;(void)arg4;
        ++format_events.candidate_short_calls;
        if(diagnostic_record!=current_record) {
            ++format_events.record_mismatch_skips;
            return;
        }
        if(!format_ops.on_format) {
            ++format_events.missing_callback_refusals;
            return;
        }
        if(!owned_short_value ||
           short_ptr!=static_cast<std::uint32_t>(
                 reinterpret_cast<std::uintptr_t>(
                     owned_short_value))) {
            ++format_events.unowned_short_refusals;
            return;
        }
        // The ORIGINAL source is MOVSX EAX, WORD PTR [EDX]
        // and passes exactly that SIGNED 16-bit value to formatter.
        const std::int32_t value=
            static_cast<std::int32_t>(*owned_short_value);
        ++format_events.accepted_short_calls;
        format_ops.on_format(format_ops.user,this,FormatKind::Short,
                             kOriginalShortFormat,value);
    }
};
static_assert(sizeof(void*)==4u,
              "original 2006 PE32 formatter only tested on Win32 x86");

} // namespace original_postnetwork_scalar_formatters
} // namespace sin_independent
