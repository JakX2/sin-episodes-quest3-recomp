#include "native_server_sequence_choice.h"
#include <limits>
namespace sin_port::native_weapon_tables {
namespace {
int __cdecl choose(const ServerSequenceChoiceEntry* entries,unsigned count,int activity,int current,bool looping,const ServerSequenceChoiceBindings* b,int* out) noexcept {
    if(!out)return 0;
    *out=-1;
    if(count>65536)return 0;
    if(!entries){if(count)return 0;*out=0;return 1;}
    int total=0,selected=-1;
    for(unsigned i=0;i<count;++i){
        const auto& e=entries[i];if(e.activity!=activity)continue;
        // Original x86 NEG of INT_MIN overflows; reject malformed metadata.
        if(e.weight==std::numeric_limits<int>::min())return 0;
        const int weight=e.weight<0?-e.weight:e.weight;
        if(!looping){
            if(weight>total){total=weight;selected=static_cast<int>(i);}
            continue;
        }
        if(static_cast<int>(i)==current&&e.weight<0){*out=current;return 1;}
        if(weight>std::numeric_limits<int>::max()-total)return 0;
        total+=weight;
        int draw=0;
        if(!b||!b->random||!b->random(b->context,0,total-1,static_cast<int>(i),&draw))return 0;
        if(total&&(draw<0||draw>=total))return 0;
        if(!total||draw<weight)selected=static_cast<int>(i);
    }
    *out=selected;return 1;
}
const ServerSequenceChoiceApi api{1,sizeof(ServerSequenceChoiceApi),&choose};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerSequenceChoiceApi=_SiN_GetServerSequenceChoiceApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerSequenceChoiceApi* __cdecl
SiN_GetServerSequenceChoiceApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerSequenceChoiceApi)?&api:nullptr;
}
