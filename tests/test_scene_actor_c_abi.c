/* C11 dlopen test of a *real loadable library*; all engine callbacks are fake.
 * No Quest rendering or SourceVR module loader is involved.
 */
#include "sin_compat/scene_actor_c_api.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t (*api_version_fn)(void);
typedef sin_scene_session_v1 *(*api_create_fn)(const sin_scene_callbacks_v1*,void*,int32_t);
typedef void (*api_destroy_fn)(sin_scene_session_v1*);
typedef int32_t (*api_spawn_fn)(sin_scene_session_v1*,const char*,const char*);
typedef int32_t (*api_begin_fn)(sin_scene_session_v1*,const char*,const char*);
typedef int32_t (*api_input_fn)(sin_scene_session_v1*,const char*,int32_t,const char*,int32_t);
typedef int32_t (*api_event_fn)(sin_scene_session_v1*,const char*);
typedef int32_t (*api_ref_fn)(sin_scene_session_v1*,sin_scene_actor_ref_v1*);

static api_version_fn fn_version;
static api_create_fn fn_create;
static api_destroy_fn fn_destroy;
static api_spawn_fn fn_spawn;
static api_begin_fn fn_begin;
static api_input_fn fn_input;
static api_event_fn fn_event;
static api_ref_fn fn_ref;

/* POSIX dlsym produces a void*: memcpy avoids nonportable object/function
 * pointer casts that pedantic C11 rejects. This test targets POSIX ELF. */
static void load(void* lib,const char* name,void* dest,size_t size){
    void *p=dlsym(lib,name);
    if (!p || size != sizeof(p)) {fprintf(stderr,"Missing %s: %s\n",name,dlerror());abort();}
    memcpy(dest,&p,size);
}
#define SYM(library,fun,field) load(library,fun,&field,sizeof(field))

struct Context {
    int creations, removals, holsters, throws, scenes;
    int fail_model, fail_choreo, fail_throw;
    int reentry_test, reentry_status;
    int health, primary, secondary;
    char base[80],original[80],model[160],target[80],scene[160],token[80],item[80];
    sin_scene_session_v1 *session;
};
static sin_scene_actor_ref_v1 cb_create(void *raw,const char *base,const char *original){
    struct Context *c=(struct Context*)raw;
    ++c->creations;
    snprintf(c->base,sizeof(c->base),"%s",base);
    snprintf(c->original,sizeof(c->original),"%s",original);
    return (sin_scene_actor_ref_v1){111,9};
}
static int cb_model(void *raw,sin_scene_actor_ref_v1 ref,const char *model){
    struct Context *c=(struct Context*)raw;
    assert(ref.index==111 && ref.generation==9);
    snprintf(c->model,sizeof(c->model),"%s",model);
    if(c->reentry_test){
        c->reentry_status=fn_input(c->session,"HolsterWeapon",SIN_SCENE_INPUT_VOID,NULL,0);
        fn_destroy(c->session); /* must refuse destruction during callback */
    }
    return !c->fail_model;
}
static int cb_health(void *raw,sin_scene_actor_ref_v1 ref,int32_t hp){
    struct Context *c=(struct Context*)raw;assert(ref.index==111);c->health=hp;return 1;
}
static int cb_name(void *raw,sin_scene_actor_ref_v1 ref,const char *name){
    struct Context *c=(struct Context*)raw;assert(ref.index==111);
    snprintf(c->target,sizeof(c->target),"%s",name);return 1;
}
static int cb_choreo(void *raw,sin_scene_actor_ref_v1 ref){
    struct Context *c=(struct Context*)raw;assert(ref.index==111);return !c->fail_choreo;
}
static int cb_scene(void *raw,sin_scene_actor_ref_v1 ref,const char *scene,const char *token){
    struct Context *c=(struct Context*)raw;assert(ref.index==111);
    snprintf(c->scene,sizeof(c->scene),"%s",scene);
    snprintf(c->token,sizeof(c->token),"%s",token);
    ++c->scenes;return 1;
}
static int cb_holster(void *raw){
    struct Context *c=(struct Context*)raw;++c->holsters;return 1;
}
static int cb_throw(void *raw,sin_scene_actor_ref_v1 ref,const char *item,int32_t primary,int32_t secondary){
    struct Context *c=(struct Context*)raw;assert(ref.index==111);
    snprintf(c->item,sizeof(c->item),"%s",item);
    c->primary=primary;c->secondary=secondary;++c->throws;
    return !c->fail_throw;
}
static void cb_remove(void *raw,sin_scene_actor_ref_v1 ref){
    struct Context *c=(struct Context*)raw;assert(ref.index==111);++c->removals;
}
static sin_scene_callbacks_v1 callbacks(void){
    sin_scene_callbacks_v1 c={0};
    c.version=SIN_SCENE_ABI_VERSION_1;
    c.struct_size=(uint32_t)sizeof(c);
    c.create_from_existing_base=cb_create;
    c.set_model=cb_model;
    c.set_health=cb_health;
    c.set_targetname=cb_name;
    c.choreography_capable=cb_choreo;
    c.play_scene=cb_scene;
    c.holster_player_weapon=cb_holster;
    c.throw_item_at_player=cb_throw;
    c.remove_actor=cb_remove;
    return c;
}
int main(int argc,char** argv){
    if(argc!=2){fprintf(stderr,"Usage: %s libsin_scene_actor_c_abi.so\n",argv[0]);return 2;}
    void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if(!lib){fprintf(stderr,"dlopen: %s\n",dlerror());return 1;}
    SYM(lib,"sin_scene_abi_version_v1",fn_version);
    SYM(lib,"sin_scene_create_v1",fn_create);
    SYM(lib,"sin_scene_destroy_v1",fn_destroy);
    SYM(lib,"sin_scene_spawn_v1",fn_spawn);
    SYM(lib,"sin_scene_begin_v1",fn_begin);
    SYM(lib,"sin_scene_input_v1",fn_input);
    SYM(lib,"sin_scene_animation_event_v1",fn_event);
    SYM(lib,"sin_scene_get_actor_v1",fn_ref);
    assert(fn_version()==1);
    assert(fn_spawn(NULL,NULL,NULL)==-1);
    assert(fn_input(NULL,"HolsterWeapon",0,NULL,0)==-1);
    assert(fn_create(NULL,NULL,0)==NULL);
    sin_scene_callbacks_v1 cb=callbacks();
    cb.struct_size=0;
    assert(fn_create(&cb,NULL,0)==NULL);
    cb=callbacks();cb.set_model=NULL;
    assert(fn_create(&cb,NULL,0)==NULL);
    cb=callbacks();
    assert(fn_create(&cb,NULL,3)==NULL);
    {
        struct Context c={0};c.reentry_test=1;
        sin_scene_session_v1 *s=fn_create(&cb,&c,SIN_SCENE_ACTOR_JC);assert(s);
        c.session=s;
        assert(fn_begin(s,"scenes/example.vcd","JC") == SIN_SCENE_INVALID_STATE);
        assert(fn_spawn(s,"JC_Hospital",NULL) == SIN_SCENE_OK);
        assert(c.reentry_status==-1);
        assert(!strcmp(c.base,"npc_citizen") && !strcmp(c.original,"npc_jc"));
        assert(!strcmp(c.model,"models/characters/JC/jc_torso.mdl"));
        assert(c.health==8 && !strcmp(c.target,"JC_Hospital"));
        sin_scene_actor_ref_v1 ref={0,0};
        assert(fn_ref(s,&ref)==SIN_SCENE_OK && ref.index==111 && ref.generation==9);
        assert(fn_spawn(s,NULL,NULL)==SIN_SCENE_INVALID_STATE);
        assert(fn_begin(s,"scenes/se1_finale/teaser_jc_hospital.vcd","JC_Hospital")==SIN_SCENE_OK);
        assert(!strcmp(c.token,"JC_Hospital") && c.scenes==1);
        assert(fn_input(s,"HolsterWeapon",SIN_SCENE_INPUT_VOID,NULL,0)==SIN_SCENE_UNSUPPORTED_INPUT);
        fn_destroy(s);
    }
    {
        struct Context c={0};
        sin_scene_session_v1 *s=fn_create(&cb,&c,SIN_SCENE_ACTOR_JESSICA);assert(s);
        assert(fn_spawn(s,"Jessica",NULL)==SIN_SCENE_OK);
        assert(!strcmp(c.base,"npc_alyx") && c.health==80);
        assert(fn_begin(s,"scenes/se1_highrise/jess_wakeupblade.vcd","!Target1")==SIN_SCENE_OK);
        assert(!strcmp(c.token,"!Target1"));
        assert(fn_input(s,"SetPrimaryAmmoCountToThrow",SIN_SCENE_INPUT_STRING,"5",0)==SIN_SCENE_WRONG_INPUT_TYPE);
        assert(fn_input(s,"ThrowItemAtPlayer",SIN_SCENE_INPUT_STRING,NULL,0)==SIN_SCENE_INVALID_ARGUMENT);
        assert(fn_input(s,"ThrowItemAtPlayer",SIN_SCENE_INPUT_STRING,"weapon_assault_rifle",0)==SIN_SCENE_OK);
        assert(fn_input(s,"SetPrimaryAmmoCountToThrow",SIN_SCENE_INPUT_INTEGER,NULL,45)==SIN_SCENE_OK);
        assert(fn_input(s,"SetSecondaryAmmoCountToThrow",SIN_SCENE_INPUT_INTEGER,NULL,6)==SIN_SCENE_OK);
        assert(fn_event(s,"not_an_original_event")==SIN_SCENE_UNSUPPORTED_INPUT);
        c.fail_throw=1;
        assert(fn_event(s,"AE_JESSICA_THROW_ITEM")==SIN_SCENE_HOST_FAILED);
        assert(c.throws==1);
        c.fail_throw=0;
        assert(fn_event(s,"AE_JESSICA_THROW_ITEM")==SIN_SCENE_OK);
        assert(c.throws==2 && c.primary==45 && c.secondary==6);
        assert(fn_event(s,"AE_JESSICA_THROW_ITEM")==SIN_SCENE_NO_PENDING_THROW);
        assert(fn_input(s,"HolsterWeapon",SIN_SCENE_INPUT_VOID,NULL,0)==SIN_SCENE_OK && c.holsters==1);
        fn_destroy(s);
    }
    {
        struct Context c={0};c.fail_choreo=1;
        sin_scene_session_v1 *s=fn_create(&cb,&c,SIN_SCENE_ACTOR_JC);assert(s);
        assert(fn_spawn(s,NULL,NULL)==SIN_SCENE_HOST_FAILED);
        assert(c.removals==1);
        fn_destroy(s);
    }
    {
        struct Context c={0};
        sin_scene_session_v1 *s=fn_create(&cb,&c,SIN_SCENE_ACTOR_GENERIC);assert(s);
        assert(fn_spawn(s,"Actor",NULL)==SIN_SCENE_INVALID_ARGUMENT);
        assert(fn_spawn(s,"Actor","models/characters/custom.mdl")==SIN_SCENE_OK);
        assert(c.health==0 && !strcmp(c.model,"models/characters/custom.mdl"));
        fn_destroy(s);
    }
    dlclose(lib);
    puts("PASS: independently authored SiN JC/Jessica C ABI dlopen, callback guards and scene events (fake host)");
    return 0;
}
