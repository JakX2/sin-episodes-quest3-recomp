#include "sin_compat/original_server_voice_mask_wire.hpp"
#include "sin_compat/original_voice_mask_wire.hpp"
#include <cassert>
#include <vector>
using namespace sin_compat::original_server_voice_mask;
int main() {
    static_assert(kRegisteredVoiceMaskBytes==9 && kRegisteredRequestStateBytes==0);
    static_assert(kAudibleBits+kBlockedBits+kStatusBits==72);
    Fields f{0x80000001u,0xdeadbeefu,1};
    auto bytes=encode(f);
    assert(bytes.size()==9 && bytes[0]==1 && bytes[3]==0x80);
    assert(bytes[4]==0xef && bytes[5]==0xbe && bytes[6]==0xad && bytes[7]==0xde);
    assert(bytes[8]==1);
    std::vector<std::uint8_t> payload(bytes.begin(),bytes.end());
    auto decoded=sin_compat::original_voice_mask_wire::decode_strict(payload);
    assert(decoded && decoded->audible_players==f.audible);
    assert(decoded->server_banned_players==f.server_banned && decoded->status==f.status);
    auto outgoing=messages_for_recipient(true,false,false,f);
    assert(outgoing.size()==1 && outgoing[0].type==MessageType::RequestState);
    assert(outgoing[0].payload.empty());
    outgoing=messages_for_recipient(false,true,false,f);
    assert(outgoing.size()==1 && outgoing[0].type==MessageType::VoiceMask);
    assert(outgoing[0].payload==payload);
    outgoing=messages_for_recipient(true,false,true,f);
    assert(outgoing.size()==2 && outgoing[0].type==MessageType::RequestState);
    assert(outgoing[0].payload.empty() && outgoing[1].type==MessageType::VoiceMask);
    assert(outgoing[1].payload==payload);
    outgoing=messages_for_recipient(false,false,false,f);
    assert(outgoing.empty());
    f={0xffffffffu,0x80000000u,0};
    bytes=encode(f);
    payload.assign(bytes.begin(),bytes.end());
    decoded=sin_compat::original_voice_mask_wire::decode_strict(payload);
    assert(decoded && decoded->audible_players==0xffffffffu);
    assert(decoded->server_banned_players==0x80000000u && decoded->status==0);
    return 0;
}