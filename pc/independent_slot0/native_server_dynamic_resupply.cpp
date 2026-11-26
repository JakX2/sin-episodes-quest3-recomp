#include "native_server_dynamic_resupply.h"
#include "original_ammo_definitions.h"
extern "C" __declspec(dllexport) const sin_port::original_ammo::Api* __cdecl
SiN_GetOriginalAmmoDefinitionApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_resupply {
namespace {
int __cdecl ratios(const float* desired,unsigned desired_count,AmmoRow* rows,unsigned row_count,const Bindings* b,Result* out) noexcept {
    if(!out)return 0;*out={};
    if(!desired||desired_count<count||!rows||row_count<count||!b||!b->has_weapon||!b->scale)return 0;
    const auto* ammo=SiN_GetOriginalAmmoDefinitionApi(original_ammo::version,sizeof(original_ammo::Api));
    if((!b->index||!b->max_carry)&&!ammo)return 0;
    auto max_for=[&](int index,int& maximum){
        if(b->max_carry)return b->max_carry(b->context,index,&maximum)!=0;
        maximum=ammo->max_carry(index);++out->max_native;return true;
    };
    for(unsigned i=0;i<count;++i){
        auto& row=rows[i];const auto& type=ammo_types[i];
        int index=-1;
        if(b->index){if(!b->index(b->context,type.name,&index))return 0;}
        else {index=ammo->index(type.name);++out->index_native;}
        bool owned=false;if(!b->has_weapon(b->context,index,&owned))return 0;
        if(!owned)row.current=1.f;
        else {
            ++out->owned;
            int maximum=0,reserve=0;if(!max_for(index,maximum))return 0;
            if(index!=-1){
                int live=0;if(!max_for(index,live))return 0;
                if(live==-2)reserve=999;
                else {if(!b->reserve||!b->reserve(b->context,index,&reserve))return 0;++out->reserve_reads;}
            }
            // IMUL low 32 bits, interpreted signed by FILD.
            const auto bits=std::uint32_t(row.nearby)*std::uint32_t(type.units);
            const auto nearby=bits<=0x7fffffffu?std::int64_t(bits):std::int64_t(bits)-0x100000000ll;
            const float denominator=static_cast<float>(maximum);
            const double numerator=double(reserve)+double(nearby);
            row.current=static_cast<float>(numerator/double(denominator));
        }
        float scale=0;if(!b->scale(b->context,&scale))return 0;++out->scale_reads;
        // FST stores desired but retains its unrounded value for delta.
        const double product=double(desired[i])*double(scale);
        row.desired=static_cast<float>(product);
        row.delta=static_cast<float>(product-double(row.current));
        // Clamp stored delta; unordered (NaN) remains unchanged.
        if(row.delta>1.f)row.delta=1.f;else if(row.delta<0.f)row.delta=0.f;
        ++out->rows;
    }
    return 1;
}
int __cdecl health(int hp,int max_hp,float desired,AmmoRow* row,const HealthBindings* b,HealthResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!row||!b||!b->vial_value||!b->scale)return 0;
    float vial=0;if(!b->vial_value(b->context,&vial))return 0;++out->vial_reads;
    // FILD/FMul/FIAdd/FILD/Fdivp; max health is not pre-rounded to float.
    row->current=float((double(row->nearby)*double(vial)+double(hp))/double(max_hp));
    float scale=0;if(!b->scale(b->context,&scale))return 0;++out->scale_reads;
    const double product=double(desired)*double(scale);
    row->desired=float(product);
    const double delta=product-double(row->current);
    // Health clamps the live x87 value, unlike ammo's stored delta.
    row->delta=delta>1.?1.f:delta<0.?0.f:float(delta);
    out->completed=1;return 1;
}
int __cdecl greatest(const AmmoRow* rows,unsigned n,int* selected) noexcept {
    if(!selected)return 0;*selected=-1;
    if(!rows||!n||n>count)return 0;
    float best=0.f;
    for(unsigned i=0;i<n;++i)if(rows[i].delta>best){best=rows[i].delta;*selected=int(i);}
    return 1;
}
int __cdecl fallback(unsigned flags,const float* desired,unsigned n,const FallbackBindings* b,FallbackResult* out) noexcept {
    if(!out)return 0;*out={};out->selected=-1;
    if(!(flags&4u))return 1;
    out->enabled=1;
    if(!desired||n<count||!b||!b->has_weapon||!b->random_float)return 0;
    const auto* ammo=SiN_GetOriginalAmmoDefinitionApi(original_ammo::version,sizeof(original_ammo::Api));
    if(!b->index&&!ammo)return 0;
    for(unsigned i=0;i<count;++i){
        int index=-1;
        if(b->index){if(!b->index(b->context,ammo_types[i].name,&index))return 0;}
        else index=ammo->index(ammo_types[i].name);
        ++out->index_reads;
        bool owned=false;if(!b->has_weapon(b->context,index,&owned))return 0;++out->ownership_reads;
        // FUCOMPP equality excludes exactly zero; NaN remains eligible.
        if(!owned||ammo_types[i].full_weight==0.f||desired[i]==0.f){out->cumulative[i]=-1.f;continue;}
        out->total=float(double(out->total)+double(ammo_types[i].full_weight));
        out->cumulative[i]=out->total;++out->eligible;
    }
    // Retail sets first threshold to one when the total is exactly zero.
    if(out->total==0.f){out->total=out->cumulative[0]=1.f;out->forced_magnum=1;}
    if(!b->random_float(b->context,0.f,out->total,&out->draw))return 0;
    out->random_called=1;
    for(unsigned i=0;i<count;++i)if(out->draw<=out->cumulative[i]){
        out->selected=int(i);out->classname=ammo_types[i].classname;break;
    }
    return 1;
}
const Api api{1,sizeof(Api),&ratios,&health,&greatest,&fallback};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerDynamicResupplyApi=_SiN_GetServerDynamicResupplyApi")
extern "C" __declspec(dllexport) const sin_port::native_resupply::Api* __cdecl
SiN_GetServerDynamicResupplyApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_resupply;
    return version==1&&size==sizeof(Api)?&api:nullptr;
}
