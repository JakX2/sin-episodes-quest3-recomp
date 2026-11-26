#include "native_server_dynamic_resupply.h"
#include "original_ammo_definitions.h"
#include "native_server_npc_active_weapon_step.h"
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEntityVoidOutputApi* __cdecl SiN_GetServerEntityVoidOutputApi(std::uint32_t,std::uint32_t) noexcept;
#include <cstring>
#include <cmath>
#include <limits>
extern "C" int __cdecl SiN_ReadResupplyEngineConvar(void*,sin_port::native_resupply::Convar,void*) noexcept;
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
    if(!desired||desired_count<count||!rows||row_count<count||!b||(!b->has_weapon&&!b->native_inventory)||(!b->scale&&!b->engine_cvar))return 0;
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
        float scale=0;
        if(!(b->scale?b->scale(b->context,&scale):SiN_ReadResupplyEngineConvar(b->engine_cvar,Convar::Modifier,&scale)))return 0;
        ++out->scale_reads;
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
    if(!row||!b||((!b->vial_value||!b->scale)&&!b->engine_cvar))return 0;
    float vial=0;
    if(!(b->vial_value?b->vial_value(b->context,&vial):SiN_ReadResupplyEngineConvar(b->engine_cvar,Convar::HealthKit,&vial)))return 0;
    ++out->vial_reads;
    // FILD/FMul/FIAdd/FILD/Fdivp; max health is not pre-rounded to float.
    row->current=float((double(row->nearby)*double(vial)+double(hp))/double(max_hp));
    float scale=0;if(!(b->scale?b->scale(b->context,&scale):SiN_ReadResupplyEngineConvar(b->engine_cvar,Convar::Modifier,&scale)))return 0;++out->scale_reads;
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
int __cdecl pickup_factory_create(const char*,const PickupFactoryBindings*,PickupFactoryResult*) noexcept;
bool create_pickup(void* actor,const char* name,const float* pos,const SpawnBindings& b,EvaluateResult& o,void*& item) noexcept {
    item=nullptr;
    if(b.create){if(!b.create(b.context,name,-1,&item))return false;}
    else {
        if(!b.native_factory)return false;
        PickupFactoryResult result{};++o.native_factory_calls;
        if(!pickup_factory_create(name,b.native_factory,&result)){++o.native_factory_failures;o.partial_pickup=result.actor;return false;}
        item=result.actor;
    }
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
    if(!actor||bytes<0x3b0||!b||
       b->entry_count>sin_independent::kEntityHandleCapacity||(b->entry_count&&!b->entries))return 0;
    const sin_independent::EntityHandleView registry(b->entries,b->entry_count);
    auto master=[&]()->void*{return b->master_handle?registry.Resolve(*b->master_handle):nullptr;};
    if(master()==actor){out->master_skipped=out->completed=1;return 1;}
    if(!player||player_bytes<0xa0)return 0;
    if(b->engine_cvar){
        if(!SiN_ReadResupplyEngineConvar(b->engine_cvar,Convar::Debug,&debug))return 0;
        out->debug_native=1;
    }
    out->debug_value=debug;
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
    auto health_bindings=b->health;
    if(b->engine_cvar)health_bindings.engine_cvar=b->engine_cvar;
    if(!health(field<int>(player,0x9c),field<int>(player,0x98),field<float>(profile,0x394),
               &out->health_row,&health_bindings,&out->health))return 0;
    float desired[count];
    for(unsigned i=0;i<count;++i)desired[i]=field<float>(profile,0x398+4*i);
    auto ammo_bindings=b->ammo;auto fallback_bindings=b->fallback;
    if(b->engine_cvar)ammo_bindings.engine_cvar=b->engine_cvar;
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
template<class T>void set_field(void* p,unsigned off,T value) noexcept {
    std::memcpy(static_cast<unsigned char*>(p)+off,&value,sizeof(value));
}
bool lifecycle_start(void* actor,unsigned bytes,const LifecycleBindings* b,LifecycleResult* o,bool constructed=true) noexcept {
    if(!o)return false;*o={};
    return actor&&bytes>=0x3b0&&b&&(!constructed||field<unsigned>(actor,0)==resupply_vtable);
}
int __cdecl construct(void* actor,unsigned bytes,const LifecycleBindings* b,LifecycleResult* o) noexcept {
    if(!lifecycle_start(actor,bytes,b,o,false)||!b->base_construct||!b->base_construct(b->context,actor,0))return 0;
    o->base_called=1;set_field(actor,0,resupply_vtable);
    const unsigned flags=field<unsigned>(actor,0xb4);
    if(!(flags&1u)){
        if(field<std::uint8_t>(actor,0x1f0)){
            set_field(actor,0x200,std::uint8_t(1));o->deferred_dirty=1;
        }else if(const unsigned edict=field<unsigned>(actor,0x16c)){
            if(!b->mark_edict||!b->mark_edict(b->context,edict,1u))return 0;
            o->edict_marked=1;
        }
        set_field(actor,0xb4,flags|1u);o->flag_added=1;
    }
    for(unsigned i=0;i<count+1;++i){set_field(actor,0x394+4*i,1.f);++o->defaults_written;}
    o->completed=1;return 1;
}
int __cdecl spawn(void* actor,unsigned bytes,const LifecycleBindings* b,LifecycleResult* o) noexcept {
    if(!lifecycle_start(actor,bytes,b,o)||!b->precache)return 0;
    if(!b->precache(b->context,health_classname,0))return 0;++o->precached;
    for(const auto& type:ammo_types){if(!b->precache(b->context,type.classname,0))return 0;++o->precached;}
    if(field<unsigned>(actor,0xb4)&2u){o->manual=o->completed=1;return 1;}
    if(!b->random_float||!b->random_float(b->context,.2f,.4f,&o->delay))return 0;o->random_called=1;
    float now=0;if(!b->now||!b->now(b->context,&now))return 0;o->clock_read=1;
    o->next_time=float(double(now)+double(o->delay));
    if(!b->schedule||!b->schedule(b->schedule_context?b->schedule_context:b->context,actor,o->next_time))return 0;o->scheduled=1;
    set_field(actor,8,check_pvs_think);o->think_installed=o->completed=1;return 1;
}
bool assign_master(void* actor,const LifecycleBindings& b,LifecycleResult& o) noexcept {
    if(!b.master_handle)return false;
    if(b.entity_handle){
        if(!b.entity_handle(b.context,actor,b.master_handle))return false;
    }else{
        // Primary +8 is 22001b00: LEA EAX,[ECX+378]; RET.
        *b.master_handle=field<unsigned>(actor,0x378);
        o.native_handle=1;
    }
    o.master_assigned=1;return true;
}
bool registry_ok(const LifecycleBindings& b) noexcept {
    return b.master_handle&&b.entry_count<=sin_independent::kEntityHandleCapacity&&(!b.entry_count||b.entries);
}
int __cdecl activate(void* actor,unsigned bytes,const LifecycleBindings* b,LifecycleResult* o) noexcept {
    if(!lifecycle_start(actor,bytes,b,o)||!registry_ok(*b)||!b->base_activate||!b->base_activate(b->context,actor))return 0;
    o->base_called=1;
    const sin_independent::EntityHandleView registry(b->entries,b->entry_count);
    if(field<unsigned>(actor,0xb4)&2u){
        if(!registry.Resolve(*b->master_handle)){
            if(!assign_master(actor,*b,*o))return 0;
        }else o->master_retained=1;
    }
    if(field<unsigned>(actor,0xb4)&1u){
        float now=0;if(!b->now||!b->now(b->context,&now))return 0;o->clock_read=1;
        // Original x87 ordered strictly-less-than 1.0, not <=.
        if(now<1.f&&!registry.Resolve(*b->master_handle)){
            if(!b->warning||!b->warning(b->context,"item_dynamic_resupply set to 'Use Master', but no item_dynamic_resupply master exists.\n"))return 0;
            o->warning_called=1;
        }
    }
    o->completed=1;return 1;
}
int __cdecl become_master(void* actor,unsigned bytes,const LifecycleBindings* b,LifecycleResult* o) noexcept {
    if(!lifecycle_start(actor,bytes,b,o)||!assign_master(actor,*b,*o))return 0;
    set_field(actor,8,0u);o->think_cleared=o->completed=1;return 1;
}
int __cdecl kill(void* actor,unsigned bytes,const LifecycleBindings* b,LifecycleResult* o) noexcept {
    if(!lifecycle_start(actor,bytes,b,o)||!b->remove||!b->remove(b->context,actor))return 0;
    o->removed=o->completed=1;return 1;
}
bool dispatch_start(void* a,unsigned n,const DispatchBindings* b,DispatchResult* o) noexcept {
    if(!o)return false;*o={};o->evaluation.selected_health=o->evaluation.selected_ammo=-1;
    return a&&n>=0x3b0&&field<unsigned>(a,0)==resupply_vtable&&b&&b->evaluation;
}
bool edict_player(const void* edict,unsigned bytes,bool check_free,const DispatchBindings& b,DispatchResult& o,void*& player) noexcept {
    player=nullptr;if(!edict)return true;o.edict_found=1;
    if(bytes<4)return false;
    if(check_free&&(field<unsigned>(edict,0)&2u)){o.edict_free=1;return true;}
    if(bytes<0x10)return false;
    const unsigned token=field<unsigned>(edict,0xc);
    if(!token)return true;
    if(!b.base_entity||!b.base_entity(b.context,token,&player))return false;
    o.unknown_called=1;o.player_found=player!=nullptr;return true;
}
bool dispatch_evaluate(void* a,unsigned n,void* player,int debug,const DispatchBindings& b,DispatchResult& o) noexcept {
    unsigned extent=0;
    const auto& e=*b.evaluation;
    // The original evaluator checks the master identity before reading player.
    const sin_independent::EntityHandleView registry(e.entries,e.entry_count);
    const bool self=e.entry_count<=sin_independent::kEntityHandleCapacity&&e.master_handle&&registry.Resolve(*e.master_handle)==a;
    if(player&&!self){
        if(!b.extent||!b.extent(b.context,player,&extent))return false;
        o.extent_read=1;
    }
    o.evaluated=1;
    if(!evaluate(a,n,player,extent,debug,&e,&o.evaluation))return false;
    o.completed=1;return true;
}
int __cdecl check_pvs(void* a,unsigned n,int debug,const DispatchBindings* b,DispatchResult* o) noexcept {
    if(!dispatch_start(a,n,b,o)||!b->pvs_edict)return 0;
    const void* edict=nullptr;unsigned bytes=0;
    if(!b->pvs_edict(b->context,field<unsigned>(a,0x16c),&edict,&bytes))return 0;
    o->pvs_called=1;void* player=nullptr;
    // Outer CheckPVSThink receives an already filtered edict.
    if(!edict_player(edict,bytes,false,*b,*o,player))return 0;
    if(player)return dispatch_evaluate(a,n,player,debug,*b,*o);
    float now=0;if(!b->now||!b->now(b->context,&now))return 0;
    o->next_time=float(double(now)+1.);
    if(!b->schedule||!b->schedule(b->schedule_context?b->schedule_context:b->context,a,o->next_time))return 0;
    o->retry=o->completed=1;return 1;
}
int __cdecl calculate_type(void* a,unsigned n,int debug,const DispatchBindings* b,DispatchResult* o) noexcept {
    if(!dispatch_start(a,n,b,o)||!b->max_clients||!b->max_clients(b->context,&o->max_clients))return 0;
    o->local_called=1;void* player=nullptr;
    if(o->max_clients==1){
        const void* edict=nullptr;unsigned bytes=0;
        if(!b->edict||!b->edict(b->context,1,&edict,&bytes))return 0;
        o->slot_checked=1;
        if(!edict_player(edict,bytes,true,*b,*o,player))return 0;
    }
    // No retry for CalculateType. Retail passes null into evaluate;
    // self-master early return is safe, other null players fail closed.
    return dispatch_evaluate(a,n,player,debug,*b,*o);
}
int __cdecl factory_create(const char* classname,const FactoryBindings* b,FactoryResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!classname||!class_equal(classname,resupply_classname)||!b||!b->allocate||!b->initialize||!b->lifecycle)return 0;
    o->allocation_called=1;
    if(!b->allocate(b->context,resupply_extent,&o->actor,&o->bytes))return 0;
    if(!o->actor)return 0; // retail dereferences allocation failure; fail closed
    o->allocated=1;
    // Allocation belongs to the caller even on a partial failure; no
    // destructor guess or double-free. Only publish interface after init.
    if(o->bytes<resupply_extent||!construct(o->actor,o->bytes,b->lifecycle,&o->lifecycle))return 0;
    o->constructed=1;
    if(!b->initialize(b->context,o->actor,classname))return 0;
    o->initialized=1;
    o->networkable=static_cast<unsigned char*>(o->actor)+resupply_networkable_offset;
    o->completed=1;return 1;
}
int __cdecl factory_destroy(void* actor,unsigned bytes,void* networkable,const FactoryBindings* b,FactoryResult* o) noexcept {
    if(!o)return 0;*o={};
    // Original factory's second virtual returns on a null networkable.
    if(!networkable){o->completed=1;return 1;}
    if(!actor||bytes<resupply_extent||field<unsigned>(actor,0)!=resupply_vtable||
       networkable!=static_cast<unsigned char*>(actor)+resupply_networkable_offset||
       !b||!b->release_networkable)return 0;
    o->actor=actor;o->bytes=bytes;
    // The service may synchronously destroy actor storage. Do not read it after.
    if(!b->release_networkable(b->context,networkable))return 0;
    o->released=o->completed=1;return 1;
}
int __cdecl ammo_pickup_construct(void* item,unsigned bytes,unsigned type,const AmmoPickupBindings* b,AmmoPickupResult* o) noexcept {
    if(!o)return 0;*o={};o->index=-1;o->type=type;
    if(!item||bytes<0x4d8||type>=count||!b||!b->base_construct||!b->base_construct(b->context,item))return 0;
    const auto& p=ammo_pickups[type];
    set_field(item,0x4cc,p.model_token);set_field(item,0x4d0,p.ammo_token);set_field(item,0x4d4,p.units);
    set_field(item,0,p.vtable);set_field(item,0x494,p.secondary);
    o->units=p.units;o->constructed=o->completed=1;return 1;
}
int signed_word(unsigned v) noexcept {
    return v<=0x7fffffffu?int(v):int(std::int64_t(v)-0x100000000ll);
}
int __cdecl player_give_ammo(void* player,int requested,int index,bool silent,const PlayerAmmoBindings* b,PlayerAmmoResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!b||!player||b->inventory.player!=player||!valid_player(b->inventory)||
       b->inventory.entry_count>sin_independent::kEntityHandleCapacity||(b->inventory.entry_count&&!b->inventory.entries))return 0;
    auto done=[&](){o->completed=1;return 1;};
    const unsigned vt=field<unsigned>(player,0);o->sin_override=vt==0x224aec3cu;
    if(index<0)return done();
    if(index>=32)return 0; // retail unsafe array access, independent guard
    const auto* api=SiN_GetOriginalAmmoDefinitionApi(original_ammo::version,sizeof(original_ammo::Api));
    if(!api)return 0;
    auto max=[&](int& v){
        ++o->max_reads;
        if(b->max_carry)return b->max_carry(b->context,index,&v)!=0;
        v=api->max_carry(index);return true;
    };
    const unsigned offset=0x64c+4*unsigned(index);
    o->before=o->after=field<int>(player,offset);
    const sin_independent::EntityHandleView registry(b->inventory.entries,b->inventory.entry_count);
    auto weapon_extent=[&](void* w,unsigned need){
        unsigned bytes=0;return b->inventory.extent&&b->inventory.extent(b->inventory.context,w,&bytes)&&bytes>=need&&direct_ammo_getters(field<unsigned>(w,0));
    };
    if(o->sin_override){
        // 2239e770 invokes HasAmmo even for nonpositive requested counts.
        int limit=0;if(!max(limit))return 0;
        o->has_ammo=limit==-2||field<int>(player,offset)!=0;
        if(!o->has_ammo){
            for(unsigned i=0;i<48&&!o->has_ammo;++i){
                ++o->slots;void* w=registry.Resolve(field<unsigned>(player,0x6cc+4*i));
                if(!w)continue;
                if(!weapon_extent(w,0x4ec)||!b->weapon_test)return 0;
                for(unsigned k=0;k<2&&!o->has_ammo;++k){
                    bool uses=false,has=false;
                    if(!b->weapon_test(b->context,w,k?0x43c:0x438,&uses))return 0;++o->weapon_tests;
                    if(!uses||field<int>(w,k?0x4e8:0x4e4)!=index)continue;
                    if(!b->weapon_test(b->context,w,k?0x334:0x330,&has))return 0;++o->weapon_tests;
                    o->has_ammo=has;
                }
            }
        }
    }
    if(requested>0){
        // CSE1Rules::CanHaveAmmo 221f26c0: max then GetAmmoCount
        // (which reads max again, returning 999 for -2).
        int first=0,live=0;
        if(!max(first)||!max(live))return 0;
        const int reserve=live==-2?999:field<int>(player,offset);
        if(reserve<first){
            o->permission=1;
            if(!max(o->maximum))return 0;
            const int room=signed_word(unsigned(o->maximum)-unsigned(field<int>(player,offset)));
            const int grant=requested<room?requested:room;
            if(grant>=1){
                if(!silent){
                    original_ammo::Definition definition{};
                    const bool special=api->definition(index,&definition)&&definition.pickup_sound[0];
                    if(!b->sound||!b->sound(b->context,player,special?definition.pickup_sound:"BaseCombatCharacter.AmmoPickup"))return 0;
                    o->sound=1;
                }
                // Original reloads reserve after sound and before dirty notification.
                const int value=signed_word(unsigned(field<int>(player,offset))+unsigned(grant));
                if(field<int>(player,offset)!=value){
                    if(field<std::uint8_t>(player,0x1f0))set_field(player,0x200,std::uint8_t(1));
                    else if(const unsigned edict=field<unsigned>(player,0x16c)){
                        if(!b->mark_edict||!b->mark_edict(b->context,edict,1))return 0;
                    }
                    ++o->dirty;set_field(player,offset,value);
                }
                o->granted=grant;
            }
        }
    }
    o->after=field<int>(player,offset);
    if(o->sin_override){
        if(requested>0&&!o->granted){
            if(!b->denied_message||!b->denied_message(b->context,player,index))return 0;o->denied=1;
        }
        bool enabled=false;if(!b->auto_switch_enabled||!b->auto_switch_enabled(b->context,&enabled))return 0;
        o->auto_checked=1;
        if(enabled&&!o->has_ammo){
            if(b->inventory.player_bytes<0x790||!b->next_best)return 0;
            auto active=[&](){return registry.Resolve(field<unsigned>(player,0x78c));};
            void* candidate=nullptr;
            if(!b->next_best(b->context,player,active(),&candidate))return 0;++o->next_best_calls;
            if(candidate){
                if(!weapon_extent(candidate,0x4ec))return 0;
                if(field<int>(candidate,0x4e4)==index){
                    void* current=active();void* best=nullptr;
                    if(!b->next_best(b->context,player,current,&best))return 0;++o->next_best_calls;
                    if(best&&best!=current){
                        if(!b->switch_weapon||!b->switch_weapon(b->context,player,best,0))return 0;o->switched=1;
                    }
                }
            }
        }
    }
    return done();
}
int __cdecl ammo_pickup_touch(void* item,unsigned bytes,void* player,const AmmoPickupBindings* b,AmmoPickupResult* o) noexcept {
    if(!o)return 0;*o={};o->index=-1;o->type=count;
    if(!item||bytes<0x4d8||!player||!b)return 0;
    for(unsigned i=0;i<count;++i)if(field<unsigned>(item,0)==ammo_pickups[i].vtable){o->type=i;break;}
    if(o->type==count||field<unsigned>(item,0x4d0)!=ammo_pickups[o->type].ammo_token)return 0;
    const char* name=ammo_types[o->type].name;
    o->units=field<int>(item,0x4d4);
    if(b->index){if(!b->index(b->context,name,&o->index))return 0;}
    else {
        const auto* ammo=SiN_GetOriginalAmmoDefinitionApi(original_ammo::version,sizeof(original_ammo::Api));
        if(!ammo)return 0;o->index=ammo->index(name);
    }
    o->index_read=1;
    if(o->index==-1){
        o->unknown=1;if(b->unknown_ammo&&!b->unknown_ammo(b->context,name))return 0;
        o->completed=1;return 1;
    }
    if(b->multiplier){if(!b->multiplier(b->context,o->index,&o->multiplier))return 0;}
    else if(b->live_metric2)o->multiplier=*b->live_metric2;else return 0;
    o->multiplier_read=1;
    // Original FILD/FSTP rounds units to binary32 before 222f4f30.
    double amount=double(float(o->units))*double(o->multiplier);
    if(amount<1.)amount=1.;
    const double truncated=std::trunc(amount);
    o->requested=(!std::isfinite(truncated)||truncated<double(std::numeric_limits<int>::min())||truncated>double(std::numeric_limits<int>::max()))?std::numeric_limits<int>::min():int(truncated);
    if(b->give){if(!b->give(b->context,player,o->requested,o->index,false,&o->granted))return 0;}
    else {if(!player_give_ammo(player,o->requested,o->index,false,b->native_grant,&o->native_grant))return 0;o->granted=o->native_grant.granted;}
    o->give_called=1;
    if(!o->granted){o->completed=1;return 1;}
    if(b->respawn_rule){if(!b->respawn_rule(b->context,item,&o->rule))return 0;}
    else if(b->native_singleplayer_rules)o->rule=6;else return 0;
    o->rule_called=1;
    if(o->rule==6){if(!b->remove||!b->remove(b->context,item))return 0;o->removed=1;}
    o->accepted=o->completed=1;return 1;
}
int __cdecl vial_construct(void* item,unsigned bytes,const VialPickupBindings* b,VialPickupResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!item||bytes<0x4cc||!b||!b->base_construct||!b->base_construct(b->context,item))return 0;
    set_field(item,0,vial_vtable);set_field(item,0x494,vial_secondary);
    o->constructed=o->completed=1;return 1;
}
int __cdecl player_take_health(void* player,float amount,unsigned flags,const PlayerHealthBindings* b,PlayerHealthResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!b||!player||b->player!=player||b->bytes<0x12a2)return 0;
    const unsigned vt=field<unsigned>(player,0);
    if(vt!=0x2245222cu&&vt!=0x224aec3cu)return 0;
    o->before=o->after=field<int>(player,0x9c);
    o->maximum_before=o->maximum_after=field<int>(player,0x98);
    auto done=[&](){o->completed=1;return 1;};
    // Player override: clear matching non-timebased damage bits, even if
    // the base routine will decline the heal. Both max-health setters'
    // network notifications (+1b0) and health notifications (+1b4) are RET.
    if(field<int>(player,0x94)!=0){
        set_field(player,0x9d0,field<unsigned>(player,0x9d0)&~(flags&0xffc07fffu));
        o->damage_mask_written=1;
    }
    if(!field<unsigned char>(player,0x12a1)&&field<int>(player,0x98)>100){
        set_field(player,0x98,100);set_field(player,0x12a1,static_cast<unsigned char>(0));
        o->maximum_reset=1;
    }
    o->maximum_after=field<int>(player,0x98);
    // CBaseCombatCharacter guard followed by CBaseEntity::TakeHealth.
    if(!field<unsigned>(player,0x16c)||field<int>(player,0x94)<2||o->before>=o->maximum_after)return done();
    const double truncated=std::trunc(double(amount));
    o->converted=(!std::isfinite(truncated)||truncated<double(std::numeric_limits<int>::min())||truncated>double(std::numeric_limits<int>::max()))?std::numeric_limits<int>::min():int(truncated);
    o->raw_health=signed_word(unsigned(o->before)+unsigned(o->converted));
    if(o->raw_health!=o->before)set_field(player,0x9c,o->raw_health);
    o->raw_delta=signed_word(unsigned(o->raw_health)-unsigned(o->before));
    // Health-change category for this routine: positive heal with flags
    // maps to 4, then >=25/10/positive ->3/2/1; zero ->0. Negative
    // categories are decoded separately below from 222b5ec0.
    if(o->raw_delta==0)o->event_type=0;
    else if(o->raw_delta>0)o->event_type=flags?4:o->raw_delta>=25?3:o->raw_delta>=10?2:1;
    else if(flags&0x40000002u)o->event_type=8;
    else if(flags&0x91u)o->event_type=7;
    else if(flags&4u)o->event_type=13;
    else if(flags&0x1200008u)o->event_type=9;
    else if(flags&0x30000u)o->event_type=11;
    else if(flags&0x100000u)o->event_type=15;
    else if(flags&0x100u)o->event_type=10;
    else if(flags&0x20u)o->event_type=16;
    else if(flags&0x10000040u)o->event_type=18;
    else if(flags&0x4000u)o->event_type=17;
    else o->event_type=(flags&0x400000u)?12:6;
    if(!b->health_event||!b->health_event(b->context,player,o->event_type,o->raw_health,o->raw_delta,flags))return 0;
    o->event_called=1;
    const int max=field<int>(player,0x98);
    if(field<int>(player,0x9c)>max)set_field(player,0x9c,max);
    o->after=field<int>(player,0x9c);o->maximum_after=max;
    o->delta=signed_word(unsigned(o->after)-unsigned(o->before));
    if(!o->delta)return done();
    constexpr unsigned char rgba[]{0,0,128,64};
    if(!b->fade||!b->fade(b->context,player,rgba,1.f,.5f,1))return 0;
    o->fade_called=1;
    if(field<int>(player,0x9c)>100){set_field(player,0x12a1,static_cast<unsigned char>(0));o->overheal_flag_cleared=1;}
    o->accepted=1;return done();
}
int __cdecl vial_touch(void* item,unsigned bytes,void* player,const VialPickupBindings* b,VialPickupResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!item||bytes<0x4cc||field<unsigned>(item,0)!=vial_vtable||!player||!b)return 0;
    if(!(b->value?b->value(b->context,&o->value):SiN_ReadResupplyEngineConvar(b->engine_cvar,Convar::HealthVial,&o->value)))return 0;
    o->value_read=1;
    if(b->heal){if(!b->heal(b->context,player,o->value,0,&o->healed))return 0;}
    else{if(!player_take_health(player,o->value,0,b->native_heal,&o->native_heal))return 0;o->healed=o->native_heal.accepted;}
    o->heal_called=1;
    if(!o->healed){o->completed=1;return 1;}
    if(!b->item_pickup||!b->item_pickup(b->context,player,field<unsigned>(item,0xc)))return 0;
    o->message=1;
    if(!b->sound||!b->sound(b->context,player,field<unsigned>(item,0x16c),"HealthVial.Touch"))return 0;
    o->sound=1;
    if(b->respawn_rule){if(!b->respawn_rule(b->context,item,&o->rule))return 0;}
    else if(b->native_singleplayer_rules)o->rule=6;else return 0;
    o->rule_called=1;
    if(o->rule){if(!b->respawn||!b->respawn(b->context,item))return 0;o->respawned=1;}
    else{if(!b->remove||!b->remove(b->context,item))return 0;o->removed=1;}
    o->accepted=o->completed=1;return 1;
}
int __cdecl item_touch(void* item,unsigned bytes,void* other,unsigned other_bytes,const ItemTouchBindings* b,ItemTouchResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!item||bytes<0x4cc||!other||other_bytes<4||!b)return 0;
    const unsigned vt=field<unsigned>(item,0);
    bool is_vial=vt==vial_vtable,known=is_vial;
    if(!is_vial&&bytes>=0x4d8)for(const auto& p:ammo_pickups)if(vt==p.vtable){known=true;break;}
    if(!known)return 0;
    auto done=[&](){o->completed=1;return 1;};
    auto native_player=[](void* a){const auto v=field<unsigned>(a,0);return v==0x2245222cu||v==0x224aec3cu;};
    void* player=other;unsigned player_bytes=other_bytes;
    if(!native_player(other)){
        void* vehicle=nullptr;
        if(!b->vehicle||!b->vehicle(b->context,other,&vehicle))return 0;++o->vehicle_reads;
        if(vehicle){
            if(!b->vehicle(b->context,other,&vehicle)||!vehicle)return 0;++o->vehicle_reads;
            if(!b->passenger||!b->passenger(b->context,vehicle,0,&player))return 0;o->passenger_read=1;
            if(!player)return done();
            if(!b->extent||!b->extent(b->context,player,&player_bytes)||player_bytes<4)return 0;++o->extent_reads;
        }
    }
    o->player=player;bool player_ok=false;
    if(native_player(player)){player_ok=true;o->player_native=1;}
    else if(!b->is_player||!b->is_player(b->context,player,&player_ok))return 0;
    o->player_checked=1;
    if(!player_ok)return done();
    if(player_bytes<0xbc)return 0;
    if(field<unsigned>(player,0xb8)&0x20000000u){o->blocked=1;return done();}
    bool allow=false;
    if(b->can_pickup){if(!b->can_pickup(b->context,player,item,&allow))return 0;}
    else if(b->native_singleplayer_rules)allow=true;else return 0;
    o->permission_read=1;if(!allow)return done();
    o->subclass_called=1;
    if(is_vial){
        if(!vial_touch(item,bytes,player,b->vial,&o->vial))return 0;o->accepted=o->vial.accepted;
    }else{
        if(!ammo_pickup_touch(item,bytes,player,b->ammo,&o->ammo))return 0;o->accepted=o->ammo.accepted;
    }
    // Subclass removal requests must be deferred: original still uses item.
    if(!o->accepted){
        bool force=false;
        if(!b->force_failed_pickup||!b->force_failed_pickup(b->context,&force))return 0;o->force_read=1;
        if(!force)return done();
    }else{
        using namespace native_weapon_tables;
        const auto* api=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
        if(!api)return 0;ServerEntityVoidOutputResult result{};
        // Unconnected output has no callbacks to invoke.
        ServerEntityVoidOutputBindings empty{};
        if(!api->fire(static_cast<unsigned char*>(item)+0x49c,bytes-0x49c,player,item,bytes,0.f,b->output?b->output:&empty,&result))return 0;
        o->output_called=1;o->output_actions=result.queued;
        set_field(item,0x7c,0u);o->touch_cleared=1;
        set_field(item,8,0u);o->think_cleared=1;
        if(b->picked_up){if(!b->picked_up(b->context,player,item))return 0;}
        else if(!b->native_singleplayer_rules)return 0;o->picked_up=1;
        if(b->respawn_rule){if(!b->respawn_rule(b->context,item,&o->rule))return 0;}
    else if(b->native_singleplayer_rules)o->rule=6;else return 0;o->rule_called=1;
        if(o->rule==5){
            if(!b->respawn||!b->respawn(b->context,item))return 0;o->respawned=1;return done();
        }
    }
    if(!b->remove||!b->remove(b->context,item))return 0;o->removed=1;return done();
}
unsigned pickup_type(unsigned vt) noexcept {
    if(vt==vial_vtable)return count;
    for(unsigned i=0;i<count;++i)if(vt==ammo_pickups[i].vtable)return i;
    return count+1;
}
int __cdecl pickup_factory_create(const char* name,const PickupFactoryBindings* b,PickupFactoryResult* o) noexcept {
    if(!o)return 0;*o={};o->type=count+1;
    if(!name||!b||!b->allocate||!b->initialize)return 0;
    if(class_equal(name,health_classname))o->type=count;
    else for(unsigned i=0;i<count;++i)if(class_equal(name,ammo_types[i].classname)){o->type=i;break;}
    if(o->type>count)return 0;
    const bool vial=o->type==count;
    if(vial?!b->vial:!b->ammo)return 0;
    const unsigned extent=pickup_factory_specs[o->type].extent;
    o->allocation_called=1;
    if(!b->allocate(b->context,extent,&o->actor,&o->bytes)||!o->actor)return 0;
    o->allocated=1;
    if(o->bytes<extent)return 0;
    if(vial){if(!vial_construct(o->actor,o->bytes,b->vial,&o->vial))return 0;}
    else if(!ammo_pickup_construct(o->actor,o->bytes,o->type,b->ammo,&o->ammo))return 0;
    o->constructed=1;
    if(!b->initialize(b->context,o->actor,name))return 0;
    o->initialized=1;o->networkable=static_cast<unsigned char*>(o->actor)+0x160;
    o->completed=1;return 1;
}
int __cdecl pickup_factory_destroy(void* actor,unsigned bytes,void* networkable,const PickupFactoryBindings* b,PickupFactoryResult* o) noexcept {
    if(!o)return 0;*o={};o->type=count+1;
    if(!networkable){o->completed=1;return 1;}
    if(!actor||bytes<4||networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    o->type=pickup_type(field<unsigned>(actor,0));
    if(o->type>count||bytes<pickup_factory_specs[o->type].extent)return 0;
    o->actor=actor;o->bytes=bytes;
    if(!b->release_networkable(b->context,networkable))return 0;
    o->released=o->completed=1;return 1;
}
bool antigen_ready(void* a,unsigned n,const AntigenBindings* b) noexcept {
    return a&&n>=antigen_extent&&field<unsigned>(a,0)==antigen_vtable&&b&&
        b->entry_count<=sin_independent::kEntityHandleCapacity&&(!b->entry_count||b->entries);
}
bool antigen_clock(const AntigenBindings& b,AntigenResult& o,float& now) noexcept {
    if(!b.clock||!b.clock(b.context,&now)||!std::isfinite(now))return false;
    ++o.clock_reads;return true;
}
bool antigen_pose(void* a,const AntigenBindings& b,AntigenResult& o) noexcept {
    o.remaining=field<float>(a,0x49c);o.accumulator=field<float>(a,0x4a4);
    const double v=double(o.remaining)*double(.01f);
    o.pose=float(v>1.?1.:v<=0.?0.:v);
    if(!b.pose||!b.pose(b.context,a,"percentFull",o.pose))return false;o.pose_called=1;
    if(!b.advance||!b.advance(b.context,a))return false;o.advanced=1;return true;
}
bool antigen_stop_steam(void* a,const AntigenBindings& b,AntigenResult& o) noexcept {
    const sin_independent::EntityHandleView registry(b.entries,b.entry_count);
    if(void* steam=registry.Resolve(field<unsigned>(a,0x4b0))){
        if(!b.remove||!b.remove(b.context,steam))return false;o.steam_removed=1;
        set_field(a,0x4b0,0xffffffffu);
        if(!b.sound||!b.sound(b.context,a,"dispenser.use_loop",false))return false;o.loop_stopped=1;
        if(!b.sound(b.context,a,"dispenser.stop",true))return false;o.stop_sound=1;
    }
    return true;
}
bool antigen_stop(void* a,const AntigenBindings& b,AntigenResult& o) noexcept {
    if(!antigen_stop_steam(a,b,o))return false;
    set_field(a,0x4a0,0.f);set_field(a,0x4ac,0xffffffffu);o.stopped=1;
    // Original stop path does not clear the primary think identity.
    return antigen_pose(a,b,o);
}
int __cdecl antigen_construct(void* a,unsigned n,const AntigenBindings* b,AntigenResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<antigen_extent||!b||!b->base_construct||!b->base_construct(b->context,a))return 0;
    set_field(a,0,antigen_vtable);set_field(a,0x494,0x2246aa5cu);
    set_field(a,0x498,static_cast<unsigned char>(1));set_field(a,0x49c,100.f);
    for(unsigned off=0x4a0;off<=0x4a8;off+=4)set_field(a,off,0u);
    for(unsigned off=0x4ac;off<=0x4b4;off+=4)set_field(a,off,0xffffffffu);
    // The two CBaseEntityOutput constructors initialize only these words.
    for(unsigned off=0x4b8u;off<=0x4d0u;off+=0x18u){
        set_field(a,off,0u);set_field(a,off+0xc,0xffffffffu);set_field(a,off+0x10,0u);
    }
    o->constructed=o->loaded=o->completed=1;o->remaining=100.f;return 1;
}
int __cdecl antigen_use(void* a,unsigned n,void* player,unsigned pn,const AntigenBindings* b,AntigenResult* o) noexcept {
    if(!o)return 0;*o={};if(!antigen_ready(a,n,b))return 0;
    auto done=[&](){o->completed=1;return 1;};
    o->loaded=field<unsigned char>(a,0x498)!=0;if(!o->loaded||!player)return done();
    if(pn<0x37c)return 0;
    const auto vt=field<unsigned>(player,0);o->player_checked=1;
    if(vt!=0x2245222cu&&vt!=0x224aec3cu)return done();
    if(field<int>(player,0x9c)>=field<int>(player,0x98))return done();
    bool near=false;
    if(!b->use_distance||!b->use_distance(b->context,a,player,80.f,&near))return 0;o->spatial_checked=1;
    if(!near)return done();
    o->remaining=field<float>(a,0x49c);if(!std::isfinite(o->remaining))return 0;
    if(o->remaining<=0.f){if(!b->sound||!b->sound(b->context,a,"dispenser.empty",true))return 0;o->empty=1;return done();}
    const sin_independent::EntityHandleView registry(b->entries,b->entry_count);
    const unsigned handle=field<unsigned>(player,0x378);
    if(registry.Resolve(handle)!=player)return 0;
    float now=0;if(!antigen_clock(*b,*o,now))return 0;
    set_field(a,0x4a0,now);set_field(a,0x4ac,handle);
    if(!b->sound||!b->sound(b->context,a,"dispenser.use_loop",true))return 0;o->loop_started=1;
    if(!registry.Resolve(field<unsigned>(a,0x4b0))){
        unsigned steam=0xffffffffu;
        if(!b->create_steam||!b->create_steam(b->context,a,&steam))return 0;
        set_field(a,0x4b0,steam);o->steam_created=steam!=0xffffffffu;
        if(void* entity=registry.Resolve(steam)){
            if(!b->configure_steam||!b->configure_steam(b->context,a,entity))return 0;o->steam_configured=1;
        }
    }
    set_field(a,8,antigen_think_token);
    if(!antigen_clock(*b,*o,o->next_time)||!b->schedule||!b->schedule(b->context,a,o->next_time))return 0;
    o->scheduled=o->started=1;return done();
}
int __cdecl antigen_think(void* a,unsigned n,const AntigenBindings* b,AntigenResult* o) noexcept {
    if(!o)return 0;*o={};if(!antigen_ready(a,n,b))return 0;
    auto stop=[&](){if(!antigen_stop(a,*b,*o))return 0;o->completed=1;return 1;};
    const sin_independent::EntityHandleView registry(b->entries,b->entry_count);
    auto user=[&]()->void*{return registry.Resolve(field<unsigned>(a,0x4ac));};
    auto refresh=[&](void* entity){
        if(!(field<unsigned>(entity,0xb8)&0x800u))return true;
        if(!b->refresh_origin||!b->refresh_origin(b->context,entity))return false;++o->origin_refreshes;return true;
    };
    if(!user())return stop();
    if(!refresh(a))return 0;
    void* player=user();unsigned bytes=0;
    if(!player)return stop();
    if(!b->extent||!b->extent(b->context,player,&bytes)||bytes<0x2c8||!refresh(player))return 0;
    float diff[3];for(unsigned i=0;i<3;++i)diff[i]=float(double(field<float>(player,0x2bc+4*i))-double(field<float>(a,0x2bc+4*i)));
    o->distance_squared=(double(diff[2])*diff[2]+double(diff[1])*diff[1])+double(diff[0])*diff[0];
    if(!(o->distance_squared<6400.))return stop();
    float now=0;if(!antigen_clock(*b,*o,now))return 0;
    const float last=field<float>(a,0x4a0),remaining=field<float>(a,0x49c),acc=field<float>(a,0x4a4);
    if(!std::isfinite(last)||!std::isfinite(remaining)||!std::isfinite(acc))return 0;
    const double elapsed=double(now)-double(last),bounded=elapsed>.1f?double(.1f):elapsed;
    const double dose=bounded*7.;
    o->elapsed=float(bounded);o->dose=float(dose);
    set_field(a,0x4a0,now);
    const double left=double(remaining)-dose;set_field(a,0x49c,float(left<0.?0.:left));
    const double sum=double(acc)+dose;set_field(a,0x4a4,float(sum));
    if(sum>=1.){
        o->integer_dose=float(std::floor(sum));set_field(a,0x4a4,float(double(field<float>(a,0x4a4))-o->integer_dose));
        if(!(field<float>(a,0x49c)>0.f))return stop();
        player=user();if(!player)return stop();
        if(!player_take_health(player,o->integer_dose,0x400000u,b->health,&o->health))return 0;
        o->healed=o->health.accepted;if(!o->healed)return stop();
    }
    if(!antigen_clock(*b,*o,now))return 0;
    o->next_time=float(double(now)+double(.1f));
    if(!b->schedule||!b->schedule(b->context,a,o->next_time))return 0;o->scheduled=1;
    if(!antigen_pose(a,*b,*o))return 0;o->completed=1;return 1;
}
bool antigen_output(void* actor,unsigned bytes,unsigned offset,void* container,const AntigenExchangeBindings& b,AntigenExchangeResult& o) noexcept {
    using namespace native_weapon_tables;
    const auto* api=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
    ServerEntityVoidOutputBindings empty{};ServerEntityVoidOutputResult result{};
    if(!api||!api->fire(static_cast<unsigned char*>(actor)+offset,bytes-offset,container,actor,bytes,0.f,b.output?b.output:&empty,&result))return false;
    o.output_called=1;o.output_actions=result.queued;return true;
}
int __cdecl antigen_insert(void* a,unsigned n,void* other,unsigned other_bytes,const AntigenExchangeBindings* b,AntigenExchangeResult* o) noexcept {
    if(!o)return 0;*o={};if(!b||!antigen_ready(a,n,&b->control))return 0;
    auto done=[&](){o->completed=1;return 1;};
    if(!other)return done();
    unsigned extent=other_bytes;
    if(other_bytes>=0x6a0&&field<unsigned>(other,0)==antigen_canister_vtable){o->container=other;o->cast_native=1;}
    else{
        if(!b->container_cast||!b->container_cast(b->context,other,&o->container,&extent))return 0;
        o->cast_called=1;
    }
    if(!o->container)return done();
    if(extent<0x630)return 0;
    void* container=o->container;
    if(!(field<unsigned char>(container,0x62c)&8u)||field<unsigned char>(a,0x498))return done();
    o->eligible=1;
    float now=0;if(!antigen_clock(b->control,o->control,now))return 0;
    const float rejected=field<float>(a,0x4a8);
    if(!std::isfinite(rejected))return 0;
    if(double(rejected)+.5>=double(now)){
        set_field(a,0x4a8,now);o->reject_time=now;o->rejected=1;return done();
    }
    o->amount=float(double(field<float>(container,0x628))*100.*double(.01f));
    if(!std::isfinite(o->amount))return 0;
    set_field(a,0x49c,o->amount);o->transferred=1;
    if(!b->bodygroup||!b->bodygroup(b->context,a,1,1))return 0;o->bodygroup=1;
    if(!antigen_pose(a,b->control,o->control))return 0;
    set_field(a,0x498,static_cast<unsigned char>(1));
    void* local=nullptr;
    if(!b->local_player||!b->local_player(b->context,&local)||!local||!b->insert_notice||!b->insert_notice(b->context,local,container))return 0;o->notice=1;
    if(!b->control.remove||!b->control.remove(b->control.context,container))return 0;o->removed=1;
    if(!b->control.sound||!b->control.sound(b->control.context,a,"dispenser.load",true))return 0;o->sound=1;
    if(!antigen_output(a,n,0x4b8,container,*b,*o))return 0;
    return done();
}
int __cdecl antigen_eject(void* a,unsigned n,const AntigenExchangeBindings* b,AntigenExchangeResult* o) noexcept {
    if(!o)return 0;*o={};if(!b||!antigen_ready(a,n,&b->control))return 0;
    auto done=[&](){o->completed=1;return 1;};
    if(!field<unsigned char>(a,0x498))return done();
    unsigned extent=0;
    if(!b->create||!b->create(b->context,"prop_antigen_explosive_barrel",-1,&o->container,&extent))return 0;
    if(!o->container||extent<0x6a0||field<unsigned>(o->container,0)!=antigen_canister_vtable)return 0;
    void* c=o->container;o->created=1;
    auto refresh=[&](void* entity){
        if(!(field<unsigned>(entity,0xb8)&0x800u))return true;
        if(!b->control.refresh_origin||!b->control.refresh_origin(b->control.context,entity))return false;
        ++o->control.origin_refreshes;return true;
    };
    float pos[3],angles[3];
    if(!refresh(a))return 0;copy_vec(a,0x2bc,pos);
    if(!b->position||!b->position(b->context,c,pos)||!refresh(a))return 0;
    copy_vec(a,0x354,angles);
    if(!b->orientation||!b->orientation(b->context,c,angles)||!b->spawn||!b->spawn(b->context,c))return 0;o->spawned=1;
    o->amount=float(double(field<float>(a,0x49c))*100.*double(.01f));if(!std::isfinite(o->amount))return 0;
    set_field(c,0x628,o->amount);o->transferred=1;
    o->container_fraction=float(double(o->amount)*double(.01f));
    if(!b->container_pose||!b->container_pose(b->context,c,"percentFull",o->container_fraction))return 0;o->container_pose=1;
    void* local=nullptr;
    if(!b->local_player||!b->local_player(b->context,&local)||!local||!refresh(c))return 0;
    copy_vec(c,0x2bc,pos);
    if(!b->eject_notice||!b->eject_notice(b->context,local,c,pos))return 0;o->notice=1;
    float now=0;if(!antigen_clock(b->control,o->control,now))return 0;
    set_field(a,0x4a0,now);set_field(a,0x498,static_cast<unsigned char>(0));set_field(a,0x49c,0.f);
    // Eject only calls 222f6eb0, not the full healing-think stop tail.
    // User and fractional accumulator deliberately survive ejection.
    if(!antigen_stop_steam(a,b->control,o->control))return 0;
    if(!b->entity_message||!b->entity_message(b->context,a,1))return 0;o->message=1;
    if(!b->bodygroup||!b->bodygroup(b->context,a,1,0))return 0;o->bodygroup=1;
    if(!b->control.sound||!b->control.sound(b->control.context,a,"dispenser.eject",true))return 0;o->sound=1;
    set_field(a,8,0u);o->think_cleared=1;
    if(!antigen_output(a,n,0x4d0,c,*b,*o))return 0;
    if(!antigen_clock(b->control,o->control,o->reject_time))return 0;set_field(a,0x4a8,o->reject_time);
    return done();
}
const Api api{1,sizeof(Api),&ratios,&health,&greatest,&fallback,&nearby,&evaluate,&owns,&construct,&spawn,&activate,&become_master,&kill,&check_pvs,&calculate_type,&factory_create,&factory_destroy,&ammo_pickup_construct,&ammo_pickup_touch,&vial_construct,&vial_touch,&item_touch,&player_give_ammo,&player_take_health,&pickup_factory_create,&pickup_factory_destroy,&antigen_construct,&antigen_use,&antigen_think,&antigen_insert,&antigen_eject};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerDynamicResupplyApi=_SiN_GetServerDynamicResupplyApi")
extern "C" __declspec(dllexport) const sin_port::native_resupply::Api* __cdecl
SiN_GetServerDynamicResupplyApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_resupply;
    return version==1&&size==sizeof(Api)?&api:nullptr;
}
