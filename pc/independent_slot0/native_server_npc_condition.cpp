#include "native_server_npc_condition.h"
#include <cstring>
#include <limits>
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
constexpr unsigned offsets[]{0x7cc,0x7ec,0x80c,0x82c};
int __cdecl map(void* actor,int input,const ServerNpcConditionMapBindings* b,ServerNpcConditionResult* out) noexcept {
    if(!out)return 0;*out={};out->global=out->bit=-1;
    if(!actor)return 0;
    int global=input;
    if(input<npc_global_condition_base){
        if(!b||!b->descriptor)return 0;
        const void* desc=nullptr;unsigned bytes=0;
        if(!b->descriptor(b->context,actor,&desc,&bytes)||!desc||bytes<0x48)return 0;
        const void* row=static_cast<const unsigned char*>(desc)+0x34;
        global=-1;
        // Retail resolves -1 after class descriptor lookup, before scanning.
        if(input!=-1){
            std::uint32_t seen[256]{};
            while(row){
                if(out->visited>=256)return 0;
                const unsigned i=out->visited++;
                const int first=read<int>(row,4),last=read<int>(row,8);
                if(first!=9999&&input>=first&&input<=last){
                    const auto value=static_cast<std::int64_t>(read<int>(row,0))-first+input;
                    if(value<std::numeric_limits<int>::min()||value>std::numeric_limits<int>::max())return 0;
                    global=static_cast<int>(value);break;
                }
                const auto next=read<std::uint32_t>(row,0x10);
                if(!next)break;
                for(unsigned j=0;j<i;++j)if(seen[j]==next)return 0;
                seen[i]=next;
                unsigned extent=0;
                if(!b->storage||!b->storage(b->context,next,&row,&extent)||!row||extent<0x14)return 0;
            }
        }
    }
    out->global=global;
    if(global==-1||global==npc_global_condition_base-1)return 1;
    const auto bit=static_cast<std::int64_t>(global)-npc_global_condition_base;
    // Each verified bank is 8 dwords. Reject corruption, never cross banks.
    if(bit<0||bit>=256)return 0;
    out->mapped=1;out->bit=static_cast<int>(bit);return 1;
}
bool valid_bank(void* actor,unsigned bytes,unsigned bank) noexcept {
    return actor&&bank<4&&bytes>=offsets[bank]+32;
}
int __cdecl test(void* actor,unsigned bytes,int input,unsigned bank,const ServerNpcConditionMapBindings* b,ServerNpcConditionResult* out) noexcept {
    if(!out)return 0;*out={};out->global=out->bit=-1;
    if(!valid_bank(actor,bytes,bank)||!map(actor,input,b,out))return 0;
    if(out->mapped)out->value=(read<unsigned>(actor,offsets[bank]+4*(out->bit/32))>>(out->bit%32))&1u;
    return 1;
}
int __cdecl store(void* actor,unsigned bytes,int input,unsigned bank,bool value,const ServerNpcConditionMapBindings* b,ServerNpcConditionResult* out) noexcept {
    if(!out)return 0;*out={};out->global=out->bit=-1;
    if(bank>NpcConditionInterrupt||!valid_bank(actor,bytes,bank)||!map(actor,input,b,out))return 0;
    if(!out->mapped)return 1;
    const unsigned offset=offsets[bank]+4*(out->bit/32),mask=1u<<(out->bit%32);
    const unsigned prior=read<unsigned>(actor,offset),next=value?prior|mask:prior&~mask;
    write(actor,offset,next);out->value=value;out->changed=prior!=next;return 1;
}
int __cdecl snapshot_mask(void* actor,unsigned bytes) noexcept {
    if(!actor||bytes<0x84c)return 0;
    std::memcpy(static_cast<unsigned char*>(actor)+0x80c,static_cast<unsigned char*>(actor)+0x7cc,32);
    for(int i=7;i>=0;--i)write(actor,0x7cc+i*4,read<unsigned>(actor,0x7cc+i*4)&read<unsigned>(actor,0x82c+i*4));
    return 1;
}
const ServerNpcConditionApi api{1,sizeof(ServerNpcConditionApi),&map,&test,&store,&snapshot_mask};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcConditionApi=_SiN_GetServerNpcConditionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl
SiN_GetServerNpcConditionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcConditionApi)?&api:nullptr;
}
