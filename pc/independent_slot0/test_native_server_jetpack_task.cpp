#include "native_server_jetpack_task.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0x1028]{},desc[0x48]{},attachment[4]{},enemy[4]{};
    sin_independent::EntityHandleEntry slots[3]{};
    std::vector<int> trace;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerJetpackTaskApi>(GetProcAddress(s,server_jetpack_task_export));
    assert(g&&!GetProcAddress(c,server_jetpack_task_export)&&!g(0,sizeof(ServerJetpackTaskApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerJetpackTaskApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.attachment);
    f.slots[2].serial=8;f.slots[2].entity=reinterpret_cast<CBaseEntity*>(f.enemy);
    f.put(f.actor,0x1024,0x7001u);f.put(f.actor,0x8a0,0x8002u);
    ServerJetpackTaskBindings b{};b.context=&f;b.grunt.context=&f;b.grunt.entities={f.slots,3,nullptr,nullptr,nullptr};
    b.grunt.lifecycle.conditions.context=&f;
    b.grunt.lifecycle.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.grunt.parent=[](void* p,void*,const ServerNpcScheduleTask* t,bool) noexcept -> int{F::self(p).trace.push_back(t->id);return 1;};
    b.grunt.turn_update=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(67);return 1;};
    b.special=[](void* p,void*,const ServerNpcScheduleTask* t) noexcept -> int{F::self(p).trace.push_back(t->id);return 1;};
    b.attachment_task=[](void* p,void* a,void* e,const ServerNpcScheduleTask* t) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&e==f.attachment);f.trace.push_back(t->id);return 1;};
    b.face_enemy=[](void* p,void*,void* attachment,void* enemy) noexcept -> int{auto& f=F::self(p);assert(attachment==f.attachment&&enemy==f.enemy);f.trace.push_back(56);return 1;};
    b.inspect_clear=[](void* p,void*,float delay) noexcept -> int{auto& f=F::self(p);assert(delay==5.f&&f.get<unsigned>(0x8a8)==0xffffffffu);f.trace.push_back(165);return 1;};
    b.default_inspect[0]=1;b.default_inspect[1]=2;b.default_inspect[2]=3;
    unsigned native=0,host=0,attachment=0,route=0;
    for(int id=0;id<200;++id){assert(api->route(id,false,&route));native+=route==JetpackTaskNative;host+=route==JetpackTaskHosted;attachment+=route==JetpackTaskAttachment;}
    assert(native==7&&host==2&&attachment==4);
    ServerJetpackTaskResult out{};
    auto execute=[&](int id,bool run=false){f.put(f.actor,0x7a4,1);f.trace.clear();ServerNpcScheduleTask t{id,0};assert(api->execute(f.actor,sizeof(f.actor),&t,run,10,&b,&out));};
    execute(160);assert(out.lifecycle.completed&&out.mode_changed&&f.get<int>(0xfec)==2&&f.get<float>(0x9f0)==0.f);
    execute(159);assert(out.lifecycle.completed&&out.mode_changed&&f.get<int>(0xfec)==0);
    execute(163);assert(!out.lifecycle.completed&&!out.timer_written&&f.trace.empty());
    execute(164);assert(out.timer_written&&!out.lifecycle.completed&&f.get<float>(0x9dc)==10.1f);
    f.put(f.actor,0x8a8,0x8002u);execute(165);
    assert(out.target_cleared&&out.lifecycle.completed&&f.get<float>(0x100c)==1.f&&f.get<float>(0x1010)==2.f&&f.get<float>(0x1014)==3.f);
    execute(56);assert(out.enemy_live&&out.attachment_live&&out.lifecycle.completed&&f.trace==std::vector<int>{56});
    f.put(f.actor,0x8a0,0x9002u);execute(60);assert(!out.enemy_live&&out.lifecycle.completed&&f.trace.empty());
    execute(43);assert(out.route==JetpackTaskAttachment&&out.attachment_live&&f.trace==std::vector<int>{43}&&f.get<float>(0x9f0)==0.f);
    execute(166);assert(out.route==JetpackTaskHosted&&f.trace==std::vector<int>{166});
    execute(67,true);assert(out.lifecycle.completed&&f.trace==std::vector<int>{67});
    f.put(f.actor,0x1024,0x8001u);ServerNpcScheduleTask task{43,0};
    assert(!api->execute(f.actor,sizeof(f.actor),&task,false,10,&b,&out)&&!out.attachment_live);
    execute(11);assert(out.delegated&&out.grunt.parent_called&&f.get<float>(0x9f0)==.75f&&f.trace==std::vector<int>{11});
    execute(147,true);assert(out.delegated&&out.grunt.lifecycle.completed);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native Jetpack task override, attachment routing and timer");
}
