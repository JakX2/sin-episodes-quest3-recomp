#pragma once
#include "weapon_network_contract.h"
#include <cassert>
#include <cstring>
namespace sin_port::weapon_net::test {
inline std::uint32_t float_bits(float f){std::uint32_t u=0;std::memcpy(&u,&f,4);return u;}
inline Values example(){
    Values v{};
    v.words[Clip1]=45;v.words[Clip2]=0xffffffffu;v.words[PrimaryAmmo]=2;
    v.words[SecondaryAmmo]=0xffffffffu;v.words[ViewModelSlot]=1;
    v.words[NextPrimary]=float_bits(12.5f);v.words[NextSecondary]=float_bits(13.f);
    v.words[NextThinkTick]=0xffffffffu;v.words[IdleTime]=float_bits(14.5f);
    v.words[ViewModelIndex]=250;v.words[WorldModelIndex]=251;v.words[State]=2;
    v.words[Owner]=(0x12345u<<12)|37u;
    return v;
}
inline void check(const Api& server,const Api& client){
    assert(server.version==1&&client.version==1);
    for(unsigned i=0;i<field_count;++i){
        FieldInfo a{},b{};
        assert(server.field(i,&a)&&client.field(i,&b));
        assert(std::memcmp(&a,&b,sizeof(a))==0);
    }
    FieldInfo p{};
    assert(server.field(NextPrimary,&p)&&p.server_offset==0x49c&&p.client_offset==0x790&&p.flags==4);
    assert(!server.field(field_count,&p));
    const auto v=example();
    OwnerObservation o{v.words[Owner],1,1,256};
    RecipientMask recipients{};
    assert(server.recipients(&o,&recipients)&&recipients.words[7]==0x80000000u);
    for(unsigned i=0;i<7;++i)assert(!recipients.words[i]);
    Payload payload{};
    assert(server.encode(&v,&o,256,&payload)&&payload.present==all_mask);
    assert(payload.values.words[Clip1]==46&&payload.values.words[Clip2]==0);
    assert(payload.values.words[SecondaryAmmo]==255&&payload.values.words[Owner]==((0x345u<<11)|37u));
    Values received{};
    assert(client.decode(&payload,1,&received));
    for(unsigned i=0;i<Owner;++i)assert(received.words[i]==v.words[i]);
    assert(received.words[Owner]==((0x345u<<12)|37u));
    assert(!client.decode(&payload,0,&received));
    assert(server.encode(&v,&o,1,&payload)&&payload.present==public_mask);
    for(unsigned i=0;i<9;++i)assert(payload.values.words[i]==0);
    received.words[Clip1]=77;
    assert(client.decode(&payload,0,&received)&&received.words[Clip1]==77);
    auto invalid=payload;invalid.values.words[Clip1]=1;
    assert(!client.decode(&invalid,0,&received)&&received.words[Clip1]==77);
    invalid=payload;invalid.values.words[State]=256;
    assert(!client.decode(&invalid,0,&received));
    auto edge=v;edge.words[Clip1]=254;
    assert(server.encode(&edge,&o,256,&payload)&&payload.values.words[Clip1]==255);
    edge.words[Clip1]=255;assert(!server.encode(&edge,&o,256,&payload)&&payload.values.words[Clip1]==255);
    edge.words[Clip1]=0xfffffffeu;assert(!server.encode(&edge,&o,256,&payload));
    edge=v;edge.words[NextPrimary]=0x7fc00000u;assert(!server.encode(&edge,&o,256,&payload));
    // Nonowner does not validate/read private values.
    assert(server.encode(&edge,&o,1,&payload)&&payload.present==public_mask);
    o.resolved=0;
    assert(server.encode(&v,&o,256,&payload)&&payload.present==public_mask&&payload.values.words[Owner]==wire_invalid_handle);
    assert(client.decode(&payload,0,&received)&&received.words[Owner]==invalid_handle);
    o.resolved=1;o.player_eligible=0;
    assert(server.encode(&v,&o,256,&payload)&&payload.present==public_mask);
    o.player_eligible=1;o.client_slot=257;
    assert(!server.encode(&v,&o,256,&payload));
    edge=v;edge.words[Owner]=0x3fffffu;o={edge.words[Owner],1,1,1};
    // Native entry 4095 cannot be represented in the 11-bit wire entry.
    assert(!server.encode(&edge,&o,1,&payload));
    edge.words[Owner]=(0x3ffu<<12)|0x7ffu;o.native_handle=edge.words[Owner];
    assert(!server.encode(&edge,&o,1,&payload)); // reserved sentinel
}
}
