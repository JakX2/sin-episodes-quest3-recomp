#include "server_gameents_touch_entry_core.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

using namespace sin_independent;
struct Entity {
    int id;
    bool dirty;
    float origin[3];
};
struct Edict {
    Entity* pointer;
    bool bad=false;
};
struct Fixture {
    Entity a{1,true,{2.f,4.f,6.f}},b{2,true,{8.f,10.f,12.f}};
    Edict ea{&a},eb{&b};
    std::string calls;
    bool bad_init=false,bad_second=false,bad_first=false,bad_mark=false;
    GameEntsTouchTrace seen{};
};
static bool Resolve(void* p,const void* e,GameEntsTouchEntity* out) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    const Edict* edict=static_cast<const Edict*>(e);
    f.calls+=edict==&f.ea?'A':'B';
    if(edict->bad)return false;
    if(edict->pointer) {
        auto& entity=*edict->pointer;
        out->opaque=&entity;
        out->dirty_origin=entity.dirty;
        for(int i=0;i<3;++i)out->origin[i]=entity.origin[i];
    }
    return true;
}
static bool Init(void* p,GameEntsTouchTrace* trace) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='I';
    if(f.bad_init)return false;
    trace->WriteFloat(0x2c,1.0f);
    // Distinct non-midpoint field from Source-owned global defaults.
    trace->WriteFloat(0x3c,123.0f);
    return true;
}
static bool Refresh(void* p,GameEntsTouchEntity* entity) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    const auto* original=static_cast<Entity*>(entity->opaque);
    const bool is_first=original==&f.a;
    assert(is_first||original==&f.b);
    f.calls+=is_first?'1':'2';
    if((is_first&&f.bad_first)||(!is_first&&f.bad_second))return false;
    entity->origin[0]=is_first?4.f:12.f;
    entity->origin[1]=is_first?6.f:14.f;
    entity->origin[2]=is_first?8.f:16.f;
    entity->dirty_origin=false;
    return true;
}
static bool Mark(void* p,void* a,void* b,const GameEntsTouchTrace* trace) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='M';
    assert(a==&f.a&&b==&f.b);
    assert(trace->ReadFloat(0x2c)==1.f);
    assert(trace->ReadFloat(0x3c)==123.f);
    f.seen=*trace;
    return !f.bad_mark;
}
static GameEntsTouchEntryHooks Hooks(Fixture& f) noexcept {
    return {&f,&Resolve,&Init,&Refresh,&Mark};
}
static int fail(int code,const char* why) {
    std::fprintf(stderr,"FAILED gameents original touch wrapper %d: %s\n",code,why);
    return code;
}
int main() {
    GameEntsTouchEntryResult out{};
    Fixture f;
    auto run=[&](){f.calls.clear();return MarkEntitiesAsTouchingEntry(&f.ea,&f.eb,Hooks(f),out);};
    if(run()!=GameEntsTouchEntryStatus::Completed || f.calls!="ABI21M" ||
       out.resolved!=2||out.trace_initialized!=1||out.second_refreshes!=1||
       out.first_refreshes!=1||out.mark_calls!=1||out.midpoint[0]!=8.f||
       out.midpoint[1]!=10.f||out.midpoint[2]!=12.f||
       out.trace.ReadFloat(0x0c)!=8.f||out.trace.ReadFloat(0x10)!=10.f||
       out.trace.ReadFloat(0x14)!=12.f||out.trace.ReadFloat(0x3c)!=123.f)
        return fail(1,"edict resolution, trace init, second-first dirty refresh, trace midpoint");
    f.a.dirty=f.b.dirty=false;
    if(run()!=GameEntsTouchEntryStatus::Completed || f.calls!="ABIM" ||
       out.second_refreshes||out.first_refreshes||out.midpoint[0]!=5.f||
       out.midpoint[1]!=7.f||out.midpoint[2]!=9.f)
        return fail(2,"clean origins do not invoke refresh, exact midpoint");
    f.ea.pointer=nullptr;
    if(run()!=GameEntsTouchEntryStatus::NoEntities||f.calls!="AB"||out.trace_initialized||out.mark_calls)
        return fail(3,"null first underlying entity short-circuits after resolving second");
    f.ea.pointer=&f.a;f.eb.pointer=nullptr;
    if(run()!=GameEntsTouchEntryStatus::NoEntities||f.calls!="AB"||out.trace_initialized)
        return fail(4,"null second underlying entity short-circuits");
    f.eb.pointer=&f.b;
    f.bad_init=true;
    if(run()!=GameEntsTouchEntryStatus::HostFailure||f.calls!="ABI"||out.mark_calls)
        return fail(5,"trace initializer failure");
    f.bad_init=false;f.a.dirty=f.b.dirty=true;
    f.bad_second=true;
    if(run()!=GameEntsTouchEntryStatus::HostFailure||f.calls!="ABI2"||
       out.second_refreshes!=1||out.first_refreshes||out.mark_calls)
        return fail(6,"failed second refresh must not refresh first");
    f.bad_second=false;f.bad_first=true;
    if(run()!=GameEntsTouchEntryStatus::HostFailure||f.calls!="ABI21"||
       out.second_refreshes!=1||out.first_refreshes!=1||out.mark_calls)
        return fail(7,"failed first refresh must not dispatch");
    f.bad_first=false;f.bad_mark=true;
    if(run()!=GameEntsTouchEntryStatus::HostFailure||f.calls!="ABI21M"||out.mark_calls!=1)
        return fail(8,"host contact service failure preserves earlier effects");
    f.bad_mark=false;f.a.dirty=f.b.dirty=false;
    f.a.origin[0]=std::numeric_limits<float>::quiet_NaN();
    if(run()!=GameEntsTouchEntryStatus::InvalidOrigin||f.calls!="ABI"||out.mark_calls)
        return fail(9,"safe finite-origin validation at independent boundary");
    f.a.origin[0]=2.f;
    f.ea.bad=true;
    if(run()!=GameEntsTouchEntryStatus::HostFailure||f.calls!="A"||out.trace_initialized)
        return fail(10,"edict conversion failure before trace initialization");
    f.ea.bad=false;
    if(MarkEntitiesAsTouchingEntry(&f.ea,&f.eb,{},out)!=GameEntsTouchEntryStatus::InvalidInput)
        return fail(11,"missing host service rejected");
    // Original 221e93e6 refreshes the second actor, then rereads the
    // FIRST actor's +b8 dirty flag at 221e93eb. When both edicts name
    // the same actor, only one refresh is required if it clears +b8.
    struct Alias {
        Entity actor{7,true,{1.f,2.f,3.f}};
        Edict ea{&actor},eb{&actor};
        unsigned refreshes=0,live_reads=0,marks=0;
        bool clear_dirty_in_init=false;
        std::string calls;
    } alias;
    GameEntsTouchEntryHooks alias_hooks{
        &alias,
        [](void* p,const void* edict,GameEntsTouchEntity* view) noexcept ->bool {
            auto& x=*static_cast<Alias*>(p);
            if(edict!=&x.ea&&edict!=&x.eb)return false;
            x.calls+=edict==&x.ea?'A':'B';
            view->opaque=&x.actor;
            view->dirty_origin=x.actor.dirty;
            for(int i=0;i<3;++i)view->origin[i]=x.actor.origin[i];
            return true;
        },
        [](void* p,GameEntsTouchTrace* trace) noexcept ->bool {
            auto& x=*static_cast<Alias*>(p);x.calls+='I';
            if(x.clear_dirty_in_init){
                x.actor.dirty=false;
                x.actor.origin[0]=4.f;x.actor.origin[1]=6.f;
                x.actor.origin[2]=8.f;
            }
            trace->WriteFloat(0x2c,1.f);return true;
        },
        [](void* p,GameEntsTouchEntity* view) noexcept ->bool {
            auto& x=*static_cast<Alias*>(p);x.calls+='R';
            if(view->opaque!=&x.actor)return false;
            ++x.refreshes;
            x.actor.dirty=false;
            x.actor.origin[0]=4.f;x.actor.origin[1]=6.f;
            x.actor.origin[2]=8.f;
            view->dirty_origin=false;
            for(int i=0;i<3;++i)view->origin[i]=x.actor.origin[i];
            return true;
        },
        [](void* p,void* first,void* second,const GameEntsTouchTrace* trace) noexcept ->bool {
            auto& x=*static_cast<Alias*>(p);x.calls+='M';++x.marks;
            return first==&x.actor&&second==first&&
                trace->ReadFloat(0x0c)==4.f&&trace->ReadFloat(0x10)==6.f&&
                trace->ReadFloat(0x14)==8.f;
        },
        [](void* p,GameEntsTouchEntity* view) noexcept ->bool {
            auto& x=*static_cast<Alias*>(p);
            if(view->opaque!=&x.actor)return false;
            ++x.live_reads;
            view->dirty_origin=x.actor.dirty;
            for(int i=0;i<3;++i)view->origin[i]=x.actor.origin[i];
            return true;
        }
    };
    if(MarkEntitiesAsTouchingEntry(&alias.ea,&alias.eb,alias_hooks,out)
       !=GameEntsTouchEntryStatus::Completed ||
       alias.calls!="ABIRM"||alias.refreshes!=1||alias.live_reads!=4||
       out.second_refreshes!=1||out.first_refreshes||
       out.midpoint[0]!=4.f||out.midpoint[1]!=6.f||out.midpoint[2]!=8.f||
       alias.marks!=1)
        return fail(12,"same entity live +b8 reread avoids duplicate refresh");
    // Legacy callers without the extra live callback still detect exact
    // actor aliasing and inherit the refreshed second actor snapshot.
    alias.actor.dirty=true;
    alias.actor.origin[0]=1.f;alias.actor.origin[1]=2.f;alias.actor.origin[2]=3.f;
    alias.refreshes=alias.live_reads=alias.marks=0;alias.calls.clear();
    alias_hooks.sync_live_origin=nullptr;
    if(MarkEntitiesAsTouchingEntry(&alias.ea,&alias.eb,alias_hooks,out)
       !=GameEntsTouchEntryStatus::Completed||
       alias.calls!="ABIRM"||alias.refreshes!=1||alias.live_reads||
       out.second_refreshes!=1||out.first_refreshes||alias.marks!=1)
        return fail(13,"legacy same-actor alias protects against stale first snapshot");
    // Retail reads SECOND +b8 only AFTER the trace initializer.
    // If an owner initializer changes that live flag, stale snapshots
    // must not cause an unnecessary origin refresh.
    alias.actor.dirty=true;
    alias.actor.origin[0]=1.f;alias.actor.origin[1]=2.f;alias.actor.origin[2]=3.f;
    alias.refreshes=alias.live_reads=alias.marks=0;alias.calls.clear();
    alias.clear_dirty_in_init=true;
    alias_hooks.sync_live_origin=
        [](void* p,GameEntsTouchEntity* view) noexcept ->bool {
            auto& x=*static_cast<Alias*>(p);
            if(view->opaque!=&x.actor)return false;
            ++x.live_reads;
            view->dirty_origin=x.actor.dirty;
            for(int i=0;i<3;++i)view->origin[i]=x.actor.origin[i];
            return true;
        };
    if(MarkEntitiesAsTouchingEntry(&alias.ea,&alias.eb,alias_hooks,out)
       !=GameEntsTouchEntryStatus::Completed ||
       alias.calls!="ABIM"||alias.refreshes||alias.live_reads!=4||
       out.second_refreshes||out.first_refreshes||alias.marks!=1||
       out.midpoint[0]!=4.f||out.midpoint[1]!=6.f||out.midpoint[2]!=8.f)
        return fail(14,"trace initialization precedes live second-origin dirty check");
    std::puts("PASS original-gameents touch entry: 2-edict resolution, 0x54 trace, refresh order, midpoint, host failure boundaries");
    return 0;
}
