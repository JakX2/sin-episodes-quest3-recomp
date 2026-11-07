/* C11/POSIX and Android Bionic dlopen regression with fake host callbacks.
 * This does not load the original 2006 DLL or a SourceVR game module.
 */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "sin_compat/assault_npc_c_api.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint32_t (*version_fn)(void);
typedef sin_assault_session_v1* (*create_fn)(const sin_assault_host_v1*, void*, sin_assault_ref_v1, uint32_t, uint32_t);
typedef int32_t (*destroy_fn)(sin_assault_session_v1*);
typedef int32_t (*bind_fn)(sin_assault_session_v1*, const float*, const sin_assault_candidate_v1*, uint32_t, int32_t, uint32_t*);
typedef int32_t (*begin_fn)(sin_assault_session_v1*);
typedef int32_t (*check_fn)(sin_assault_session_v1*, uint32_t*);
typedef int32_t (*finish_fn)(sin_assault_session_v1*);
typedef int32_t (*state_fn)(sin_assault_session_v1*, sin_assault_state_v1*);

static void symbol(void* lib, const char* name, void* out, size_t n) {
    void* entry=dlsym(lib,name);
    assert(entry && n == sizeof(entry));
    memcpy(out,&entry,n);
}
struct Mock {
    sin_assault_session_v1* session;
    state_fn state;
    destroy_fn destroy;
    int32_t reenter_state;
    int32_t reenter_destroy;
    uint32_t held, acquired, released, init_calls, reset_calls;
    uint32_t cond_queries, last_set, last_query;
    int32_t deny_lock, fail_init, fail_reset, fail_set, fail_query, hear_combat;
};
static int32_t is_live(void* ctx, sin_assault_ref_v1 ref) {
    (void)ctx;
    return ref.index == 22 && ref.generation == 1;
}
static int32_t try_lock(void* ctx, uint32_t point, sin_assault_ref_v1 actor) {
    struct Mock* m=(struct Mock*)ctx;
    assert(actor.index == 11 && actor.generation == 2);
    if (m->deny_lock || m->held) return 0;
    m->held=point;
    ++m->acquired;
    return 1;
}
static int32_t resolve(void* ctx,uint32_t point) {
    (void)ctx;
    return point == 400 ? 0 : 1;
}
static int32_t unlock(void* ctx,uint32_t point,sin_assault_ref_v1 actor) {
    struct Mock* m=(struct Mock*)ctx;
    assert(actor.index == 11 && actor.generation == 2);
    if (m->held != point) return 0;
    m->held=0;
    ++m->released;
    return 1;
}
static int32_t init(void* ctx,sin_assault_ref_v1 actor,uint32_t point) {
    struct Mock* m=(struct Mock*)ctx;
    assert(actor.index == 11 && point == m->held);
    ++m->init_calls;
    if (m->session) {
        sin_assault_state_v1 state;
        m->reenter_state=m->state(m->session,&state);
        m->reenter_destroy=m->destroy(m->session);
    }
    return m->fail_init ? 0 : 1;
}
static int32_t reset(void* ctx,sin_assault_ref_v1 actor) {
    struct Mock* m=(struct Mock*)ctx;
    assert(actor.index == 11);
    ++m->reset_calls;
    return m->fail_reset ? 0 : 1;
}
static int32_t set_cond(void* ctx,sin_assault_ref_v1 actor,uint32_t condition) {
    struct Mock* m=(struct Mock*)ctx;
    assert(actor.index == 11);
    m->last_set=condition;
    return m->fail_set ? 0 : 1;
}
static int32_t has_cond(void* ctx,sin_assault_ref_v1 actor,uint32_t condition) {
    struct Mock* m=(struct Mock*)ctx;
    assert(actor.index == 11);
    ++m->cond_queries;
    m->last_query=condition;
    return m->fail_query ? -1 : m->hear_combat;
}

int main(int argc,char** argv) {
    assert(argc == 2);
    void* lib=dlopen(argv[1],RTLD_NOW | RTLD_LOCAL);
    if (!lib) fprintf(stderr,"dlopen: %s\n",dlerror());
    assert(lib);
    version_fn version=0; create_fn create=0; destroy_fn destroy=0;
    bind_fn bind=0; begin_fn begin=0; check_fn check=0;
    finish_fn finish=0; state_fn state=0;
    symbol(lib,"sin_assault_abi_version_v1",&version,sizeof(version));
    symbol(lib,"sin_assault_create_v1",&create,sizeof(create));
    symbol(lib,"sin_assault_destroy_v1",&destroy,sizeof(destroy));
    symbol(lib,"sin_assault_bind_v1",&bind,sizeof(bind));
    symbol(lib,"sin_assault_begin_v1",&begin,sizeof(begin));
    symbol(lib,"sin_assault_check_cue_v1",&check,sizeof(check));
    symbol(lib,"sin_assault_finish_v1",&finish,sizeof(finish));
    symbol(lib,"sin_assault_get_state_v1",&state,sizeof(state));
    assert(version()==SIN_ASSAULT_ABI_VERSION_1);
    sin_assault_host_v1 callbacks={
        SIN_ASSAULT_ABI_VERSION_1,sizeof(sin_assault_host_v1),
        is_live,try_lock,resolve,unlock,init,reset,set_cond,has_cond
    };
    struct Mock mock={0};
    mock.state=state;
    mock.destroy=destroy;
    sin_assault_ref_v1 actor={11,2};
    assert(!create(&callbacks,&mock,(sin_assault_ref_v1){0,2},9001,9002));
    assert(!create(&callbacks,&mock,actor,UINT32_MAX,9002));
    assert(!create(&callbacks,&mock,actor,9001,9001));
    callbacks.version=2;
    assert(!create(&callbacks,&mock,actor,9001,9002));
    callbacks.version=1;
    sin_assault_session_v1* session=create(&callbacks,&mock,actor,9001,9002);
    assert(session);
    mock.session=session;
    const float origin[]={0,0,0};
    const sin_assault_candidate_v1 points[]={
        {100,9,{1,0,0},{22,1},1},
        {200,3,{2,0,0},{0,0},1},
        {300,3,{8,0,0},{0,0},1}
    };
    uint32_t selected=0,ready=0;
    sin_assault_state_v1 snapshot={0};
    assert(begin(session)==SIN_ASSAULT_NOT_BOUND);
    assert(bind(session,origin,points,3,2,&selected)==SIN_ASSAULT_OK && selected==200);
    assert(mock.acquired==1 && mock.held==200 && mock.init_calls==1);
    assert(mock.reenter_state==SIN_ASSAULT_REENTRANT);
    assert(mock.reenter_destroy==SIN_ASSAULT_REENTRANT);
    assert(destroy(session)==SIN_ASSAULT_ALREADY_BOUND);
    assert(state(session,&snapshot)==SIN_ASSAULT_OK);
    assert(snapshot.configured_mode==2 && snapshot.received_cue==0 && snapshot.rally_id==200);
    assert(check(session,&ready)==SIN_ASSAULT_OK && ready==0);
    assert(mock.cond_queries==1 && mock.last_query==9002);
    mock.hear_combat=1;
    assert(check(session,&ready)==SIN_ASSAULT_OK && ready==1);
    mock.fail_query=1;
    assert(check(session,&ready)==SIN_ASSAULT_HOST_FAILED && ready==0);
    mock.fail_query=0;
    assert(begin(session)==SIN_ASSAULT_OK && mock.reset_calls==1);
    assert(mock.last_set==9001);
    assert(state(session,&snapshot)==SIN_ASSAULT_OK && snapshot.received_cue==1);
    assert(finish(session)==SIN_ASSAULT_OK && mock.held==0);
    assert(state(session,&snapshot)==SIN_ASSAULT_OK);
    assert(snapshot.configured_mode==0 && snapshot.received_cue==1 && snapshot.rally_id==0);
    assert(check(session,&ready)==SIN_ASSAULT_NOT_BOUND);
    assert(mock.released==1);

    sin_assault_candidate_v1 missing={0};
    missing.rally_id=400;
    missing.priority=1;
    missing.eligible=1;
    assert(bind(session,origin,&missing,1,3,&selected)==SIN_ASSAULT_UNRESOLVED_RALLY);
    assert(mock.held==0 && mock.released==2 && mock.init_calls==1);
    assert(state(session,&snapshot)==SIN_ASSAULT_OK);
    assert(snapshot.configured_mode==0 && snapshot.received_cue==1);
    assert(bind(session,origin,0,0,3,&selected)==SIN_ASSAULT_NO_CANDIDATE);
    assert(bind(session,origin,points,3,0,&selected)==SIN_ASSAULT_INVALID_ARGUMENT);
    assert(bind(session,origin,points,3,4,&selected)==SIN_ASSAULT_INVALID_ARGUMENT);
    sin_assault_candidate_v1 duplicate[]={points[1],points[1]};
    assert(bind(session,origin,duplicate,2,3,&selected)==SIN_ASSAULT_INVALID_ARGUMENT);

    mock.deny_lock=1;
    assert(bind(session,origin,points,3,3,&selected)==SIN_ASSAULT_LOCK_REJECTED);
    assert(mock.held==0);
    mock.deny_lock=0;
    mock.fail_init=1;
    assert(bind(session,origin,points,3,3,&selected)==SIN_ASSAULT_HOST_FAILED);
    assert(mock.held==0 && mock.released==3);
    mock.fail_init=0;
    assert(bind(session,origin,points,3,3,&selected)==SIN_ASSAULT_OK && selected==200);
    const uint32_t before=mock.cond_queries;
    assert(check(session,&ready)==SIN_ASSAULT_OK && ready==1);
    assert(mock.cond_queries==before);
    assert(state(session,&snapshot)==SIN_ASSAULT_OK && snapshot.received_cue==3);
    mock.fail_reset=1;
    assert(begin(session)==SIN_ASSAULT_HOST_FAILED);
    assert(state(session,&snapshot)==SIN_ASSAULT_OK && snapshot.received_cue==3);
    mock.fail_reset=0;
    mock.fail_set=1;
    assert(begin(session)==SIN_ASSAULT_HOST_FAILED);
    assert(state(session,&snapshot)==SIN_ASSAULT_OK && snapshot.received_cue==3);
    mock.fail_set=0;
    assert(finish(session)==SIN_ASSAULT_OK);
    assert(destroy(session)==SIN_ASSAULT_OK);
    assert(dlclose(lib)==0);
    puts("PASS: independent assault NPC C ABI dlopen with fake host, exclusive rally locks, cue and registered condition callbacks");
    return 0;
}
