#include "server_reciprocal_contact_core.h"
#include <array>
#include <cstdio>
#include <initializer_list>

class CBaseEntity { public: unsigned flags=0; };
using namespace sin_independent;
struct Trace {
    ContactNode* expectedNode;
    ContactNode* root;
    CBaseEntity* departing;
    CBaseEntity* partner;
    ContactCleanupState* state;
    unsigned end=0, flags=0, freed=0, destroyed=0;
    bool valid=true;
};
static void End(CBaseEntity* target,CBaseEntity* other,void* ctx) noexcept {
    auto& t=*static_cast<Trace*>(ctx);
    ++t.end;
    if(target!=t.partner || other!=t.departing || t.freed ||
        t.expectedNode->previous->next!=t.expectedNode ||
        t.expectedNode->next->previous!=t.expectedNode) t.valid=false;
}
static void Flags(CBaseEntity* e,std::uint32_t mask,void* ctx) noexcept {
    auto& t=*static_cast<Trace*>(ctx);
    ++t.flags;
    if(e!=t.departing || mask!=1 || t.freed ||
        t.expectedNode->previous->next!=t.expectedNode ||
        t.expectedNode->next->previous!=t.expectedNode) t.valid=false;
    e->flags &= ~mask;
}
static void Free(ContactNode* n,ContactKind kind,void* ctx) noexcept {
    auto& t=*static_cast<Trace*>(ctx);
    ++t.freed;
    if(n!=t.expectedNode || n->previous->next!=n->next ||
        n->next->previous!=n->previous ||
        (kind==ContactKind::Touch ? t.state->touchAllocated : t.state->groundAllocated)!=0)
        t.valid=false;
    delete n;
}
static void Destroy(CBaseEntity* e,ContactNode* n,ContactKind,void* ctx) noexcept {
    auto& t=*static_cast<Trace*>(ctx);
    if(e!=t.partner || n!=t.root || t.freed!=1 ||
        n->next!=n || n->previous!=n) t.valid=false;
    ++t.destroyed;
}
static int fail(int code,const char* msg) {
    std::fprintf(stderr,"FAIL reciprocal contact %d: %s\n",code,msg);
    return code;
}
int main() {
    CBaseEntity a{3}, b{};
    std::array<EntityHandleEntry,3> entries{};
    entries[1].entity=&a;
    entries[1].serial=3;
    EntityHandleView view(entries.data(),entries.size());
    for(auto kind:{ContactKind::Touch,ContactKind::Ground}) {
        for(bool suppress:{false,true}) {
            for(bool started:{false,true}) {
                ContactNode root{};
                auto* n=new ContactNode{0x3001u,&root,&root,started?1u:0u};
                root.next=root.previous=n;
                ContactList list{&root,true};
                ContactCleanupState state{
                    kind==ContactKind::Touch?1u:0u,
                    kind==ContactKind::Ground?1u:0u,!suppress};
                a.flags=3;
                Trace t{n,&root,&a,&b,&state};
                ReciprocalHooks hooks{&End,&Flags,&Free,&Destroy,&t};
                // Invalid stale handle leaves both nodes and callbacks untouched.
                n->handle=0x2001u;
                if(RemoveReciprocalContact(&a,&b,kind,list,state,view,hooks)
                    !=ReciprocalResult::NoContact || t.freed || t.end || t.flags)
                    return fail(1,"stale handle");
                n->handle=0x3001u;
                n->previous=nullptr;
                if(RemoveReciprocalContact(&a,&b,kind,list,state,view,hooks)
                    !=ReciprocalResult::InvalidList || t.freed)
                    return fail(2,"broken previous link");
                n->previous=&root;
                if(RemoveReciprocalContact(&a,&a,kind,list,state,view,hooks)
                    !=ReciprocalResult::InvalidInput || t.freed)
                    return fail(3,"self contact");
                if(RemoveReciprocalContact(&a,&b,kind,list,state,view,{})
                    !=ReciprocalResult::InvalidInput || t.freed)
                    return fail(4,"incomplete hook set");
                if(RemoveReciprocalContact(&a,&b,kind,list,state,view,hooks)
                    !=ReciprocalResult::Removed)
                    return fail(5,"remove matching node");
                const bool touch=kind==ContactKind::Touch;
                const bool destroys=!touch || !suppress;
                if(!t.valid || t.freed!=1 || t.end!=(touch&&started?1u:0u) ||
                    t.flags!=(touch?0u:1u) || a.flags!=(touch?3u:2u) ||
                    t.destroyed!=(destroys?1u:0u) ||
                    list.present==destroys || list.root!=(destroys?nullptr:&root) ||
                    root.next!=&root || root.previous!=&root ||
                    state.touchAllocated || state.groundAllocated ||
                    state.cleanupDataObjects==suppress)
                    return fail(6,"callback direction, flags, unlink or root policy");
                if(RemoveReciprocalContact(&a,&b,kind,list,state,view,hooks)
                    !=ReciprocalResult::NoContact || t.freed!=1)
                    return fail(7,"repeat removal");
            }
        }
    }
    std::puts("PASS reciprocal contact lookup, EndTouch gate, direct ground flag removal, unlink, suppression");
    return 0;
}
