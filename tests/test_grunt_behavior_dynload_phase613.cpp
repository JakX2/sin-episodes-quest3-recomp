// Phase613: test actual shared-library exports through LoadLibrary/dlopen.
#include "sin_compat/grunt_behavior_c_api_v1.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#if defined(_WIN32)
# include <windows.h>
#else
# include <dlfcn.h>
#endif

template<class Function> static Function load_symbol(void* library,const char* name){
#if defined(_WIN32)
  FARPROC p=GetProcAddress(static_cast<HMODULE>(library),name);
  assert(p);
  Function out;
  static_assert(sizeof(out)==sizeof(p),"Host function pointer ABI differs");
  std::memcpy(&out,&p,sizeof(p));
#else
  void* p=dlsym(library,name);
  assert(p);
  Function out;
  static_assert(sizeof(out)==sizeof(p),"Function pointer ABI differs");
  std::memcpy(&out,&p,sizeof(p));
#endif
  return out;
}
static int32_t yes_actor(void*,sin_grunt_actor_v1){return 1;}
static int32_t no_behavior(void*,sin_grunt_actor_v1,uint32_t){return 0;}
static int32_t no_owner(void*,sin_grunt_actor_v1){return 0;}
static int32_t no_cond(void*,sin_grunt_actor_v1,uint32_t){return 0;}
static int32_t no_serial(void*,uint32_t,uint32_t){return 0;}
static int32_t no_field(void*,uint32_t,uint8_t*){return 0;}
static int32_t ok_condition(void*,sin_grunt_actor_v1,uint32_t){return 1;}
static int32_t no_entity(void*,uint32_t,uint64_t*){return 0;}
static int32_t read_link(void*,uint64_t,uint32_t* out){*out=0xffffffffu;return 1;}
static int32_t write_link(void*,uint64_t,uint32_t){return 1;}
static int32_t ok_lifecycle(void*,sin_grunt_actor_v1,uint32_t,uint32_t){return 1;}
static int32_t ok_notify(void*,sin_grunt_actor_v1,int32_t,int32_t){return 1;}
int main(int argc,char** argv){
  assert(argc==2);
#if defined(_WIN32)
  void* lib=static_cast<void*>(LoadLibraryA(argv[1]));
#else
  void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
#endif
  assert(lib);
  using version_fn=uint32_t (*)();
  using create_fn=sin_grunt_behavior_session_v1*(*) (
    const sin_grunt_behavior_host_v1*,void*,sin_grunt_actor_v1,uint64_t,
    uint32_t,uint32_t,const sin_grunt_assault_state_v1*);
  using step_fn=int32_t (*)(sin_grunt_behavior_session_v1*,float,int32_t*);
  using snap_fn=int32_t (*)(sin_grunt_behavior_session_v1*,sin_grunt_snapshot_v1*);
  using destroy_fn=int32_t (*)(sin_grunt_behavior_session_v1*);
  auto version=load_symbol<version_fn>(lib,"sin_grunt_behavior_abi_version_v1");
  auto create=load_symbol<create_fn>(lib,"sin_grunt_behavior_create_v1");
  auto step=load_symbol<step_fn>(lib,"sin_grunt_behavior_step_v1");
  auto snapshot=load_symbol<snap_fn>(lib,"sin_grunt_behavior_snapshot_v1");
  auto destroy=load_symbol<destroy_fn>(lib,"sin_grunt_behavior_destroy_v1");
  assert(version()==SIN_GRUNT_BEHAVIOR_ABI_V1);
  sin_grunt_behavior_host_v1 host={SIN_GRUNT_BEHAVIOR_ABI_V1,
    sizeof(sin_grunt_behavior_host_v1),yes_actor,no_behavior,no_owner,
    no_cond,no_owner,no_serial,no_field,ok_condition,no_entity,
    read_link,write_link,ok_lifecycle,ok_notify};
  sin_grunt_assault_state_v1 state={0xffffffffu,0xffffffffu,0u,
    0u,0u,0u,0u,0.f,0.f};
  auto* s=create(&host,nullptr,{42u,8u},33u,7001u,7002u,&state);
  assert(s);
  int32_t current=777;
  assert(step(s,1.f,&current)==SIN_GRUNT_BEHAVIOR_NO_ELIGIBLE);
  assert(current==-1);
  sin_grunt_snapshot_v1 snap{};
  assert(snapshot(s,&snap)==SIN_GRUNT_BEHAVIOR_OK);
  assert(snap.current_index==-1);
  assert(destroy(s)==SIN_GRUNT_BEHAVIOR_OK);
#if defined(_WIN32)
  assert(FreeLibrary(static_cast<HMODULE>(lib)));
#else
  assert(dlclose(lib)==0);
#endif
}
