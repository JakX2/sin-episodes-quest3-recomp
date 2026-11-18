#include "server_save_system_lifecycle.h"
#include <cstdio>
#include <cstring>
using namespace sin_independent::server_systems;
struct History {
    int entries[128]{};
    unsigned count=0;
    bool valid=true;
    void Push(int x) noexcept {
        if(count==128){valid=false;return;}
        entries[count++]=x;
    }
};
class Block:public SaveBlockHandler {
public:
    const char* name="";
    const char* __thiscall Name() noexcept override{return name;}
    void __thiscall PreSave(void*) noexcept override{}
    void __thiscall Save(void*) noexcept override{}
    void __thiscall WriteHeaders(void*) noexcept override{}
    void __thiscall PostSave() noexcept override{}
    void __thiscall PreRestore() noexcept override{}
    void __thiscall ReadHeaders(void*) noexcept override{}
    void __thiscall Restore(void*,bool) noexcept override{}
    void __thiscall PostRestore() noexcept override{}
};
class Blocks:public SaveBlockSet {
public:
    History* log=nullptr;
    SaveRegistration* owner=nullptr;
    SaveHandlers expected{};
    unsigned added=0,removed=0;
    const char* __thiscall Name() noexcept override{return "Game";}
    void __thiscall PreSave(void*) noexcept override{log->valid=false;}
    void __thiscall Save(void*) noexcept override{log->valid=false;}
    void __thiscall WriteHeaders(void*) noexcept override{log->valid=false;}
    void __thiscall PostSave() noexcept override{log->valid=false;}
    void __thiscall PreRestore() noexcept override{log->valid=false;}
    void __thiscall ReadHeaders(void*) noexcept override{log->valid=false;}
    void __thiscall Restore(void*,bool) noexcept override{log->valid=false;}
    void __thiscall PostRestore() noexcept override{log->valid=false;}
    void __thiscall AddHandler(SaveBlockHandler* p) noexcept override {
        if(added>=5 || expected[added]!=p || owner->Remove() ||
           owner->Add(this,expected))log->valid=false;
        log->Push(100+added++);
    }
    void __thiscall RemoveHandler(SaveBlockHandler* p) noexcept override {
        if(removed>=5 || expected[4-removed]!=p || owner->Remove())log->valid=false;
        log->Push(200+4-removed++);
    }
    void __thiscall RestoreHandler(SaveBlockHandler*,int,void*,bool) noexcept override{log->valid=false;}
};
class System:public AutoGameSystem {
public:
    History* log=nullptr;
    GameSystemList<12>* owner=nullptr;
    System* late=nullptr;
    int id=0;
    bool accept=true;
    int* depth=nullptr;
    bool __thiscall Init() noexcept override {
        if(*depth || owner->State()!=SystemState::Initializing ||
           owner->Initialize(nullptr)!=InitResult::AlreadyAttempted)
            log->valid=false;
        log->Push(id);
        if(late && (!owner->AppendDuringInit(late) ||
                    owner->AppendDuringInit(late)))log->valid=false;
        return accept;
    }
    void __thiscall Shutdown() noexcept override {
        if(*depth!=1 || owner->State()!=SystemState::ShuttingDown)log->valid=false;
        log->Push(-id);
    }
    void __thiscall LevelInitPreEntity() noexcept override{}
    void __thiscall LevelInitPostEntity() noexcept override{}
    void __thiscall LevelShutdownPreEntity() noexcept override{}
    void __thiscall LevelShutdownPostEntity() noexcept override{}
    void __thiscall OnSave() noexcept override{}
    void __thiscall OnRestore() noexcept override{}
    void __thiscall FrameUpdatePreEntityThink() noexcept override{}
    void __thiscall FrameUpdatePostEntityThink() noexcept override{}
    void __thiscall PreClientUpdate() noexcept override{}
};
struct Cache {
    History* log;
    int depth=0;
    static void Enter(void* context) noexcept {
        auto& c=*static_cast<Cache*>(context);
        if(c.depth++)c.log->valid=false;
        c.log->Push(90);
    }
    static void Leave(void* context) noexcept {
        auto& c=*static_cast<Cache*>(context);
        if(--c.depth)c.log->valid=false;
        c.log->Push(91);
    }
    CacheHooks Hooks(){return {&Enter,&Leave,this};}
};
struct Fixture {
    History log;
    Cache cache{&log};
    GameSystemList<12> manager;
    System systems[9];
    ExplicitSystems explicitSystems{};
    Fixture() {
        for(int i=0;i<9;++i) {
            systems[i].id=i+1;systems[i].log=&log;
            systems[i].owner=&manager;systems[i].depth=&cache.depth;
            if(i<6)explicitSystems[i]=&systems[i];
        }
        systems[6].next=&systems[1]; // duplicate explicit
        systems[1].next=&systems[7];
        systems[0].late=&systems[8];
    }
};
static int fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL save and systems %d: %s\n",n,why);return n;
}
int main() {
    History saveLog;SaveRegistration saves;Blocks set;Block blocks[5];
    set.log=&saveLog;set.owner=&saves;
    const char* names[]={"Entities","Physics","AI","Templates","ResponseSystem"};
    for(unsigned i=0;i<5;++i){blocks[i].name=names[i];set.expected[i]=&blocks[i];}
    auto bad=set.expected;bad[1]=bad[0];
    if(saves.Add(nullptr,set.expected) || saves.Add(&set,bad) ||
       saves.Remove() || saveLog.count)return fail(1,"save validation");
    if(!saves.Add(&set,set.expected) || !saves.Active() ||
       saves.Add(&set,set.expected) || set.added!=5)return fail(2,"save registration");
    if(!saves.Remove() || saves.Active() || saves.Remove() ||
       !saveLog.valid || saveLog.count!=10 || set.removed!=5)
        return fail(3,"save removal");
    for(int i=0;i<5;++i)
        if(saveLog.entries[i]!=100+i || saveLog.entries[5+i]!=204-i)
            return fail(4,"save registration and reverse removal order");

    Fixture f;
    if(f.manager.Initialize(nullptr)!=InitResult::RejectedInput ||
       f.manager.AppendDuringInit(&f.systems[8]))return fail(5,"unregistered");
    auto duplicate=f.explicitSystems;duplicate[5]=duplicate[0];
    if(f.manager.AddExplicit(duplicate) || f.manager.Count() ||
       !f.manager.AddExplicit(f.explicitSystems) ||
       f.manager.AddExplicit(f.explicitSystems))return fail(6,"explicit list");
    f.systems[7].next=&f.systems[6];
    if(f.manager.Initialize(&f.systems[6])!=InitResult::RejectedInput ||
       f.manager.Count()!=6 || f.log.count || f.manager.State()!=SystemState::Pending)
        return fail(7,"cyclic auto preflight");
    f.systems[7].next=nullptr;
    if(f.manager.Initialize(&f.systems[6])!=InitResult::Ready ||
       !f.log.valid || f.manager.Count()!=9 || f.manager.InitializedCount()!=9 ||
       f.manager.DuplicateAutos()!=1 || f.cache.depth ||
       f.manager.At(9) || f.log.count!=9)return fail(8,"init and late append");
    for(int i=0;i<9;++i)
        if(f.log.entries[i]!=i+1 || f.manager.At(i)!=&f.systems[i])
            return fail(9,"original explicit/auto/late order");
    if(f.manager.Shutdown({}) || f.manager.AppendDuringInit(&f.systems[0]) ||
       !f.manager.Shutdown(f.cache.Hooks()) || f.manager.Shutdown(f.cache.Hooks()) ||
       !f.log.valid || f.manager.State()!=SystemState::Stopped ||
       f.cache.depth || f.log.count!=36)return fail(10,"shutdown lifecycle");
    for(int i=0;i<9;++i) {
        const unsigned j=9+3*i;
        if(f.log.entries[j]!=90 || f.log.entries[j+1]!=-(9-i) ||
           f.log.entries[j+2]!=91)return fail(11,"cache-wrapped reverse shutdown");
    }

    Fixture rejected;
    rejected.systems[2].accept=false;
    if(!rejected.manager.AddExplicit(rejected.explicitSystems) ||
       rejected.manager.Initialize(&rejected.systems[6])!=InitResult::CallbackFailed ||
       rejected.manager.State()!=SystemState::Failed ||
       rejected.manager.InitializedCount()!=2 || rejected.log.count!=3 ||
       rejected.manager.Initialize(nullptr)!=InitResult::AlreadyAttempted ||
       rejected.manager.Shutdown(rejected.cache.Hooks()) || !rejected.log.valid)
        return fail(12,"first failure with no automatic rollback");
    std::puts("PASS five save block registrations, six explicit system starts, auto deduplication, late append, failure and reverse cache-wrapped shutdown");
    return 0;
}
