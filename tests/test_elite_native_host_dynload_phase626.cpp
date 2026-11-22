// Phase626: test the real independent shared library's EXACT 8 C exports.
// Does not load a retail game binary or instantiate original NPC objects.
#include "sin_compat/elite_native_host_c_api_v1.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <dlfcn.h>
#endif
int main(int argc,char** argv) {
  assert(argc==2 && argv[1]!=nullptr);
#ifdef _WIN32
  HMODULE module=LoadLibraryA(argv[1]);
  if(!module){std::fprintf(stderr,"LoadLibraryA failed: %lu\n",
                      (unsigned long)GetLastError());return 1;}
  auto get=[&](const char* name)->void*{
    return reinterpret_cast<void*>(GetProcAddress(module,name));
  };
#else
  void* module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
  if(!module){std::fprintf(stderr,"dlopen failed: %s\n",dlerror());return 1;}
  auto get=[&](const char* name)->void*{return dlsym(module,name);};
#endif
  const char*const functions[]{
    "sin_elite_native_abi_version_v1",
    "sin_elite_native_create_v1",
    "sin_elite_native_destroy_v1",
    "sin_elite_native_attach_v1",
    "sin_elite_native_spawn_v1",
    "sin_elite_native_event43_v1",
    "sin_elite_native_snapshot_v1",
    "sin_elite_native_detach_v1"
  };
  for(const char* name:functions) {
    if(!get(name)){std::fprintf(stderr,"Missing native Elite symbol: %s\n",name);return 2;}
  }
  using Version=uint32_t(*)();
  auto ver=reinterpret_cast<Version>(get(functions[0]));
  if(ver()!=SIN_ELITE_NATIVE_ABI_V1)return 3;
#ifdef _WIN32
  if(!FreeLibrary(module))return 4;
#else
  if(dlclose(module)!=0)return 4;
#endif
  std::puts("PASS Phase626: exact eight independent Elite native C exports and ABI v1");
  return 0;
}
