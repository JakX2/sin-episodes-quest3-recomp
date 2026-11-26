#include "native_server_dynamic_resupply.h"
#include "original_ammo_definitions.h"
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::original_ammo::Api* __cdecl
SiN_GetOriginalAmmoDefinitionApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_resupply {
namespace {
template<class T>T field(const void* p,unsigned off) noexcept {
    T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;
}
bool valid_player(const InventoryView& v) noexcept {
    return v.player&&v.player_bytes>=0x78cu&&
        (field<unsigned>(v.player,0)==0x2245222cu||field<unsigned>(v.player,0)==0x224aec3cu);
}
bool direct_ammo_getters(unsigned table) noexcept {
    switch(table){
    case 0x2245a554u:case 0x2241b3a4u:case 0x22459194u:case 0x2245968cu:
    case 0x2247445cu:case 0x22474974u:case 0x224776acu:case 0x22477c8cu:
    case 0x22478214u:case 0x2247875cu:case 0x22478d0cu:case 0x2247924cu:
    case 0x2247a1e4u:case 0x2247ab9cu:return true;
    default:return false;
    }
}
int __cdecl owns(const InventoryView* v,int index,OwnershipResult* out) noexcept {
    if(!out)return 0;*out={};out->slot=-1;
    if(!v||!valid_player(*v)||v->entry_count>sin_independent::kEntityHandleCapacity||
       (v->entry_count&&!v->entries))return 0;
    const sin_independent::EntityHandleView registry(v->entries,v->entry_count);
    for(unsigned i=0;i<48;++i){
        ++out->slots;void* weapon=registry.Resolve(field<unsigned>(v->player,0x6cc+4*i));
        if(!weapon)continue;
        ++out->weapons;unsigned bytes=0;
        if(!v->extent||!v->extent(v->context,weapon,&bytes)||bytes<0x4ec||
           !direct_ammo_getters(field<unsigned>(weapon,0)))return 0;
        ++out->primary_reads;
        bool match=field<int>(weapon,0x4e4)==index;
        if(!match){++out->secondary_reads;match=field<int>(weapon,0x4e8)==index;}
        if(match){out->slot=int(i);out->weapon=weapon;out->found=1;return 1;}
    }
    return 1;
}
bool owned_by(const InventoryView* v,int index,bool& owned) noexcept {
    OwnershipResult result{};if(!owns(v,index,&result))return false;
    owned=result.found!=0;return true;
}
bool raw_reserve(const InventoryView* v,int index,int& reserve) noexcept {
    if(!v||!valid_player(*v)||index<0||index>=32)return false;
    reserve=field<int>(v->player,0x64c+4*unsigned(index));return true;
}
int __cdecl ratios(const float* desired,unsigned desired_count,AmmoRow* rows,unsigned row_count,const Bindings* b,Result* out) noexcept {
    if(!out)return 0;*out={};
    if(!desired||desired_count<count||!rows||row_count<count||!b||(!b->has_weapon&&!b->native_inventory)||!b->scale)return 0;
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
        bool owned=false;
        if(b->has_weapon){if(!b->has_weapon(b->context,index,&owned))return 0;}
        else {if(!owned_by(b->native_inventory,index,owned))return 0;++out->ownership_native;}
        if(!owned)row.current=1.f;
        else {
            ++out->owned;
            int maximum=0,reserve=0;if(!max_for(index,maximum))return 0;
            if(index!=-1){
                int live=0;if(!max_for(index,live))return 0;
                if(live==-2)reserve=999;
                else {
                    if(b->reserve){if(!b->reserve(b->context,index,&reserve))return 0;}
                    else {if(!raw_reserve(b->native_inventory,index,reserve))return 0;++out->reserve_native;}
                    ++out->reserve_reads;
                }
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
    if(!desired||n<count||!b||(!b->has_weapon&&!b->native_inventory)||!b->random_float)return 0;
    const auto* ammo=SiN_GetOriginalAmmoDefinitionApi(original_ammo::version,sizeof(original_ammo::Api));
    if(!b->index&&!ammo)return 0;
    for(unsigned i=0;i<count;++i){
        int index=-1;
        if(b->index){if(!b->index(b->context,ammo_types[i].name,&index))return 0;}
        else index=ammo->index(ammo_types[i].name);
        ++out->index_reads;
        bool owned=false;
        if(b->has_weapon){if(!b->has_weapon(b->context,index,&owned))return 0;}
        else {if(!owned_by(b->native_inventory,index,owned))return 0;++out->ownership_native;}
        ++out->ownership_reads;
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
void copy_vec(const void* p,unsigned off,float* out) noexcept {
    std::memcpy(out,static_cast<const unsigned char*>(p)+off,12);
}
bool refresh_origin(void* actor,const SpawnBindings& b,EvaluateResult& o) noexcept {
    if(!(field<unsigned>(actor,0xb8)&0x800u))return true;
    if(!b.refresh_origin||!b.refresh_origin(b.context,actor))return false;
    ++o.origin_refreshes;return true;
}
// 22146c40: factory, position, angles, owner and dispatch spawn, even
// when the factory returns null. The caller decides whether null is fatal.
bool create_pickup(void* actor,const char* name,const float* pos,const SpawnBindings& b,EvaluateResult& o,void*& item) noexcept {
    item=nullptr;
    if(!b.create||!b.create(b.context,name,-1,&item))return false;
    ++o.factories;
    if(item){
        if(!b.position||!b.position(b.context,item,pos))return false;
        float angles[3];copy_vec(actor,0x354,angles);
        if(!b.orientation||!b.orientation(b.context,item,angles)||!b.owner||!b.owner(b.context,item,actor))return false;
    }else ++o.null_factory;
    if(!b.finalize||!b.finalize(b.context,item))return false;
    ++o.finalizes;return true;
}
bool spawn_normal(void* actor,const char* name,float* pos,const SpawnBindings& b,EvaluateResult& o) noexcept {
    if(!refresh_origin(actor,b,o))return false;
    void* item=nullptr;
    if(!create_pickup(actor,name,pos,b,o,item)||!item)return false;
    if(field<unsigned>(actor,0xb8)&0x1000u){
        if(!b.refresh_motion||!b.refresh_motion(b.context,actor))return false;
        ++o.motion_refreshes;
    }
    float velocity[3];copy_vec(actor,0x250,velocity);
    if(!b.linear||!b.linear(b.context,item,velocity))return false;
    copy_vec(actor,0x25c,velocity);
    if(!b.angular||!b.angular(b.context,item,velocity))return false;
    float mins[3]{},maxs[3]{};
    if(!b.world_bounds||!b.world_bounds(b.context,item,mins,maxs))return false;
    if(mins[2]<pos[2]){
        const double dz=double(pos[2])-double(mins[2]);
        pos[2]=float(double(pos[2])+dz);
        maxs[2]=float(double(maxs[2])+dz);
        if(!b.move_origin||!b.move_origin(b.context,item,pos))return false;
        ++o.corrections;
    }
    pos[2]=float(double(maxs[2])+6.);
    for(unsigned i=0;i<2;++i){
        float draw=0;
        if(!b.random_float||!b.random_float(b.context,-6.f,6.f,&draw))return false;
        pos[i]=float(double(pos[i])+double(draw));++o.randoms;
    }
    ++o.normal_spawns;return true;
}
int __cdecl evaluate(void* actor,unsigned bytes,const void* player,unsigned player_bytes,int debug,const EvaluateBindings* b,EvaluateResult* out) noexcept {
    if(!out)return 0;*out={};out->selected_health=out->selected_ammo=-1;out->fallback.selected=-1;
    if(!actor||bytes<0x3b0||!player||player_bytes<0xa0||!b||
       b->entry_count>sin_independent::kEntityHandleCapacity||(b->entry_count&&!b->entries))return 0;
    const sin_independent::EntityHandleView registry(b->entries,b->entry_count);
    auto master=[&]()->void*{return b->master_handle?registry.Resolve(*b->master_handle):nullptr;};
    if(master()==actor){out->master_skipped=out->completed=1;return 1;}
    if(!nearby(actor,true,debug,&out->health_row,1,&b->nearby,&out->health_nearby)||
       !nearby(actor,false,debug,out->ammo_rows,count,&b->nearby,&out->ammo_nearby))return 0;
    void* profile=actor;
    if(field<unsigned>(actor,0xb4)&1u){
        if(void* m=master()){
            unsigned extent=0;
            if(!b->extent||!b->extent(b->extent_context,m,&extent)||extent<0x3b0)return 0;
            profile=m;out->master_profile=1;
        }
    }
    if(!health(field<int>(player,0x9c),field<int>(player,0x98),field<float>(profile,0x394),
               &out->health_row,&b->health,&out->health))return 0;
    float desired[count];
    for(unsigned i=0;i<count;++i)desired[i]=field<float>(profile,0x398+4*i);
    auto ammo_bindings=b->ammo;auto fallback_bindings=b->fallback;
    const InventoryView inventory{player,player_bytes,b->entries,b->entry_count,b->extent_context,b->extent};
    if(b->native_player_inventory){
        if(!valid_player(inventory))return 0;
        ammo_bindings.native_inventory=&inventory;fallback_bindings.native_inventory=&inventory;
    }
    if(!ratios(desired,count,out->ammo_rows,count,&ammo_bindings,&out->ammo))return 0;
    const auto& s=b->spawn;
    if(!refresh_origin(actor,s,*out))return 0;
    copy_vec(actor,0x2bc,out->next_position);
    if(!greatest(&out->health_row,1,&out->selected_health)||!greatest(out->ammo_rows,count,&out->selected_ammo))return 0;
    if(out->selected_health>=0&&!spawn_normal(actor,health_classname,out->next_position,s,*out))return 0;
    if(out->selected_ammo>=0&&!spawn_normal(actor,ammo_types[out->selected_ammo].classname,out->next_position,s,*out))return 0;
    if(out->selected_health<0&&out->selected_ammo<0){
        for(unsigned i=0;i<count;++i)desired[i]=field<float>(profile,0x398+4*i);
        if(!fallback(field<unsigned>(actor,0xb4),desired,count,&fallback_bindings,&out->fallback))return 0;
        if(out->fallback.selected>=0){
            if(!refresh_origin(actor,s,*out)||!refresh_origin(actor,s,*out))return 0;
            float pos[3];copy_vec(actor,0x2bc,pos);void* item=nullptr;
            if(!create_pickup(actor,out->fallback.classname,pos,s,*out,item))return 0;
            // Retail fallback does not dereference the factory result.
            if(item)++out->fallback_spawns;
        }
    }
    // 2214d6f0(0,0,0) writes the raw think pointer +8 to zero.
    const unsigned zero=0;std::memcpy(static_cast<unsigned char*>(actor)+8,&zero,4);out->think_cleared=1;
    if(!s.remove||!s.remove(s.context,actor))return 0;
    out->removed=out->completed=1;return 1;
}
const Api api{1,sizeof(Api),&ratios,&health,&greatest,&fallback,&nearby,&evaluate,&owns};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerDynamicResupplyApi=_SiN_GetServerDynamicResupplyApi")
extern "C" __declspec(dllexport) const sin_port::native_resupply::Api* __cdecl
SiN_GetServerDynamicResupplyApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_resupply;
    return version==1&&size==sizeof(Api)?&api:nullptr;
}
