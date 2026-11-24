#pragma once
#include "test_weapon_network_contract_core.h"
namespace sin_port::weapon_net::test {
inline void check_inventory(const InventoryApi& server,const InventoryApi& client){
    assert(server.version==1&&client.version==1);
    for(unsigned i=0;i<4;++i){
        InventoryInfo x{},y{};assert(server.field(i,&x)&&client.field(i,&y));
        assert(std::memcmp(&x,&y,sizeof(x))==0);
    }
    InventoryInfo x{};assert(server.field(0,&x)&&x.elements==32&&x.bits==10&&x.server_offset==0x64c);
    assert(server.field(2,&x)&&x.elements==48&&x.bits==21&&x.client_offset==0xc80);
    InventoryValues v{};InventoryHandleObservations handles{};
    v.ammo[0]=1023;v.ammo[31]=7;v.next_attack=float_bits(4.5f);
    v.weapons[0]=(0x12345u<<12)|7u;handles.weapons[0]=1;
    v.weapons[47]=0xffffffffu;v.active_weapon=v.weapons[0];handles.active_weapon=1;
    InventoryRecipients r{1,2};
    InventoryPayload p{};
    assert(server.encode(&v,&handles,&r,1,&p)&&p.present==(InventoryAmmo|InventoryPublic));
    assert(p.values.ammo[31]==7&&p.values.weapons[0]==0&&p.values.next_attack==0);
    InventoryValues out{};out.weapons[0]=99;
    assert(client.decode(&p,p.present,&out)&&out.ammo[0]==1023&&out.weapons[0]==99);
    assert(!client.decode(&p,InventoryPublic,&out));
    assert(server.encode(&v,&handles,&r,2,&p)&&p.present==(InventoryCombat|InventoryPublic));
    assert(p.values.ammo[0]==0&&p.values.weapons[0]==((0x345u<<11)|7u)&&p.values.weapons[47]==wire_invalid_handle);
    assert(client.decode(&p,p.present,&out)&&out.ammo[0]==1023&&out.weapons[0]==((0x345u<<12)|7u));
    assert(out.weapons[47]==invalid_handle&&out.active_weapon==out.weapons[0]&&out.next_attack==v.next_attack);
    assert(server.encode(&v,&handles,&r,3,&p)&&p.present==InventoryPublic);
    auto malformed=p;malformed.values.ammo[31]=1;
    assert(!client.decode(&malformed,InventoryPublic,&out));
    malformed=p;malformed.values.active_weapon=0x200000;
    assert(!client.decode(&malformed,InventoryPublic,&out));
    r={256,256};assert(server.encode(&v,&handles,&r,256,&p)&&p.present==7);
    assert(client.decode(&p,7,&out)&&out.ammo[31]==7);
    v.ammo[31]=1024;
    assert(!server.encode(&v,&handles,&r,256,&p)&&p.values.ammo[31]==7);
    assert(server.encode(&v,&handles,&r,1,&p)&&p.present==InventoryPublic);
    v.ammo[31]=0;v.next_attack=0x7fc00000;
    assert(!server.encode(&v,&handles,&r,256,&p));
    v.next_attack=0;handles.weapons[47]=2;
    assert(!server.encode(&v,&handles,&r,256,&p));
    handles.weapons[47]=0;r={257,0};
    assert(!server.encode(&v,&handles,&r,1,&p));
}
}
