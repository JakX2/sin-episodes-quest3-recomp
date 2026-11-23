// Phase630: load the separate 3-export C bridge as an actual DLL/.so.
#include "sin_compat/elite_native_source_gate_phase630.h"
#include <cstdint>
#include <cstdio>
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <dlfcn.h>
#endif
int main(int argc,char** argv) {
  if(argc!=2 || !argv[1])return 1;
#ifdef _WIN32
  HMODULE lib=LoadLibraryA(argv[1]);
  if(!lib){std::fprintf(stderr,"LoadLibrary failed %lu\n",
      (unsigned long)GetLastError());return 2;}
  auto sym=[&](const char* name)->void*{
    return reinterpret_cast<void*>(GetProcAddress(lib,name));};
#else
  void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
  if(!lib){std::fprintf(stderr,"dlopen: %s\n",dlerror());return 2;}
  auto sym=[&](const char* name)->void*{return dlsym(lib,name);};
#endif
  const char* names[]{
    "sin_elite_source_gate_abi_version_v1",
    "sin_elite_source_gate_event43_v1",
    "sin_elite_source_gate_update_v1"
  };
  for(const char* name:names) {
    if(!sym(name)){std::fprintf(stderr,"Missing native export: %s\n",name);return 3;}
  }
  using GetVersion=std::uint32_t(*)();
  if(reinterpret_cast<GetVersion>(sym(names[0]))()!=
     SIN_ELITE_SOURCE_GATE_ABI_V1)return 4;
#ifdef _WIN32
  if(!FreeLibrary(lib))return 5;
#else
  if(dlclose(lib)!=0)return 5;
#endif
  std::puts("PASS Phase630: 3 separate source-gate C exports and original ABI dependency");
  return 0;
}
