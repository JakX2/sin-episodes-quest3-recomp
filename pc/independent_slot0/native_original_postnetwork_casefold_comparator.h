// Phase1681: original-free MSVC x86 reconstruction of the native
// PostNetwork record-name comparison called by 0x24197ED0.
//
// Original retail 0x2425F81B: complete 67B CDECL two-pointer RET0.
// Reads global DWORD at 0x243895D0; when zero, TAIL-JMPs to
// 0x24261C60, 78B ASCII-fast casefold compare. When nonzero,
// invokes separate 0x2424C373 char conversion for EACH BYTE,
// subtracts mapped SECOND minus mapped FIRST and returns.
// The real locale mapping/ctype tables are engine-CRT-dependent;
// this test supplies an explicitly named invented mapping callback.
// Source uses two cdecl stack args and has NO pointer validity
// checks. Only explicit host-owned strings may be dereferenced.
#pragma once
#include "native_original_postnetwork_record_name_lookup.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sin_independent {
namespace original_postnetwork_casefold_comparator {
constexpr std::uintptr_t kOriginalComparatorVa=0x2425F81Bu;
constexpr unsigned kOriginalComparatorBytes=67u;
constexpr std::uintptr_t kOriginalAsciiFastVa=0x24261C60u;
constexpr unsigned kOriginalAsciiFastBytes=78u;
constexpr std::uintptr_t kOriginalLocaleFlagVa=0x243895D0u;
constexpr std::uintptr_t kOriginalCharCasefoldVa=0x2424C373u;
constexpr unsigned kOriginalNativeArguments=2u;
constexpr unsigned kOriginalCallerCleanup=8u;
constexpr unsigned kOriginalRetPop=0u;
constexpr unsigned kSafeMaxStrings=32u;
constexpr unsigned kSafeMaxStringBytes=128u;
static_assert(sizeof(void*)==4u &&
              kOriginalNativeArguments*4u==kOriginalCallerCleanup &&
              original_postnetwork_record_name_lookup::
                 kOriginalNameCompareVa==kOriginalComparatorVa,
              "native retail cdecl name comparison dependency drift");

struct Context {
    std::array<const char*,kSafeMaxStrings> authorized{};
    unsigned authorized_count=0u;
    bool locale_flag=false;
    void* locale_user=nullptr;
    // This is the still-opaque native 0x2424C373 path.
    // Returns a synthetic mapped 32-bit character value.
    std::int32_t (*lower_in_locale)(
        void*,std::uint8_t) noexcept=nullptr;
    unsigned attempted=0u;
    unsigned ascii_calls=0u;
    unsigned locale_calls=0u;
    unsigned locale_char_map_calls=0u;
    unsigned refused_unowned=0u;
    unsigned refused_unterminated=0u;
    unsigned refused_missing_locale=0u;
    bool authorize(const char* x) noexcept {
        if(!x || authorized_count>=authorized.size())return false;
        authorized[authorized_count++]=x;
        return true;
    }
    bool owns(const char* x) const noexcept {
        for(unsigned i=0u;i<authorized_count;++i)
            if(authorized[i]==x)return true;
        return false;
    }
    // Host-only bounded read, not a purported native original guard.
    bool terminated(const char* x) const noexcept {
        for(unsigned i=0u;i<kSafeMaxStringBytes;++i)
            if(x[i]=='\0')return true;
        return false;
    }
};
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
inline unsigned ascii_fold(unsigned c) noexcept {
    return c>=static_cast<unsigned>('A') &&
           c<=static_cast<unsigned>('Z') ? c+32u:c;
}
struct NativeCompare {
    // One compiled real MSVC x86 CDECL 2-stack-DWORD RET0 function.
    static __declspec(noinline) std::int32_t __cdecl Compare(
        const char* first,const char* second) noexcept {
        auto* ctx=active_context;
        if(!ctx)return 1;
        ++ctx->attempted;
        if(!ctx->owns(first) || !ctx->owns(second)) {
            ++ctx->refused_unowned;
            return 1;
        }
        if(!ctx->terminated(first) || !ctx->terminated(second)) {
            ++ctx->refused_unterminated;
            return 1;
        }
        if(!ctx->locale_flag) {
            ++ctx->ascii_calls;
            for(unsigned i=0u;i<kSafeMaxStringBytes;++i) {
                unsigned a=static_cast<unsigned char>(first[i]);
                unsigned b=static_cast<unsigned char>(second[i]);
                // Original ASCII helper returns after first
                // casefolded mismatch; fast method returns sign
                // {-1,0,1}, not arithmetic difference.
                if(a!=b) {
                    a=ascii_fold(a);
                    b=ascii_fold(b);
                    if(a!=b)return a>b?1:-1;
                }
                if(a==0u)return 0;
            }
            return 1; // unreachable with bounded NUL grant
        }
        if(!ctx->lower_in_locale) {
            ++ctx->refused_missing_locale;
            return 1;
        }
        ++ctx->locale_calls;
        for(unsigned i=0u;i<kSafeMaxStringBytes;++i) {
            // Original slow branch maps SECOND input first
            // (ESI), then FIRST input (EDI), and returns
            // mapped_SECOND - mapped_FIRST as 32-bit EAX.
            const auto mapped_second=ctx->lower_in_locale(
                ctx->locale_user,
                static_cast<std::uint8_t>(second[i]));
            ++ctx->locale_char_map_calls;
            const auto mapped_first=ctx->lower_in_locale(
                ctx->locale_user,
                static_cast<std::uint8_t>(first[i]));
            ++ctx->locale_char_map_calls;
            if(mapped_second==0 ||
               mapped_second!=mapped_first) {
                // Both inputs are host-synthetic int32 values.
                // Two's-complement wrapping mirrors native SUB EAX.
                const std::uint32_t bits=
                    static_cast<std::uint32_t>(mapped_second)-
                    static_cast<std::uint32_t>(mapped_first);
                std::int32_t result{};
                std::memcpy(&result,&bits,sizeof(result));
                return result;
            }
        }
        return 1; // unreachable with bounded NUL grant
    }
};
// Adapter to Phase1679 comparator callback. The actual generated
// Compare method above is still directly invoked by this wrapper.
inline int CompareForFinder(void*,const char* first,
                           const char* second) noexcept {
    return NativeCompare::Compare(first,second);
}
} // namespace original_postnetwork_casefold_comparator
} // namespace sin_independent
