#pragma once
#include "native_weapon_datatables.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct Fixture {
    alignas(4) unsigned char bytes[0x4f4]{};
    n::OwnerObservation owner{};
};
inline void* __cdecl local(const s::SendProp*,const void* structure,const void* data,s::Recipients* out,int){
    const auto* f=static_cast<const Fixture*>(structure);
    if(!f||!out)return nullptr;
    for(auto& word:out->words)word=0;
    if(f->owner.resolved&&f->owner.player_eligible&&f->owner.client_slot>=1&&f->owner.client_slot<=256){
        const auto i=f->owner.client_slot-1;out->words[i/32]=1u<<(i%32);
    }
    return const_cast<void*>(data);
}
// The test host callback resolves an actual fixture observation instead
// of pretending the original engine's global EHANDLE table exists.
inline void __cdecl owner(const s::SendProp*,const void* structure,const void* data,s::DVariant* out,int,int){
    const auto* f=static_cast<const Fixture*>(structure);
    std::uint32_t value=0;std::memcpy(&value,data,4);
    assert(value==f->owner.native_handle);
    out->words[0]=f->owner.resolved?((value&0x7ffu)|(((value>>12)&0x3ffu)<<11)):n::wire_invalid_handle;
}
inline void check(const SendApi& sa,const RecvApi& ra,const n::Api& contract,const n::Values& values){
    s::SendProp dummy;
    s::SendTable sb;assert(sb.Construct(&dummy,1,"DT_BaseAnimating"));
    RecvProp rdummy;RecvTable rb{&rdummy,1,nullptr,"DT_BaseAnimating"};
    SendView sv{};RecvView rv{};
    SendBindings bindings{&sb,&local,&local,&owner};
    RecvBindings rec{&rb};
    auto bad=bindings;bad.local_active=nullptr;
    assert(!sa.create(&bad,&sv)&&!sv.handle);
    bad=bindings;bad.base_animating=nullptr;
    assert(!sa.create(&bad,&sv)&&!sv.handle);
    assert(!ra.create(nullptr,&rv)&&!rv.handle);
    assert(sa.create(&bindings,&sv)&&ra.create(&rec,&rv));
    assert(!sa.create(&bindings,&sv)&&!ra.create(&rec,&rv));
    const unsigned parent[]={BaseCombat,BaseHL,MachineGun,BaseHL,SelectFire,BaseHL};
    for(unsigned i=0;i<TableCount;++i){
        const auto& a=*sv.tables[i];const auto& b=*rv.tables[i];
        assert(std::strcmp(a.name,table_names[i])==0&&std::strcmp(b.name,a.name)==0);
        assert(a.count==table_counts[i]&&b.count==a.count);
        if(i>=BaseHL){
            assert(a.props[0].table==sv.tables[parent[i-BaseHL]]&&b.props[0].table==rv.tables[parent[i-BaseHL]]);
            assert(a.props[0].flags==0x200&&a.props[0].offset==0&&b.props[0].offset==0);
        }
    }
    assert(sv.tables[BaseCombat]->props[0].table==&sb&&rv.tables[BaseCombat]->props[0].table==&rb);
    assert(sv.tables[BaseCombat]->props[1].table==sv.tables[LocalWeapon]&&
        sv.tables[BaseCombat]->props[2].table==sv.tables[LocalActive]);
    assert(rv.tables[BaseCombat]->props[1].table==rv.tables[LocalWeapon]&&
        rv.tables[BaseCombat]->props[2].table==rv.tables[LocalActive]);
    Fixture f;f.owner={values.words[n::Owner],1,1,7};
    alignas(4) unsigned char client[0x7e8]{};
    for(unsigned i=0;i<n::field_count;++i){
        n::FieldInfo info{};assert(contract.field(i,&info));
        auto& send=sv.tables[info.group]->props[info.slot];auto& recv=rv.tables[info.group]->props[info.slot];
        assert(std::strcmp(send.name,info.name)==0&&std::strcmp(recv.name,info.name)==0);
        assert(send.offset==static_cast<int>(info.server_offset)&&recv.offset==static_cast<int>(info.client_offset));
        assert(send.flags==static_cast<int>(info.flags)&&send.bits==static_cast<int>(info.bits)&&recv.flags==0);
        assert(send.elements==1&&recv.elements==1&&send.scalarProxy&&recv.proxy);
        std::memcpy(f.bytes+send.offset,&values.words[i],4);
        s::DVariant variant{};
        send.scalarProxy(&send,&f,f.bytes+send.offset,&variant,0,1);
        // Raw proxy result BEFORE bitstream truncation; all valid test
        // inputs decode correctly without a fake Source wire packet.
        if(i==n::Clip2)assert(variant.words[0]==0);
        RecvData received{&recv,variant,0,1};
        recv.proxy(&received,client,client+recv.offset);
        std::uint32_t result=0;std::memcpy(&result,client+recv.offset,4);
        assert(result==(i==n::Owner?((0x345u<<12)|37u):values.words[i]));
    }
    s::Recipients mask{};
    auto& localProp=sv.tables[BaseCombat]->props[1];
    assert(localProp.tableProxy(&localProp,&f,&f,&mask,1)==&f&&mask.words[0]==64);
    f.owner.player_eligible=0;
    assert(localProp.tableProxy(&localProp,&f,&f,&mask,1)==&f&&mask.words[0]==0);
    void* dest=nullptr;
    auto& receiveBase=rv.tables[BaseCombat]->props[0];
    receiveBase.table_proxy(&receiveBase,&dest,client,1);assert(dest==client);
    sv.tables[BaseCombat]->initialized=true;
    rv.tables[BaseCombat]->decoder=&f;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));
    sv.tables[BaseCombat]->initialized=false;rv.tables[BaseCombat]->decoder=nullptr;
    assert(sa.destroy(&sv)&&ra.destroy(&rv)&&!sv.handle&&!rv.handle);
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));
}
}
