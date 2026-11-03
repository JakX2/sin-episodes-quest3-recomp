#include "sin_compat/phase100_sha256.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string_view>
using namespace sin_compat::phase100;
static bool digest_equals(Digest256 d,std::string_view h){
    static constexpr char alphabet[]="0123456789abcdef";
    if(h.size()!=64)return false;
    for(unsigned i=0;i<32;++i){
       if(h[i*2]!=alphabet[d[i]>>4] || h[i*2+1]!=alphabet[d[i]&15])return false;
    }
    return true;
}
int main(){
    const auto* empty=reinterpret_cast<const std::uint8_t*>("");
    assert(digest_equals(sha256(empty,0),
      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    const auto* abc=reinterpret_cast<const std::uint8_t*>("abc");
    assert(digest_equals(sha256(abc,3),
      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    const auto* quick=reinterpret_cast<const std::uint8_t*>("The quick brown fox jumps over the lazy dog");
    assert(digest_equals(sha256(quick,43),
      "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"));
    assert(digest_equals(original_keymap_sha,
      "6c743e453ce5d6b91214fa2db2fa6b935d7b1ca886c34ebda9538f1ab80a10b0"));
    std::uint8_t input[1024]{};
    assert(digest_equals(sha256(input,sizeof input),
      "5f70bf18a086007016e948b04aed3b82103a36bea41755b6cddfaf10ace3c6ef"));
    input[40]=1;
    assert(sha256(input,sizeof input)!=original_keymap_sha);
}
