#include "sin_compat/original_voice_mask_wire.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
using namespace sin_compat::original_voice_mask_wire;
namespace {
void put_bits(std::vector<std::uint8_t>& into,std::size_t offset,std::uint32_t value,std::size_t count) {
    for(std::size_t i=0;i<count;++i){
        if((value>>i)&1u) into[(offset+i)/8] |= static_cast<std::uint8_t>(1u<<((offset+i)%8));
    }
}
}
int main(){
    static_assert(kPayloadBits == 72, "Original VoiceMask fields occupy 72 bits");
    static_assert(kOriginalAudibleMaskOffset==0x14, "original CVoiceStatus field layout");
    static_assert(kOriginalServerBlockedMaskOffset==0x1C, "original CVoiceStatus field layout");
    static_assert(kOriginalStatusOffset==0x08, "original CVoiceStatus field layout");
    const std::uint32_t audible=0xfeedfaceu,blocked=0x80000001u;
    const std::uint8_t status=0xa5;
    std::vector<std::uint8_t> wire(9,0);
    put_bits(wire,0,audible,32);
    put_bits(wire,32,blocked,32);
    put_bits(wire,64,status,8);
    auto parsed=decode_strict(wire);
    assert(parsed && parsed->audible_players==audible);
    assert(parsed->server_banned_players==blocked && parsed->status==status);
    // Native bf_read supports bit positions that do not align to bytes.
    std::vector<std::uint8_t> nonaligned(10,0);
    put_bits(nonaligned,3,audible,32);
    put_bits(nonaligned,35,blocked,32);
    put_bits(nonaligned,67,status,8);
    parsed=decode_strict(nonaligned,3);
    assert(parsed && parsed->audible_players==audible);
    assert(parsed->server_banned_players==blocked && parsed->status==status);
    assert(!decode_strict(nonaligned,9)); // insufficient 72 remaining bits
    assert(!decode_strict(std::vector<std::uint8_t>(8,0)));
    assert(!decode_strict({}));
    BitReader reader(wire);
    std::uint32_t data=0;
    assert(reader.get(32,data) && data==audible);
    assert(reader.get(32,data) && data==blocked);
    assert(reader.get(8,data) && data==status);
    assert(reader.position()==72);
    assert(!reader.get(1,data));
    assert(!reader.get(33,data));
    return 0;
}