/* Original-free host integration, compiled as genuine C11 on Linux/Win32. */
#include "sin_compat/grunt_native_host_c_api_v1.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

struct Mock {
    sin_grunt_native_registry_v1 *registry;
    uint32_t index, generation;
    uint64_t owner;
    const char *classname;
    int allow, live, candidate, reenter, fail;
    int confirmations, lifecycle_calls, reentry_destroy, reentry_step;
};
static int32_t confirm(void *u, const char *class_name,
                       sin_grunt_actor_v1 actor, uint64_t owner) {
    struct Mock *m=(struct Mock*)u;
    ++m->confirmations;
    if(m->reenter && m->registry) {
        int32_t selected=44,code=44;
        m->reentry_destroy=sin_grunt_native_destroy_v1(m->registry);
        m->reentry_step=sin_grunt_native_step_v1(
            m->registry,actor,3.0f,&selected,&code);
        assert(selected==-1 && code==SIN_GRUNT_BEHAVIOR_INVALID);
    }
    if(m->fail)return -1;
    return m->allow && actor.index==m->index &&
           actor.generation==m->generation && owner==m->owner &&
           strcmp(class_name,m->classname)==0;
}
static int32_t actor_live(void *u,sin_grunt_actor_v1 a) {
    struct Mock *m=(struct Mock*)u;
    return m->live && a.index==m->index && a.generation==m->generation;
}
static int32_t eligible(void *u,sin_grunt_actor_v1 a,uint32_t index) {
    struct Mock *m=(struct Mock*)u;
    return (a.index==m->index && m->candidate && index==0) ? 1 : 0;
}
static int32_t no_gate(void *u,sin_grunt_actor_v1 a) {
    (void)u;(void)a;return 0;
}
static int32_t no_condition(void *u,sin_grunt_actor_v1 a,uint32_t cond) {
    (void)u;(void)a;(void)cond;return 0;
}
static int32_t no_serial(void *u,uint32_t i,uint32_t g) {
    (void)u;(void)i;(void)g;return 0;
}
static int32_t no_byte(void *u,uint32_t handle,uint8_t *out) {
    (void)u;(void)handle;(void)out;return 0;
}
static int32_t yes_condition(void *u,sin_grunt_actor_v1 a,uint32_t cond) {
    (void)u;(void)a;(void)cond;return 1;
}
static int32_t no_entity(void *u,uint32_t encoded,uint64_t *out) {
    (void)u;(void)encoded;(void)out;return 0;
}
static int32_t read_link(void *u,uint64_t entity,uint32_t *out) {
    (void)u;(void)entity;*out=UINT32_MAX;return 1;
}
static int32_t write_link(void *u,uint64_t entity,uint32_t encoded) {
    (void)u;(void)entity;(void)encoded;return 1;
}
static int32_t lifecycle(void *u,sin_grunt_actor_v1 a,
                         uint32_t index,uint32_t stage) {
    struct Mock *m=(struct Mock*)u;
    (void)a;(void)index;(void)stage;
    ++m->lifecycle_calls;return 1;
}
static int32_t notify(void *u,sin_grunt_actor_v1 a,int32_t old,int32_t next) {
    struct Mock *m=(struct Mock*)u;
    (void)a;(void)old;(void)next;
    ++m->lifecycle_calls;return 1;
}
static sin_grunt_behavior_host_v1 behavior_callbacks(void) {
    sin_grunt_behavior_host_v1 h={
        SIN_GRUNT_BEHAVIOR_ABI_V1,sizeof(sin_grunt_behavior_host_v1),
        actor_live,eligible,no_gate,no_condition,no_gate,no_serial,no_byte,
        yes_condition,no_entity,read_link,write_link,lifecycle,notify};
    return h;
}
int main(void) {
    struct Mock m={0};
    sin_grunt_native_host_v1 host={
        SIN_GRUNT_NATIVE_ABI_V1,sizeof(sin_grunt_native_host_v1),confirm};
    sin_grunt_native_attachment_v1 a={0};
    sin_grunt_snapshot_v1 snap={0};
    sin_grunt_actor_v1 actor={33u,9u},wrong={33u,8u};
    sin_grunt_native_registry_v1 *registry;
    int32_t selected=555,underlying=555, callbacks_before;

    assert(sin_grunt_native_abi_version_v1()==SIN_GRUNT_NATIVE_ABI_V1);
    host.version=2;
    assert(sin_grunt_native_create_v1(&host,&m)==0);
    host.version=1;host.struct_size--;
    assert(sin_grunt_native_create_v1(&host,&m)==0);
    host.struct_size=sizeof(host);
    assert(sin_grunt_native_destroy_v1(0)==SIN_GRUNT_NATIVE_INVALID_ARGUMENT);

    registry=sin_grunt_native_create_v1(&host,&m);
    assert(registry!=0);
    m.registry=registry;m.index=33u;m.generation=9u;m.owner=1007u;
    m.allow=1;m.live=1;m.candidate=1;
    m.classname="npc_druglab_grunt_scattergun";
    a.version=SIN_GRUNT_NATIVE_ABI_V1;
    a.struct_size=sizeof(a);
    a.original_classname="npc_druglab_grunt_scattergun";
    a.actor=actor;
    a.owner_entity_token=m.owner;
    a.registered_condition_3b=7001u;
    a.registered_condition_18=7002u;
    a.initial_assault.handle_14=UINT32_MAX;
    a.initial_assault.handle_18=UINT32_MAX;
    a.behavior_callbacks=behavior_callbacks();
    a.behavior_userdata=&m;

    a.version=5;
    assert(sin_grunt_native_attach_v1(registry,&a)==
           SIN_GRUNT_NATIVE_INVALID_ARGUMENT);
    a.version=1;
    a.original_classname="npc_mobgrunt";
    assert(sin_grunt_native_attach_v1(registry,&a)==
           SIN_GRUNT_NATIVE_UNSUPPORTED_CLASS);
    a.original_classname="npc_druglab_grunt_scattergun";
    assert(sin_grunt_native_attach_v1(registry,&a)==SIN_GRUNT_NATIVE_OK);
    assert(sin_grunt_native_attach_v1(registry,&a)==
           SIN_GRUNT_NATIVE_DUPLICATE_INDEX);
    assert(m.confirmations==1);

    assert(sin_grunt_native_step_v1(registry,wrong,1.f,&selected,&underlying)==
           SIN_GRUNT_NATIVE_STALE_GENERATION);
    assert(m.confirmations==1); /* Stale generation never reaches host. */
    assert(sin_grunt_native_step_v1(registry,actor,1.f,&selected,&underlying)==
           SIN_GRUNT_NATIVE_OK);
    assert(selected==0 && underlying==SIN_GRUNT_BEHAVIOR_OK);
    assert(m.lifecycle_calls==3);
    assert(sin_grunt_native_snapshot_v1(registry,actor,&snap)==SIN_GRUNT_NATIVE_OK);
    assert(snap.current_index==0);

    m.allow=0;
    callbacks_before=m.lifecycle_calls;
    assert(sin_grunt_native_step_v1(registry,actor,2.f,&selected,&underlying)==
           SIN_GRUNT_NATIVE_HOST_REJECTED);
    assert(selected==-1 && underlying==SIN_GRUNT_BEHAVIOR_ACTOR_EXPIRED);
    assert(m.lifecycle_calls==callbacks_before);
    snap.current_index=456;
    assert(sin_grunt_native_snapshot_v1(registry,actor,&snap)==
           SIN_GRUNT_NATIVE_HOST_REJECTED);
    assert(snap.current_index==456);

    m.allow=1;m.fail=1;
    assert(sin_grunt_native_step_v1(registry,actor,2.f,&selected,&underlying)==
           SIN_GRUNT_NATIVE_HOST_REJECTED);
    assert(underlying==SIN_GRUNT_BEHAVIOR_HOST_FAILED);
    m.fail=0;m.reenter=1;
    assert(sin_grunt_native_step_v1(registry,actor,2.f,&selected,&underlying)==
           SIN_GRUNT_NATIVE_OK);
    assert(m.reentry_destroy==SIN_GRUNT_NATIVE_REENTRANT);
    assert(m.reentry_step==SIN_GRUNT_NATIVE_REENTRANT);
    m.reenter=0;

    m.live=0; /* Native NPC died; exact old-generation detach is still safe. */
    assert(sin_grunt_native_detach_v1(registry,wrong)==
           SIN_GRUNT_NATIVE_STALE_GENERATION);
    assert(sin_grunt_native_detach_v1(registry,actor)==SIN_GRUNT_NATIVE_OK);
    assert(sin_grunt_native_detach_v1(registry,actor)==SIN_GRUNT_NATIVE_NOT_FOUND);
    m.generation=10u;m.live=1;a.actor.generation=10u;
    /* Phase621: a fourth retail registered CNPC_MobGrunt classname. */
    m.classname="npc_druglab_grunt";
    a.original_classname="npc_druglab_grunt";
    assert(sin_grunt_native_attach_v1(registry,&a)==SIN_GRUNT_NATIVE_OK);
    assert(sin_grunt_native_detach_v1(registry,actor)==
           SIN_GRUNT_NATIVE_STALE_GENERATION);
    assert(sin_grunt_native_detach_v1(registry,a.actor)==SIN_GRUNT_NATIVE_OK);
    assert(sin_grunt_native_destroy_v1(registry)==SIN_GRUNT_NATIVE_OK);
    return 0;
}
