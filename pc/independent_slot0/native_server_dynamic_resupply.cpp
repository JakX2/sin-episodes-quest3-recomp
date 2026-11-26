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
bool class_equal(const char* a,const char* b) noexcept {
    if(!a||!b)return false;
    auto lower=[](unsigned char c){return c>='A'&&c<='Z'?c+32:c;};
    while(*a&&lower(*a)==lower(*b)){++a;++b;}
    return lower(*a)==lower(*b);
}
int __cdecl nearby(void* actor,bool health_only,int debug,AmmoRow* rows,unsigned n,const NearbyBindings* b,NearbyResult* out) noexcept {
    if(!out)return 0;*out={};
    const unsigned limit=health_only?1:count;
    if(!actor||!rows||n<limit||!b||!b->next_visible||!b->center||!b->classname)return 0;
    auto name=[&](unsigned i){return health_only?health_classname:ammo_types[i].classname;};
    for(unsigned i=0;i<limit;++i)rows[i].nearby=0;
    void* previous=nullptr;
    for(;;){
        void* entity=nullptr;
        if(!b->next_visible(b->context,actor,previous,&entity))return 0;
        if(!entity)break;
        // Independent boundary guard: retail iterator itself guarantees progress.
        if(entity==previous||out->visited>=8192)return 0;
        ++out->visited;previous=entity;
        float a[3]{},e[3]{};
        if(!b->center(b->context,actor,a))return 0;++out->center_reads;
        if(!b->center(b->context,entity,e))return 0;++out->center_reads;
        const double x=double(e[0])-a[0],y=double(e[1])-a[1],z=double(e[2])-a[2];
        // FCOMP accepts <= radius squared, including unordered as in retail.
        if((z*z+y*y)+x*x>1048576.)continue;
        ++out->within;
        const char* cls=nullptr;if(!b->classname(b->context,entity,&cls))return 0;++out->class_reads;
        for(unsigned i=0;i<limit;++i){
            if(!class_equal(cls,name(i)))continue;
            if(debug==2){
                // Original refreshes candidate center before actor center for debug.
                if(!b->center(b->context,entity,e))return 0;++out->center_reads;
                if(!b->center(b->context,actor,a))return 0;++out->center_reads;
                if(!b->debug_line||!b->debug_line(b->context,a,e,0,255,0,true,20.f))return 0;
                ++out->lines;
            }
            ++rows[i].nearby;++out->matched;break;
        }
    }
    if(debug){
        for(unsigned i=0;i<limit;++i){
            if(!b->debug_count||!b->debug_count(b->context,name(i),rows[i].nearby))return 0;
            ++out->reports;
        }
    }
    out->completed=1;return 1;
}
const Api api{1,sizeof(Api),&ratios,&health,&greatest,&fallback,&nearby};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerDynamicResupplyApi=_SiN_GetServerDynamicResupplyApi")
extern "C" __declspec(dllexport) const sin_port::native_resupply::Api* __cdecl
SiN_GetServerDynamicResupplyApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_resupply;
    return version==1&&size==sizeof(Api)?&api:nullptr;
}
