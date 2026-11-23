// Phase657: owner-side Win32 x86 retail CreateInterface validation.
// No private DLL files or binary bytes are bundled with this probe.
// Compile with MSVC /W4 /WX /EHsc /std:c++17, link bcrypt.lib.
// Usage: sin_phase657_factory_probe.exe C:\trusted\bin\client.dll C:\trusted\SE1\bin\server.dll
// ONLY trusted owner retail images with the pinned SHA256 are loaded.
// LoadLibraryEx invokes DllMain. No object vtable methods are ever called.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#pragma comment(lib, "bcrypt.lib")

#if !defined(_M_IX86) && !defined(__i386__)
#error Retail factory validation requires native 32-bit Win32 x86.
#endif

namespace {
struct NameExpected { const char* name; std::uint32_t object_rva; };
struct ModuleExpected {
 const char* side;
 const char* sha256;
 std::uint32_t factory_rva;
 const NameExpected* expected;
 std::size_t count;
};
constexpr NameExpected kClient[]={
 {"VClient011",0x2fa4b8u},
 {"VClientEntityList003",0x34fddcu},
 {"VClientPrediction001",0x37fa00u}
};
constexpr NameExpected kServer[]={
 {"ServerGameDLL003",0x510650u},
 {"ServerGameEnts001",0x510654u},
 {"ServerGameClients003",0x510658u}
};
constexpr ModuleExpected kModules[]={
 {"client","6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
  0x164180u,kClient,3u},
 {"server","568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
  0x1fb5f0u,kServer,3u}
};
using CreateInterfaceFn=void* (__cdecl *)(const char*,int*);

bool retail_sha256(const char* path,const char* expected) noexcept {
 std::FILE* f=nullptr;
 if(fopen_s(&f,path,"rb")!=0 || !f)return false;
 BCRYPT_ALG_HANDLE alg=nullptr;
 BCRYPT_HASH_HANDLE hash=nullptr;
 ULONG written=0,object_size=0,digest_size=0;
 bool ok=false;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0 &&
    BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&object_size),
                      sizeof(object_size),&written,0)>=0 &&
    BCryptGetProperty(alg,BCRYPT_HASH_LENGTH,reinterpret_cast<PUCHAR>(&digest_size),
                      sizeof(digest_size),&written,0)>=0 &&
    digest_size==32u && object_size>0u && object_size<1048576u) {
  std::vector<UCHAR> object(object_size,0u);
  if(BCryptCreateHash(alg,&hash,object.data(),object_size,nullptr,0,0)>=0) {
   std::array<UCHAR,32768> block{};
   ok=true;
   for(;;) {
    const auto n=std::fread(block.data(),1u,block.size(),f);
    if(n && BCryptHashData(hash,block.data(),static_cast<ULONG>(n),0)<0) {
     ok=false;break;
    }
    if(n<block.size()) {
     if(std::ferror(f))ok=false;
     break;
    }
   }
   if(ok) {
    std::array<UCHAR,32> digest{};
    if(BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)<0) {
     ok=false;
    } else {
     char hex[65]{};
     constexpr char kDigits[]="0123456789abcdef";
     for(std::size_t i=0;i<digest.size();++i) {
      hex[2u*i]=kDigits[digest[i]>>4u];
      hex[2u*i+1u]=kDigits[digest[i]&15u];
     }
     ok=std::strcmp(hex,expected)==0;
    }
   }
  }
 }
 if(hash)BCryptDestroyHash(hash);
 if(alg)BCryptCloseAlgorithmProvider(alg,0);
 std::fclose(f);
 return ok;
}

bool inspect(const char* path,const ModuleExpected& spec) noexcept {
 // Resolve paths before LOAD_WITH_ALTERED_SEARCH_PATH.
 std::array<char,32768> absolute{};
 const DWORD size=GetFullPathNameA(path,static_cast<DWORD>(absolute.size()),
                                  absolute.data(),nullptr);
 if(!size || size>=absolute.size()) {
  std::fprintf(stderr,"%s path resolution failed\n",spec.side);
  return false;
 }
 if(!retail_sha256(absolute.data(),spec.sha256)) {
  std::fprintf(stderr,"%s retail SHA256 mismatch/unreadable; refusing LoadLibrary\n",spec.side);
  return false;
 }
 HMODULE module=LoadLibraryExA(absolute.data(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 if(!module) {
  std::fprintf(stderr,"%s trusted retail DLL LoadLibraryEx error=%lu\n",
               spec.side,static_cast<unsigned long>(GetLastError()));
  return false;
 }
 bool ok=true;
 auto* symbol=GetProcAddress(module,"CreateInterface");
 if(!symbol) {
  std::fprintf(stderr,"%s missing CreateInterface export\n",spec.side);
  ok=false;
 } else {
  const auto base=reinterpret_cast<std::uintptr_t>(module);
  const auto proc=reinterpret_cast<std::uintptr_t>(symbol);
  if(proc-base!=spec.factory_rva) {
   std::fprintf(stderr,"%s CreateInterface export offset mismatch\n",spec.side);
   ok=false;
  } else {
   auto* factory=reinterpret_cast<CreateInterfaceFn>(symbol);
   for(std::size_t i=0;i<spec.count;++i) {
    const auto& entry=spec.expected[i];
    int status=-777;
    void* ptr=factory(entry.name,&status);
    const auto result=reinterpret_cast<std::uintptr_t>(ptr);
    const bool match=(ptr!=nullptr && status==0 &&
                      result-base==entry.object_rva);
    std::printf("{\"module\":\"%s\",\"interface\":\"%s\","
                "\"return_code\":%d,\"singleton_rva\":\"0x%08x\","
                "\"status\":\"%s\"}\n",
                spec.side,entry.name,status,
                ptr?static_cast<unsigned int>(result-base):0u,
                match?"pinned_factory_return":"mismatch");
    if(!match)ok=false;
   }
   int missing_rc=-777;
   const void* absent=factory("__SiN_Phase657_Invalid_Interface__",&missing_rc);
   const bool miss_ok=(absent==nullptr && missing_rc==1);
   std::printf("{\"module\":\"%s\",\"interface\":\"absent_interface_probe\","
               "\"return_code\":%d,\"status\":\"%s\"}\n",
               spec.side,missing_rc,miss_ok?"not_found_as_expected":"mismatch");
   if(!miss_ok)ok=false;
  }
 }
 FreeLibrary(module);
 return ok;
}
} // namespace

int main(int argc,char** argv) {
 if(argc!=3) {
  std::fprintf(stderr,"usage: %s <original-client.dll> <original-server.dll>\n",
               argc>0?argv[0]:"sin_phase657_factory_probe.exe");
  return 2;
 }
 bool all=true;
 // Continue to check both trusted modules if the first has a load failure.
 for(int i=0;i<2;++i)
  if(!inspect(argv[i+1],kModules[i]))all=false;
 return all?0:1;
}
