// Phase1678: ORIGINAL-FREE reconstruction of the complete native
// 93-byte 2006 PE32 cdecl diagnostic formatter at 0x24197FE0.
// Exact retail source: allocate 4096 scratch, call imported
// vstdlib!Q_vsnprintf(buffer,4096,format,va_list), then imported
// tier0!Msg("%i %s %s : %s\n",global+0x18,this+0x38,
//           record[this+0x18]+0x04,buffer), clean stack, plain RET.
// All real original pointers are replaced with explicit authorizations
// and owned synthetic strings. Runtime std::snprintf is deliberately
// NOT claimed to be the original vstdlib implementation.
#pragma once
#include "native_original_postnetwork_scalar_formatters.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace sin_independent {
namespace original_postnetwork_cdecl_msg_formatter {

namespace sw=original_postnetwork_type_switch;
namespace fm=original_postnetwork_scalar_formatters;

constexpr std::uintptr_t kOriginalMethodVa=0x24197FE0u;
constexpr unsigned kOriginalNativeBytes=93u;
constexpr unsigned kOriginalScratchBytes=4096u;
constexpr unsigned kOriginalRetPopBytes=0u;
constexpr unsigned kOriginalCdeclStackArguments=3u;
constexpr unsigned kOriginalCallerCleanupBytes=12u;
constexpr unsigned kOriginalCurrentRecordOffset=0x18u;
constexpr unsigned kOriginalRecordNameOffset=0x04u;
constexpr unsigned kOriginalDiagnosticLabelOffset=0x38u;
constexpr unsigned kOriginalGlobalNumberOffset=0x18u;
constexpr char kOriginalMsgFormat[]="%i %s %s : %s\n";
static_assert(sizeof(void*)==4u &&
              kOriginalCdeclStackArguments*4u==
                  kOriginalCallerCleanupBytes &&
              offsetof(sw::Fixture,current_record)==
                  kOriginalCurrentRecordOffset &&
              offsetof(sw::Fixture,diagnostic_label_word)==
                  kOriginalDiagnosticLabelOffset &&
              fm::kOriginalOpaqueDownstreamVa==kOriginalMethodVa,
              "retail diagnostic cdecl ABI or original label layout drift");

inline std::uint32_t rawptr(const void* ptr) noexcept {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(ptr));
}
struct OutputOps {
    void* user=nullptr;
    void (*on_msg)(void*,const char*) noexcept=nullptr;
};
struct OutputEvents {
    unsigned candidate=0u;
    unsigned emitted=0u;
    unsigned refused_unowned_record=0u;
    unsigned refused_unowned_label=0u;
    unsigned refused_bad_format=0u;
    unsigned refused_missing_output=0u;
    unsigned refused_oversized_string=0u;
};
struct Fixture : fm::Fixture {
    // All following host-only data lies AFTER original source +0x38.
    sw::Record* owned_record=nullptr;
    const char* owned_record_name=nullptr;
    const char* owned_label=nullptr;
    std::int32_t global_number=0;
    OutputOps output_ops{};
    OutputEvents output_events{};

    void authorize(sw::Record* record,const char* record_name,
                   const char* label,std::int32_t source_global_number)
                   noexcept {
        owned_record=record;
        owned_record_name=record_name;
        owned_label=label;
        global_number=source_global_number;
    }

    // The retail function is a plain CDECL (3 stack DWORDs at the
    // observed scalar caller sites) and returns with RET, not RET12.
    // Compiler-generated native x86 code is executed in the test.
    // The original function accepts varargs beyond the known one-I32
    // formats, but the original-free fixture intentionally supports
    // ONLY the two source-pinned, single-integer diagnostic literals.
    static __declspec(noinline) void __cdecl FormatScalar(
            Fixture* owner,const char* format,std::int32_t value) noexcept {
        if(!owner)return;
        ++owner->output_events.candidate;
        if(!owner->owned_record ||
           owner->current_record!=rawptr(owner->owned_record) ||
           owner->owned_record->read32(kOriginalRecordNameOffset)!=
                     rawptr(owner->owned_record_name) ||
           !owner->owned_record_name) {
            ++owner->output_events.refused_unowned_record;return;
        }
        if(!owner->owned_label ||
           owner->diagnostic_label_word!=rawptr(owner->owned_label)) {
            ++owner->output_events.refused_unowned_label;return;
        }
        if(format!=fm::kOriginalBinaryFormat &&
           format!=fm::kOriginalShortFormat) {
            ++owner->output_events.refused_bad_format;return;
        }
        if(!owner->output_ops.on_msg) {
            ++owner->output_events.refused_missing_output;return;
        }
        // The original has a 4096-byte stack buffer. Use bounded
        // host printing with pre-authorized literal formats only.
        std::array<char,kOriginalScratchBytes> scratch{};
        const int n=std::snprintf(
            scratch.data(),scratch.size(),format,value);
        if(n<0 || static_cast<std::size_t>(n)>=scratch.size()) {
            ++owner->output_events.refused_oversized_string;return;
        }
        // Names are synthetic host-authorized C strings; bound the
        // reads before formatting, not arbitrary 2006 game pointers.
        auto bounded_string=[](const char* x) noexcept {
            for(unsigned i=0u;i<128u;++i)
                if(x[i]=='\0')return true;
            return false;
        };
        if(!bounded_string(owner->owned_label) ||
           !bounded_string(owner->owned_record_name)) {
            ++owner->output_events.refused_oversized_string;return;
        }
        std::array<char,kOriginalScratchBytes+512u> line{};
        const int out=std::snprintf(
            line.data(),line.size(),kOriginalMsgFormat,
            owner->global_number,owner->owned_label,
            owner->owned_record_name,scratch.data());
        if(out<0 || static_cast<std::size_t>(out)>=line.size()) {
            ++owner->output_events.refused_oversized_string;return;
        }
        ++owner->output_events.emitted;
        owner->output_ops.on_msg(owner->output_ops.user,line.data());
    }
};
static_assert(offsetof(sw::Record,raw)==0u &&
              sizeof(sw::Record)==0x34u,
              "original 2006 0x34 record layout drift");

} // namespace original_postnetwork_cdecl_msg_formatter
} // namespace sin_independent
