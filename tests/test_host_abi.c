#include "sin_compat/host_abi_v1.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* A genuine C client that loads the compiled C++ .so dynamically. This does
 * not implement any Valve APIs or pretend to load into SourceVR. */
#define REQ(x) do {if(!(x)){fprintf(stderr,"ASSERT FAILED %s:%d: %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
typedef struct {
  sin_entity_ref_v1 owner,weapon,selected,model,slots[2];
  int owner_live,weapon_live,model_live,player,creates,removes,hides,model_hides,fovs,binds,plays;
} host;
static sin_entity_ref_v1 invalid_ref(void){return (sin_entity_ref_v1){UINT32_MAX,UINT32_MAX};}
static int same(sin_entity_ref_v1 a,sin_entity_ref_v1 b){return a.index==b.index&&a.generation==b.generation;}
static int live(void*u,sin_entity_ref_v1 r){host*h=u;return (h->owner_live&&same(r,h->owner))||(h->weapon_live&&same(r,h->weapon))||(h->model_live&&same(r,h->model));}
static sin_entity_ref_v1 owner_of(void*u,sin_entity_ref_v1 w){host*h=u;return same(w,h->weapon)?h->owner:invalid_ref();}
static sin_entity_ref_v1 selected(void*u,sin_entity_ref_v1 p){host*h=u;return same(p,h->owner)?h->selected:invalid_ref();}
static void set_weapon(void*u,sin_entity_ref_v1 w,int hide){host*h=u;assert(same(w,h->weapon)&&hide);h->hides++;}
static int player(void*u,sin_entity_ref_v1 r){host*h=u;return h->player&&same(r,h->owner);}
static int32_t model_index(void*u,sin_entity_ref_v1 w){host*h=u;assert(same(w,h->weapon));return 1;}
static int valid_slot(void*u,sin_entity_ref_v1 o,int32_t index){host*h=u;return same(o,h->owner)&&index>=0&&index<2;}
static sin_entity_ref_v1 get_slot(void*u,sin_entity_ref_v1 p,int32_t slot){host*h=u;return same(p,h->owner)&&slot>=0&&slot<2?h->slots[slot]:invalid_ref();}
static int model_owned(void*u,sin_entity_ref_v1 m,sin_entity_ref_v1 p){host*h=u;return same(m,h->model)&&same(p,h->owner);}
static void set_model(void*u,sin_entity_ref_v1 m,int hide){host*h=u;assert(same(m,h->model)&&hide);h->model_hides++;}
static int32_t count(void*u,sin_entity_ref_v1 p){host*h=u;return same(p,h->owner)?2:0;}
static sin_entity_ref_v1 create(void*u,const char*n){host*h=u;assert(strcmp(n,"viewmodel")==0);h->creates++;h->model=(sin_entity_ref_v1){30,4};h->model_live=1;return h->model;}
static int config(void*u,sin_entity_ref_v1 m,sin_entity_ref_v1 p,int32_t slot){host*h=u;return same(m,h->model)&&same(p,h->owner)&&slot==1;}
static int store(void*u,sin_entity_ref_v1 p,int32_t slot,sin_entity_ref_v1 m){host*h=u;if(!same(p,h->owner)||slot<0||slot>1)return 0;h->slots[slot]=m;return 1;}
static void remove_model(void*u,sin_entity_ref_v1 m){host*h=u;assert(same(m,h->model));h->model_live=0;h->removes++;}
static void clear_slot(void*u,sin_entity_ref_v1 p,int32_t slot){host*h=u;assert(same(p,h->owner));h->slots[slot]=invalid_ref();}
static int32_t resolve(void*u,int32_t activity){(void)u;return activity>=0?activity+100:-1;}
static void bind(void*u,const char*model,const float origin[3]){host*h=u;assert(strcmp(model,"fake_model.mdl")==0&&origin[0]==1.f);h->binds++;}
static void play(void*u,int32_t sequence){host*h=u;assert(sequence>=100);h->plays++;}
static void fov(void*u,int32_t f,float duration){host*h=u;(void)f;(void)duration;h->fovs++;}

#define LOAD(n) do { *(void **)(&p_##n)=dlsym(handle,"sin_" #n "_v1");REQ(p_##n!=NULL); }while(0)
int main(int argc,char**argv){
  REQ(argc==2);
  void *handle=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);if(!handle){fprintf(stderr,"dlopen: %s\n",dlerror());return 1;}
  uint32_t (*p_abi_version)(void)=NULL;
  sin_session_v1* (*p_session_create)(const sin_host_callbacks_v1*,void*,sin_entity_ref_v1,sin_entity_ref_v1,int32_t,const sin_weapon_config_v1*,int)=NULL;
  void (*p_session_destroy)(sin_session_v1*)=NULL;
  int32_t (*p_session_ensure_viewmodel)(sin_session_v1*)=NULL;
  int32_t (*p_session_hide_weapon)(sin_session_v1*)=NULL;
  int32_t (*p_session_usercmd)(sin_session_v1*,uint32_t,int)=NULL;
  int32_t (*p_session_reload_begin)(sin_session_v1*,int32_t,int32_t)=NULL;
  int32_t (*p_session_reload_event)(sin_session_v1*,int32_t*,int32_t*)=NULL;
  int32_t (*p_session_reload_end)(sin_session_v1*)=NULL;
  int32_t (*p_session_set_ammo)(sin_session_v1*,sin_ammo_v1)=NULL;
  int32_t (*p_session_get_ammo)(sin_session_v1*,sin_ammo_v1*)=NULL;
  int32_t (*p_session_holster)(sin_session_v1*)=NULL;
  int32_t (*p_session_cleanup)(sin_session_v1*)=NULL;
  LOAD(abi_version); LOAD(session_create); LOAD(session_destroy);
  LOAD(session_ensure_viewmodel); LOAD(session_hide_weapon); LOAD(session_usercmd);
  LOAD(session_reload_begin); LOAD(session_reload_event); LOAD(session_reload_end);
  LOAD(session_set_ammo); LOAD(session_get_ammo); LOAD(session_holster); LOAD(session_cleanup);
  REQ(p_abi_version()==1);
  host h={0};h.owner=(sin_entity_ref_v1){8,3};h.weapon=(sin_entity_ref_v1){14,7};h.selected=h.weapon;h.owner_live=h.weapon_live=h.player=1;
  h.slots[0]=invalid_ref();h.slots[1]=invalid_ref();
  sin_host_callbacks_v1 c={0};c.version=1;c.struct_size=(uint32_t)sizeof(c);
  c.is_live=live;c.owner_of=owner_of;c.selected_weapon_of=selected;c.set_weapon_nodraw=set_weapon;
  c.is_player_owner=player;c.viewmodel_index_of=model_index;c.is_valid_viewmodel_index=valid_slot;
  c.viewmodel_of=get_slot;c.is_viewmodel_owned_by=model_owned;c.set_viewmodel_nodraw=set_model;
  c.is_player=player;c.viewmodel_slot_count=count;c.viewmodel_at=get_slot;
  c.create_named_entity=create;c.configure_viewmodel=config;c.store_viewmodel_handle=store;
  c.remove_viewmodel=remove_model;c.clear_viewmodel_handle=clear_slot;c.resolve_sequence=resolve;
  c.bind_viewmodel=bind;c.play_sequence=play;c.set_flatscreen_fov=fov;
  sin_weapon_config_v1 w={"fake_model.mdl",{1.f,0.f,0.f},55,6,-1};
  c.version=99;REQ(p_session_create(&c,&h,h.owner,h.weapon,0,&w,1)==NULL);
  c.version=1;c.struct_size-=4;REQ(p_session_create(&c,&h,h.owner,h.weapon,0,&w,1)==NULL);
  c.struct_size=sizeof(c);c.is_live=NULL;REQ(p_session_create(&c,&h,h.owner,h.weapon,0,&w,1)==NULL);
  c.is_live=live;REQ(p_session_create(&c,&h,h.owner,h.weapon,3,&w,1)==NULL);
  sin_session_v1*s=p_session_create(&c,&h,h.owner,h.weapon,0,&w,1);REQ(s!=NULL);
  REQ(p_session_ensure_viewmodel(s)==6); /* Created enum ordinal */
  REQ(p_session_ensure_viewmodel(s)==2); /* AlreadyPresent enum ordinal */
  REQ(h.creates==1);
  REQ(p_session_hide_weapon(s)==3 && h.hides==1 && h.model_hides==1);
  REQ(p_session_usercmd(s,0x80000u,0)&1);REQ(h.fovs==0); /* HMD FOV untouched */
  REQ(p_session_set_ammo(s,(sin_ammo_v1){2,0,5,0})==0);
  REQ(p_session_reload_begin(s,-1,-1)==1);
  REQ(p_session_set_ammo(s,(sin_ammo_v1){0,0,100,0})==-1); /* active reload */
  sin_ammo_v1 ammo={0};REQ(p_session_get_ammo(s,&ammo)==0 && ammo.clip==2 && ammo.reserve==5);
  int32_t p=-1,q=-1;REQ(p_session_reload_event(s,&p,&q)==0&&p==4&&q==0);
  REQ(p_session_reload_event(s,&p,&q)==0&&p==0&&q==0);
  REQ(p_session_get_ammo(s,&ammo)==0&&ammo.clip==6&&ammo.reserve==1);
  REQ(p_session_reload_end(s)==0);
  REQ(p_session_set_ammo(s,(sin_ammo_v1){-1,0,0,0})==-1);
  h.selected=(sin_entity_ref_v1){50,5};REQ(p_session_hide_weapon(s)==2);
  REQ(p_session_usercmd(s,0,0)&2);
  REQ(p_session_reload_begin(s,-1,-1)==0);
  h.selected=h.weapon;
  REQ(p_session_cleanup(s)==0 && h.removes==1);
  REQ(p_session_cleanup(s)==0 && h.removes==1);
  REQ(p_session_holster(s)==1);
  REQ(p_session_ensure_viewmodel(NULL)==-1);
  REQ(p_session_reload_event(NULL,&p,&q)==-1);
  p_session_destroy(s);REQ(dlclose(handle)==0);
  puts("C ABI dlopen v1 smoke test passed");return 0;
}
