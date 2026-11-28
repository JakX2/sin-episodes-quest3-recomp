#pragma once
// Source-authenticated original SiN 2006 client sound-name routing subset.
// Original 0x241BF880 checks Q_stristr(filename, ".wav") then ".mp3";
// only when BOTH miss, it calls 0x241BCC50 (sound emitter virtual chain).
// All resulting branches call 0x2413D250 to INTERN the filename, then
// a second C-string into original this+0x2C.
// Portable owned behavior, NOT byte-identical CSoundPatch layout.
// Original input null semantics, sound emitter ABI, and numeric fields are
// intentionally outside this implementation.
#include "original_client_game_string_pool.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace sin_compat::original_client_sound_name {

using original_client_game_string_pool::AsciiCaseLess;
using original_client_game_string_pool::Pool;

enum class Route {
    InvalidInput,
    LiteralFileToken,
    EmitterResolved,
    EmitterRejected,
};

inline constexpr bool EqualAsciiFold(char a, char b) noexcept {
    return AsciiCaseLess::Fold(static_cast<unsigned char>(a)) ==
           AsciiCaseLess::Fold(static_cast<unsigned char>(b));
}

inline bool ContainsAsciiFold(std::string_view text,
                              std::string_view needle) noexcept {
    if(needle.empty())return true;
    if(text.size()<needle.size())return false;
    for(std::size_t start=0;start<=text.size()-needle.size();++start) {
        bool match=true;
        for(std::size_t k=0;k<needle.size();++k) {
            if(!EqualAsciiFold(text[start+k],needle[k])) {
                match=false;
                break;
            }
        }
        if(match)return true;
    }
    return false;
}

// Original imported Q_stristr checks SUBSTRINGS, never just suffixes.
// The default ASCII casefold is a deliberate portable approximation;
// alternate CRT locale behavior and unbounded C-string reads are excluded.
inline bool HasLiteralFileToken(std::string_view name) noexcept {
    return ContainsAsciiFold(name,".wav") ||
           ContainsAsciiFold(name,".mp3");
}

inline bool SafeCName(std::string_view name) noexcept {
    return !name.empty() && name.find('\0')==std::string_view::npos;
}

// Original parameter helper 0x241BF5F0 starts flags at 1, then invokes
// provider vtable byte +4 followed by +8. It ORs 2 and 4 for true results.
// This reconstructs only the two Boolean-flag transitions, not the
// native provider object or the following variable-sized record array.
template<class QuerySlot4,class QuerySlot8>
std::uint32_t SourceParameterFlags(QuerySlot4&& first,QuerySlot8&& second) {
    std::uint32_t flags=1u;
    if(static_cast<bool>(std::forward<QuerySlot4>(first)()))flags|=2u;
    if(static_cast<bool>(std::forward<QuerySlot8>(second)()))flags|=4u;
    return flags;
}

struct Binding {
    Route route=Route::InvalidInput;
    const char* internedName=nullptr;
    const char* internedAuxiliary=nullptr;
    bool emitterCalled=false;
    bool emitterAccepted=false;
};

// Engine-supplied resolver hook is NEVER called for names containing either
// literal token. On fallback both resolver outcomes still intern the name,
// matching original caller's two source branches. The caller supplies any
// auxiliary C-string returned by its own engine parameter context.
// These pointers belong to Pool, not the source filename; after Clear
// their lifetime ends. Missing/embedded NUL names safely refuse.
template<class Resolver>
Binding Bind(Pool& pool,std::string_view filename,
             std::string_view auxiliary,Resolver&& resolve) {
    Binding out{};
    if(!SafeCName(filename) ||
       auxiliary.find('\0')!=std::string_view::npos)return out;
    const bool direct=HasLiteralFileToken(filename);
    const bool found=direct?false:static_cast<bool>(
        std::forward<Resolver>(resolve)(filename));
    out.emitterCalled=!direct;
    out.emitterAccepted=found;
    out.route=direct?Route::LiteralFileToken:
        (found?Route::EmitterResolved:Route::EmitterRejected);
    out.internedName=pool.Intern(filename);
    if(!auxiliary.empty())out.internedAuxiliary=pool.Intern(auxiliary);
    if(!out.internedName)out.route=Route::InvalidInput;
    return out;
}

} // namespace sin_compat::original_client_sound_name
