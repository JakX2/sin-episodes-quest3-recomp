#pragma once
// Original 2006 client.dll CGameStringPool semantic reconstruction.
// Source: 0x2413D250 (allocate nonempty), 0x2413D270 (lookup),
// 0x241C71D0 (lookup or owned copy), 0x241C7010 (lookup only),
// 0x241C5EB0 (case-insensitive comparison), and native slot5
// 0x2413D2F0 -> 0x241C7050 (free strings, reset tree).
// The source tree has 16-bit indices, 0xffff invalid, and 12-byte
// nodes. This is a portable owned implementation of the observable
// string/pointer behavior, NOT the original PE32 tree memory ABI.
// No original executable code or data is linked.
#include <cstddef>
#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sin_compat::original_client_game_string_pool {

inline constexpr std::size_t kInvalidIndex=0xffffu;
inline constexpr std::size_t kMaximumUniqueStrings=kInvalidIndex;
inline constexpr std::size_t kOriginalInitialNodes=256u;
inline constexpr std::size_t kOriginalGrowNodes=32u;
inline constexpr std::size_t kOriginalNodeBytes=12u;

// Source 0x2425F81B takes a locale-sensitive path when its original
// CRT locale flag is set; default path 0x24261C60 folds ASCII A-Z
// only, compares unsigned bytes and treats '\0' as the terminator.
// Implement the default C-locale behavior deterministically on
// Windows, Linux and Android. A different original active locale
// has not been reconstructed; do not silently use host locale.
struct AsciiCaseLess {
    using is_transparent=void;
    static constexpr unsigned char Fold(unsigned char c) noexcept {
        return c>='A' && c<='Z' ? static_cast<unsigned char>(c+32u) : c;
    }
    bool operator()(std::string_view lhs,std::string_view rhs) const noexcept {
        const auto n=lhs.size()<rhs.size()?lhs.size():rhs.size();
        for(std::size_t i=0;i<n;++i) {
            const auto a=Fold(static_cast<unsigned char>(lhs[i]));
            const auto b=Fold(static_cast<unsigned char>(rhs[i]));
            if(a!=b)return a<b;
        }
        return lhs.size()<rhs.size();
    }
};

class Pool final {
public:
    Pool()=default;
    Pool(const Pool&)=delete;
    Pool& operator=(const Pool&)=delete;
    Pool(Pool&&)=delete;
    Pool& operator=(Pool&&)=delete;

    // Original allocate wrapper rejects the empty C-string. An existing
    // case-insensitive key returns its original allocated pointer, not
    // an alias of the input nor a new allocation. Node-based ownership
    // ensures returned C-string addresses remain stable across inserts.
    // All pointers are invalidated by Clear; never dereference old ones.
    // Null input, embedded NUL views and capacity/allocation exhaustion
    // are explicit independent safety refusals, not original crash ABI.
    const char* Intern(std::string_view value) noexcept {
        if(!Valid(value))return nullptr;
        const auto old=values_.find(value);
        if(old!=values_.end())return old->c_str();
        if(values_.size()>=kMaximumUniqueStrings)return nullptr;
        try {
            return values_.emplace(value).first->c_str();
        } catch(const std::bad_alloc&) {
            return nullptr;
        } catch(const std::length_error&) {
            return nullptr;
        }
    }
    const char* Intern(const char* value) noexcept {
        return value?Intern(std::string_view(value)):nullptr;
    }

    // Original 0x2413D270 -> 0x241C7010 never inserts a missing key.
    const char* Find(std::string_view value) const noexcept {
        if(!Valid(value))return nullptr;
        const auto found=values_.find(value);
        return found==values_.end()?nullptr:found->c_str();
    }
    const char* Find(const char* value) const noexcept {
        return value?Find(std::string_view(value)):nullptr;
    }

    // Original virtual slot5 adjusts this+4 and clears its owned
    // string tree. The original retains node-array storage/free list;
    // STL node storage is released here. No raw tree layout or allocator
    // parity is claimed, only observable string ownership and lookup.
    void Clear() noexcept {values_.clear();}
    std::size_t Size() const noexcept {return values_.size();}

private:
    static bool Valid(std::string_view value) noexcept {
        return !value.empty() && value.find('\0')==std::string_view::npos;
    }
    std::set<std::string,AsciiCaseLess> values_;
};

} // namespace sin_compat::original_client_game_string_pool
