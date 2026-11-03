#include "sin_compat/source_provider_c_api.h"
#include <cassert>
#include <cstring>
int main(int argc,char**argv){
 assert(argc==2);
 sin101_context* c=nullptr;
 assert(sin101_create(SIN101_ABI_VERSION,argv[1],&c)==SIN101_OK);
 auto lookup=[](void*,const char* n,uint32_t size,int64_t* out)->int32_t {
   if(size!=4 || std::memcmp(n,"step",4))return 1;
   *out=77;
   return 0;};
 auto valid=[](void*,uint32_t token,uint32_t* ok)->int32_t{
   *ok=(token==77);return 0;};
 auto name=[](void*,uint32_t token,char* out,uint32_t cap,uint32_t* bytes)->int32_t{
   if(token!=77 || cap<5)return 1;
   std::memcpy(out,"step",5);
   *bytes=5;
   return 0;};
 sin101_provider_v1 p{sizeof p,SIN101_ABI_VERSION,SIN101_PROVIDER_SOURCEVR_NATIVE,
     0,nullptr,lookup,valid,name};
 assert(sin101_register_provider(c,&p)==SIN101_NATIVE_ABI_NOT_VERIFIED);
 p.kind=SIN101_PROVIDER_MOCK;
 assert(sin101_register_provider(c,&p)==SIN101_OK);
 assert(sin101_map_sound(c,70000,100000,"step",4)==SIN101_OK);
 uint32_t got=0;
 assert(sin101_old_to_native(c,70000,&got)==SIN101_OK && got==77);
 assert(sin101_native_to_old(c,77,&got)==SIN101_OK && got==70000);
 sin101_destroy(c);
}
