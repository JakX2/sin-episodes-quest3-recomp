#include "sin_compat/source_provider_c_api.h"
#include <cassert>
int main(int argc,char**argv){assert(argc==2);sin101_context* c=nullptr;assert(sin101_create(SIN101_ABI_VERSION,argv[1],&c)==SIN101_OK);assert(c);assert(sin101_map_sound(c,70000,100000,"step",4)==SIN101_PROVIDER_NOT_READY);sin101_destroy(c);}
