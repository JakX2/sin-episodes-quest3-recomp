#include "sin_compat/original_client_string_token_compare.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <string_view>
using namespace sin_compat::original_client_token_compare;
int main() {
    assert(sizeof(OriginalKeyDescriptor)==4);
    assert(uses_inline_string({0xffff,0xffff}));
    assert(!uses_inline_string({0xffff,1}));
    assert(!uses_inline_string({1,0xffff}));
    assert(constructor_installs_callback(false));
    assert(!constructor_installs_callback(true));
    assert(constructor_selection(false)==ComparatorKind::BytewiseCaseSensitive);
    assert(constructor_selection(true)==ComparatorKind::CaseFolded);
    assert(!less_unsigned_c_string("", ""));
    assert(less_unsigned_c_string("", "A"));
    assert(!less_unsigned_c_string("A", ""));
    assert(less_unsigned_c_string("Alpha", "alpha")); // original does not fold
    assert(!less_unsigned_c_string("alpha", "Alpha"));
    assert(less_unsigned_c_string("a", "ab"));
    assert(less_unsigned_c_string("abc", "abd"));
    assert(!less_unsigned_c_string("abc", "abc"));
    assert(!less_unsigned_c_string(std::string_view("abc\0z",5),"abc"));
    assert(less_unsigned_c_string("\x7f", "\x80")); // unsigned byte ordering
    assert(!less_ascii_folded_c_string("HELLO","hello"));
    assert(!less_ascii_folded_c_string("hello","HELLO"));
    assert(less_ascii_folded_c_string("a", "AB"));
    assert(less_ascii_folded_c_string("Az", "BA"));
    assert(!less_ascii_folded_c_string("A\0z", "a"));
    assert(!less_ascii_folded_c_string("AZ", "az"));
    assert(less_ascii_folded_c_string("ABC", "abd"));
    assert(ascii_fold('A')=='a' && ascii_fold('Z')=='z');
    assert(ascii_fold('@')=='@' && ascii_fold('[')=='[');
    assert(ascii_fold(static_cast<std::uint8_t>(0xc0))==0xc0);
    std::puts("PASS original client token comparator selection, unsigned C-string and ASCII fallback references");
}