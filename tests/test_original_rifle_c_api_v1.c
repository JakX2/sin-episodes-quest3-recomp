/* Genuine C-language mock-host test of the independently authored C ABI.
 * No Valve engine objects are constructed and this is not SourceVR gameplay. */
#include "sin_compat/original_rifle_c_api_v1.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    sin_entity_ref_v1 player, gun;
    int fov_calls, last_fov, binds, plays;
    const char *last_model;
} test_host;

static sin_entity_ref_v1 invalid(void) { return (sin_entity_ref_v1){UINT32_MAX, UINT32_MAX}; }
static int equal(sin_entity_ref_v1 a, sin_entity_ref_v1 b) {
    return a.index == b.index && a.generation == b.generation;
}
static int live(void *u, sin_entity_ref_v1 ref) {
    test_host *h = (test_host *)u;
    return equal(ref, h->player) || equal(ref, h->gun);
}
static sin_entity_ref_v1 owner(void *u, sin_entity_ref_v1 ref) {
    test_host *h = (test_host *)u;
    return equal(ref,h->gun) ? h->player : invalid();
}
static sin_entity_ref_v1 selected(void *u, sin_entity_ref_v1 ref) {
    test_host *h = (test_host *)u;
    return equal(ref,h->player) ? h->gun : invalid();
}
static void hide(void *u,sin_entity_ref_v1 e,int flag) {(void)u;(void)e;(void)flag;}
static int is_player(void *u,sin_entity_ref_v1 ref) {
    return equal(ref,((test_host*)u)->player);
}
static int32_t slot(void *u,sin_entity_ref_v1 e) {(void)u;(void)e;return 0;}
static int valid(void *u,sin_entity_ref_v1 e,int32_t n) {
    (void)u;(void)e;return n==0;
}
static sin_entity_ref_v1 get_viewmodel(void *u,sin_entity_ref_v1 e,int32_t n) {
    (void)u;(void)e;(void)n;return invalid();
}
static int is_owned(void *u,sin_entity_ref_v1 e,sin_entity_ref_v1 p) {
    (void)u;(void)e;(void)p;return 0;
}
static int32_t slots(void *u,sin_entity_ref_v1 e) {(void)u;(void)e;return 1;}
static sin_entity_ref_v1 create_entity(void *u,const char *name) {
    (void)u;(void)name;return invalid();
}
static int configure(void *u,sin_entity_ref_v1 a,sin_entity_ref_v1 b,int32_t n) {
    (void)u;(void)a;(void)b;(void)n;return 0;
}
static int store(void *u,sin_entity_ref_v1 a,int32_t n,sin_entity_ref_v1 b) {
    (void)u;(void)a;(void)n;(void)b;return 0;
}
static void remove_entity(void *u,sin_entity_ref_v1 e) {(void)u;(void)e;}
static void clear(void *u,sin_entity_ref_v1 e,int32_t n) {(void)u;(void)e;(void)n;}
static int32_t resolve(void *u,int32_t activity) {(void)u;return activity + 1000;}
static void bind(void *u,const char *model,const float offset[3]) {
    test_host *h=(test_host *)u;
    assert(offset[0]==0 && offset[1]==0 && offset[2]==0);
    h->last_model=model;
    h->binds++;
}
static void play(void *u,int32_t sequence) {
    test_host *h=(test_host *)u;
    assert(sequence>=1000);
    h->plays++;
}
static void fov(void *u,int32_t value,float time) {
    test_host *h=(test_host *)u;
    assert(time>0);
    h->fov_calls++;
    h->last_fov=value;
}
static sin_host_callbacks_v1 callbacks(void) {
    sin_host_callbacks_v1 c={0};
    c.version=SIN_COMPAT_ABI_VERSION_1;
    c.struct_size=sizeof c;
    c.is_live=live;
    c.owner_of=owner;
    c.selected_weapon_of=selected;
    c.set_weapon_nodraw=hide;
    c.is_player_owner=is_player;
    c.viewmodel_index_of=slot;
    c.is_valid_viewmodel_index=valid;
    c.viewmodel_of=get_viewmodel;
    c.is_viewmodel_owned_by=is_owned;
    c.set_viewmodel_nodraw=hide;
    c.is_player=is_player;
    c.viewmodel_slot_count=slots;
    c.viewmodel_at=get_viewmodel;
    c.create_named_entity=create_entity;
    c.configure_viewmodel=configure;
    c.store_viewmodel_handle=store;
    c.remove_viewmodel=remove_entity;
    c.clear_viewmodel_handle=clear;
    c.resolve_sequence=resolve;
    c.bind_viewmodel=bind;
    c.play_sequence=play;
    c.set_flatscreen_fov=fov;
    return c;
}
int main(void) {
    sin_original_rifle_info_v1 info={0};
    assert(sin_original_rifle_abi_version_v1()==SIN_ORIGINAL_RIFLE_ABI_VERSION_1);
    assert(sin_original_rifle_get_info_v1(1,&info)==-1);
    info.struct_bytes=sizeof info;
    assert(sin_original_rifle_get_info_v1(99,&info)==-2);
    assert(sin_original_rifle_get_info_v1(1,&info)==0);
    assert(info.abi_version==1 && info.struct_bytes==sizeof info);
    assert(strcmp(info.classname,"weapon_assault_rifle")==0);
    assert(strcmp(info.viewmodel,"models/weapons/v_assault_rifle/v_assault_rifle.mdl")==0);
    assert(strcmp(info.worldmodel,"models/weapons/w_assault_rifle/w_assault_rifle.mdl")==0);
    assert(strcmp(info.npc_model,"models/characters/MobGrunt/mobgrunt.mdl")==0);
    assert(strcmp(info.primary_ammo,"AR")==0);
    assert(strcmp(info.secondary_ammo,"ARNADE")==0);
    assert(strcmp(info.secondary_projectile,"grenade_ar")==0);
    assert(info.clip_size==45 && info.default_clip==45 && info.clip2_size==-1);
    assert(info.zoom_fov==40);

    test_host h={0};
    h.player=(sin_entity_ref_v1){1,1};
    h.gun=(sin_entity_ref_v1){2,1};
    sin_host_callbacks_v1 c=callbacks();
    assert(sin_original_rifle_create_session_v1("weapon_ar2",&c,&h,h.player,h.gun,0)==NULL);
    assert(sin_original_rifle_create_session_v1(NULL,&c,&h,h.player,h.gun,0)==NULL);
    c.version=99;
    assert(sin_original_rifle_create_session_v1("weapon_assault_rifle",&c,&h,h.player,h.gun,0)==NULL);
    c.version=1;

    sin_session_v1 *s=sin_original_rifle_create_session_v1(
        "weapon_assault_rifle",&c,&h,h.player,h.gun,0
    );
    assert(s);
    sin_ammo_v1 ammo={0};
    assert(sin_session_get_ammo_v1(s,&ammo)==0);
    assert(ammo.clip==45 && ammo.clip2==0 && ammo.reserve==0);
    assert(sin_session_usercmd_v1(s,0x00080000u,0)&1);
    assert(h.fov_calls==1 && h.last_fov==40);
    assert(h.binds==1 && h.plays==1);
    assert(strcmp(h.last_model,info.viewmodel)==0);
    assert(sin_session_set_ammo_v1(s,(sin_ammo_v1){20,0,30,0})==0);
    assert(sin_session_reload_begin_v1(s,-1,-1)==1);
    int32_t primary=-1, secondary=-1;
    assert(sin_session_reload_event_v1(s,&primary,&secondary)==0);
    assert(primary==25 && secondary==0);
    assert(sin_session_get_ammo_v1(s,&ammo)==0);
    assert(ammo.clip==45 && ammo.reserve==5);
    sin_session_destroy_v1(s);

    test_host vr_host={0};
    vr_host.player=h.player;vr_host.gun=h.gun;
    s=sin_original_rifle_create_session_v1(
        "weapon_assault_rifle",&c,&vr_host,vr_host.player,vr_host.gun,1
    );
    assert(s);
    assert(sin_session_usercmd_v1(s,0x00080000u,0)&1);
    assert(vr_host.binds==1 && vr_host.fov_calls==0);
    sin_session_destroy_v1(s);
    puts("Original SiN rifle native C bridge mock-host test passed");
    return 0;
}
