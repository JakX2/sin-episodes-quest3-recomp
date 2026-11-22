// Phase619: verify a REAL native shared library's seven C symbols.
#include "sin_compat/grunt_native_host_c_api_v1.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#if defined(_WIN32)
# include <windows.h>
#else
# include <dlfcn.h>
#endif
template<typename T> static T function(void* lib,const char* symbol) {
#if defined(_WIN32)
  const auto p=GetProcAddress(static_cast<HMODULE>(lib),symbol);
#else
  const auto p=dlsym(lib,symbol);
#endif
  assert(p);
  T result{};
  static_assert(sizeof(result)==sizeof(p),"function pointer width");
  std::memcpy(&result,&p,sizeof(p));
  return result;
}
int main(int argc,char** argv) {
  assert(argc==2);
#if defined(_WIN32)
  void* lib=LoadLibraryA(argv[1]);
#else
  void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
#endif
  assert(lib);
  using Version=uint32_t(*)();
  using Create=sin_grunt_native_registry_v1*(*)(
      const sin_grunt_native_host_v1*,void*);
  using Destroy=int32_t(*)(sin_grunt_native_registry_v1*);
  using Attach=int32_t(*)(sin_grunt_native_registry_v1*,
                          const sin_grunt_native_attachment_v1*);
  using Step=int32_t(*)(sin_grunt_native_registry_v1*,sin_grunt_actor_v1,
                        float,int32_t*,int32_t*);
  using Snap=int32_t(*)(sin_grunt_native_registry_v1*,sin_grunt_actor_v1,
                        sin_grunt_snapshot_v1*);
  using Detach=int32_t(*)(sin_grunt_native_registry_v1*,sin_grunt_actor_v1);
  const auto version=function<Version>(lib,"sin_grunt_native_abi_version_v1");
  const auto create=function<Create>(lib,"sin_grunt_native_create_v1");
  const auto destroy=function<Destroy>(lib,"sin_grunt_native_destroy_v1");
  const auto attach=function<Attach>(lib,"sin_grunt_native_attach_v1");
  const auto step=function<Step>(lib,"sin_grunt_native_step_v1");
  const auto snap=function<Snap>(lib,"sin_grunt_native_snapshot_v1");
  const auto detach=function<Detach>(lib,"sin_grunt_native_detach_v1");
  assert(version()==SIN_GRUNT_NATIVE_ABI_V1);
  assert(!create(nullptr,nullptr));
  assert(destroy(nullptr)==SIN_GRUNT_NATIVE_INVALID_ARGUMENT);
  assert(attach(nullptr,nullptr)==SIN_GRUNT_NATIVE_INVALID_ARGUMENT);
  assert(step(nullptr,{1u,1u},0.f,nullptr,nullptr)==
         SIN_GRUNT_NATIVE_INVALID_ARGUMENT);
  assert(snap(nullptr,{1u,1u},nullptr)==SIN_GRUNT_NATIVE_INVALID_ARGUMENT);
  assert(detach(nullptr,{1u,1u})==SIN_GRUNT_NATIVE_INVALID_ARGUMENT);
#if defined(_WIN32)
  assert(FreeLibrary(static_cast<HMODULE>(lib)));
#else
  assert(dlclose(lib)==0);
#endif
  return 0;
}
